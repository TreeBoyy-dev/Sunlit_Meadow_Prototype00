#include <cstring>

#include "EntityManager.h"
#include "LoadShader.h"
#include "BuildAbsolutePath.h"

EntityManager::EntityManager() {}

// Pipeline: its own vertex/fragment shaders, but reuses the engine WorldVertex
// layout and UBO so it slots into the same view/projection math as the world.
bool EntityManager::init(AppState* state) {
    SDL_GPUShader* vert = loadShader(state->gpu, "entity.vert.spv", 1, 0);
    SDL_GPUShader* frag = loadShader(state->gpu, "entity.frag.spv", 0, 1);
    if (!vert || !frag) {
        SDL_Log("[Entity] shader load failed");
        return false;
    }

    SDL_GPUVertexAttribute vertex_attrs[4] = {
        {.location = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = (Uint32)offsetof(ModelVertex, position) },
        {.location = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = (Uint32)offsetof(ModelVertex, normal)   },
        {.location = 2, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = (Uint32)offsetof(ModelVertex, uv)       },
        {.location = 3, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = (Uint32)offsetof(ModelVertex, color)    },
    };

    SDL_GPUVertexBufferDescription vbDesc = {
        .slot = 0,
        .pitch = sizeof(ModelVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    SDL_GPUColorTargetBlendState blend = {
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .enable_blend = true,
    };
    SDL_GPUColorTargetDescription colorTarget = {
        .format = SDL_GetGPUSwapchainTextureFormat(state->gpu, state->window),
        .blend_state = blend,
    };
    SDL_GPURasterizerState raster = {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_FRONT,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
    };
    SDL_GPUDepthStencilState depth = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &vbDesc,
            .num_vertex_buffers = 1,
            .vertex_attributes = vertex_attrs,
            .num_vertex_attributes = SDL_arraysize(vertex_attrs),
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = raster,
        .depth_stencil_state = depth,
        .target_info = {
            .color_target_descriptions = &colorTarget,
            .num_color_targets = 1,
            .depth_stencil_format = depth_texture_format,
            .has_depth_stencil_target = true,
        },
    };

    pipeline = SDL_CreateGPUGraphicsPipeline(state->gpu, &pipeInfo);
    SDL_ReleaseGPUShader(state->gpu, vert);
    SDL_ReleaseGPUShader(state->gpu, frag);
    if (!pipeline) {
        SDL_Log("[Entity] pipeline failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUSamplerCreateInfo samplerInfo = {
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    };
    sampler = SDL_CreateGPUSampler(state->gpu, &samplerInfo);
    if (!sampler) {
        SDL_Log("[Entity] sampler failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

void EntityManager::destroy(AppState* state) {
    for (auto& asset : assets) {
        if (asset->vertexBuffer) SDL_ReleaseGPUBuffer(state->gpu, asset->vertexBuffer);
        if (asset->indexBuffer)  SDL_ReleaseGPUBuffer(state->gpu, asset->indexBuffer);
        if (asset->texture)      SDL_ReleaseGPUTexture(state->gpu, asset->texture);
    }
    assets.clear();
    assetsById.clear();
    assetsByName.clear();
    spawnDefaults.clear();
    entities.clear();

    if (sampler)  SDL_ReleaseGPUSampler(state->gpu, sampler);
    if (pipeline) SDL_ReleaseGPUGraphicsPipeline(state->gpu, pipeline);
    sampler = nullptr;
    pipeline = nullptr;
}

// Type registration
EntityAsset* EntityManager::registerType(
    std::unique_ptr<EntityAsset> asset,
    SpawnData                    defaults)
{
    Uint16 id = nextTypeId++;
    asset->id = id;

    EntityAsset* ptr = asset.get();

    assetsById[id] = ptr;
    assetsByName[asset->name] = ptr;

    // spawnDefaults stays index-aligned with the type id: because ids are
    // handed out 0,1,2,... and this is the only place that grows the list,
    // push_back lands defaults at exactly index == id.
    spawnDefaults.push_back(defaults);

    assets.push_back(std::move(asset));
    return ptr;
}

bool EntityManager::loadEntityType(
    AppState* state,
    const std::string& name,
    const char* modelPath, const char* modelFile,
    const char* texturePath, const char* textureFile,
    Hitbox    hitbox,
    SpawnData spawnDefaults)
{
    if (assetsByName.find(name) != assetsByName.end()) {
        SDL_Log("[Entity] type '%s' already registered", name.c_str());
        return false;
    }

    auto asset = std::make_unique<EntityAsset>();
    asset->name = name;
    asset->hitbox = hitbox;

    std::vector<ModelVertex> vertices;
    std::vector<Uint16> indices;
    if (!loadModelFromFile(modelPath, modelFile, vertices, indices)) {
        SDL_Log("[Entity] failed to load model '%s'", modelFile);
        return false;
    }

    if (!uploadMeshToGPU(state, asset.get(), vertices, indices)) {
        SDL_Log("[Entity] failed to upload mesh for '%s'", name.c_str());
        return false;
    }

    GPUTextureWH gpuTextureWH;
    if(!loadTextureFromFile(&gpuTextureWH, state->gpu, texturePath, textureFile))
        SDL_Log("[Entity] failed to load texture '%s'", textureFile);
    asset->texture = gpuTextureWH.texture;
    if (!asset->texture) {
        SDL_Log("[Entity] failed to load texture '%s'", textureFile);
        if (asset->vertexBuffer) SDL_ReleaseGPUBuffer(state->gpu, asset->vertexBuffer);
        if (asset->indexBuffer)  SDL_ReleaseGPUBuffer(state->gpu, asset->indexBuffer);
        return false;
    }

    registerType(std::move(asset), spawnDefaults);
    return true;
}

bool EntityManager::loadModelFromFile(
    const char* filePath,
    const char* fileName,
    std::vector<ModelVertex>& outVertices,
    std::vector<Uint16>& outIndices)
{
    if (obj_parse(
        BuildAbsolutePath(filePath, fileName),
        outVertices,
        outIndices
    ))
        return true;

    return false;
}

bool EntityManager::uploadMeshToGPU(
    AppState* state,
    EntityAsset* asset,
    const std::vector<ModelVertex>& vertices,
    const std::vector<Uint16>& indices)
{
    if (vertices.empty() || indices.empty()) {
        SDL_Log("[Entity] empty mesh, nothing to upload");
        return false;
    }

    const Uint32 vbSize = (Uint32)(vertices.size() * sizeof(ModelVertex));
    const Uint32 ibSize = (Uint32)(indices.size() * sizeof(Uint16));

    SDL_GPUBufferCreateInfo vbInfo = { .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = vbSize };
    asset->vertexBuffer = SDL_CreateGPUBuffer(state->gpu, &vbInfo);

    SDL_GPUBufferCreateInfo ibInfo = { .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = ibSize };
    asset->indexBuffer = SDL_CreateGPUBuffer(state->gpu, &ibInfo);

    if (!asset->vertexBuffer || !asset->indexBuffer) {
        SDL_Log("[Entity] buffer creation failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo tVB = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = vbSize };
    SDL_GPUTransferBuffer* transferVB = SDL_CreateGPUTransferBuffer(state->gpu, &tVB);
    SDL_GPUTransferBufferCreateInfo tIB = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = ibSize };
    SDL_GPUTransferBuffer* transferIB = SDL_CreateGPUTransferBuffer(state->gpu, &tIB);

    void* vbMapped = SDL_MapGPUTransferBuffer(state->gpu, transferVB, false);
    SDL_memcpy(vbMapped, vertices.data(), vbSize);
    SDL_UnmapGPUTransferBuffer(state->gpu, transferVB);

    void* ibMapped = SDL_MapGPUTransferBuffer(state->gpu, transferIB, false);
    SDL_memcpy(ibMapped, indices.data(), ibSize);
    SDL_UnmapGPUTransferBuffer(state->gpu, transferIB);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(state->gpu);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation vbSrc = { .transfer_buffer = transferVB, .offset = 0 };
    SDL_GPUBufferRegion           vbDst = { .buffer = asset->vertexBuffer, .offset = 0, .size = vbSize };
    SDL_UploadToGPUBuffer(copy, &vbSrc, &vbDst, false);

    SDL_GPUTransferBufferLocation ibSrc = { .transfer_buffer = transferIB, .offset = 0 };
    SDL_GPUBufferRegion           ibDst = { .buffer = asset->indexBuffer, .offset = 0, .size = ibSize };
    SDL_UploadToGPUBuffer(copy, &ibSrc, &ibDst, false);

    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(state->gpu, transferVB);
    SDL_ReleaseGPUTransferBuffer(state->gpu, transferIB);

    asset->numIndices = (Uint32)indices.size();
    return true;
}

// Spawning / updating
Entity* EntityManager::spawnInternal(EntityAsset* asset, Vec3 position, const SpawnData& sd) {
    // Build this entity's own per-instance data from the spawn parameters.
    EntityData data;
    data.id = nextEntityId++;
    data.health = sd.health;
    data.maxHealth = sd.maxHealth;
    data.alive = true;

    PhysicsBody physics;
    physics.mass = sd.mass;
    physics.affectedByGravity = sd.affectedByGravity;

    // The entity stores a pointer to the shared asset; it does NOT copy it.
    auto entity = std::make_unique<Entity>(asset, data, physics, position);
    Entity* raw = entity.get();
    entities.push_back(std::move(entity));
    return raw;
}

Entity* EntityManager::spawn(const std::string& typeName, Vec3 position) {
    SDL_Log("[Entity] Spawning '%s'", typeName.c_str());

    EntityAsset* asset = getAssetByName(typeName);
    if (!asset) {
        SDL_Log("[Entity] spawn: unknown type '%s' (register it first)", typeName.c_str());
        return nullptr;
    }
    return spawnInternal(asset, position, spawnDefaults[asset->id]);
}

Entity* EntityManager::spawn(const std::string& typeName, Vec3 position, SpawnData overrideData) {
    EntityAsset* asset = getAssetByName(typeName);
    if (!asset) {
        SDL_Log("[Entity] spawn: unknown type '%s' (register it first)", typeName.c_str());
        return nullptr;
    }
    return spawnInternal(asset, position, overrideData);
}

void EntityManager::update(float dt, WorldManager* worldManager) {
    for (auto& entity : entities)
        entity->update(dt, worldManager);
}

// Draw — one bind of the pipeline, then per-entity MVP + asset bind + draw.
void EntityManager::draw(
    SDL_GPUCommandBuffer* cmd,
    SDL_GPURenderPass* pass,
    const Mat4& viewProj)
{
    if (!pipeline) return;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    for (auto& entity : entities) {
        const EntityAsset* asset = entity->getAsset();
        if (!asset || !asset->vertexBuffer || !asset->indexBuffer ||
            !asset->texture || asset->numIndices == 0)
            continue;

        UBO ubo = { .mvp = mat4Mul(viewProj, entity->getModelMatrix()) };

        SDL_GPUBufferBinding vbBind = { .buffer = asset->vertexBuffer, .offset = 0 };
        SDL_GPUBufferBinding ibBind = { .buffer = asset->indexBuffer,  .offset = 0 };
        SDL_GPUTextureSamplerBinding texBind = { .texture = asset->texture, .sampler = sampler };

        SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
        SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUFragmentSamplers(pass, 0, &texBind, 1);
        SDL_PushGPUVertexUniformData(cmd, 0, &ubo, sizeof(UBO));
        SDL_DrawGPUIndexedPrimitives(pass, asset->numIndices, 1, 0, 0, 0);
    }
}

// Type-asset lookups (mirror BlockManager::getById / getByName)
EntityAsset* EntityManager::getAssetById(Uint16 id) {
    auto it = assetsById.find(id);
    return it != assetsById.end() ? it->second : nullptr;
}

EntityAsset* EntityManager::getAssetByName(const std::string& name) {
    auto it = assetsByName.find(name);
    return it != assetsByName.end() ? it->second : nullptr;
}