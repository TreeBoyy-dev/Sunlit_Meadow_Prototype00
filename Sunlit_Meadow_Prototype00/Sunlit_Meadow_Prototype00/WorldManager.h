#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <memory>

#include "Chunk.h"
#include "Region.h"

ChunkCoord getPlayerChunkCoord(Vec3 playerPosition);

class WorldManager {
private:
	std::unordered_map<RegionCoord, std::unique_ptr<Region>, RegionCoordHash> regions;	//std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> loadedChunks;
	std::vector<ChunkCoord> visibleChunkCoordsRelative;
	std::vector<Chunk*> renderList;

public:
	WorldManager();

	//call when renderdistance is changed and after inizialiizing
	void calcVisibleChunksList(int renderDistance);
	//call when player moved to a different chunk or changed renderdistance
	void updateRenderList(
		Vec3 playerPosition,
		int renderDistance,
		AppState* state,
		SDL_GPUTexture* textureArray
	);

	void drawChunks(
		AppState* state,
		SDL_GPUCommandBuffer* cmd,
		SDL_GPURenderPass* pass,
		const UBO& ubo
	);

private:
	Region* getRegion(RegionCoord regionCoordinates);
};