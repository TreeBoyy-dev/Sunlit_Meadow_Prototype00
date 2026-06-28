#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "LoadTextureFromFile.h"
#include "BuildAbsolutePath.h"

// Uploads a tightly-packed RGBA32 surface to a fresh 2D SAMPLER texture.
// Does not take ownership of the surface; the caller frees it.
static bool uploadSurfaceAsTexture(
    GPUTextureWH* gpuTextureWH,
    SDL_GPUDevice* gpu,
    SDL_Surface* surface)
{
    const Uint32 width = (Uint32)surface->w;
    const Uint32 height = (Uint32)surface->h;
    const Uint32 dataSize = width * height * 4;

    SDL_GPUTextureCreateInfo texInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture* texture = SDL_CreateGPUTexture(gpu, &texInfo);
    if (!texture) {
        SDL_Log("SDL_CreateGPUTexture failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo tInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = dataSize,
    };
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(gpu, &tInfo);

    void* mapped = SDL_MapGPUTransferBuffer(gpu, transfer, false);
    SDL_memcpy(mapped, surface->pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(gpu, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {
        .transfer_buffer = transfer,
        .offset = 0,
        .pixels_per_row = width,
        .rows_per_layer = height,
    };
    SDL_GPUTextureRegion dst = {
        .texture = texture,
        .mip_level = 0,
        .layer = 0,
        .x = 0, .y = 0, .z = 0,
        .w = width, .h = height, .d = 1,
    };
    SDL_UploadToGPUTexture(copy, &src, &dst, false);

    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(gpu, transfer);

    gpuTextureWH->width = width;
    gpuTextureWH->height = height;
    gpuTextureWH->texture = texture;
    return true;
}

// Loads a PNG and converts it to RGBA32. Returns null on failure (or if
// fileName is null). Caller frees the returned surface.
static SDL_Surface* loadSurfaceRGBA(const char* filePath, const char* fileName)
{
    if (!fileName) return nullptr;

    std::string fullPath = BuildAbsolutePath(filePath, fileName);

    SDL_Surface* loaded = SDL_LoadSurface(fullPath.c_str());
    if (!loaded) {
        SDL_Log("SDL_LoadSurface failed for '%s': %s", fullPath.c_str(), SDL_GetError());
        return nullptr;
    }
    SDL_Surface* surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!surface) {
        SDL_Log("SDL_ConvertSurface failed for '%s': %s", fullPath.c_str(), SDL_GetError());
        return nullptr;
    }
    return surface;
}

bool loadTextureFromFile(
    GPUTextureWH* gpuTextureWH,
    SDL_GPUDevice* gpu,
    const char* filePath,
    const char* fileName)
{
    std::string fullPath = BuildAbsolutePath(filePath, fileName);

    SDL_Surface* loaded = SDL_LoadSurface(fullPath.c_str());
    if (!loaded) {
        SDL_Log("SDL_LoadSurface failed for '%s': %s", fullPath.c_str(), SDL_GetError());
        return false;
    }
    SDL_Surface* surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!surface) {
        SDL_Log("SDL_ConvertSurface failed: %s", SDL_GetError());
        return false;
    }

    const Uint32 width = (Uint32)surface->w;
    const Uint32 height = (Uint32)surface->h;
    const Uint32 dataSize = width * height * 4;

    SDL_GPUTextureCreateInfo texInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture* texture = SDL_CreateGPUTexture(gpu, &texInfo);
    if (!texture) {
        SDL_Log("SDL_CreateGPUTexture failed: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_GPUTransferBufferCreateInfo tInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = dataSize,
    };
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(gpu, &tInfo);

    void* mapped = SDL_MapGPUTransferBuffer(gpu, transfer, false);
    SDL_memcpy(mapped, surface->pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(gpu, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {
        .transfer_buffer = transfer,
        .offset = 0,
        .pixels_per_row = width,
        .rows_per_layer = height,
    };
    SDL_GPUTextureRegion dst = {
        .texture = texture,
        .mip_level = 0,
        .layer = 0,
        .x = 0, .y = 0, .z = 0,
        .w = width, .h = height, .d = 1,
    };
    SDL_UploadToGPUTexture(copy, &src, &dst, false);

    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(gpu, transfer);
    SDL_DestroySurface(surface);

    gpuTextureWH->width = width;
    gpuTextureWH->height = height;
    gpuTextureWH->texture = texture;

    return true;
}

bool buildBlockIconTexture(
    GPUTextureWH* gpuTextureWH,
    SDL_GPUDevice* gpu,
    const char* filePath,
    const char* sideFile,
    const char* topFile,
    const char* bottomFile)
{
    SDL_Surface* side = loadSurfaceRGBA(filePath, sideFile);
    if (!side) {
        SDL_Log("buildBlockIconTexture: side texture is required");
        return false;
    }
    // top/bottom are optional; null cells reuse the side image.
    SDL_Surface* top = loadSurfaceRGBA(filePath, topFile);
    SDL_Surface* bottom = loadSurfaceRGBA(filePath, bottomFile);

    const int tileW = side->w;
    const int tileH = side->h;

    // Cell order: 0 = side, 1 = top, 2 = bottom (must match Block::buildItemModel).
    SDL_Surface* cells[3] = { side, top ? top : side, bottom ? bottom : side };

    SDL_Surface* atlas = SDL_CreateSurface(tileW * 3, tileH, SDL_PIXELFORMAT_RGBA32);
    bool ok = false;
    if (!atlas) {
        SDL_Log("buildBlockIconTexture: SDL_CreateSurface failed: %s", SDL_GetError());
    }
    else {
        for (int c = 0; c < 3; ++c) {
            SDL_Rect srcRect = { 0, 0, cells[c]->w, cells[c]->h };
            SDL_Rect dstRect = { c * tileW, 0, tileW, tileH };
            // Scale-blit so a differently sized top/bottom still fills its cell.
            SDL_BlitSurfaceScaled(cells[c], &srcRect, atlas, &dstRect, SDL_SCALEMODE_NEAREST);
        }
        ok = uploadSurfaceAsTexture(gpuTextureWH, gpu, atlas);
        SDL_DestroySurface(atlas);
    }

    SDL_DestroySurface(side);
    if (top)    SDL_DestroySurface(top);
    if (bottom) SDL_DestroySurface(bottom);
    return ok;
}