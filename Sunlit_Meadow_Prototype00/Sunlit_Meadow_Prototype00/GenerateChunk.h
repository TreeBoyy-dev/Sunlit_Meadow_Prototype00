#pragma once

#include "ChunkTypes.h"

bool generateChunk(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates
);

void generateShape(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
);

void generateFeatures(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
);

void generateFeatures_GrassAndDirt(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
);
void generateFeatures_Trees(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
);
void generateFeatures_Boulders(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates,
	int heightmap[CHUNK_SIZE][CHUNK_SIZE]
);
