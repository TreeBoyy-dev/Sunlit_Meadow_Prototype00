#include "Region.h"
#include "Chunk.h"

#include <utility>

Region::Region(RegionCoord regionCoordinates)
    : regionCoordinates(regionCoordinates)
{
}

Chunk* Region::getChunk(
    ChunkCoord chunkCoordinates,
    AppState* state,
    SDL_GPUTexture* textureArray
) {
    auto it = chunks.find(chunkCoordinates);

    if (it == chunks.end()) {
        auto [newIt, inserted] = chunks.try_emplace(
            chunkCoordinates,
            std::make_unique<Chunk>(chunkCoordinates)
        );
        Chunk& chunk = *newIt->second;

        chunk.getChunkGenerated();

        if (!chunk.initMeshes(state, textureArray)) {
            chunks.erase(newIt);
            return nullptr;
        }

        return &chunk;
    }

    return &*it->second;
}

RegionCoord Region::getCoordinates() {
    return regionCoordinates;
}

void Region::destroyRegion(AppState* state) {
    for (auto& [coord, chunk] : chunks) {
        chunk->destroyMeshes(state);
    }

    chunks.clear();
}