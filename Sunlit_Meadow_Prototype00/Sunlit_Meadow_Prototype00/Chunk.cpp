#include "GenerateChunk.h"
#include "Chunk.h"
#include "Globals.h"

Chunk::Chunk() :
	chunkCoordinates({0,0,0}),
	isGenerated(false),
	drawOpaqueMesh(false),
	drawTransparentMesh(false)
{}

Chunk::Chunk(ChunkCoord chunkCoordinates) :
	chunkCoordinates(chunkCoordinates),
	isGenerated(false),
	drawOpaqueMesh(false),
	drawTransparentMesh(false)
{}

Chunk::Chunk(Chunk* other) :
	chunkCoordinates(other->getChunkCoordinates()),
	isGenerated(other->getIsGenerated()),
	storage(other->storage),
	drawOpaqueMesh(false),
	drawTransparentMesh(false)
{}

Chunk::Chunk(ChunkCoord chunkCoordinates, PalettedContainer storage) :
	chunkCoordinates(chunkCoordinates),
	isGenerated(true),
	storage(std::move(storage)),
	drawOpaqueMesh(false),
	drawTransparentMesh(false)
{}

void Chunk::transferMeshesFrom(Chunk& src) {
	opaqueMesh = std::move(src.opaqueMesh);
	drawOpaqueMesh = src.drawOpaqueMesh;
	transparentMesh = std::move(src.transparentMesh);
	drawTransparentMesh = src.drawTransparentMesh;
}

void Chunk::createMeshes(BlockManager& blockManager) {
	drawOpaqueMesh = true;
	opaqueMesh.buildMesh(&storage, chunkCoordinates, blockManager, false);

	drawTransparentMesh = true;
	transparentMesh.buildMesh(&storage, chunkCoordinates, blockManager, true);
}

void Chunk::optimizeMeshes() {
	opaqueMesh.optimizeMesh();
	transparentMesh.optimizeMesh();
}

bool Chunk::uploadMeshes(AppState* state, SDL_GPUTexture* textureArray) {
	if (drawOpaqueMesh) {
		if (!opaqueMesh.uploadToGPU(state, textureArray)) {
			//SDL_Log("failed to upload opaqueMesh at %d|%d|%d",
			//	chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
		}
	}
	if (drawTransparentMesh) {
		if (!transparentMesh.uploadToGPU(state, textureArray)) {
			//SDL_Log("failed to upload transparentMesh at %d|%d|%d",
			//	chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
		}
	}
	//SDL_Log("uploaded Meshes at %d|%d|%d",
	//	chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
	return true;
}

bool Chunk::drawMeshes(
	AppState* state,
	SDL_GPUCommandBuffer* cmd,
	SDL_GPURenderPass* pass,
	const UBO& ubo
) {
	if (drawOpaqueMesh)	
	{
		opaqueMesh.draw(state, cmd, pass, ubo);
	}
	if (drawTransparentMesh)
	{
		transparentMesh.draw(state, cmd, pass, ubo);
	}
	return true;
}

void Chunk::destroyMeshes(AppState* state) {
	opaqueMesh.destroy(state);
	drawOpaqueMesh = false;
	transparentMesh.destroy(state);
	drawTransparentMesh = false;
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

ChunkCoord Chunk::getChunkCoordinates() {
	return chunkCoordinates;
}

/*
void Chunk::getChunkGenerated(BlockManager& blockManager, FastNoiseLite& standartNoise) {
	Uint16 dense[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	generateChunk(dense, chunkCoordinates, blockManager, standartNoise);
	storage.fromDenseIds(&dense[0][0][0]);
	isGenerated = true;
}//*/
