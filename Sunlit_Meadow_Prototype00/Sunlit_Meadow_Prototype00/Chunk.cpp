#include "GenerateChunk.h"
#include "Chunk.h"
#include "Globals.h"

Chunk::Chunk() :
	chunkCoordinates({0,0,0}),
	isGenerated(false),
	drawOpaque(false),
	drawTransparent(false)
{}

Chunk::Chunk(ChunkCoord chunkCoordinates) :
	chunkCoordinates(chunkCoordinates),
	isGenerated(false),
	drawOpaque(false),
	drawTransparent(false)
{}

Chunk::Chunk(Chunk* other) :
	chunkCoordinates(other->getChunkCoordinates()),
	isGenerated(other->getIsGenerated()),
	storage(other->storage),
	fluidStorage(other->fluidStorage),
	drawOpaque(false),
	drawTransparent(false)
{}

Chunk::Chunk(ChunkCoord chunkCoordinates, PalettedContainer storage) :
	chunkCoordinates(chunkCoordinates),
	isGenerated(true),
	storage(std::move(storage)),
	drawOpaque(false),
	drawTransparent(false)
{}

void Chunk::transferMeshesFrom(AppState* state, Chunk& src) {
	opaqueMesh.destroy(state);        // releases old GPU buffers, clears vectors
	transparentMesh.destroy(state);

	opaqueMesh = std::move(src.opaqueMesh);
	drawOpaque = src.drawOpaque;
	transparentMesh = std::move(src.transparentMesh);
	drawTransparent = src.drawTransparent;
}

void Chunk::createMeshes(BlockManager& blockManager) {
	drawOpaque = true;
	opaqueMesh.buildMesh(&storage, chunkCoordinates, blockManager, false);

	drawTransparent = true;
	transparentMesh.buildMesh(&storage, chunkCoordinates, blockManager, true);
}

void Chunk::optimizeMeshes() {
	//SDL_Log("[Chunk]: optimize mesh at: %d|%d|%d",
	//	chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
	opaqueMesh.optimizeMesh();
	transparentMesh.optimizeMesh();
}

bool Chunk::uploadMeshes(AppState* state, SDL_GPUTexture* textureArray) {
	if (drawOpaque) {
		if (!opaqueMesh.uploadToGPU(state, textureArray)) {
			//SDL_Log("failed to upload opaqueMesh at %d|%d|%d",
			//	chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
		}
	}
	if (drawTransparent) {
		if (!transparentMesh.uploadToGPU(state, textureArray)) {
			//SDL_Log("failed to upload transparentMesh at %d|%d|%d",
			//	chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
		}
	}
	//SDL_Log("uploaded Meshes at %d|%d|%d",
	//	chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
	return true;
}

bool Chunk::drawOpaqueMesh(
	AppState* state,
	SDL_GPUCommandBuffer* cmd,
	SDL_GPURenderPass* pass,
	const UBO& ubo
) {
	if (drawOpaque)
	{
		opaqueMesh.draw(state, cmd, pass, ubo);
	}
	return true;
}
bool Chunk::drawTransparentMesh(
	AppState* state,
	SDL_GPUCommandBuffer* cmd,
	SDL_GPURenderPass* pass,
	const UBO& ubo
) {
	if (drawTransparent)
	{
		transparentMesh.draw(state, cmd, pass, ubo);
	}
	return true;
}

void Chunk::destroyMeshes(AppState* state) {
	opaqueMesh.destroy(state);
	drawOpaque = false;
	transparentMesh.destroy(state);
	drawTransparent = false;
}

bool Chunk::getIsGenerated() {
	return isGenerated;
}

