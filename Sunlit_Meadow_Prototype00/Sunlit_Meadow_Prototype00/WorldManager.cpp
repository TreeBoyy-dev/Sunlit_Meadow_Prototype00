#include <math.h>
#include <chrono>
#include <algorithm>
#include "WorldManager.h"
#include "Globals.h"
#include "Frustum.h"
#include "LoadShader.h"
#include "InitNoise.h"

WorldManager::WorldManager() {}

bool WorldManager::init(SDL_GPUDevice* gpu, SDL_GPUTextureFormat swapchainFormat) {

    SDL_GPUShader* vert = loadShader(gpu, "shader.vert.spv", 1, 0);
    SDL_GPUShader* frag = loadShader(gpu, "shader.frag.spv", 0, 1);

    if (!vert || !frag) { return SDL_APP_FAILURE; }

    SDL_GPUVertexAttribute vertex_attrs[5] = {
    {
        .location = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = (Uint32)offsetof(WorldVertex, position),
    },
    {
        .location = 1,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = (Uint32)offsetof(WorldVertex, normal),
    },
    {
        .location = 2,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        .offset = (Uint32)offsetof(WorldVertex, uv),
    },
    {
        .location = 3,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
        .offset = (Uint32)offsetof(WorldVertex, color),
    },
    {
        .location = 4,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
        .offset = (Uint32)offsetof(WorldVertex, materialIndex),
    },
    };

    SDL_GPUColorTargetDescription color_target_desc = {
        .format = swapchainFormat,
    };
    SDL_GPUVertexBufferDescription vertex_buffer_descriptions = {
        .slot = 0,
        .pitch = sizeof(WorldVertex),
    };
    SDL_GPUDepthStencilState depth_stencil_state = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &vertex_buffer_descriptions,
            .num_vertex_buffers = 1,
            .vertex_attributes = vertex_attrs,
            .num_vertex_attributes = SDL_arraysize(vertex_attrs),
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .depth_stencil_state = depth_stencil_state,
        .target_info = {
            .color_target_descriptions = &color_target_desc,
            .num_color_targets = 1,
            .depth_stencil_format = depth_texture_format,
            .has_depth_stencil_target = true,
        },
    };

    pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);

    SDL_ReleaseGPUShader(gpu, vert);
    SDL_ReleaseGPUShader(gpu, frag);

    if (!pipeline) {
        SDL_Log("SDL_CreateGPUGraphicsPipeline failed: %s", SDL_GetError());
        return false;
    }

    Uint32 layerCount = (Uint32)MATERIAL_COUNT;
    SDL_GPUTextureCreateInfo texInfo = {
    .type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    .width = 32,
    .height = 32,
    .layer_count_or_depth = layerCount,
    .num_levels = 1,
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
    .props = 0
    };
    textureArray = SDL_CreateGPUTexture(gpu, &texInfo);

    if (!UploadTextureArrayLayers(gpu, textureArray))
    {
        SDL_Log("failed loading textures");
        return SDL_APP_FAILURE;
    }


    blockManager.init();
    initNoise(&standartNoise);

    return true;
}

void WorldManager::destroy(AppState* state) {
    for (auto& [coord, region] : regions) {
        region->destroyRegion(state);
    }
    regions.clear();
    renderList.clear();

    if (pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(state->gpu, pipeline);
        pipeline = nullptr;
    }
}


