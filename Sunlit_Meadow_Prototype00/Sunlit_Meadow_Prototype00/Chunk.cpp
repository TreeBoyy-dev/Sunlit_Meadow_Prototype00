#include "Chunk.h"
#include "StoneBlocks.h"
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

bool Chunk::initMeshes(
	AppState* state,
	SDL_GPUTexture* textureArray
) {
	std::vector<Uint16> opaqueblocks;
	std::vector<Uint16> transparentblocks;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				Block* b= blockManager.getById(blocks[x][y][z]);

				if (b.getTransperency())
					transparentblocks.push_back(b);
				else
					opaqueblocks.push_back(b);
			}
		}
	}
	if (opaqueblocks.size() > 0) {
		drawOpaqueMesh = true;
		if (!opaqueMesh.init(
			state,
			opaqueblocks,
			textureArray
		)) {
			//return false;
			SDL_Log("failed to init opaqueMesh at %d|%d|%d",
				chunkCoordinates.x,
				chunkCoordinates.y,
				chunkCoordinates.z
			);
		}
	}
	if (transparentblocks.size() > 0) {
		drawTransparentMesh = true;
		if (!transparentMesh.init(
			state,
			transparentblocks,
			textureArray
		)) {
			//return false;
			SDL_Log("failed to init transparentMesh at %d|%d|%d",
				chunkCoordinates.x,
				chunkCoordinates.y,
				chunkCoordinates.z
			);
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
		int xAbs = chunkCoordinates.x * CHUNK_SIZE + x;
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int yAbs = chunkCoordinates.y * CHUNK_SIZE + y;
			for (int z = 0; z < CHUNK_SIZE; z++) {
				int zAbs = chunkCoordinates.z * CHUNK_SIZE + z;

				if (zAbs <=2) {
					blocks[x][y][z] = Cobblestone_MinableBlock({ xAbs, yAbs, zAbs });
				}
				else if (zAbs == 3) {
					if (rand() % 2 == 0)
						blocks[x][y][z] = Cobblestone_MinableBlock({ xAbs, yAbs, zAbs });
					else
						blocks[x][y][z] = Dirt_MinableBlock({ xAbs, yAbs, zAbs });
				}
				else if (zAbs == 4) {
					int r = rand() % 3;

					if (r == 0)
						blocks[x][y][z] = Cobblestone_MinableBlock({ xAbs, yAbs, zAbs });
					else if (r == 1)
						blocks[x][y][z] = Dirt_MinableBlock({ xAbs, yAbs, zAbs });
					else
						blocks[x][y][z] = Block();
				}
				//else if (zAbs > 20) {
				//	blocks[x][y][z] = Diorite_MinableBlock({ xAbs, yAbs, zAbs });
				//}
				else {
					blocks[x][y][z] = Block();
				}
			}
		}
	}
}