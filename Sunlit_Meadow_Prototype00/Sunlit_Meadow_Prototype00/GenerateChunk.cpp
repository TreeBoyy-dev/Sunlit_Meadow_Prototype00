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
	generateFeatures_GrassAndDirt(
		blockIDs,
		chunkCoordinates,
		heightmap
	);
	generateFeatures_Trees(
		blockIDs,
		chunkCoordinates,
		heightmap
	);
	generateFeatures_Boulders(
		blockIDs,
		chunkCoordinates,
		heightmap
	);
}

void generateFeatures_GrassAndDirt(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
) {
	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int zGround = heightmap[x][y] - chunkCoordinates.z * CHUNK_SIZE;
			if (zGround < 0 || zGround >= CHUNK_SIZE)
				continue;

			Block* block = nullptr;

			block = blockManager.getByName("grass_block");
			if (block != nullptr)
			{
				blockIDs[x][y][zGround] = block->getID(); block = nullptr;
			}
			block = blockManager.getByName("dirt");
			if (block != nullptr)
			{
				if (zGround - 1 > 0) blockIDs[x][y][zGround - 1] = block->getID();
				if (zGround - 2 > 0) blockIDs[x][y][zGround - 2] = block->getID();
				if (zGround - 3 > 0) blockIDs[x][y][zGround - 3] = block->getID();
				block = nullptr;
			}
		}
	}
}
void generateFeatures_Trees(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
)
{
	// 1 in 5 chance to generate a boulder
	if (rand() % 2 != 0)
		return;

	Block* Log;
	Block* Leave;
	if (rand() % 2)
	{
		Log = blockManager.getByName("birch_log");
		Leave = blockManager.getByName("birch_leaves");
	}
	else
	{
		Log = blockManager.getByName("chestnut_log");
		Leave = blockManager.getByName("chestnut_leaves");
	}
	if (Log == nullptr || Leave == nullptr)
		return;

	// Random boulder origin between 3 and 11
	int bx = 3 + rand() % 9;
	int by = 3 + rand() % 9;

	int zGround = heightmap[bx][by] - chunkCoordinates.z * CHUNK_SIZE;
	if (zGround < 0 || zGround + 7 >= CHUNK_SIZE)
		return;

	// 5x5x5 cube with corners cut out (where |dx|==2 && |dy|==2)
	for (int dz = 0; dz <= 7; dz++)
	{
		for (int dx = -2; dx <= 2; dx++)
		{
			for (int dy = -2; dy <= 2; dy++)
			{
				// Skip the 4 vertical edge corners to round it out
				if (abs(dx) == 2 && abs(dy) == 2)
					continue;
				if (dz == 7 && (abs(dx) == 2 || abs(dy) == 2))
					continue;
				if ((dx != 0 || dy != 0) && dz <= 3)
					continue;

				int wx = bx + dx;
				int wy = by + dy;
				int wz = zGround + dz;

				if (wx < 0 || wx >= CHUNK_SIZE) continue;
				if (wy < 0 || wy >= CHUNK_SIZE) continue;
				if (wz < 0 || wz >= CHUNK_SIZE) continue;

				if (dx == 0 && dy == 0 && dz != 7)
					blockIDs[wx][wy][wz] = Log->getID();
				else
					blockIDs[wx][wy][wz] = Leave->getID();
			}
		}
	}
}
void generateFeatures_Boulders(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
)
{
	// 1 in 5 chance to generate a boulder
	if (rand() % 5 != 0)
		return;

	Block* stoneBlock = blockManager.getByName("gneiss");
	if (stoneBlock == nullptr)
		return;

	// Random boulder origin between 3 and 11
	int bx = 3 + rand() % 9;
	int by = 3 + rand() % 9;

	int zGround = heightmap[bx][by] - chunkCoordinates.z * CHUNK_SIZE;
	if (zGround - 2 < 0 || zGround + 3 >= CHUNK_SIZE)
		return;

	// 5x5x5 cube with corners cut out (where |dx|==2 && |dy|==2)
	for (int dz = -2; dz <= 2; dz++)
	{
		for (int dx = -2; dx <= 2; dx++)
		{
			for (int dy = -2; dy <= 2; dy++)
			{
				// Skip the 4 vertical edge corners to round it out
				if (abs(dx) == 2 && abs(dy) == 2)
					continue;
				if (abs(dz) == 2 && (abs(dx) == 2 || abs(dy) == 2))
					continue;

				int wx = bx		 + dx;
				int wy = by		 + dy;
				int wz = zGround + dz;

				if (wx < 0 || wx >= CHUNK_SIZE) continue;
				if (wy < 0 || wy >= CHUNK_SIZE) continue;
				if (wz < 0 || wz >= CHUNK_SIZE) continue;

				blockIDs[wx][wy][wz] = stoneBlock->getID();
			}
		}
	}
}
