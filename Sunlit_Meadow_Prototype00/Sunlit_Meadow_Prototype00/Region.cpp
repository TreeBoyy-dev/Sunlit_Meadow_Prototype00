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

Chunk* Region::getChunk(ChunkCoord chunkCoordinates, bool queueChunk) {
    // Already fully ready
    auto it = chunks.find(chunkCoordinates);
    if (it != chunks.end())
        return it->second.get();

    if (queueChunk) {
        // Already queued, still generating
        if (pendingChunks.count(chunkCoordinates))
            return nullptr;

        // New request — send to worker
        pendingChunks.insert(chunkCoordinates);
        g_worker.requestChunk(chunkCoordinates);
    }
    return nullptr;
}

bool Region::update(AppState* state, SDL_GPUTexture* textureArray) {
    bool changed = false;

    // --- drain g_worker: newly generated chunks ---
    const int MAX_UPLOADS_PER_FRAME = 999;
    int uploads = 0;
    std::vector<ChunkCoord> newlyAdded;

    while (uploads < MAX_UPLOADS_PER_FRAME) {
        auto result = g_worker.tryGetChunk();
        if (!result) break;

        std::unique_ptr<Chunk> chunk = std::move(*result);
        ChunkCoord coord = chunk->getChunkCoordinates();
        pendingChunks.erase(coord);

        chunk->uploadMeshes(state, textureArray);
        chunks.emplace(coord, std::move(chunk));
        newlyAdded.push_back(coord);

        uploads++;
        changed = true;
    }

    // Queue mesh updates only after all new chunks are in the map,
    // so neighbors arriving in the same frame are visible to each other.
    for (const auto& coord : newlyAdded)
        queueMeshUpdate(coord);

    // --- drain m_worker: re-meshed chunks ---
    if (collectMeshResults(state, textureArray))
        changed = true;

    return changed;
}

// Samples the 6 neighboring chunks (if they exist) to fill the air-border data.
// Defaults to all-true (open air) for any neighbor that isn't loaded yet.
// the else statements would result in the chunk only beeing redrawn if all 6 neighbors are generated
bool Region::buildBorderAir(ChunkBorderAir* border, ChunkCoord coord) {

    bool allFacesLoaded = true;

    if (Chunk* c = getChunk({ coord.x + 1, coord.y,     coord.z }, false))
        if (auto* face = c->getBorderAir({ -1,  0,  0 }))  // neighbor's x- face
            memcpy(border->front, face, sizeof(border->front));
        else {
            //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor +X exists but getBorderAir returned nullptr",
            //    coord.x, coord.y, coord.z);
            allFacesLoaded = false;
        }
    else {
        //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor +X wasn't found",
        //    coord.x, coord.y, coord.z);
        allFacesLoaded = false;
    }

    if (Chunk* c = getChunk({ coord.x - 1, coord.y,     coord.z }, false))
        if (auto* face = c->getBorderAir({ 1,  0,  0 }))  // neighbor's x+ face
            memcpy(border->back, face, sizeof(border->back));
        else {
            //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor -X exists but getBorderAir returned nullptr",
            //    coord.x, coord.y, coord.z);
            allFacesLoaded = false;
        }
    else {
        //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor -X wasn't found",
         //   coord.x, coord.y, coord.z);
        allFacesLoaded = false;
    }

    if (Chunk* c = getChunk({ coord.x,     coord.y + 1, coord.z }, false))
        if (auto* face = c->getBorderAir({ 0, -1,  0 }))  // neighbor's y- face
            memcpy(border->right, face, sizeof(border->right));
        else {
            //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor +Y exists but getBorderAir returned nullptr",
            //    coord.x, coord.y, coord.z);
            allFacesLoaded = false;
        }
    else {
        //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor +Y wasn't found",
        //    coord.x, coord.y, coord.z);
        allFacesLoaded = false;
    }

    if (Chunk* c = getChunk({ coord.x,     coord.y - 1, coord.z }, false))
        if (auto* face = c->getBorderAir({ 0,  1,  0 }))  // neighbor's y+ face
            memcpy(border->left, face, sizeof(border->left));
        else {
            //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor -Y exists but getBorderAir returned nullptr",
            //    coord.x, coord.y, coord.z);
            allFacesLoaded = false;
        }
    else {
        //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor -Y wasn't found",
        //    coord.x, coord.y, coord.z);
        allFacesLoaded = false;
    }

    if (Chunk* c = getChunk({ coord.x,     coord.y,     coord.z + 1 }, false))
        if (auto* face = c->getBorderAir({ 0,  0, -1 }))  // neighbor's z- face
            memcpy(border->top, face, sizeof(border->top));
        else {
            //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor +Z exists but getBorderAir returned nullptr",
            //    coord.x, coord.y, coord.z);
            allFacesLoaded = false;
        }
    else {
        //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor +Z wasn't found",
        //    coord.x, coord.y, coord.z);
        allFacesLoaded = false;
    }

    if (Chunk* c = getChunk({ coord.x,     coord.y,     coord.z - 1 }, false))
        if (auto* face = c->getBorderAir({ 0,  0,  1 }))  // neighbor's z+ face
            memcpy(border->bottom, face, sizeof(border->bottom));
        else {
            //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor -Z exists but getBorderAir returned nullptr",
            //    coord.x, coord.y, coord.z);
            allFacesLoaded = false;
        }
    else {
        //SDL_Log("[BorderAir] %d|%d|%d  TOP: neighbor -Z wasn't found",
        //    coord.x, coord.y, coord.z);
        allFacesLoaded = false;
    }

    return allFacesLoaded ? true : false;
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

        bool allLoaded = buildBorderAir(&borderAir, c);
        if (allLoaded) {
            SDL_Log("[Mesh] %d|%d|%d  clean remesh",
                c.x, c.y, c.z);
        }
        else {
            SDL_Log("[Mesh] %d|%d|%d  PARTIAL remesh",
                c.x, c.y, c.z);
            //continue;
        }

        auto it = chunks.find(c);
        pendingMeshChunks.insert(c);
        Chunk chunkCopy = Chunk(it->second.get());
        m_worker.requestChunk(chunkCopy, borderAir);
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

        ChunkCoord coord = result->getChunkCoordinates();
        pendingMeshChunks.erase(coord);

        auto it = chunks.find(coord);
        if (it == chunks.end()) continue; // unloaded while meshing

        it->second->transferMeshesFrom(*result); // swap in built mesh data
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