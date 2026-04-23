#pragma once

#include "DataStructures.h"
#include <string>

typedef enum {
	BLOCK_COBBLESTONE,
	BLOCK_DIORITE,
	BLOCK_DIRT,
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

