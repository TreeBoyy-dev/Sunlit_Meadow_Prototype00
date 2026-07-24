#include "Globals.h"
#include "BlockManager.h"
#include "GenerateFeatures.h"
#include "Materials.h"

void generateFeatures(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
) {
	generateFeatures_GrassAndDirt(
		blockIDs,
		columnCoordinates,
		regionChunkZStart,
		heightmap,
		blockManager
	);
	generateFeatures_Trees(
		blockIDs,
		columnCoordinates,
		regionChunkZStart,
		heightmap,
		blockManager
	);
	generateFeatures_Boulders(
		blockIDs,
		columnCoordinates,
		regionChunkZStart,
		heightmap,
		blockManager
	);
	generateFeatures_BlockPallette(
		blockIDs,
		columnCoordinates,
		regionChunkZStart,
		heightmap,
		blockManager
	);
}

void generateFeatures_GrassAndDirt(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
) {
	int columnZStartBlocks = regionChunkZStart * CHUNK_SIZE;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			float zGround = heightmap[x][y] - columnZStartBlocks;
			if (zGround < 0 || zGround >= COLUMN_HEIGHT)
				continue;

			Block* block = nullptr;

			float decimalPart = zGround - floor(zGround);
			if (decimalPart >= 0.5f)
				block = blockManager.getByName("grass_block");
			else
				if (rand() % 200 != 0)
					block = blockManager.getByName("grass_block_slab");
				else
					block = blockManager.getByName("cobble_stone_stair");

			if (block != nullptr)
			{
				blockIDs[x][y][(int)zGround] = block->getID(); block = nullptr;
			}
			block = blockManager.getByName("dirt");
			if (block != nullptr)
			{
				if (zGround - 1 > 0) blockIDs[x][y][(int)zGround - 1] = block->getID();
				if (zGround - 2 > 0) blockIDs[x][y][(int)zGround - 2] = block->getID();
				if (zGround - 3 > 0) blockIDs[x][y][(int)zGround - 3] = block->getID();
				block = nullptr;
			}
		}
	}
}
void generateFeatures_Trees(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
)
{
	// 1 in 2 chance to generate a tree
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

	// Random tree origin between 3 and 11
	int bx = 3 + rand() % 9;
	int by = 3 + rand() % 9;

	int columnZStartBlocks = regionChunkZStart * CHUNK_SIZE;
	int zGround = (int)heightmap[bx][by] - columnZStartBlocks;
	if (zGround < 0 || zGround + 7 >= COLUMN_HEIGHT)
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
				if (wz < 0 || wz >= COLUMN_HEIGHT) continue;

				if (dx == 0 && dy == 0 && dz != 7)
					blockIDs[wx][wy][wz] = Log->getID();
				else
					blockIDs[wx][wy][wz] = Leave->getID();
			}
		}
	}
}
void generateFeatures_Boulders(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
)
{
	// 1 in 5 chance to generate a boulder
	if (rand() % 5 != 0)
		return;

	Block* stoneBlock = blockManager.getByName("diorite");
	if (stoneBlock == nullptr)
		return;
	Block* flowerBlock = blockManager.getByName("flower_alpine_quill");
	if (flowerBlock == nullptr)
		return;

	// Random boulder origin between 3 and 11
	int bx = 3 + rand() % 9;
	int by = 3 + rand() % 9;

	int columnZStartBlocks = regionChunkZStart * CHUNK_SIZE;
	int zGround = (int)heightmap[bx][by] - columnZStartBlocks;
	if (zGround - 2 < 0 || zGround + 3 >= COLUMN_HEIGHT)
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

				int wx = bx + dx;
				int wy = by + dy;
				int wz = zGround + dz;

				if (wx < 0 || wx >= CHUNK_SIZE) continue;
				if (wy < 0 || wy >= CHUNK_SIZE) continue;
				if (wz < 0 || wz >= COLUMN_HEIGHT) continue;

				blockIDs[wx][wy][wz] = stoneBlock->getID();
			}
		}
	}
	blockIDs[bx][by][zGround + 3] = flowerBlock->getID();
}

void generateFeatures_BlockPallette(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
) {
	if (columnCoordinates.y != 0)
		return;

	int blocks = columnCoordinates.x * 256 + 1;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		int xAbs = columnCoordinates.x * CHUNK_SIZE + x;
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int yAbs = columnCoordinates.x * CHUNK_SIZE + y;
			if (blocks >= blockManager.getNumberOfBlocks())
				return;
			blocks++;
			blockIDs[x][y][80] = blocks;
		}
	}
}
