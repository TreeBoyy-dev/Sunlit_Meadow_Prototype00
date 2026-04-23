#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Materials.h"
#include <string>

std::string BuildAbsolutePath(
    const char* filePath,
    const char* fileName
)
{
    const char* basePath = SDL_GetBasePath();
    if (!basePath) {
        SDL_Log("SDL_GetBasePath failed: %s", SDL_GetError());
        return "";
    }

    std::string fullPath = std::string(basePath);

    if (filePath && filePath[0] != '\0') {
        fullPath += filePath;

        if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += "/";
        }
    }

    fullPath += fileName;
    return fullPath;
}

bool UploadTextureArrayLayer(
    AppState* state,
    SDL_GPUTexture* textureArray,
    const char* filePath,
    const char* fileName,
    Material material
)
{
    if (!state || !state->gpu || !textureArray || !filePath || !fileName) {
        SDL_Log("UploadTextureArrayLayer: invalid argument");
        return false;
    }

    std::string fullPath = BuildAbsolutePath(filePath, fileName);

    SDL_Surface* loadedSurface = SDL_LoadSurface(fullPath.c_str());
    if (!loadedSurface) {
        SDL_Log("SDL_LoadSurface failed for '%s': %s", fullPath.c_str(), SDL_GetError());
        return false;
    }

    SDL_Surface* surface = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loadedSurface);

    if (!surface) {
        SDL_Log("SDL_ConvertSurface failed for '%s': %s", fullPath.c_str(), SDL_GetError());
        return false;
    }

    const Uint32 width = (Uint32)surface->w;
    const Uint32 height = (Uint32)surface->h;
    const Uint32 bytesPerPixel = 4;
    const Uint32 dataSize = width * height * bytesPerPixel;

    SDL_GPUTransferBufferCreateInfo transferInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = dataSize
    };

    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(state->gpu, &transferInfo);

    if (!transferBuffer) {
        SDL_Log("SDL_CreateGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(state->gpu, transferBuffer, false);
    if (!mapped) {
        SDL_Log("SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(state->gpu, transferBuffer);
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_memcpy(mapped, surface->pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(state->gpu, transferBuffer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(state->gpu);
    if (!cmd) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(state->gpu, transferBuffer);
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {
        .transfer_buffer = transferBuffer,
        .offset = 0,
        .pixels_per_row = width,
        .rows_per_layer = height
    };

    SDL_GPUTextureRegion dst = {
        .texture = textureArray,
        .mip_level = 0,
        .layer = (Uint32)material,
        .x = 0,
        .y = 0,
        .z = 0,
        .w = width,
        .h = height,
        .d = 1
    };

    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(state->gpu, transferBuffer);
    SDL_DestroySurface(surface);

    return true;
}

bool UploadTextureArrayLayers(
    AppState* state,
    SDL_GPUTexture* textureArray
){
    if (!UploadTextureArrayLayer(state, textureArray, "Textures/", "cobblestone.png", BLOCK_COBBLESTONE))
    {
        SDL_Log("Failed to load Texture: cobblestone");
        return false;
    }
    if (!UploadTextureArrayLayer(state, textureArray, "Textures/", "diorite.png", BLOCK_DIORITE))
    {
        SDL_Log("Failed to load Texture: diorite");
        return false;
    }
    if (!UploadTextureArrayLayer(state, textureArray, "Textures/", "cobblestone.png", BLOCK_DIRT))
    {
        SDL_Log("Failed to load Texture: dirt");
        return false;
    }

    return true;
}