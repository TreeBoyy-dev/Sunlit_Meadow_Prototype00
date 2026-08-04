#include "Region.h"
#include "Chunk.h"
#include "Globals.h"
#include "WorldGenRegistry.h"
#include "WorldGenSampler.h"

#include <utility>

// Floor division for cell math. lbx / BIOME_CELL truncates toward zero in
// C++, so lbx = -1 would land in cell 0 instead of apron cell -1 — every
// block->cell conversion in this file must go through here.
static int floorDivCell(int v) {
    return (v >= 0) ? (v / BIOME_CELL) : ((v - (BIOME_CELL - 1)) / BIOME_CELL);
}

Region::Region(RegionCoord regionCoordinates, BlockManager* blockManager, FastNoiseLite* standartNoise,
    const WorldGenNoise* worldGenNoise, const WorldGenRegistry* worldGenRegistry, Uint64 worldSeed)
    : regionCoordinates(regionCoordinates),
    m_blockManager(blockManager),
    m_standartNoise(standartNoise),
    m_worldGenNoise(worldGenNoise),
    m_worldGenRegistry(worldGenRegistry),
    m_worldSeed(worldSeed)
{}

Region::~Region() {
    g_worker.stop();
    m_worker.stop();
}

void Region::setShape(RegionShape* shape) {
    // Order matters here: layer, then maps, then workers. The workers get
    // raw pointers into m_layer/zoneMap/biomeMap, so those must be fully
    // built (and never mutated again) before the first worker thread runs.

    // 1) Resolve which layer this region belongs to (by region z).
    m_layer = std::move(shape->m_layer);

    // 2) Build both maps synchronously. Every cell — including the apron
    //    cells past the region edge — is computed by the same pure sampler
    //    (world cell coords + layer + seed), so neighboring regions agree
    //    on the overlap by construction. No neighbor-region access, ever.
    zoneMap  = std::move(shape->zoneMap);
    biomeMap = std::move(shape->biomeMap);

    // 3) Only now start the workers. The maps and layer are immutable from
    //    here on, so handing the generator worker raw const pointers is a
    //    safe lock-free share; the region stops the worker before dying
    //    (~Region / destroyRegion), so lifetime is covered too.
    g_worker.start(m_blockManager, m_standartNoise, m_worldGenNoise, m_worldGenRegistry,
        { regionCoordinates.x * REGION_SIZE_YX,
          regionCoordinates.y * REGION_SIZE_YX,
          regionCoordinates.z * REGION_SIZE_Z },
        m_worldSeed,
        m_layer, &zoneMap, &biomeMap);
    m_worker.start(m_blockManager);
}

Chunk* Region::getChunk(ChunkCoord chunkCoordinates) {
    auto it = chunks.find(chunkCoordinates);
    return (it != chunks.end()) ? it->second.get() : nullptr;
}

void Region::requestChunkGeneration(ChunkCoord chunkCoordinates) {
    ColumnCoord column = { chunkCoordinates.x, chunkCoordinates.y };

    if (requestedColumns.count(column)) return; // column already generated or in flight
    //SDL_Log("[Region] requesting column: %d|%d", chunkCoordinates.x, chunkCoordinates.y);

    requestedColumns.insert(column);
    g_worker.requestColumn(column);
}

void Region::cancelChunkGeneration(ChunkCoord chunkCoordinates) {
    ColumnCoord column = { chunkCoordinates.x, chunkCoordinates.y };

    if (!requestedColumns.count(column)) return;

    if (g_worker.cancelColumn(column))
        requestedColumns.erase(column);
}

bool Region::update(AppState* state,
    SDL_GPUTexture* textureArray,
    std::vector<ChunkCoord>& outNewlyReady)
{
    bool changed = false;

    // --- drain g_worker: newly generated chunks ---
    const int MAX_UPLOADS_PER_FRAME = 5;
    int uploads = 0;
    std::vector<ChunkCoord> newlyAdded;

    while (uploads < MAX_UPLOADS_PER_FRAME) {
        auto result = g_worker.tryGetChunk();
        if (!result) break;

        std::unique_ptr<Chunk> chunk = std::move(*result);
        ChunkCoord coord = chunk->getChunkCoordinates();

        chunk->uploadMeshes(state, textureArray);
        chunks.emplace(coord, std::move(chunk));

        outNewlyReady.push_back(coord);
        newlyAdded.push_back(coord);

        uploads++;
        changed = true;
    }

    if(doRemeshingSeperately)
        for (const auto& coord : newlyAdded)
            queueMeshUpdate(coord);

    // --- drain m_worker: re-meshed chunks (these are the "drawable" events) ---
    if (collectMeshResults(state, textureArray, outNewlyReady))
        changed = true;

    return changed;
}

