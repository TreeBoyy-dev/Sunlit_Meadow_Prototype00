#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "PalettedGrid2D.h"
#include "Chunk.h"
#include "ChunkGeneratorWorker.h"
#include "ChunkMeshWorker.h"

class WorldGenRegistry;

class Region {
private:
    RegionCoord regionCoordinates;

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;

    std::unordered_set<ColumnCoord, ColumnCoordHash>    requestedColumns;
    std::unordered_set<ChunkCoord,  ChunkCoordHash>     pendingMeshChunks;
    std::unordered_set<ChunkCoord,  ChunkCoordHash>     meshedChunks;

    ChunkGeneratorWorker g_worker;
    ChunkMeshWorker m_worker;

    BlockManager* m_blockManager = nullptr;
    FastNoiseLite* m_standartNoise = nullptr;

    // ---- layer / zone / biome ----
    // All three are IMMUTABLE after the constructor finishes, and the
    // constructor builds them BEFORE starting the workers — that ordering
    // is what makes the generator worker's lock-free reads safe.
    const LayerDef* m_layer = nullptr;            // registry-owned, never freed here
    PalettedGrid2D  zoneMap  { MAP_GRID_SIZE };   // 136x136 cells incl. 1-chunk apron per side
    PalettedGrid2D  biomeMap { MAP_GRID_SIZE };

    Uint64 m_worldSeed = 0;

public:
    Region(
        RegionCoord regionCoordinates,
        BlockManager* blockManager,
        FastNoiseLite* standartNoise,
        const WorldGenRegistry* worldGenRegistry,
        Uint64 worldSeed);
    ~Region();

    Chunk* getChunk(ChunkCoord chunkCoordinates);

    void requestChunkGeneration(ChunkCoord chunkCoordinates);
    void cancelChunkGeneration(ChunkCoord chunkCoordinates);

    bool update(AppState* state,
        SDL_GPUTexture* textureArray,
        std::vector<ChunkCoord>& outNewlyReady);

    RegionCoord getCoordinates();
    void destroyRegion(AppState* state);

    void queueMeshUpdate(ChunkCoord coord);

    // ---- layer / zone / biome reads ----
    // Region-LOCAL block coordinates. The apron makes the valid range
    // [-CHUNK_SIZE, 512 + CHUNK_SIZE) on both axes; anything outside
    // that logs and returns 0.
    void setShape(RegionShape* shape);
    Uint16 getZoneIdLocal (int lbx, int lby) const;
    Uint16 getBiomeIdLocal(int lbx, int lby) const;
    const LayerDef* getLayer() const;

private:
    bool collectMeshResults(AppState* state,
        SDL_GPUTexture* textureArray,
        std::vector<ChunkCoord>& outNewlyReady);
};
