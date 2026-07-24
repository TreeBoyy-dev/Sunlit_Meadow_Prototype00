#pragma once

#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "BlockManager.h"
#include "WorldGenNoise.h"

// All generation now works on a full-height column buffer:
//   blockIDs[x][y][z], z in [0, COLUMN_HEIGHT)
// where z == 0 maps to absolute block z = regionChunkZStart * CHUNK_SIZE.
// columnCoordinates carries the shared (x, y) of every chunk in the column.

// Dispatcher: picks the layer's shape generator (see the switch inside).
// Writes heightmap[x][y] as ABSOLUTE world Z — generateFeatures_* reads
// that contract back.
void generateShape(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager,
	const LayerDef& layer,
	const WorldGenNoise& worldGenNoise
);

// Meadow layer: noise -> spline -> heightmap, then a 3D cave carve.
void generateShape_Meadow(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager,
	const WorldGenNoise& worldGenNoise
);
