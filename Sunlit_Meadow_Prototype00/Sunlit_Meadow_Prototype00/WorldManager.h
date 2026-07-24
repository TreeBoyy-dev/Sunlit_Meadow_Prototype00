#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "Chunk.h"
#include "Region.h"
#include "FastNoiseLite.h"
#include "BlockManager.h"
#include "WorldTypes.h"
#include "WorldGenRegistry.h"
#include "WorldGenNoise.h"
#include "RegionGeneratorWorker.h"

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
    SDL_GPUGraphicsPipeline* pipeline_op = nullptr;
    SDL_GPUGraphicsPipeline* pipeline_tr = nullptr;

    SDL_GPUTexture* textureArray = nullptr;

    std::unordered_map<RegionCoord, std::unique_ptr<Region>, RegionCoordHash> regions;
    RegionGeneratorWorker regionWorker;

    std::vector<ChunkCoord> visibleChunkCoordsRelative;
    std::unordered_set<ChunkCoord, ChunkCoordHash> visibleRelativeSet;

    std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash> renderList;
    std::unordered_set<ChunkCoord, ChunkCoordHash> pendingChunks;

    int        m_renderDistance = 0;
    ChunkCoord m_lastPlayerChunkPos = { 1000, 1000, 10000 };

    BlockManager* blockManager;
    FastNoiseLite standartNoise;

    // Worldgen definitions (layers/zones/biomes). Populated once in init(),
    // read-only afterwards — regions hold raw pointers into it, so it must
    // outlive them (it does: same owner, destroyed after regions.clear()).
    WorldGenRegistry worldGenRegistry;

    // Shape-generation noise bundle. init-once from m_worldSeed, then
    // read-only — worker threads sample it lock-free (WorldGenNoise.h).
    // standartNoise stays on its hardcoded 1337 seed so the feature pass
    // is untouched; this is the first real consumer of the world seed.
    WorldGenNoise worldGenNoise;

    Uint64 m_worldSeed = 0;   // hardcoded in init() for now

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

    bool    isBlockSolid(int bx, int by, int bz);
    void    setBlockIdAt(AppState* state, Vec3 pos, Uint16 id, Uint16 blockState = 0);
    Uint16  getBlockIdAt(Vec3 pos);
    Vec3    getBlockLookingAt(
        Camera cam,
        const float MAX_REACH,
        int* outFace = nullptr);
    Collision* getBlockCollision(Vec3 pos);

    // ---- layer / zone / biome ----
    // World-space queries, mirroring getBlockIdAt: resolve the region,
    // convert to region-local block coords, delegate. Interior only — any
    // world position converts into the region's own [0, 512) range.
    Uint16 getZoneIdAt (Vec3 pos);
    Uint16 getBiomeIdAt(Vec3 pos);
    const LayerDef* getLayerAt(Vec3 pos);

private:
    void    updatePlayerPosition(Vec3 playerPosition);

    void    drawChunks(AppState*, SDL_GPUCommandBuffer*, SDL_GPURenderPass*, const UBO&);

    Region* getRegion(RegionCoord regionCoordinates);
    void    onPlayerChunkChanged();
};
