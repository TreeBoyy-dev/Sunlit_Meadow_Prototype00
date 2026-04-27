#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

#include "ChunkTypes.h"
#include "Chunk.h"

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
public:
    Region(RegionCoord regionCoordinates);

    Chunk* getChunk(
        ChunkCoord chunkCoordinates,
        AppState* state,
        SDL_GPUTexture* textureArray
    );
    RegionCoord getCoordinates();
    void destroyRegion(AppState* state);
};