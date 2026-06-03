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