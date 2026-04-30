#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "Chunk.h"
#include "Region.h"

ChunkCoord getPlayerChunkCoord(Vec3 playerPosition);

class WorldManager {
private:
    std::unordered_map<RegionCoord, std::unique_ptr<Region>, RegionCoordHash> regions;
    std::vector<ChunkCoord> visibleChunkCoordsRelative;
    std::vector<Chunk*> renderList;

    ChunkCoord m_lastPlayerChunkPos = { 0, 0, 0 };
    bool m_needsRenderListUpdate = false;

public:
    WorldManager();

    // Call when render distance changes or on init
    void calcVisibleChunksList(int renderDistance);

    // Call every frame collects generated chunks and inits their GPU meshes
    void update(AppState* state, SDL_GPUTexture* textureArray);

    // Call when player moves to a different chunk or render distance changes
    void updateRenderList(Vec3 playerPosition);

    void drawChunks(
        AppState* state,
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const UBO& ubo
    );

    void destroyManager(AppState* state);

private:
    Region* getRegion(RegionCoord regionCoordinates);
    void rebuildRenderList();
};