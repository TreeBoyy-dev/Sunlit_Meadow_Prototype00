#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>
#include <string>

typedef struct GPUTextureWH {
    SDL_GPUTexture* texture;
    Uint32 width;
    Uint32 height;
};

bool loadTextureFromFile(
    GPUTextureWH* gpuTextureWH,
    SDL_GPUDevice* gpu,
    const char* filePath,
    const char* fileName
);

// Composites up to three block-material PNGs into a single 2D texture laid
// out as a horizontal 3-cell atlas: [ side | top | bottom ]. topFile and/or
// bottomFile may be null, in which case that cell reuses sideFile. The cell
// order must match Block::buildItemModel's UV remap. Fills gpuTextureWH on
// success. sideFile is required.
bool buildBlockIconTexture(
    GPUTextureWH* gpuTextureWH,
    SDL_GPUDevice* gpu,
    const char* filePath,
    const char* sideFile,
    const char* topFile,
    const char* bottomFile
);