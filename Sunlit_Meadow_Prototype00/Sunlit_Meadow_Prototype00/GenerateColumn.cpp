#include "GenerateColumn.h"
#include "GenerateChunk.h"   // reuse generateShape / generateFeatures
#include "Globals.h"

std::vector<GeneratedChunkData> generateColumn(
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	BlockManager& blockManager,
	FastNoiseLite& noise
) {
	std::vector<GeneratedChunkData> column;
	column.reserve(REGION_SIZE_Z);

	// Generate the whole region-height column in one pass so the heightmap
	// (and shape/feature work) is only computed once for this (x, y).
	Uint16 dense[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT];
	float  heightmap[CHUNK_SIZE][CHUNK_SIZE] = { 0.0f };

	// generate shape: air/stone
	generateShape(dense, columnCoordinates, regionChunkZStart, heightmap, blockManager, noise);

	// generate biomes
	//TODO

	// generate features: grass, vegitation, structures
	generateFeatures(dense, columnCoordinates, regionChunkZStart, heightmap, blockManager);

	// Split the tall column into REGION_SIZE_Z separate chunks.
	for (int k = 0; k < REGION_SIZE_Z; k++) {
		Uint16 chunkDense[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

		for (int x = 0; x < CHUNK_SIZE; x++)
			for (int y = 0; y < CHUNK_SIZE; y++)
				for (int z = 0; z < CHUNK_SIZE; z++)
					chunkDense[x][y][z] = dense[x][y][k * CHUNK_SIZE + z];

		GeneratedChunkData chunkData;
		chunkData.coordinates = { columnCoordinates.x, columnCoordinates.y, regionChunkZStart + k };
		chunkData.storage.fromDenseIds(&chunkDense[0][0][0]);

		column.push_back(std::move(chunkData));
	}

	return column;
}