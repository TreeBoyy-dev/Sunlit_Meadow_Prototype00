#pragma once
#include <SDL3/SDL.h>
#include <functional>
#include "Vectors.h"

#define CHUNK_SIZE 16
#define REGION_SIZE_YX 32
#define REGION_SIZE_Z 32

#define COLUMN_HEIGHT (CHUNK_SIZE * REGION_SIZE_Z)

typedef struct {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    SDL_FColor color;
    float materialIndex;
}WorldVertex;

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

struct ChunkCoord {
    int x, y, z;
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};
struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const {
        return std::hash<int>()(c.x) ^
            (std::hash<int>()(c.y) << 1) ^
            (std::hash<int>()(c.z) << 2);
    }
};

struct ColumnCoord {
    int x, y;
    bool operator==(const ColumnCoord& other) const {
        return x == other.x && y == other.y;
    }
};
struct ColumnCoordHash {
    size_t operator()(const ColumnCoord& c) const {
        return std::hash<int>()(c.x) ^
            (std::hash<int>()(c.y) << 1);
    }
};

struct LocationalBlockID {
    int x, y, z;
    Uint16 id;
    bool operator==(const LocationalBlockID& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};
struct LocationalBlockIDHash {
    size_t operator()(const LocationalBlockID& c) const {
        return std::hash<int>()(c.x) ^
            (std::hash<int>()(c.y) << 1) ^
            (std::hash<int>()(c.z) << 2);
    }
};