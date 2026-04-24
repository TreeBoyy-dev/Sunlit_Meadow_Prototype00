#include "Chunk.h"
#include "StoneBlocks.h"

Chunk::Chunk(ChunkCoord chunkCoordinates) :
	chunkCoordinates(chunkCoordinates),
	isGenerated(false),
	drawOpaqueMesh(false),
	drawTransparentMesh(false)
{}

bool Chunk::initMeshes(
	AppState* state,
	SDL_GPUTexture* textureArray
) {
	std::vector<Block> opaqueblocks;
	std::vector<Block> transparentblocks;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {

				if (blocks[x][y][z].getTransperency())
					transparentblocks.push_back(blocks[x][y][z]);
				else
					opaqueblocks.push_back(blocks[x][y][z]);
			}
		}
	}

	opaqueMesh.init(
		state,
		opaqueblocks,
		textureArray
	);
	transparentMesh.init(
		state,
		transparentblocks,
		textureArray
	);
}
bool Chunk::drawMeshes(
	AppState* state,
	SDL_GPUCommandBuffer* cmd,
	SDL_GPURenderPass* pass,
	const UBO& ubo
) {
	opaqueMesh.draw(
		state,
		cmd,
		pass,
		ubo
	);
	transparentMesh.draw(
		state,
		cmd,
		pass,
		ubo
	);
}
void Chunk::destroyMeshes(AppState* state) {
	opaqueMesh.destroy(state);
	transparentMesh.destroy(state);
}


bool Chunk::generateChunk() {

	//generate shape: air/stone
	generateShape(blocks, chunkCoordinates);

	//generate biomes

	//generate features: grass, vegitation, structures

	return true;
}

void Chunk::generateShape(Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE], ChunkCoord chunkCoordinates)
{
	for (int x = 0; x < CHUNK_SIZE; x++) {
		int xAbs = chunkCoordinates.x + x;
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int yAbs = chunkCoordinates.y + y;
			for (int z = 0; z < CHUNK_SIZE; z++) {
				int zAbs = chunkCoordinates.z + z;

				if (zAbs > 0 && zAbs < 3)
				{
					blocks[x][y][z] = Cobblestone_MinableBLock({ xAbs, yAbs, zAbs });
				}
				else
				{
					blocks[x][y][z] = Block();
				}
			}
		}
	}
}