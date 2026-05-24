#pragma once

#include "DataStructures.h"
#include <string>

typedef enum {
    //Stone Types
    MATERIAL_COBBLESTONE,
    MATERIAL_DIORITE,
    MATERIAL_MARBLE,
    MATERIAL_CHALK,
    MATERIAL_GNEISS,
    //dirt like blocks
    MATERIAL_DIRT,
    MATERIAL_GRASS_BLOCK,
    //wood types
    MATERIAL_BIRCH_LOG_SIDE,
    MATERIAL_BIRCH_LOG_TOP,
    MATERIAL_BIRCH_LEAVES,
    MATERIAL_CHESTNUT_LOG_SIDE,
    MATERIAL_CHESTNUT_LOG_TOP,
    MATERIAL_CHESTNUT_LEAVES,

    MATERIAL_COUNT //ALWAYS LAST!!
}Material;

std::string BuildAbsolutePath(
    const char* filePath,
    const char* fileName
);

bool UploadTextureArrayLayer(
    SDL_GPUDevice* gpu,
    SDL_GPUTexture* textureArray,
    const char* filePath,
    const char* fileName,
    Material material
);

bool UploadTextureArrayLayers(
    SDL_GPUDevice* gpu,
    SDL_GPUTexture* textureArray
);

