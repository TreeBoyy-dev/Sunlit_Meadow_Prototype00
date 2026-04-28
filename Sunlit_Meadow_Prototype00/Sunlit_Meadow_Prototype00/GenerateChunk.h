#pragma once

#include "ChunkTypes.h"

bool generateChunk(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates
);

void generateShape(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
	ChunkCoord chunkCoordinates
);