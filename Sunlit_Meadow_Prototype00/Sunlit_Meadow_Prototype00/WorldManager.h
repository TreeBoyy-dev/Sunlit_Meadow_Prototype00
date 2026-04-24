#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>

#include "Chunk.h"
#include "Region.h"

class WorldManager {
private:
	std::unordered_map<RegionCoord, Region, ChunkCoordHash> regions;
	std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> loadedChunks;
	std::unordered_set<ChunkCoord, ChunkCoordHash> visibleChunkCoords;
	std::vector<Chunk*> renderList;

public:
	WorldManager();

	bool calcVisibleChunks(int rendedistance);
};