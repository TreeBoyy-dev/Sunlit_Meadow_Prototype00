#pragma once

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stddef.h>

static SDL_GPUShader* loadShader(
    SDL_GPUDevice* gpu,
    const char* path,
    Uint32 num_uniform_buffers,
    Uint32 num_samplers)
{
    // Infer stage from file extension
    SDL_GPUShaderStage stage;
    if (SDL_strstr(path, ".vert.spv")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (SDL_strstr(path, ".frag.spv")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else {
        SDL_Log("loadShader: could not infer shader stage from '%s'", path);
        return NULL;
    }

    size_t code_size = 0;
    void* code = SDL_LoadFile(path, &code_size);
    if (!code) {
        SDL_Log("loadShader: could not read '%s': %s", path, SDL_GetError());
        return NULL;
    }

    SDL_GPUShaderCreateInfo info = {
        .code_size = code_size,
        .code = (const Uint8*)code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = num_samplers,
        .num_uniform_buffers = num_uniform_buffers,
    };

    SDL_GPUShader* shader = SDL_CreateGPUShader(gpu, &info);
    SDL_free(code);

    if (!shader) {
        SDL_Log("loadShader: SDL_CreateGPUShader failed for '%s': %s",
            path, SDL_GetError());
    }
    return shader;
}
