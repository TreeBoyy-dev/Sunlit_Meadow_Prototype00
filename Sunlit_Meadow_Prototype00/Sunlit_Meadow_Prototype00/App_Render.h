#pragma once

#include "Globals.h"

void drawDebugUI(void* appstate) {
    AppState* state = (AppState*)appstate;

    ChunkCoord playerChunkCoords = getPlayerChunkCoord(camera.position);
    RegionCoord playerRegionCoords = getPlayerRegionCoord(camera.position);
    char buffer[256];
    const float lineH = 16.0f; // adjust to match your font size
    float y = 8.0f;
    const SDL_FColor white = { 1.0f, 1.0f, 1.0f, 1.0f };

    snprintf(buffer, sizeof(buffer), "FPS: %4.1f", fps);
    state->ui.drawText(buffer, 8.0f, y, white); y += lineH;

    snprintf(buffer, sizeof(buffer), "Pos: %3.1f  %3.1f  %3.1f",
        camera.position.x, camera.position.y, camera.position.z);
    state->ui.drawText(buffer, 8.0f, y, white); y += lineH;

    snprintf(buffer, sizeof(buffer), "Chunk: %d  %d  %d",
        playerChunkCoords.x, playerChunkCoords.y, playerChunkCoords.z);
    state->ui.drawText(buffer, 8.0f, y, white); y += lineH;

    snprintf(buffer, sizeof(buffer), "Region: %d  %d  %d",
        playerRegionCoords.x, playerRegionCoords.y, playerRegionCoords.z);
    state->ui.drawText(buffer, 8.0f, y, white); y += lineH;
}

SDL_AppResult App_Render(void* appstate)
{
    AppState* state = (AppState*)appstate;

    //state->ui.clearTextCache(state->gpu);

    float cx = state->ui.screenW * 0.5f;
    float cy = state->ui.screenH * 0.5f;
    state->ui.drawCrosshair(cx, cy, 12.0f, 2.0f, 4.0f, 1.0f, 1.0f, 1.0f, 0.9f);
    //state->ui.drawText("hello UI", 20.0f, 20.0f, { 1.0f, 1.0f, 1.0f, 1.0f });

    if (renderDebugUI)
        drawDebugUI(state);

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

    state->ui.upload(state->gpu, cmd);

    Mat4 viewMat = mat4LookAt(camera.position, camera.lookTarget, { 0.0f, 0.0f, -1.0f });

    Mat4 modelMat = mat4Mul(
        mat4Translate(0.0f, 0.0f, 0.0f),
        mat4Rotate(0.0f, 0.0f, 0.0f)
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

        SDL_GPURenderPass* worldPass = SDL_BeginGPURenderPass(
            cmd,
            &color_target,
            1,
            &depth_target
        );

        worldManager.drawChunks(state, cmd, worldPass, ubo);

        SDL_EndGPURenderPass(worldPass);

        // UI pass
        SDL_GPUColorTargetInfo ui_target = {
            .texture = swapchain_tex,
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPURenderPass* uiPass = SDL_BeginGPURenderPass(cmd, &ui_target, 1, nullptr);
        state->ui.draw(uiPass);

        SDL_EndGPURenderPass(uiPass);
    }

    SDL_SubmitGPUCommandBuffer(cmd);

    return SDL_APP_CONTINUE;
}
