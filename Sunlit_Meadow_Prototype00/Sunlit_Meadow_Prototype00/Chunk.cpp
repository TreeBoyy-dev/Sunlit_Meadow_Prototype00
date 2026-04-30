#include "GenerateChunk.h"
#include "Chunk.h"
#include "Globals.h"
#include "BlockManager.h"

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

void Chunk::createMeshes() {
	std::vector<LocationalBlockID> opaqueblocks;
	std::vector<LocationalBlockID> transparentblocks;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {

				LocationalBlockID absLocationalBlockID = {
					x + chunkCoordinates.x * CHUNK_SIZE,
					y + chunkCoordinates.y * CHUNK_SIZE,
					z + chunkCoordinates.z * CHUNK_SIZE,
					blockIDs[x][y][z]
				};
				Block* block = blockManager.getById(blockIDs[x][y][z]);

				if (block == nullptr)
					SDL_Log("Block = nullptr in Chunk init meshes!!!");
				else if (!block->isTransparent())
					opaqueblocks.push_back(absLocationalBlockID);
				else if (block->getName() != "air")
					transparentblocks.push_back(absLocationalBlockID);
			}
		}
	}
	if (opaqueblocks.size() > 0) {
		drawOpaqueMesh = true;
		opaqueMesh.buildMesh(opaqueblocks);
	}
	if (transparentblocks.size() > 0) {
		drawTransparentMesh = true;
		transparentMesh.buildMesh(transparentblocks);
	}
}


bool Chunk::uploadMeshes(AppState* state, SDL_GPUTexture* textureArray) {
	if (drawOpaqueMesh) {
		if (!opaqueMesh.uploadToGPU(state, textureArray)) {
			SDL_Log("failed to upload opaqueMesh at %d|%d|%d",
				chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
		}
	}
	if (drawTransparentMesh) {
		if (!transparentMesh.uploadToGPU(state, textureArray)) {
			SDL_Log("failed to upload transparentMesh at %d|%d|%d",
				chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
		}
	}
	return true;
}

bool Chunk::drawMeshes(
	AppState* state,
	SDL_GPUCommandBuffer* cmd,
	SDL_GPURenderPass* pass,
	const UBO& ubo
) {
	if(drawOpaqueMesh) opaqueMesh.draw(
		state,
		cmd,
		pass,
		ubo
	);
	if (drawTransparentMesh) transparentMesh.draw(
		state,
		cmd,
		pass,
		ubo
	);
	return true;
}
void Chunk::destroyMeshes(AppState* state) {
	opaqueMesh.destroy(state);
	transparentMesh.destroy(state);
}

bool Chunk::getIsGenerated() {
	return isGenerated;
}
ChunkCoord Chunk::getChunkCoordinates() {
	return chunkCoordinates;
}

void Chunk::getChunkGenerated() {
	generateChunk(blockIDs, chunkCoordinates);
	isGenerated = true;
}
