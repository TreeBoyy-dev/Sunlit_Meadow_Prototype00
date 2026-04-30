#pragma once

#include "ChunkMesh.h"
#include "ChunkTypes.h"

//chunksize in blockIDs -> side of a cube
#define CHUNK_SIZE 16

class Chunk {
private:
    bool isGenerated;
    ChunkCoord chunkCoordinates;
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	//Biome* biome;
	//Zone* zone;
	//Layer* layer;


	ChunkMesh opaqueMesh;
	ChunkMesh transparentMesh;
    bool drawOpaqueMesh;
    bool drawTransparentMesh;
public:
    Chunk();
    Chunk(ChunkCoord chunkCoordinates);

    void getChunkGenerated();

    bool getIsGenerated();
    ChunkCoord getChunkCoordinates();

    void createMeshes();
    bool uploadMeshes(
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
