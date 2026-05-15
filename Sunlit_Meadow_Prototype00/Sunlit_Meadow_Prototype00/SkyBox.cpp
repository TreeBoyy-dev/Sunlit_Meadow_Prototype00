#include <cmath>
#include <cstring>

#include "LoadShader.h"
#include "SkyBox.h"

Skybox::Skybox()
{}

bool Skybox::init(
    AppState* state,
    SDL_GPUTextureFormat swapchainFormat,
    const char* filePath,
    const char* fileName)
{
    verts = {
        // +X (Front)
        { 1,-1,-1}, { 1,-1, 1}, { 1, 1, 1},
        { 1, 1, 1}, { 1, 1,-1}, { 1,-1,-1},

        // -X (Back)
        {-1,-1, 1}, {-1,-1,-1}, {-1, 1,-1},
        {-1, 1,-1}, {-1, 1, 1}, {-1,-1, 1},

        // +Y (Right)
        {-1, 1,-1}, { 1, 1,-1}, { 1, 1, 1},
        { 1, 1, 1}, {-1, 1, 1}, {-1, 1,-1},

        // -Y (Left)
        {-1,-1, 1}, { 1,-1, 1}, { 1,-1,-1},
        { 1,-1,-1}, {-1,-1,-1}, {-1,-1, 1},

        // +Z (Top)
        {-1,-1, 1}, {-1, 1, 1}, { 1, 1, 1},
        { 1, 1, 1}, { 1,-1, 1}, {-1,-1, 1},

        // -Z (Bottom)
        { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
        {-1, 1,-1}, {-1,-1,-1}, { 1,-1,-1},
    };

    cubemap = LoadCubemap(state, filePath, fileName);
    if (!cubemap) {
        SDL_Log("[Sky] loading cubemao texture failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUShader* vert = loadShader(state->gpu, "skybox.vert.spv", 1, 0);
    SDL_GPUShader* frag = loadShader(state->gpu, "skybox.frag.spv", 0, 1);

    if (!vert || !frag) return false;

    SDL_GPUVertexAttribute attrs[1] = {
        {
            .location = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0
        },
    };
    SDL_GPUVertexBufferDescription vbDesc = {
        .slot = 0,
        .pitch = sizeof(Vec3),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    SDL_GPUDepthStencilState depth = {
        .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
        .enable_depth_test = false,
        .enable_depth_write = false,
    };
    SDL_GPURasterizerState raster = {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_NONE,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
    };
    SDL_GPUColorTargetDescription colorTarget = {
        .format = SDL_GetGPUSwapchainTextureFormat(state->gpu, state->window)
    };
    SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &vbDesc,
            .num_vertex_buffers = 1,
            .vertex_attributes = attrs,
            .num_vertex_attributes = 1,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = raster,
        .depth_stencil_state = depth,
        .target_info = {
            .color_target_descriptions = &colorTarget,
            .num_color_targets = 1,
            .has_depth_stencil_target = false,
        }
    };

    pipeline = SDL_CreateGPUGraphicsPipeline(state->gpu, &pipeInfo);
    SDL_ReleaseGPUShader(state->gpu, vert);
    SDL_ReleaseGPUShader(state->gpu, frag);
    if (!pipeline) {
        SDL_Log("[Sky] pipeline failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUBufferCreateInfo bufInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = (Uint32)(verts.size() * sizeof(Vec3)),
    };
    vertexBuffer = SDL_CreateGPUBuffer(state->gpu, &bufInfo);
    if (!vertexBuffer) {
        SDL_Log("[sky] — SDL_CreateGPUBuffer failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUSamplerCreateInfo samplerInfo = {
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };

    sampler = SDL_CreateGPUSampler(state->gpu, &samplerInfo);
    if (!sampler) {
        SDL_Log("[sky] — SDL_CreateGPUSampler failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

void Skybox::destroy(SDL_GPUDevice* gpu)
{
    if (vertexBuffer) SDL_ReleaseGPUBuffer(gpu, vertexBuffer);
    if (pipeline)     SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);
}

void Skybox::upload(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd) {

    Uint32 bufSize = (Uint32)(verts.size() * sizeof(Vec3));

    // Staging buffer
    SDL_GPUTransferBufferCreateInfo tInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = bufSize,
    };
    SDL_GPUTransferBuffer* transferBuf = SDL_CreateGPUTransferBuffer(gpu, &tInfo);

    // Copy verts into staging
    void* mapped = SDL_MapGPUTransferBuffer(gpu, transferBuf, false);
    memcpy(mapped, verts.data(), bufSize);
    SDL_UnmapGPUTransferBuffer(gpu, transferBuf);

    // Upload via the provided command buffer
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = { .transfer_buffer = transferBuf, .offset = 0 };
    SDL_GPUBufferRegion           dst = { .buffer = vertexBuffer, .offset = 0, .size = bufSize };

    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);

    verts.clear();
    // Release staging — safe once the copy pass is ended
    SDL_ReleaseGPUTransferBuffer(gpu, transferBuf);
}

void Skybox::draw(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd, const UBO& ubo)
{
    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUBufferBinding vbBind = { .buffer = vertexBuffer, .offset = 0 };
    SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);

    SDL_GPUTextureSamplerBinding texBind = { .texture = cubemap, .sampler = sampler };
    SDL_BindGPUFragmentSamplers(pass, 0, &texBind, 1);

    SDL_PushGPUVertexUniformData(cmd, 0, &ubo, sizeof(UBO));

    SDL_DrawGPUPrimitives(pass, 36, 1, 0, 0);
}

SDL_GPUTexture* Skybox::LoadCubemap(
    AppState* state,
    const char* filePath,
    const char* fileName)
{
    if (!state || !state->gpu || !filePath || !fileName) {
        SDL_Log("LoadCubemap: invalid argument");
        return nullptr;
    }

    const char* basePath = SDL_GetBasePath();
    if (!basePath) {
        SDL_Log("SDL_GetBasePath failed: %s", SDL_GetError());
        return nullptr;
    }

    std::string fullPath = std::string(basePath);

    if (filePath && filePath[0] != '\0') {
        fullPath += filePath;

        if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += "/";
        }
    }

    fullPath += fileName;

    SDL_Surface* loadedSurface = SDL_LoadSurface(fullPath.c_str());
    if (!loadedSurface) {
        SDL_Log("SDL_LoadSurface failed for '%s': %s", fullPath.c_str(), SDL_GetError());
        return nullptr;
    }

    SDL_Surface* surface = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loadedSurface);
    if (!surface) {
        SDL_Log("SDL_ConvertSurface failed: %s", SDL_GetError());
        return nullptr;
    }

    // Assumes horizontal cross layout (4:3 ratio):
    //        [ +Y ]
    //  [-X]  [ +Z ]  [+X]  [-Z]
    //        [ -Y ]
    const Uint32 faceSize = (Uint32)surface->w / 4;

    // (col, row) offsets in face units, indexed by SDL cubemap face order
    // +X=0  -X=1  +Y=2  -Y=3  +Z=4  -Z=5
    const int faceOffsets[6][2] = {
        {2, 1}, // +X
        {0, 1}, // -X
        {1, 0}, // +Y
        {1, 2}, // -Y
        {1, 1}, // +Z
        {3, 1}, // -Z
    };

    // Create cubemap texture
    SDL_GPUTextureCreateInfo texInfo = {
        .type = SDL_GPU_TEXTURETYPE_CUBE,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = faceSize,
        .height = faceSize,
        .layer_count_or_depth = 6,
        .num_levels = 1
    };
    SDL_GPUTexture* cubemap = SDL_CreateGPUTexture(state->gpu, &texInfo);
    if (!cubemap) {
        SDL_Log("SDL_CreateGPUTexture failed: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return nullptr;
    }

    // One transfer buffer for all 6 faces
    const Uint32 faceBytes = faceSize * faceSize * 4;
    SDL_GPUTransferBufferCreateInfo tInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = faceBytes * 6
    };
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(state->gpu, &tInfo);
    if (!transferBuffer) {
        SDL_Log("SDL_CreateGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return nullptr;
    }

    Uint8* mapped = (Uint8*)SDL_MapGPUTransferBuffer(state->gpu, transferBuffer, false);
    if (!mapped) {
        SDL_Log("SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(state->gpu, transferBuffer);
        SDL_DestroySurface(surface);
        return nullptr;
    }

    // Extract each face row-by-row from the cross image
    for (int face = 0; face < 6; face++) {
        const Uint32 col = faceOffsets[face][0];
        const Uint32 row = faceOffsets[face][1];

        const Uint8* srcBase = (Uint8*)surface->pixels
            + (row * faceSize) * surface->pitch
            + (col * faceSize) * 4;

        Uint8* dstBase = mapped + face * faceBytes;

        for (Uint32 y = 0; y < faceSize; y++) {
            SDL_memcpy(dstBase + y * faceSize * 4,
                srcBase + y * surface->pitch,
                faceSize * 4);
        }
    }

    SDL_UnmapGPUTransferBuffer(state->gpu, transferBuffer);
    SDL_DestroySurface(surface);

    // Upload all 6 faces in one copy pass
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(state->gpu);
    if (!cmd) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(state->gpu, transferBuffer);
        return nullptr;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    for (int face = 0; face < 6; face++) {
        SDL_GPUTextureTransferInfo src = {
            .transfer_buffer = transferBuffer,
            .offset = (Uint32)(face * faceBytes),
            .pixels_per_row = faceSize,
            .rows_per_layer = faceSize
        };
        SDL_GPUTextureRegion dst = {
            .texture = cubemap,
            .mip_level = 0,
            .layer = (Uint32)face,
            .w = faceSize, .h = faceSize, .d = 1
        };
        SDL_UploadToGPUTexture(copyPass, &src, &dst, false);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(state->gpu, transferBuffer);

    return cubemap;
}