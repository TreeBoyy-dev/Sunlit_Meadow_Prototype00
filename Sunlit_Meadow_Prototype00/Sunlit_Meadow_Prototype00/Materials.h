#pragma once

#include "DataStructures.h"
#include <string>

typedef enum {
    //Stone Types
    MATERIAL_COBBLESTONE,
    MATERIAL_DIORITE,
    MATERIAL_MARBLE,
    MATERIAL_GNEISS,
    //dirt like blocks
    MATERIAL_DIRT,
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
    AppState* state,
    SDL_GPUTexture* textureArray,
    const char* filePath,
    const char* fileName,
    Material material
);

bool UploadTextureArrayLayers(
    AppState* state,
    SDL_GPUTexture* textureArray
);

