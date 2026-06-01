#pragma once

#include <vector>

#include "WorldTypes.h"
#include "BlockManager.h"
#include "FastNoiseLite.h"
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
std::vector<GeneratedChunkData> generateColumn(
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	BlockManager& blockManager,
	FastNoiseLite& standartNoise
);