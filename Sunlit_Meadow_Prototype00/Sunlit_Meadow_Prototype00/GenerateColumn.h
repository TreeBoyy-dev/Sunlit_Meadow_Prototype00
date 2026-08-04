#pragma once

#include <vector>

#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "PalettedGrid2D.h"
#include "BlockManager.h"
#include "WorldGenNoise.h"
#include "PalettedContainer.h"

class WorldGenRegistry;

// One generated chunk's worth of data: its coordinate plus its block storage.
struct GeneratedChunkData {
	ChunkCoord        coordinates;
	PalettedContainer storage;
};

// Generates the entire vertical span (the full height of one region) of chunks
// at the given (x, y) column in a single tall buffer, then splits it into the
// individual chunks. regionChunkZStart is the bottom chunk-z of the region and
// regionColumnStart its bottom-left chunk (x, y) — together they place the
// column inside its region, which is what the biome map is indexed by.
//
// layer / zoneMap / biomeMap are the owning region's worldgen context —
// immutable, read lock-free from the worker thread (see Region.h). The layer
// drives the shape pass, the biome map drives the surface and feature passes.
// worldGenNoise and the registry are WorldManager-owned, init-once/read-only —
// same lock-free contract (see WorldGenNoise.h / WorldGenRegistry.h).
std::vector<GeneratedChunkData> generateColumn(
	ColumnCoord columnCoordinates,
	ColumnCoord regionColumnStart,
	int regionChunkZStart,
	const LayerDef&       layer,
	const PalettedGrid2D& zoneMap,
	const PalettedGrid2D& biomeMap,
	BlockManager& blockManager,
	const WorldGenNoise& worldGenNoise,
	const WorldGenRegistry& registry,
	Uint64 worldSeed
);
