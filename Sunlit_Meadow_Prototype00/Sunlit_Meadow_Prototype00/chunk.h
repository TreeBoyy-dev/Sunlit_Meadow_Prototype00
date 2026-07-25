#pragma once

#include "ChunkMesh.h"
#include "WorldTypes.h"
#include "BlockManager.h"
#include "FastNoiseLite.h"
#include "PalettedContainer.h"
#include "StateLayout.h"

//chunksize in blockIDs -> side of a cube
#define CHUNK_SIZE 16

// What lives in a fluid cell. Bit 15 of the BLOCK state only says "a fluid
// is in this cell"; which fluid and what level live here, in the chunk's
// second PalettedContainer. id 0 = no fluid.
struct FluidCell {
    Uint16 fluidId = 0;
    Uint16 level = 0;
};

class Chunk {
private:
    bool isGenerated;
    ChunkCoord chunkCoordinates;
    PalettedContainer storage;

    // Fluid storage: same paletted trick as blocks — a chunk with no fluids
    // has a 1-entry palette and costs almost nothing. PaletteEntry is reused
    // as { id = fluidId, state = level }. Kept in sync with bit 15 of the
    // block state by setFluid() (the ONE code path that touches both).
    PalettedContainer fluidStorage;
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
    void   setBlockId(int x, int y, int z, Uint16 id, Uint16 blockState);
    ChunkCoord getChunkCoordinates();

    // ---- fluids ----
    // THE one code path that touches both stores: writes the fluid container
    // AND flips bit 15 (STATE_FLUID_MASK) of the block state so they can
    // never drift apart. fluidId 0 = remove the fluid.
    void      setFluid(int x, int y, int z, Uint16 fluidId, Uint16 level);
    // Cheap path: only consults the fluid container when the block state's
    // fluid bit is set — the 99% empty case is a single palette read.
    FluidCell getFluid(int x, int y, int z);
    bool      hasFluid(int x, int y, int z);

    // Future home of connection-bit updates (connect4 / wallSide4 for fences
    // and walls). Declared now so callers have a stable name; intentionally
    // empty this pass — see the blockstate plan §6.
    void onNeighborChanged(int x, int y, int z);

    void createMeshes(BlockManager& blockManager);
    void optimizeMeshes();

    // Prints ONE line combining storage shape (palette size, bits/index) with
    // both meshes' BuildStats. Called by the workers after create+optimize,
    // gated by the logMeshStats global.
    void logMeshStats() const;
    size_t  getPaletteSize() const { return storage.paletteSize(); }
    uint8_t getStorageBits() const { return storage.bitsPerIndex(); }
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
