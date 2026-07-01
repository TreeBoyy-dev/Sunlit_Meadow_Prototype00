#pragma once

#include "DataStructures.h"
#include "BuildAbsolutePath.h"
#include <string>

typedef enum {
    MATERIAL_AIR,
    //Stone Types
    MATERIAL_COBBLESTONE,
    MATERIAL_DIORITE,
    //dirt like blocks
    MATERIAL_DIRT,
    MATERIAL_GRASS_BLOCK_TOP,
    MATERIAL_GRASS_BLOCK_SIDE,
    MATERIAL_GRASS_BLOCK_SIDE_OV,
    //wood types
    MATERIAL_BIRCH_LOG_SIDE,
    MATERIAL_BIRCH_LOG_TOP,
    MATERIAL_BIRCH_LEAVES,
    MATERIAL_CHESTNUT_LOG_SIDE,
    MATERIAL_CHESTNUT_LOG_TOP,
    MATERIAL_CHESTNUT_LEAVES,

    MATERIAL_COUNT //ALWAYS LAST!!
}Material;

struct ModelFace {
    Material material;
    int overlayMaterial = -1;   // -1 = no overlay
};

const char* materialTextureFile(Material material);

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

