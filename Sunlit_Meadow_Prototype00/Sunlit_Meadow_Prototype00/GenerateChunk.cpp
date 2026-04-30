#include "Globals.h"
#include "BlockManager.h"
#include "GenerateChunk.h"

bool generateChunk(Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE], ChunkCoord chunkCoordinates) {

	int heightmap[CHUNK_SIZE][CHUNK_SIZE] = { 0 };

	//generate shape: air/stone
	generateShape(blockIDs, chunkCoordinates, heightmap);

	//generate biomes
	//TODO

	//generate features: grass, vegitation, structures
	generateFeatures(blockIDs, chunkCoordinates, heightmap);

	return true;
}

void generateShape(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
){
	for (int x = 0; x < CHUNK_SIZE; x++) {
		int xAbs = chunkCoordinates.x * CHUNK_SIZE + x;
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int yAbs = chunkCoordinates.y * CHUNK_SIZE + y;

			float zGenerated = standartNoise.GetNoise((float)xAbs, (float)yAbs);

			int zShape = (int)(10 + zGenerated * 10);
			heightmap[x][y] = zShape;

			for (int z = 0; z < CHUNK_SIZE; z++) {
				int zAbs = chunkCoordinates.z * CHUNK_SIZE + z;

				Block* block;

				if (zAbs <= zShape) {
					block = blockManager.getByName("cobble_stone");
				}
				else {
					block = blockManager.getByName("air");
				}

				if (block != nullptr)
					blockIDs[x][y][z] = block->getID();
				else
					SDL_Log("Block = nullptr in Chunk generation!!!");
			}
		}
	}
}

void generateFeatures(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
) {
	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int zGround = heightmap[x][y] - chunkCoordinates.z * CHUNK_SIZE;
			Block* block = nullptr

			


			block = blockManager.getByName("cobble_stone");

			if (block != nullptr)
				blockIDs[x][y][z] = block->getID();
		}
	}
}
