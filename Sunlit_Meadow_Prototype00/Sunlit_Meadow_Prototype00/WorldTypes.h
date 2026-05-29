#pragma once
#include <SDL3/SDL.h>
#include <functional>
#include "Vectors.h"

#define CHUNK_SIZE 16
#define REGION_SIZE_YX 32
#define REGION_SIZE_Z 16

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

struct ChunkBorderAir{
    Uint16 front  [CHUNK_SIZE][CHUNK_SIZE]; // x+
    Uint16 back   [CHUNK_SIZE][CHUNK_SIZE]; // x-
    Uint16 right  [CHUNK_SIZE][CHUNK_SIZE]; // y+
    Uint16 left   [CHUNK_SIZE][CHUNK_SIZE]; // y-
    Uint16 top    [CHUNK_SIZE][CHUNK_SIZE]; // z+
    Uint16 bottom [CHUNK_SIZE][CHUNK_SIZE]; // z-

    ChunkBorderAir() {
        memset(front, 0, sizeof(front));
        memset(back, 0, sizeof(back));
        memset(right, 0, sizeof(right));
        memset(left, 0, sizeof(left));
        memset(top, 0, sizeof(top));
        memset(bottom, 0, sizeof(bottom));
    }
};