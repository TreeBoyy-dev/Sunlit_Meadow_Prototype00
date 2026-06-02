#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "LoadTextureFromFile.h"
#include "BuildAbsolutePath.h"

SDL_GPUTexture* loadTextureFromFile(
    SDL_GPUDevice* gpu,
    const char* filePath,
    const char* fileName)
{
    std::string fullPath = BuildAbsolutePath(filePath, fileName);

    SDL_Surface* loaded = SDL_LoadSurface(fullPath.c_str());
    if (!loaded) {
        SDL_Log("SDL_LoadSurface failed for '%s': %s", fullPath.c_str(), SDL_GetError());
        return nullptr;
    }
    SDL_Surface* surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!surface) {
        SDL_Log("SDL_ConvertSurface failed: %s", SDL_GetError());
        return nullptr;
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
        return nullptr;
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

    return texture;
}
