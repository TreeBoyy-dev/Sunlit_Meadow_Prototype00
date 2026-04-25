#include <math.h>

#include "WorldManager.h"

WorldManager::WorldManager() {}

void WorldManager::calcVisibleChunksList(int renderDistance) {
	visibleChunkCoordsRelative.clear();

	for (int x = -renderDistance; x <= renderDistance; x++) {
		for (int y = -renderDistance; y <= renderDistance; y++) {
			for (int z = -renderDistance; z <= renderDistance; z++) {
				if (sqrt(x * x + y * y + z * z) <= (double)renderDistance) {
					ChunkCoord cords = { x, y, z };
					visibleChunkCoordsRelative.push_back(cords);
					//SDL_Log("new chunk: %d, %d, %d", x, y, z);
				}
			}
		}
	}
}

void WorldManager::updateRenderList(
	Vec3 playerPosition,
	int renderDistance,
	AppState* state,
	SDL_GPUTexture* textureArray
) {
	ChunkCoord playerChunkPos = {
		(int)std::floor(playerPosition.x / CHUNK_SIZE),
		(int)std::floor(playerPosition.y / CHUNK_SIZE),
		(int)std::floor(playerPosition.z / CHUNK_SIZE)
	};

	renderList.clear();

	for (const ChunkCoord& relCoord : visibleChunkCoordsRelative) {
		// 1. absolute world-space chunk coordinate
		ChunkCoord absChunk = {
			relCoord.x + playerChunkPos.x,
			relCoord.y + playerChunkPos.y,
			relCoord.z + playerChunkPos.z
		};

		// 2. which region owns this chunk
		RegionCoord regionCoord = {
			(int)std::floor((float)absChunk.x / REGION_SIZE_YX),
			(int)std::floor((float)absChunk.y / REGION_SIZE_YX),
			(int)std::floor((float)absChunk.z / REGION_SIZE_Z)
		};

		// 3. fetch (or create) region, then chunk
		Region* region = getRegion(regionCoord);
		Chunk* chunk = region->getChunk(absChunk, state, textureArray);
		if (chunk) renderList.push_back(chunk);
	}
}
void WorldManager::drawChunks(
	AppState* state,
	SDL_GPUCommandBuffer* cmd,
	SDL_GPURenderPass* pass,
	const UBO& ubo
) {
	for (int i = 0; i < renderList.size(); i++) {
		Chunk* chunk = renderList.at(i);
		if (chunk != nullptr) {
			chunk->drawMeshes(
				state,
				cmd,
				pass,
				ubo
			);
			//ChunkCoord coords = chunk->getChunkCoordinates();
			//SDL_Log("rendered chunk: %d|%d|%d", coords.x, coords.y, coords.z);
		}
		else
			SDL_Log("tried to draw nullptr");
	}
}


Region* WorldManager::getRegion(RegionCoord regionCoordinates) {
	auto it = regions.find(regionCoordinates);

	if (it == regions.end()) {
		auto [newIt, inserted] = regions.emplace(
			regionCoordinates,
			std::make_unique<Region>(regionCoordinates)
		);

		return newIt->second.get();
	}

	return it->second.get();
}