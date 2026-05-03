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

void Chunk::createMeshes(ChunkBorderAir borderAir) {
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
		opaqueMesh.buildMesh(opaqueblocks, borderAir);
	}
	if (transparentblocks.size() > 0) {
		drawTransparentMesh = true;
		transparentMesh.buildMesh(transparentblocks, borderAir);
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

	for (int a = 0; a < CHUNK_SIZE; a++) {
		for (int b = 0; b < CHUNK_SIZE; b++) {
			// front/back: remaining axes are [y][z]
			borderAir.front	 [a][b] = (blockIDs[CHUNK_SIZE - 1][a][b] == 0) ? true : false; // x+
			borderAir.back	 [a][b] = (blockIDs[0][a][b] == 0)				? true : false; // x-

			// right/left: remaining axes are [x][z]
			borderAir.right	 [a][b] = (blockIDs[a][CHUNK_SIZE - 1][b] == 0) ? true : false; // y+
			borderAir.left	 [a][b] = (blockIDs[a][0][b] == 0)				? true : false; // y-

			// top/bottom: remaining axes are [x][y]
			borderAir.top	 [a][b] = (blockIDs[a][b][CHUNK_SIZE - 1] == 0) ? true : false; // z+
			borderAir.bottom [a][b] = (blockIDs[a][b][0] == 0)				? true : false;	// z-
		}
	}
}

// Pass direction as e.g. {1,0,0}, {-1,0,0}, {0,1,0} ...
// Returns a pointer to the [CHUNK_SIZE][CHUNK_SIZE] face, or nullptr if invalid.
bool (*Chunk::getBorderAir(ChunkCoord direction))[CHUNK_SIZE] {
	if      (direction.x ==  1) return borderAir.front;
	else if (direction.x == -1) return borderAir.back;
	else if (direction.y ==  1) return borderAir.right;
	else if (direction.y == -1) return borderAir.left;
	else if (direction.z ==  1) return borderAir.top;
	else if (direction.z == -1) return borderAir.bottom;
	else                        return nullptr;
}