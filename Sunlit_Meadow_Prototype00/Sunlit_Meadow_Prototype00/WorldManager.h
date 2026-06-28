#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "Chunk.h"
#include "Region.h"
#include "FastNoiseLite.h"
#include "BlockManager.h"
#include "WorldTypes.h"

enum BlockFace : int {
    FACE_FRONT = 0,  // +X
    FACE_BACK = 1,  // -X
    FACE_RIGHT = 2,  // +Y
    FACE_LEFT = 3,  // -Y
    FACE_UP = 4,  // +Z
    FACE_DOWN = 5,  // -Z
    FACE_NONE = -1  // ray started inside a solid block, no face crossed
};

Vec3 getBlockLookingAt(Camera cam, const float MAX_REACH, int* outFace = nullptr);
ChunkCoord getPlayerChunkCoord(Vec3 playerPosition);
RegionCoord getPlayerRegionCoord(Vec3 playerPosition);
RegionCoord getRegionCoordForChunk(ChunkCoord c);

class WorldManager {
private:
    SDL_GPUGraphicsPipeline* pipeline = nullptr;

    SDL_GPUTexture* textureArray = nullptr;


    std::unordered_map<RegionCoord, std::unique_ptr<Region>, RegionCoordHash> regions;
    std::vector<ChunkCoord> visibleChunkCoordsRelative;
    std::unordered_set<ChunkCoord, ChunkCoordHash> visibleRelativeSet;

    std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash> renderList;
    std::unordered_set<ChunkCoord, ChunkCoordHash> pendingChunks;

    int        m_renderDistance = 0;
    ChunkCoord m_lastPlayerChunkPos = { 1000, 1000, 10000 };

    BlockManager* blockManager;
    FastNoiseLite standartNoise;

public:
    WorldManager();

    bool    init(
        SDL_GPUDevice* gpu,
        SDL_GPUTextureFormat swapchainFormat,
        BlockManager* blockManagerIn);
    void    destroy(AppState* state);

    void    update(AppState* state, Vec3 playerPosition);
    void    draw(AppState*, SDL_GPUCommandBuffer*, SDL_GPURenderPass*, const UBO&);

    void    calcVisibleChunksList(int renderDistance);


    void    setBlockIdAt(Vec3 pos, Uint16 id, AppState* state);
    Uint16  getBlockIdAt(Vec3 pos);
    Vec3    getBlockLookingAt(
        Camera cam,
        const float MAX_REACH,
        int* outFace = nullptr);
    float   getBlockCollision(Vec3 pos);

private:
    void    updatePlayerPosition(Vec3 playerPosition);

    void    drawChunks(AppState*, SDL_GPUCommandBuffer*, SDL_GPURenderPass*, const UBO&);

    Region* getRegion(RegionCoord regionCoordinates);
    void    onPlayerChunkChanged();
};