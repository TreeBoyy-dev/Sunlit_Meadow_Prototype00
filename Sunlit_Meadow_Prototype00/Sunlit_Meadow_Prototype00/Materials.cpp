#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Materials.h"

const char* baseTexturePathMaterials = "Textures/Blocks/";

bool UploadTextureArrayLayer(
    SDL_GPUDevice* gpu,
    SDL_GPUTexture* textureArray,
    const char* filePath,
    const char* fileName,
    Material material
)
{
    if (!gpu || !textureArray || !filePath || !fileName) {
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
        SDL_CreateGPUTransferBuffer(gpu, &transferInfo);

    if (!transferBuffer) {
        SDL_Log("SDL_CreateGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(gpu, transferBuffer, false);
    if (!mapped) {
        SDL_Log("SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu, transferBuffer);
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_memcpy(mapped, surface->pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(gpu, transferBuffer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
    if (!cmd) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu, transferBuffer);
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

    SDL_ReleaseGPUTransferBuffer(gpu, transferBuffer);
    SDL_DestroySurface(surface);

    return true;
}

const char* materialTextureFile(Material material) {
    switch (material) {
    case MATERIAL_COBBLESTONE:       return "cobblestone.png";
    case MATERIAL_DIORITE:           return "diorite.png";
    case MATERIAL_MARBLE:            return "marble.png";
    case MATERIAL_CHALK:             return "chalk.png";
    case MATERIAL_GNEISS:            return "gneiss.png";
    case MATERIAL_DIRT:              return "dirt.png";
    case MATERIAL_GRASS_BLOCK:       return "grass_block.png";
    case MATERIAL_BIRCH_LOG_SIDE:    return "birch_log_side.png";
    case MATERIAL_BIRCH_LOG_TOP:     return "birch_log_top.png";
    case MATERIAL_BIRCH_LEAVES:      return "birch_leaves.png";
    case MATERIAL_CHESTNUT_LOG_SIDE: return "chestnut_log_side.png";
    case MATERIAL_CHESTNUT_LOG_TOP:  return "chestnut_log_top.png";
    case MATERIAL_CHESTNUT_LEAVES:   return "chestnut_leaves.png";
    case MATERIAL_AIR:
    default:                         return nullptr;
    }
}

bool UploadTextureArrayLayers(
    SDL_GPUDevice* gpu,
    SDL_GPUTexture* textureArray
) {
    // Skip MATERIAL_AIR (no texture); every other material maps to one layer.
    for (int m = MATERIAL_AIR + 1; m < MATERIAL_COUNT; ++m) {
        Material material = (Material)m;
        const char* file = materialTextureFile(material);
        if (!file) continue;

        if (!UploadTextureArrayLayer(gpu, textureArray, baseTexturePathMaterials, file, material)) {
            SDL_Log("Failed to load Texture for material %d ('%s')", m, file);
            return false;
        }
    }
    return true;
}