#include "Region.h"
#include "Chunk.h"

#include <utility>

Region::Region(RegionCoord regionCoordinates, BlockManager* blockManager, FastNoiseLite* standartNoise)
    : regionCoordinates(regionCoordinates),
    m_blockManager(blockManager),
    m_standartNoise(standartNoise)
{
    g_worker.start(m_blockManager, m_standartNoise, regionCoordinates.z * REGION_SIZE_Z);
    m_worker.start(m_blockManager);
}

Region::~Region() {
    g_worker.stop();
    m_worker.stop();
}

Chunk* Region::getChunk(ChunkCoord chunkCoordinates) {
    auto it = chunks.find(chunkCoordinates);
    return (it != chunks.end()) ? it->second.get() : nullptr;
}

void Region::requestChunkGeneration(ChunkCoord chunkCoordinates) {
    ColumnCoord column = { chunkCoordinates.x, chunkCoordinates.y };

    if (requestedColumns.count(column)) return; // column already generated or in flight
    SDL_Log("[Region] requesting column: %d|%d", chunkCoordinates.x, chunkCoordinates.y);

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
    const int MAX_UPLOADS_PER_FRAME = 2;
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

    //for (const auto& coord : newlyAdded)
    //    queueMeshUpdate(coord);

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
        Chunk chunkCopy = Chunk(it->second.get());
        m_worker.requestChunk(chunkCopy);
    }
}

bool Region::collectMeshResults(AppState* state,
    SDL_GPUTexture* textureArray,
    std::vector<ChunkCoord>& outNewlyReady)
{
    if (pendingMeshChunks.empty()) return false;

    const int MAX_UPLOADS_PER_FRAME = 2;
    int uploads = 0;
    bool any = false;

    while (uploads < MAX_UPLOADS_PER_FRAME) {
        auto result = m_worker.tryGetChunk();
        if (!result) break;

        ChunkCoord coord = result->getChunkCoordinates();
        pendingMeshChunks.erase(coord);

        auto it = chunks.find(coord);
        if (it == chunks.end()) continue; // unloaded while meshing

        it->second->transferMeshesFrom(*result);
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