#pragma once

#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "BlockManager.h"
#include "WorldGenNoise.h"

// All generation now works on a full-height column buffer:
//   blockIDs[x][y][z], z in [0, COLUMN_HEIGHT)
// where z == 0 maps to absolute block z = regionChunkZStart * CHUNK_SIZE.
// columnCoordinates carries the shared (x, y) of every chunk in the column.
//
// Every tunable the shape pass reads now lives on the LayerDef (loaded
// from Assets/WorldGen/Layers/*.json) — spline, sea level, vertical
// bounds, base block, cave settings. Retuning the terrain is a JSON
// edit, not a recompile.

// Dispatcher: runs the generator named by layer.shape.
// Writes heightmap[x][y] as ABSOLUTE world Z — generateFeatures reads
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

// ShapeGenerator::SplineTerrain — control noise -> spline -> heightmap,
// then a 3D cave carve.
void generateShape_splineTerrain(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	Uint16 airID,
	const LayerDef& layer,
	const WorldGenNoise& worldGenNoise
);

// ShapeGenerator::AirFill / SolidFill — the whole column is one block.
// heightmap is filled with a surface Z BELOW the column so the feature
// pass finds no ground to decorate (these layers are pure filler).
void generateShape_fill(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	int regionChunkZStart,
	Uint16 blockID
);
