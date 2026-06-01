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
{
	// Fill the border-air faces straight from the storage, the same way
	// getChunkGenerated used to read them out of the dense array.
	for (int a = 0; a < CHUNK_SIZE; a++) {
		for (int b = 0; b < CHUNK_SIZE; b++) {
			// front/back: remaining axes are [y][z]
			borderAir.front[a][b] = this->storage.getId(CHUNK_SIZE - 1, a, b);
			borderAir.back[a][b] = this->storage.getId(0, a, b);
			// right/left: remaining axes are [x][z]
			borderAir.right[a][b] = this->storage.getId(a, CHUNK_SIZE - 1, b);
			borderAir.left[a][b] = this->storage.getId(a, 0, b);
			// top/bottom: remaining axes are [x][y]
			borderAir.top[a][b] = this->storage.getId(a, b, CHUNK_SIZE - 1);
			borderAir.bottom[a][b] = this->storage.getId(a, b, 0);
		}
	}
}

void Chunk::transferMeshesFrom(Chunk& src) {
	opaqueMesh = std::move(src.opaqueMesh);
	drawOpaqueMesh = src.drawOpaqueMesh;
	transparentMesh = std::move(src.transparentMesh);
	drawTransparentMesh = src.drawTransparentMesh;
}

void Chunk::createMeshes(ChunkBorderAir borderAir, BlockManager& blockManager) {
	std::vector<LocationalBlockID> opaqueblocks;
	std::vector<LocationalBlockID> transparentblocks;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {

				Uint16 id = storage.getId(x, y, z);
				LocationalBlockID absLocationalBlockID = {
					x + chunkCoordinates.x * CHUNK_SIZE,
					y + chunkCoordinates.y * CHUNK_SIZE,
					z + chunkCoordinates.z * CHUNK_SIZE,
					id
				};
				Block* block = blockManager.getById(id);

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
		opaqueMesh.buildMesh(opaqueblocks, borderAir, chunkCoordinates, blockManager);
	}
	if (transparentblocks.size() > 0) {
		drawTransparentMesh = true;
		transparentMesh.buildMesh(transparentblocks, borderAir, chunkCoordinates, blockManager);
	}
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
ChunkCoord Chunk::getChunkCoordinates() {
	return chunkCoordinates;
}

/*
void Chunk::getChunkGenerated(BlockManager& blockManager, FastNoiseLite& standartNoise) {
	Uint16 dense[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	generateChunk(dense, chunkCoordinates, blockManager, standartNoise);
	storage.fromDenseIds(&dense[0][0][0]);
	isGenerated = true;

	for (int a = 0; a < CHUNK_SIZE; a++) {
		for (int b = 0; b < CHUNK_SIZE; b++) {
			// front/back: remaining axes are [y][z]
			borderAir.front[a][b]	= dense[CHUNK_SIZE - 1][a][b];
			borderAir.back[a][b]	= dense[0][a][b];
			// right/left: remaining axes are [x][z]
			borderAir.right[a][b]	= dense[a][CHUNK_SIZE - 1][b];
			borderAir.left[a][b]	= dense[a][0][b];
			// top/bottom: remaining axes are [x][y]
			borderAir.top[a][b]		= dense[a][b][CHUNK_SIZE - 1];
			borderAir.bottom[a][b]	= dense[a][b][0];
		}
	}
}//*/

// Pass direction as e.g. {1,0,0}, {-1,0,0}, {0,1,0} ...
// Returns a pointer to the [CHUNK_SIZE][CHUNK_SIZE] face, or nullptr if invalid.
Uint16 (*Chunk::getBorderAir(ChunkCoord direction))[CHUNK_SIZE] {
	if      (direction.x ==  1) return borderAir.front;
	else if (direction.x == -1) return borderAir.back;
	else if (direction.y ==  1) return borderAir.right;
	else if (direction.y == -1) return borderAir.left;
	else if (direction.z ==  1) return borderAir.top;
	else if (direction.z == -1) return borderAir.bottom;
	else                        return nullptr;
}