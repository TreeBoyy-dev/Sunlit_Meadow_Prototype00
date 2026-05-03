#pragma once
#include <SDL3/SDL.h>
#include <functional>

#define CHUNK_SIZE 16

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
    bool front  [CHUNK_SIZE][CHUNK_SIZE]; // x+
    bool back   [CHUNK_SIZE][CHUNK_SIZE]; // x-
    bool right  [CHUNK_SIZE][CHUNK_SIZE]; // y+
    bool left   [CHUNK_SIZE][CHUNK_SIZE]; // y-
    bool top    [CHUNK_SIZE][CHUNK_SIZE]; // z+
    bool bottom [CHUNK_SIZE][CHUNK_SIZE]; // z-

    ChunkBorderAir() {
        memset(front, true, sizeof(front));
        memset(back, true, sizeof(back));
        memset(right, true, sizeof(right));
        memset(left, true, sizeof(left));
        memset(top, true, sizeof(top));
        memset(bottom, true, sizeof(bottom));
    }
};