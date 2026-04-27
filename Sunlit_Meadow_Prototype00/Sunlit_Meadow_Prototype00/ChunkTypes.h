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