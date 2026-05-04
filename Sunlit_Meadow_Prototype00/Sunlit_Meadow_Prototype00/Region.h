#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

#include "ChunkTypes.h"
#include "Chunk.h"
#include "ChunkGeneratorWorker.h"
#include "ChunkMeshWorker.h"

#define REGION_SIZE_YX 32
#define REGION_SIZE_Z 16

struct RegionCoord {
    int x, y, z;

    bool operator==(const RegionCoord& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct RegionCoordHash {
    size_t operator()(const RegionCoord& c) const {
        return std::hash<int>()(c.x) ^
            (std::hash<int>()(c.y) << 1) ^
            (std::hash<int>()(c.z) << 2);
    }
};

class Region {
private:
    RegionCoord regionCoordinates;

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;

    std::unordered_set<ChunkCoord, ChunkCoordHash> pendingChunks;
    std::unordered_set<ChunkCoord, ChunkCoordHash> pendingMeshChunks;
    std::unordered_set<ChunkCoord, ChunkCoordHash> meshedChunks;

    ChunkGeneratorWorker g_worker;
    ChunkMeshWorker m_worker;
public:
    Region(RegionCoord regionCoordinates);
    ~Region();

    Chunk* getChunk(ChunkCoord chunkCoordinates);
    bool update(AppState* state, SDL_GPUTexture* textureArray);

    RegionCoord getCoordinates();
    void destroyRegion(AppState* state);

private:
    bool buildBorderAir(ChunkBorderAir* border, ChunkCoord coord);
    void queueMeshUpdate(ChunkCoord coord);
    bool collectMeshResults(AppState* state, SDL_GPUTexture* textureArray);
};