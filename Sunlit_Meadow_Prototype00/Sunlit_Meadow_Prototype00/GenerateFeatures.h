#pragma once

#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "BlockManager.h"
#include "PalettedGrid2D.h"

class WorldGenRegistry;

// =====================================================================
//  Feature generation — biome driven.
//
//  Nothing in here is hardcoded per biome any more. The pass looks up
//  which biome owns the column (via the region's biome map), then:
//    1) lays the biome's surface / filler blocks over the heightmap,
//    2) rolls each entry in the biome's `features` list and runs that
//       FeatureDef's generator,
//    3) (structures: not implemented — see generateFeatures.cpp).
//
//  Randomness is a deterministic stream seeded from
//  (worldSeed, columnX, columnY) — see WorldGenRandom.h for why rand()
//  cannot be used here.
//
//  regionColumnStart is the region's bottom-left chunk (x, y); it turns
//  the absolute columnCoordinates into the region-local coordinates the
//  biome map is indexed by.
// =====================================================================

void generateFeatures(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	ColumnCoord regionColumnStart,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager,
	const WorldGenRegistry& registry,
	const PalettedGrid2D& biomeMap,
	Uint64 worldSeed
);

// Debug pass: lays every registered block id out in a strip along
// y == 0, x >= 0 at Z 80. Not biome driven, not part of worldgen.
void generateFeatures_BlockPallette(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	BlockManager& blockManager
);
