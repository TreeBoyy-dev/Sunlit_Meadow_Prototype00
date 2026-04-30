#include "Region.h"
#include "Chunk.h"

#include <utility>

Region::Region(RegionCoord regionCoordinates)
    : regionCoordinates(regionCoordinates)
{
    m_worker.start();
}

Region::~Region() {
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
    m_worker.requestChunk(chunkCoordinates);
    return nullptr;
}

bool Region::update(AppState* state, SDL_GPUTexture* textureArray) {
    if (pendingChunks.empty()) return false;

    const int MAX_UPLOADS_PER_FRAME = 2;
    int uploadsThisFrame = 0;
    bool addedChunks = false;

    while (uploadsThisFrame < MAX_UPLOADS_PER_FRAME) {
        auto result = m_worker.tryGetChunk();
        if (!result) break;

        std::unique_ptr<Chunk> chunk = std::move(*result);
        ChunkCoord coord = chunk->getChunkCoordinates();
        pendingChunks.erase(coord);

        chunk->uploadMeshes(state, textureArray);
        chunks.emplace(coord, std::move(chunk));
        uploadsThisFrame++;
        addedChunks = true;
    }

    return addedChunks;
}

RegionCoord Region::getCoordinates() {
    return regionCoordinates;
}

void Region::destroyRegion(AppState* state) {
    m_worker.stop();   // finish any in-flight work first

    for (auto& [coord, chunk] : chunks)
        chunk->destroyMeshes(state);

    chunks.clear();
    pendingChunks.clear();
}