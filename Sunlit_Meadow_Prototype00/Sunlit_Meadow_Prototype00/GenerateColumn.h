#pragma once

#include <vector>

#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "PalettedGrid2D.h"
#include "BlockManager.h"
#include "WorldGenNoise.h"
#include "PalettedContainer.h"

// One generated chunk's worth of data: its coordinate plus its block storage.
struct GeneratedChunkData {
	ChunkCoord        coordinates;
	PalettedContainer storage;
};

// Generates the entire vertical span (the full height of one region) of chunks
// at the given (x, y) column in a single tall buffer, then splits it into the
// individual chunks. regionChunkZStart is the bottom chunk-z of the region.
// Returns one PalettedContainer per chunk in that column, in ascending z order.
//
// layer / zoneMap / biomeMap are the owning region's worldgen context —
// immutable, read lock-free from the worker thread (see Region.h). layer
// now selects the shape generator; zoneMap/biomeMap stay unused until
// the surface-blocks/features pass. worldGenNoise is WorldManager-owned,
// init-once/read-only — same lock-free contract (see WorldGenNoise.h).
std::vector<GeneratedChunkData> generateColumn(
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	const LayerDef&       layer,
	const PalettedGrid2D& zoneMap,
	const PalettedGrid2D& biomeMap,
	BlockManager& blockManager,
	const WorldGenNoise& worldGenNoise
);
