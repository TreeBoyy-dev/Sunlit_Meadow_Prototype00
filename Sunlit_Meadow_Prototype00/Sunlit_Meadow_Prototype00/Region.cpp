#include "Region.h"
#include "Chunk.h"

#include <utility>

Region::Region(RegionCoord regionCoordinates)
    : regionCoordinates(regionCoordinates)
{
    g_worker.start();
    m_worker.start();
}

Region::~Region() {
    g_worker.stop();
    m_worker.stop();
}

Chunk* Region::getChunk(ChunkCoord chunkCoordinates) {
    // Already fully ready
    auto it = chunks.find(chunkCoordinates);
    if (it != chunks.end())
        return it->second.get();

    // Already queued, still generating
    if (pendingChunks.count(chunkCoordinates))
        return nullptr;

    // New request — send to worker
    pendingChunks.insert(chunkCoordinates);
    g_worker.requestChunk(chunkCoordinates);
    return nullptr;
}

bool Region::update(AppState* state, SDL_GPUTexture* textureArray) {
    bool changed = false;

    // --- drain g_worker: newly generated chunks ---
    const int MAX_UPLOADS_PER_FRAME = 5;
    int uploads = 0;
    while (uploads < MAX_UPLOADS_PER_FRAME) {
        auto result = g_worker.tryGetChunk();
        if (!result) break;

        std::unique_ptr<Chunk> chunk = std::move(*result);
        ChunkCoord coord = chunk->getChunkCoordinates(); // was broken before (coord = coord)
        pendingChunks.erase(coord);

        chunk->uploadMeshes(state, textureArray);
        chunks.emplace(coord, std::move(chunk));

        // New chunk arrived — remesh it and its neighbors
        queueMeshUpdate(coord);

        uploads++;
        changed = true;
    }

    // --- drain m_worker: re-meshed chunks ---
    if (collectMeshResults(state, textureArray))
        changed = true;

    return changed;
}

// Samples the 6 neighboring chunks (if they exist) to fill the air-border data.
// Defaults to all-true (open air) for any neighbor that isn't loaded yet.
// the else statements would result in the chunk only beeing redrawn if all 6 neighbors are generated
bool Region::buildBorderAir(ChunkBorderAir* border, ChunkCoord coord) {

    auto getNeighbor = [&](ChunkCoord c) -> Chunk* {
        auto it = chunks.find(c);
        return (it != chunks.end()) ? it->second.get() : nullptr;
    };

    if (auto* n = getNeighbor({ coord.x + 1, coord.y,     coord.z }))
        if (auto* face = n->getBorderAir({ -1,  0,  0 }))  // neighbor's x- face
            memcpy(border->front, face, sizeof(border->front));
        else return false;

    if (auto* n = getNeighbor({ coord.x - 1, coord.y,     coord.z }))
        if (auto* face = n->getBorderAir({ 1,  0,  0 }))  // neighbor's x+ face
            memcpy(border->back, face, sizeof(border->back));
        else return false;

    if (auto* n = getNeighbor({ coord.x,     coord.y + 1, coord.z }))
        if (auto* face = n->getBorderAir({ 0, -1,  0 }))  // neighbor's y- face
            memcpy(border->right, face, sizeof(border->right));
        else return false;

    if (auto* n = getNeighbor({ coord.x,     coord.y - 1, coord.z }))
        if (auto* face = n->getBorderAir({ 0,  1,  0 }))  // neighbor's y+ face
            memcpy(border->left, face, sizeof(border->left));
        else return false;

    if (auto* n = getNeighbor({ coord.x,     coord.y,     coord.z + 1 }))
        if (auto* face = n->getBorderAir({ 0,  0, -1 }))  // neighbor's z- face
            memcpy(border->top, face, sizeof(border->top));
        else return false;

    if (auto* n = getNeighbor({ coord.x,     coord.y,     coord.z - 1 }))
        if (auto* face = n->getBorderAir({ 0,  0,  1 }))  // neighbor's z+ face
            memcpy(border->bottom, face, sizeof(border->bottom));
        else return false;

    return true;
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
        if (c != coord && !meshedChunks.count(c)) continue;

        if (pendingMeshChunks.count(c)) {
            // Already queued — try to pull it back out so we can resubmit
            // with fresh border data. If it's already being processed by the
            // worker thread (cancel returns false), leave it; the slightly
            // stale mesh will be overwritten on the next trigger.
            if (!m_worker.cancelRequest(c)) continue;
            pendingMeshChunks.erase(c);
        }

        ChunkBorderAir borderAir;
        if (!buildBorderAir(&borderAir, c)) {
            SDL_Log("not all adjacent chunks loaded at %d|%d|%d", c.x, c.y, c.z);
            //continue;
        }
        else {
            SDL_Log("remeshed: %d|%d|%d", c.x, c.y, c.z);
        }

        auto it = chunks.find(c);
        pendingMeshChunks.insert(c);
        m_worker.requestChunk(it->second.get(), borderAir);
    }
}

bool Region::collectMeshResults(AppState* state, SDL_GPUTexture* textureArray) {
    if (pendingMeshChunks.empty()) return false;

    const int MAX_UPLOADS_PER_FRAME = 5;
    int uploads = 0;
    bool any = false;

    while (uploads < MAX_UPLOADS_PER_FRAME) {
        auto result = m_worker.tryGetChunk();
        if (!result) break;

        ChunkCoord coord = *result;
        pendingMeshChunks.erase(coord);

        auto it = chunks.find(coord);
        if (it == chunks.end()) continue; // chunk was unloaded while meshing, skip

        //it->second->destroyMeshes(state);
        it->second->uploadMeshes(state, textureArray);
        meshedChunks.insert(coord);

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
    pendingChunks.clear();
    pendingMeshChunks.clear();
    meshedChunks.clear();
}