Uint16 Chunk::getBlockId(int x, int y, int z) {
	if (x < 0 || x > 15 ||
		y < 0 || y > 15 ||
		z < 0 || z > 15)
		SDL_Log("[Chunk] couldn't get BlockID at: %d:%d:%d in chunk %d:%d:%d",
			x, y, z, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
	else
		return storage.getId(x, y, z);
}

void Chunk::setBlockId(int x, int y, int z, Uint16 id, Uint16 blockState) {
	if (x < 0 || x > 15 ||
		y < 0 || y > 15 ||
		z < 0 || z > 15)
		SDL_Log("[Chunk] couldn't get BlockID at: %d:%d:%d in chunk %d:%d:%d",
			x, y, z, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
	else {
		storage.set(x, y, z, id, blockState);
		SDL_Log("[Chunk] setting block (id: %d) at: %d:%d:%d in chunk %d:%d:%d",
			id, x, y, z, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
	}
}

ChunkCoord Chunk::getChunkCoordinates() {
	return chunkCoordinates;
}

// ---------------------------------------------------------------------
//  Fluids.
//
//  Bit 15 of the BLOCK state only says "a fluid is in this cell". Which
//  fluid and what level live in fluidStorage, reusing PaletteEntry as
//  { id = fluidId, state = level }. A chunk with no fluids keeps a 1-entry
//  palette (all zero) and costs 0 bytes of cell data — same trick as blocks.
//
//  setFluid() is deliberately the ONLY function that writes either side of
//  this pairing, so the block-state flag and the fluid container can never
//  drift apart.
// ---------------------------------------------------------------------
void Chunk::setFluid(int x, int y, int z, Uint16 fluidId, Uint16 level) {
	if (x < 0 || x > 15 ||
		y < 0 || y > 15 ||
		z < 0 || z > 15) {
		SDL_Log("[Chunk] couldn't set fluid at: %d:%d:%d in chunk %d:%d:%d",
			x, y, z, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
		return;
	}

	// 1) the fluid itself (id 0 = no fluid, level ignored in that case)
	fluidStorage.set(x, y, z, fluidId, fluidId != 0 ? level : 0);

	// 2) mirror it into bit 15 of the block state, preserving the template
	//    bits (rotation, connections, ...) below it.
	Uint16 id = storage.getId(x, y, z);
	Uint16 state = storage.getState(x, y, z);
	if (fluidId != 0) state = (Uint16)(state | STATE_FLUID_MASK);
	else              state = (Uint16)(state & ~STATE_FLUID_MASK);
	storage.set(x, y, z, id, state);
}

FluidCell Chunk::getFluid(int x, int y, int z) {
	FluidCell cell;
	if (x < 0 || x > 15 ||
		y < 0 || y > 15 ||
		z < 0 || z > 15) {
		SDL_Log("[Chunk] couldn't get fluid at: %d:%d:%d in chunk %d:%d:%d",
			x, y, z, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
		return cell;
	}
	// The fluid bit is an optimization: skip the second container lookup for
	// the (usual) case that there is no fluid here.
	if ((storage.getState(x, y, z) & STATE_FLUID_MASK) == 0)
		return cell;

	cell.fluidId = fluidStorage.getId(x, y, z);
	cell.level   = fluidStorage.getState(x, y, z);
	return cell;
}

bool Chunk::hasFluid(int x, int y, int z) {
	if (x < 0 || x > 15 ||
		y < 0 || y > 15 ||
		z < 0 || z > 15)
		return false;
	return (storage.getState(x, y, z) & STATE_FLUID_MASK) != 0;
}

// Future home of the connection-bit write path: when a neighbor changes,
// fences recompute their connect4 bits and walls their wallSide4 bits, then
// the chunk gets remeshed. Intentionally empty this pass (plan §6) — the
// READ path (multipart meshing) is done, so testing is possible by writing
// connection bits manually via setBlockId.
void Chunk::onNeighborChanged(int x, int y, int z) {
	(void)x; (void)y; (void)z;
}

/*
void Chunk::getChunkGenerated(BlockManager& blockManager, FastNoiseLite& standartNoise) {
	Uint16 dense[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	generateChunk(dense, chunkCoordinates, blockManager, standartNoise);
	storage.fromDenseIds(&dense[0][0][0]);
	isGenerated = true;
}//*/
