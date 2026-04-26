#pragma once

#include "Globals.h"

SDL_AppResult App_Render(void* appstate)
{
    AppState* state = (AppState*)appstate;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(state->gpu);
    if (!cmd) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture* swapchain_tex = NULL;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
        cmd, state->window, &swapchain_tex, NULL, NULL))
    {
        SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    Mat4 viewMat = mat4LookAt(camera.position, camera.lookTarget, { 0.0f, 0.0f, -1.0f });

    Mat4 modelMat = mat4Mul(
        mat4Translate(0.0f, 0.0f, 0.0f),
        mat4Rotate(0.0f, 0.0f, 0.0f) //mat4Rotate(-3.1415962/2, 0.0f, 0.0f)
    );
    UBO ubo = {
        .mvp = mat4Mul(state->projMat, mat4Mul(viewMat, modelMat)),
    };

    if (swapchain_tex) {

        SDL_GPUColorTargetInfo color_target = {
            .texture = swapchain_tex,
            .clear_color = { 0.0f, 0.2f, 0.4f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };

        SDL_GPUDepthStencilTargetInfo depth_target = {
            .texture = state->depth_texture,
            .clear_depth = 1,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
        };

        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(
            cmd,
            &color_target,
            1,
            &depth_target
        );

        testManager.drawChunks(
            state,
            cmd,
            pass,
            ubo
        );

        SDL_EndGPURenderPass(pass);
    }

    SDL_SubmitGPUCommandBuffer(cmd);

    return SDL_APP_CONTINUE;
}
