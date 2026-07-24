#pragma once

#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "BlockManager.h"
#include "WorldGenNoise.h"

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
void generateFeatures_BlockPallette(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);