#pragma once

#include "WorldTypes.h"
#include "BlockManager.h"
#include "FastNoiseLite.h"

bool generateChunk(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	BlockManager& blockManager,
	FastNoiseLite& standartNoise
);

void generateShape(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager,
	FastNoiseLite& standartNoise
);

void generateFeatures(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);

void generateFeatures_GrassAndDirt(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);
void generateFeatures_Trees(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);
void generateFeatures_Boulders(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager
);