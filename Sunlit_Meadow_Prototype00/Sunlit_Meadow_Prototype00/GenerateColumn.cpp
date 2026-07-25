#include "GenerateColumn.h"
#include "GenerateShape.h"
#include "GenerateFeatures.h"
#include "Globals.h"

#include <chrono>

std::vector<GeneratedChunkData> generateColumn(
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	const LayerDef&       layer,
	const PalettedGrid2D& zoneMap,
	const PalettedGrid2D& biomeMap,
	BlockManager& blockManager,
	const WorldGenNoise& worldGenNoise
) {
	using clock = std::chrono::steady_clock;
	auto t0 = clock::now();

	// layer now selects the shape generator (see generateShape's switch).
	// TODO worldgen: zoneMap/biomeMap will drive per-cell surface blocks
	// & features — received but unused this pass.
	(void)zoneMap; (void)biomeMap;

	std::vector<GeneratedChunkData> column;
	column.reserve(REGION_SIZE_Z);

	// Generate the whole region-height column in one pass so the heightmap
	// (and shape/feature work) is only computed once for this (x, y).
	Uint16 dense[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT];
	float  heightmap[CHUNK_SIZE][CHUNK_SIZE] = { 0.0f };

	// generate shape: air/stone (layer-dispatched, spline + caves)
	generateShape(dense, columnCoordinates, regionChunkZStart, heightmap, blockManager, layer, worldGenNoise);
	auto t1 = clock::now();

	// generate biomes
	//TODO -> actually do that in the region

	// generate features: grass, vegitation, structures
	generateFeatures(dense, columnCoordinates, regionChunkZStart, heightmap, blockManager);
	auto t2 = clock::now();

	// Split the tall column into REGION_SIZE_Z separate chunks.
	for (int k = 0; k < REGION_SIZE_Z; k++) {
		Uint16 chunkDense[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

		// z is the innermost, contiguous axis in BOTH layouts, so
		// each (x, y) row is one 16-cell memcpy out of the 512-tall column
		// instead of 16 individually-indexed copies.
		for (int x = 0; x < CHUNK_SIZE; x++)
			for (int y = 0; y < CHUNK_SIZE; y++)
				SDL_memcpy(&chunkDense[x][y][0],
					&dense[x][y][k * CHUNK_SIZE],
					CHUNK_SIZE * sizeof(Uint16));

		GeneratedChunkData chunkData;
		chunkData.coordinates = { columnCoordinates.x, columnCoordinates.y, regionChunkZStart + k };
		chunkData.storage.fromDenseIds(&chunkDense[0][0][0]);

		column.push_back(std::move(chunkData));
	}
	auto t3 = clock::now();
	//Logger to display time usage:
	/*
	using us = std::chrono::microseconds;
	SDL_Log("time usage Collumn generation: step1=%lldus step2=%lldus step3=%lldus (total=%lldus)",
		(long long)std::chrono::duration_cast<us>(t1 - t0).count(),
		(long long)std::chrono::duration_cast<us>(t2 - t1).count(),
		(long long)std::chrono::duration_cast<us>(t3 - t2).count(),
		(long long)std::chrono::duration_cast<us>(t3 - t0).count());
	//*/


	return column;
}
