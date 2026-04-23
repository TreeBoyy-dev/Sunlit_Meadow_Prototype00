#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stddef.h>

#include "DataStructures.h"

extern const SDL_GPUTextureFormat depth_texture_format;

//loading a shader from a file to the gpu device
static SDL_GPUShader* loadShader(SDL_GPUDevice* gpu,
    const char* path,
    SDL_GPUShaderStage stage,
    Uint32 num_uniform_buffers,
    Uint32 num_samplers);

//Initializing the Graphics pipeline
SDL_AppResult App_InitPipeline(void* appstate);
