#include "Globals.h"
#include "BlockManager.h"
#include "GenerateChunk.h"

bool generateChunk(Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE], ChunkCoord chunkCoordinates) {

	//generate shape: air/stone
	generateShape(blockIDs, chunkCoordinates);

	//generate biomes
	//TODO

	//generate features: grass, vegitation, structures
	//TODO

	return true;
}

void generateShape(Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE], ChunkCoord chunkCoordinates)
{
	for (int x = 0; x < CHUNK_SIZE; x++) {
		int xAbs = chunkCoordinates.x * CHUNK_SIZE + x;
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int yAbs = chunkCoordinates.y * CHUNK_SIZE + y;

			float zGenerated = standartNoise.GetNoise((float)xAbs, (float)yAbs);

			int zShape = (int)(10 + zGenerated * 10);

			for (int z = 0; z < CHUNK_SIZE; z++) {
				int zAbs = chunkCoordinates.z * CHUNK_SIZE + z;

				Block* block;

				if (zAbs <= zShape) {
					if(zAbs%2)
						block = blockManager.getByName("dirt");
					else
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