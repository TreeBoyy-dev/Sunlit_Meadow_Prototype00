#pragma once

#include "ChunkMesh.h"
#include "WorldTypes.h"
#include "BlockManager.h"
#include "FastNoiseLite.h"
#include "PalettedContainer.h"

//chunksize in blockIDs -> side of a cube
#define CHUNK_SIZE 16

class Chunk {
private:
    bool isGenerated;
    ChunkCoord chunkCoordinates;
    PalettedContainer storage;
	//Biome* biome;
	//Zone* zone;
	//Layer* layer;

	ChunkMesh opaqueMesh;
	ChunkMesh transparentMesh;
    bool drawOpaque;
    bool drawTransparent;
public:
    Chunk();
    Chunk(ChunkCoord chunkCoordinates);
    Chunk(Chunk* other);
    Chunk(ChunkCoord chunkCoordinates, PalettedContainer storage);
    void transferMeshesFrom(AppState* state, Chunk& src);

    //void getChunkGenerated(BlockManager& blockManager, FastNoiseLite& standartNoise);

    bool getIsGenerated();
    Uint16 getBlockId(int x, int y, int z);
    void   setBlockId(int x, int y, int z, Uint16 id);
    ChunkCoord getChunkCoordinates();

    void createMeshes(BlockManager& blockManager);
    void optimizeMeshes();
    bool uploadMeshes(
        AppState* state,
        SDL_GPUTexture* textureArray
    );
    bool drawOpaqueMesh(
        AppState* state,
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const UBO& ubo
    );
    bool drawTransparentMesh(
        AppState* state,
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const UBO& ubo
    );
    void destroyMeshes(AppState* state);
};
