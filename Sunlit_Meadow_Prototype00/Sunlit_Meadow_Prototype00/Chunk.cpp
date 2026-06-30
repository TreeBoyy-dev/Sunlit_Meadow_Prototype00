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

void Chunk::transferMeshesFrom(Chunk& src) {
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

void Chunk::setBlockId(int x, int y, int z, Uint16 id) {
	if (x < 0 || x > 15 ||
		y < 0 || y > 15 ||
		z < 0 || z > 15)
		SDL_Log("[Chunk] couldn't get BlockID at: %d:%d:%d in chunk %d:%d:%d",
			x, y, z, chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
	else
		storage.set(x, y, z, id);
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