void Region::queueMeshUpdate(ChunkCoord coord) {
    const ChunkCoord candidates[7] = {
        coord,
        { coord.x + 1, coord.y,     coord.z     },
        { coord.x - 1, coord.y,     coord.z     },
        { coord.x,     coord.y + 1, coord.z     },
        { coord.x,     coord.y - 1, coord.z     },
        { coord.x,     coord.y,     coord.z + 1 },
        { coord.x,     coord.y,     coord.z - 1 },
    };

    for (const auto& c : candidates) {
        if (chunks.find(c) == chunks.end())  continue; // not loaded

        if (pendingMeshChunks.count(c)) {
            // Already queued — try to pull it back out so we can resubmit
            // with fresh border data. If it's already being processed by the
            // worker thread (cancel returns false), leave it; the slightly
            // stale mesh will be overwritten on the next trigger.
            if (!m_worker.cancelRequest(c)) continue;
            pendingMeshChunks.erase(c);
        }

        auto it = chunks.find(c);
        pendingMeshChunks.insert(c);
        // exactly ONE snapshot copy, then ownership moves through
        // the worker (the old path copied the chunk three times).
        m_worker.requestChunk(std::make_unique<Chunk>(it->second.get()));
    }
}

bool Region::collectMeshResults(AppState* state,
    SDL_GPUTexture* textureArray,
    std::vector<ChunkCoord>& outNewlyReady)
{
    if (pendingMeshChunks.empty()) return false;

    const int MAX_UPLOADS_PER_FRAME = 5;
    int uploads = 0;
    bool any = false;

    while (uploads < MAX_UPLOADS_PER_FRAME) {
        auto result = m_worker.tryGetChunk();
        if (!result) break;

        std::unique_ptr<Chunk> remeshed = std::move(*result);
        ChunkCoord coord = remeshed->getChunkCoordinates();
        pendingMeshChunks.erase(coord);

        auto it = chunks.find(coord);
        if (it == chunks.end()) continue; // unloaded while meshing

        it->second->transferMeshesFrom(state, *remeshed);
        it->second->uploadMeshes(state, textureArray);
        meshedChunks.insert(coord);
        outNewlyReady.push_back(coord);   // <-- report drawable

        uploads++;
        any = true;
    }

    return any;
}

RegionCoord Region::getCoordinates() {
    return regionCoordinates;
}

// ---------------------------------------------------------------------
//  Layer / zone / biome reads.
//
//  Block coords in, ids out. Valid range is the region interior PLUS the
//  one-chunk apron: [-CHUNK_SIZE, 512 + CHUNK_SIZE). The apron exists so
//  feature generation near a region edge can sample "past" the edge —
//  those cells hold the same values the neighbor computed for itself.
//
//  Safe to call from the generator worker without a lock: the maps are
//  immutable after the constructor (see the member comments in Region.h).
// ---------------------------------------------------------------------
Uint16 Region::getZoneIdLocal(int lbx, int lby) const {
    const int REGION_BLOCKS = CHUNK_SIZE * REGION_SIZE_YX;
    if (lbx < -CHUNK_SIZE || lbx >= REGION_BLOCKS + CHUNK_SIZE ||
        lby < -CHUNK_SIZE || lby >= REGION_BLOCKS + CHUNK_SIZE) {
        SDL_Log("[Region] getZoneIdLocal out of range: %d|%d in region %d|%d|%d",
            lbx, lby, regionCoordinates.x, regionCoordinates.y, regionCoordinates.z);
        return 0;
    }
    return zoneMap.get(floorDivCell(lbx) + MAP_APRON_CELLS,
                       floorDivCell(lby) + MAP_APRON_CELLS);
}

Uint16 Region::getBiomeIdLocal(int lbx, int lby) const {
    const int REGION_BLOCKS = CHUNK_SIZE * REGION_SIZE_YX;
    if (lbx < -CHUNK_SIZE || lbx >= REGION_BLOCKS + CHUNK_SIZE ||
        lby < -CHUNK_SIZE || lby >= REGION_BLOCKS + CHUNK_SIZE) {
        SDL_Log("[Region] getBiomeIdLocal out of range: %d|%d in region %d|%d|%d",
            lbx, lby, regionCoordinates.x, regionCoordinates.y, regionCoordinates.z);
        return 0;
    }
    return biomeMap.get(floorDivCell(lbx) + MAP_APRON_CELLS,
                        floorDivCell(lby) + MAP_APRON_CELLS);
}

const LayerDef* Region::getLayer() const {
    return m_layer;
}

void Region::destroyRegion(AppState* state) {
    g_worker.stop();
    m_worker.stop();

    for (auto& [coord, chunk] : chunks)
        chunk->destroyMeshes(state);

    chunks.clear();
    requestedColumns.clear();
    pendingMeshChunks.clear();
    meshedChunks.clear();
}
