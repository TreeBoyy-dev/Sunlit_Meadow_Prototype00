#pragma once

#include "Block.h"
#include "BlockMesh.h"

//chunksize in blocks -> side of a cube
#define CHUNK_SIZE 16

// worldcoordinates /CHUNK_SIZE
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

class Chunk {
private:
    bool isGenerated;
    ChunkCoord chunkCoordinates;
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	//Biome* biome;
	//Zone* zone;
	//Layer* layer;

	BlockMesh opaqueMesh;
	BlockMesh transparentMesh;
    bool drawOpaqueMesh;
    bool drawTransparentMesh;
public:
    Chunk(ChunkCoord chunkCoordinates);

    bool generateChunk();
    void generateShape(Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE], ChunkCoord chunkCoordinates);

    bool initMeshes(
        AppState* state,
        SDL_GPUTexture* textureArray
    );
    bool drawMeshes(
        AppState* state,
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const UBO& ubo
    );
    void destroyMeshes(AppState* state);
};
