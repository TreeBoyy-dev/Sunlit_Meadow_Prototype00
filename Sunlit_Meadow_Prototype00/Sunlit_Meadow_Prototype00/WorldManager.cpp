#include <math.h>
#include "WorldManager.h"
#include "Globals.h"
#include "Frustum.h"

WorldManager::WorldManager() {}

void WorldManager::calcVisibleChunksList(int renderDistance) {
    visibleChunkCoordsRelative.clear();

    for (int x = -renderDistance; x <= renderDistance; x++) {
        for (int y = -renderDistance; y <= renderDistance; y++) {
            for (int z = -renderDistance; z <= renderDistance; z++) {
                if (sqrt(x * x + y * y + z * z) <= (double)renderDistance) {
                    visibleChunkCoordsRelative.push_back({ x, y, z });
                }
            }
        }
    }
}

void WorldManager::update(AppState* state, SDL_GPUTexture* textureArray) {
    m_needsRenderListUpdate = false;

    for (auto& [coord, region] : regions) {
        bool newChunksArrived = region->update(state, textureArray);
        if (newChunksArrived)
            m_needsRenderListUpdate = true;
    }

    if (m_needsRenderListUpdate)
        rebuildRenderList();
}

void WorldManager::updateRenderList(Vec3 playerPosition) {
    m_lastPlayerChunkPos = getPlayerChunkCoord(playerPosition);
    rebuildRenderList();
}

void WorldManager::rebuildRenderList() {
    renderList.clear();

    for (const ChunkCoord& relCoord : visibleChunkCoordsRelative) {
        ChunkCoord absChunk = {
            relCoord.x + m_lastPlayerChunkPos.x,
            relCoord.y + m_lastPlayerChunkPos.y,
            relCoord.z + m_lastPlayerChunkPos.z
        };

        RegionCoord regionCoord = {
            (int)std::floor((float)absChunk.x / REGION_SIZE_YX),
            (int)std::floor((float)absChunk.y / REGION_SIZE_YX),
            (int)std::floor((float)absChunk.z / REGION_SIZE_Z)
        };

        Region* region = getRegion(regionCoord);
        Chunk* chunk = region->getChunk(absChunk);
        if (chunk) renderList.push_back(chunk);
    }
}

void WorldManager::drawChunks(
    AppState* state,
    SDL_GPUCommandBuffer* cmd,
    SDL_GPURenderPass* pass,
    const UBO& ubo
) {
    Frustum frustum = buildFrustum(camera, fovX, aspect, NEAR_PLANE, FAR_PLANE);

    for (Chunk* chunk : renderList) {
        if (!chunk) { SDL_Log("tried to draw nullptr"); continue; }

        ChunkCoord cc = chunk->getChunkCoordinates();
        Vec3 cMin = { cc.x * CHUNK_SIZE,  cc.y * CHUNK_SIZE,  cc.z * CHUNK_SIZE };
        Vec3 cMax = { cMin.x + CHUNK_SIZE, cMin.y + CHUNK_SIZE, cMin.z + CHUNK_SIZE };

        if (aabbInsideFrustum(frustum, cMin, cMax))
            chunk->drawMeshes(state, cmd, pass, ubo);
    }
}

Region* WorldManager::getRegion(RegionCoord regionCoordinates) {
    auto it = regions.find(regionCoordinates);
    if (it != regions.end())
        return it->second.get();

    auto [newIt, inserted] = regions.emplace(
        regionCoordinates,
        std::make_unique<Region>(regionCoordinates)
    );
    return newIt->second.get();
}

ChunkCoord getPlayerChunkCoord(Vec3 playerPosition) {
    return {
        (int)std::floor(playerPosition.x / CHUNK_SIZE),
        (int)std::floor(playerPosition.y / CHUNK_SIZE),
        (int)std::floor(playerPosition.z / CHUNK_SIZE)
    };
}
RegionCoord getPlayerRegionCoord(Vec3 playerPosition) {
    return {
        (int)std::floor(playerPosition.x / (CHUNK_SIZE * REGION_SIZE_YX)),
        (int)std::floor(playerPosition.y / (CHUNK_SIZE * REGION_SIZE_YX)),
        (int)std::floor(playerPosition.z / (CHUNK_SIZE * REGION_SIZE_Z))
    };
}

void WorldManager::destroyManager(AppState* state) {
    for (auto& [coord, region] : regions) {
        region->destroyRegion(state);
    }
    regions.clear();
    renderList.clear();
}