// Updaating the world ----------------------------------------------------
void WorldManager::update(AppState* state, Vec3 playerPosition) {
    updatePlayerPosition(playerPosition);

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
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();

    // 1) Build the new visible set in absolute coords.
    std::unordered_set<ChunkCoord, ChunkCoordHash> nowVisible;
    nowVisible.reserve(visibleChunkCoordsRelative.size());
    for (const ChunkCoord& rel : visibleChunkCoordsRelative) {
        nowVisible.insert({ rel.x + m_lastPlayerChunkPos.x,
                            rel.y + m_lastPlayerChunkPos.y,
                            rel.z + m_lastPlayerChunkPos.z });
    }
    auto t1 = clock::now();

    // 2) Drop renderList entries that left the view.
    for (auto it = renderList.begin(); it != renderList.end(); ) {
        if (!nowVisible.count(it->first)) it = renderList.erase(it);
        else                              ++it;
    }
    auto t2 = clock::now();

    // 3) Drop pendingChunks entries that left the view, and cancel them.
    /*
    for (auto it = pendingChunks.begin(); it != pendingChunks.end(); ) {
        if (!nowVisible.count(*it)) {
            getRegion(regionCoordForChunk(*it))->cancelChunkGeneration(*it);
            it = pendingChunks.erase(it);
        }
        else {
            ++it; 
        }
    }//*/
    auto t3 = clock::now();

    // 4) For every newly visible chunk: ready -> renderList, else -> pending.
    std::vector<ChunkCoord> toRequest;
    toRequest.reserve(64);

    for (const ChunkCoord& coord : nowVisible) {
        if (renderList.count(coord) || pendingChunks.count(coord)) continue;

        Region* r = getRegion(getRegionCoordForChunk(coord));
        if (Chunk* c = r->getChunk(coord)) {
            renderList.emplace(coord, c);
        }
        else {
            pendingChunks.insert(coord);
            toRequest.push_back(coord);
        }
    }
    auto t4 = clock::now();

    // 5) Sort the new pending ones by distance, closest first, then request.
    const ChunkCoord pp = m_lastPlayerChunkPos;
    std::sort(toRequest.begin(), toRequest.end(),
        [pp](const ChunkCoord& a, const ChunkCoord& b) {
            int ax = a.x - pp.x, ay = a.y - pp.y, az = a.z - pp.z;
            int bx = b.x - pp.x, by = b.y - pp.y, bz = b.z - pp.z;
            return (ax * ax + ay * ay + az * az) < (bx * bx + by * by + bz * bz);
        });
    auto t5 = clock::now();

    for (const ChunkCoord& coord : toRequest)
        getRegion(getRegionCoordForChunk(coord))->requestChunkGeneration(coord);
    auto t6 = clock::now();

    //Logger to display time usage:
    ///*
    using us = std::chrono::microseconds;
    SDL_Log("time usage onPlayerChunkChanged: step1=%lldus step2=%lldus step3=%lldus step4=%lldus step5=%lldus step6=%lldus (total=%lldus)",
        (long long)std::chrono::duration_cast<us>(t1 - t0).count(),
        (long long)std::chrono::duration_cast<us>(t2 - t1).count(),
        (long long)std::chrono::duration_cast<us>(t3 - t2).count(),
        (long long)std::chrono::duration_cast<us>(t4 - t3).count(),
        (long long)std::chrono::duration_cast<us>(t5 - t4).count(),
        (long long)std::chrono::duration_cast<us>(t6 - t5).count(),
        (long long)std::chrono::duration_cast<us>(t6 - t0).count());
    //*/
}


//drawing the world -------------------------------------------------------
void WorldManager::draw(AppState* state,
    SDL_GPUCommandBuffer* cmd,
    SDL_GPURenderPass* pass,
    const UBO& ubo) {

    drawChunks(state, cmd, pass, ubo);
}

void WorldManager::drawChunks(AppState* state,
    SDL_GPUCommandBuffer* cmd,
    SDL_GPURenderPass* pass,
    const UBO& ubo) {

    Frustum frustum = buildFrustum(camera, fovX, aspect, NEAR_PLANE, FAR_PLANE);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    for (auto& [cc, chunk] : renderList) {
        Vec3 cMin = { cc.x * CHUNK_SIZE,  cc.y * CHUNK_SIZE,  cc.z * CHUNK_SIZE };
        Vec3 cMax = { cMin.x + CHUNK_SIZE, cMin.y + CHUNK_SIZE, cMin.z + CHUNK_SIZE };
        if (aabbInsideFrustum(frustum, cMin, cMax))
            chunk->drawMeshes(state, cmd, pass, ubo);
    }
}


//helpers -----------------------------------------------------------------
Region* WorldManager::getRegion(RegionCoord regionCoordinates) {
    auto it = regions.find(regionCoordinates);
    if (it != regions.end())
        return it->second.get();

    auto [newIt, inserted] = regions.emplace(
        regionCoordinates,
        std::make_unique<Region>(regionCoordinates, &blockManager, &standartNoise)
    );
    return newIt->second.get();
}

void WorldManager::calcVisibleChunksList(int renderDistance) {
    m_renderDistance = renderDistance;
    visibleChunkCoordsRelative.clear();
    for (int x = -renderDistance; x <= renderDistance; x++)
        for (int y = -renderDistance; y <= renderDistance; y++)
            for (int z = -renderDistance; z <= renderDistance; z++)
                if (sqrt(x * x + y * y + z * z) <= (double)renderDistance)
                    visibleChunkCoordsRelative.push_back({ x, y, z });
}


//global helpers -----------------------------------------------------------------
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
RegionCoord getRegionCoordForChunk(ChunkCoord c) {
    return {
        (int)std::floor((float)c.x / REGION_SIZE_YX),
        (int)std::floor((float)c.y / REGION_SIZE_YX),
        (int)std::floor((float)c.z / REGION_SIZE_Z)
    };
}

