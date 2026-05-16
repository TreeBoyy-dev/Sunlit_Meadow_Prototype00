#include <math.h>
#include <algorithm>
#include "WorldManager.h"
#include "Globals.h"
#include "Frustum.h"

WorldManager::WorldManager() {}

void WorldManager::calcVisibleChunksList(int renderDistance) {
    m_renderDistance = renderDistance;
    visibleChunkCoordsRelative.clear();
    for (int x = -renderDistance; x <= renderDistance; x++)
        for (int y = -renderDistance; y <= renderDistance; y++)
            for (int z = -renderDistance; z <= renderDistance; z++)
                if (sqrt(x * x + y * y + z * z) <= (double)renderDistance)
                    visibleChunkCoordsRelative.push_back({ x, y, z });
}


void WorldManager::update(AppState* state, SDL_GPUTexture* textureArray) {
    std::vector<ChunkCoord> newlyReady;
    newlyReady.reserve(8);

    for (auto& [rc, region] : regions) {
        newlyReady.clear();
        region->update(state, textureArray, newlyReady);

        for (const ChunkCoord& cc : newlyReady) {
            auto it = pendingChunks.find(cc);
            if (it == pendingChunks.end()) continue;
            pendingChunks.erase(it);

            if (Chunk* c = region->getChunk(cc))
                renderList.emplace(cc, c);
        }
    }
}

void WorldManager::updatePlayerPosition(Vec3 playerPosition) {
    ChunkCoord now = getPlayerChunkCoord(playerPosition);
    if (now == m_lastPlayerChunkPos) return;

    m_lastPlayerChunkPos = now;
    onPlayerChunkChanged();
}

void WorldManager::onPlayerChunkChanged() {
    // 1) Build the new visible set in absolute coords.
    std::unordered_set<ChunkCoord, ChunkCoordHash> nowVisible;
    nowVisible.reserve(visibleChunkCoordsRelative.size());
    for (const ChunkCoord& rel : visibleChunkCoordsRelative) {
        nowVisible.insert({ rel.x + m_lastPlayerChunkPos.x,
                            rel.y + m_lastPlayerChunkPos.y,
                            rel.z + m_lastPlayerChunkPos.z });
    }

    // 2) Drop renderList entries that left the view.
    for (auto it = renderList.begin(); it != renderList.end(); ) {
        if (!nowVisible.count(it->first)) it = renderList.erase(it);
        else                              ++it;
    }

    // 3) Drop pendingChunks entries that left the view, and cancel them.
    for (auto it = pendingChunks.begin(); it != pendingChunks.end(); ) {
        if (!nowVisible.count(*it)) {
            getRegion(regionCoordForChunk(*it))->cancelChunkGeneration(*it);
            it = pendingChunks.erase(it);
        }
        else {
            ++it;
        }
    }

    // 4) For every newly visible chunk: ready -> renderList, else -> pending.
    std::vector<ChunkCoord> toRequest;
    toRequest.reserve(64);

    for (const ChunkCoord& coord : nowVisible) {
        if (renderList.count(coord) || pendingChunks.count(coord)) continue;

        Region* r = getRegion(regionCoordForChunk(coord));
        if (Chunk* c = r->getChunk(coord)) {
            renderList.emplace(coord, c);
        }
        else {
            pendingChunks.insert(coord);
            toRequest.push_back(coord);
        }
    }

    // 5) Sort the new pending ones by distance, closest first, then request.
    const ChunkCoord pp = m_lastPlayerChunkPos;
    std::sort(toRequest.begin(), toRequest.end(),
        [pp](const ChunkCoord& a, const ChunkCoord& b) {
            int ax = a.x - pp.x, ay = a.y - pp.y, az = a.z - pp.z;
            int bx = b.x - pp.x, by = b.y - pp.y, bz = b.z - pp.z;
            return (ax * ax + ay * ay + az * az) < (bx * bx + by * by + bz * bz);
        });

    for (const ChunkCoord& coord : toRequest)
        getRegion(regionCoordForChunk(coord))->requestChunkGeneration(coord);
}

void WorldManager::drawChunks(AppState* state,
    SDL_GPUCommandBuffer* cmd,
    SDL_GPURenderPass* pass,
    const UBO& ubo) {
    Frustum frustum = buildFrustum(camera, fovX, aspect, NEAR_PLANE, FAR_PLANE);

    for (auto& [cc, chunk] : renderList) {
        Vec3 cMin = { cc.x * CHUNK_SIZE,  cc.y * CHUNK_SIZE,  cc.z * CHUNK_SIZE };
        Vec3 cMax = { cMin.x + CHUNK_SIZE, cMin.y + CHUNK_SIZE, cMin.z + CHUNK_SIZE };
        if (aabbInsideFrustum(frustum, cMin, cMax))
            chunk->drawMeshes(state, cmd, pass, ubo);
    }
}

RegionCoord WorldManager::regionCoordForChunk(ChunkCoord c) {
    return {
        (int)std::floor((float)c.x / REGION_SIZE_YX),
        (int)std::floor((float)c.y / REGION_SIZE_YX),
        (int)std::floor((float)c.z / REGION_SIZE_Z)
    };
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
