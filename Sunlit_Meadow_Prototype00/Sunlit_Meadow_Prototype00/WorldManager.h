#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "Chunk.h"
#include "Region.h"

ChunkCoord getPlayerChunkCoord(Vec3 playerPosition);
RegionCoord getPlayerRegionCoord(Vec3 playerPosition);

class WorldManager {
private:
    std::unordered_map<RegionCoord, std::unique_ptr<Region>, RegionCoordHash> regions;
    std::vector<ChunkCoord> visibleChunkCoordsRelative;

    std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash> renderList;
    std::unordered_set<ChunkCoord, ChunkCoordHash> pendingChunks;

    int        m_renderDistance = 0;
    ChunkCoord m_lastPlayerChunkPos = { 1000, 1000, 10000 };

public:
    WorldManager();
    void calcVisibleChunksList(int renderDistance);
    void update(AppState* state, SDL_GPUTexture* textureArray);
    void updatePlayerPosition(Vec3 playerPosition);   // replaces updateRenderList
    void drawChunks(AppState*, SDL_GPUCommandBuffer*, SDL_GPURenderPass*, const UBO&);
    void destroyManager(AppState* state);

private:
    Region* getRegion(RegionCoord regionCoordinates);
    RegionCoord regionCoordForChunk(ChunkCoord c);
    void        onPlayerChunkChanged();
};