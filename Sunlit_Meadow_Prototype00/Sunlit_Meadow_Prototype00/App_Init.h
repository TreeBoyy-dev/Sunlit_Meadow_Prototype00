#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Globals.h"
#include "InitPipeline.h"
#include "Materials.h"

SDL_AppResult App_Init(void* appstate)
{
    AppState* state = (AppState*)appstate;

    SDL_SetAppMetadata("Sunlit Meadow", "0.1", "dev.sunlit-meadow");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->window = SDL_CreateWindow("Hello SDL3", 1280, 780, 0);
    if (!state->window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!state->gpu) {
        SDL_Log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(state->gpu, state->window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    int w, h;
    SDL_GetWindowSize(state->window, &w, &h);
    SDL_GPUTextureCreateInfo depth_tex_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = depth_texture_format,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = (Uint32)w,
        .height = (Uint32)h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    state->depth_texture = SDL_CreateGPUTexture(state->gpu, &depth_tex_info);

    App_InitPipeline(state);

    Uint32 layerCount = (Uint32)MATERIAL_COUNT;
    SDL_GPUTextureCreateInfo texInfo = {
    .type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    .width = 32,
    .height = 32,
    .layer_count_or_depth = layerCount,
    .num_levels = 1,
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
    .props = 0
    };
    state->textureArray = SDL_CreateGPUTexture(state->gpu, &texInfo);

    if (!UploadTextureArrayLayers(state, state->textureArray))
    {
        SDL_Log("failed loading textures");
        return SDL_APP_FAILURE;
    }

    testManager.calcVisibleChunksList(RENDER_DISTANCE);
    testManager.updateRenderList(camera.position, RENDER_DISTANCE, state, state->textureArray);

    SDL_GPUSamplerCreateInfo sampler_info = {};
    state->sampler = SDL_CreateGPUSampler(state->gpu, &sampler_info);

    return SDL_APP_CONTINUE;
}