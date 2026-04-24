#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>

#include "Chunk.h"

#define REGION_SIZE_YX 64
#define REGION_SIZE_Z 16

struct RegionCoord {
    int x, y, z;
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct RegionCoordHash {
    size_t operator()(const ChunkCoord& c) const {
        return std::hash<int>()(c.x) ^
            (std::hash<int>()(c.y) << 1) ^
            (std::hash<int>()(c.z) << 2);
    }
};

class Region {
private:
    RegionCoord regionCoordinates;

public:
    Region();

    void destroyRegion(AppState* state);
};