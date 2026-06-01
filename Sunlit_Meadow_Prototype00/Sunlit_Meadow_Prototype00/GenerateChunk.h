#pragma once

#include "WorldTypes.h"
#include "BlockManager.h"
#include "FastNoiseLite.h"

// All generation now works on a full-height column buffer:
//   blockIDs[x][y][z], z in [0, COLUMN_HEIGHT)
// where z == 0 maps to absolute block z = regionChunkZStart * CHUNK_SIZE.
// columnCoordinates carries the shared (x, y) of every chunk in the column.

void generateShape(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager,
	FastNoiseLite& standartNoise
);

void generateFeatures(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);

void generateFeatures_GrassAndDirt(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);
void generateFeatures_Trees(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);
void generateFeatures_Boulders(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);