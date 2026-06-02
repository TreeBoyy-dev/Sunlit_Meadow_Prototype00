#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>
#include <string>

SDL_GPUTexture* loadTextureFromFile(
    SDL_GPUDevice* gpu,
    const char* filePath,
    const char* fileName
);