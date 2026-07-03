#pragma once

#include "Globals.h"
#include "SurvivalUI.h"
#include "Inventory.h"

void drawDebugUI(void* appstate) {
    AppState* state = (AppState*)appstate;

    ChunkCoord playerChunkCoords = getPlayerChunkCoord(camera.position);
    RegionCoord playerRegionCoords = getPlayerRegionCoord(camera.position);
    char buffer[256];
    const float lineH = 16.0f; // adjust to match your font size
    float y = 8.0f;
    const SDL_FColor white = { 1.0f, 1.0f, 1.0f, 1.0f };

    snprintf(buffer, sizeof(buffer), "FPS: %4.1f", fps);
    ui.drawText(buffer, 8.0f, y, white); y += lineH;

    snprintf(buffer, sizeof(buffer), "Pos: %3.1f  %3.1f  %3.1f",
        camera.position.x, camera.position.y, camera.position.z);
    ui.drawText(buffer, 8.0f, y, white); y += lineH;

    snprintf(buffer, sizeof(buffer), "Chunk: %d  %d  %d",
        playerChunkCoords.x, playerChunkCoords.y, playerChunkCoords.z);
    ui.drawText(buffer, 8.0f, y, white); y += lineH;

    snprintf(buffer, sizeof(buffer), "Region: %d  %d  %d",
        playerRegionCoords.x, playerRegionCoords.y, playerRegionCoords.z);
    ui.drawText(buffer, 8.0f, y, white); y += lineH;

    Vec3 pos = worldManager.getBlockLookingAt(camera, 20.0f);
    if (std::isnan(pos.x))
    {
        ui.drawText("[no Block in reach]", 8.0f, y, white); y += lineH;
    }
    else
    {
        Block* b = blockManager.getById(worldManager.getBlockIdAt(pos));
        snprintf(buffer, sizeof(buffer),
            "closesed Block: %.0f  %.0f  %.0f | %s",
            pos.x, pos.y, pos.z, b->getName().c_str());
        ui.drawText(buffer, 8.0f, y, white); y += lineH;
    }
}

SDL_AppResult App_Render(void* appstate)
{
    AppState* state = (AppState*)appstate;

    //ui.clearTextCache(state->gpu);

    float cx = ui.screenW * 0.5f;
    float cy = ui.screenH * 0.5f;
    ui.drawCrosshair(cx, cy, 12.0f, 2.0f, 4.0f, 1.0f, 1.0f, 1.0f, 0.9f);
    drawSurvivalUI(&ui);
    menuManager.draw(&ui);

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

    ui.upload(state->gpu, cmd);

    Mat4 viewMat = mat4LookAt(camera.position, camera.lookTarget, { 0.0f, 0.0f, -1.0f });

    Mat4 rotOnlyView = mat4LookAt(
        { 0.0f, 0.0f, 0.0f },          
        camera.forward,                  
        { 0.0f, 0.0f, -1.0f }    
    );

    Mat4 modelMat = mat4Mul(
        mat4Translate(0.0f, 0.0f, 0.0f),
        mat4Rotate(0.0f, 0.0f, 0.0f)
    );

    UBO worldUBO = {
        .mvp = mat4Mul(state->projMat, mat4Mul(viewMat, modelMat)),
    };
    UBO skyboxUBO = {
        .mvp = mat4Mul(state->projMat, rotOnlyView),
    };

    if (swapchain_tex) {

        SDL_GPUColorTargetInfo skybox_color = {
        .texture = swapchain_tex,
        .clear_color = { 0.0f, 0.2f, 0.4f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        };

        SDL_GPURenderPass* skyboxPass = SDL_BeginGPURenderPass(
            cmd,
            &skybox_color,
            1,
            nullptr
        );
        skybox.draw(skyboxPass, cmd, skyboxUBO);
        SDL_EndGPURenderPass(skyboxPass);

        SDL_GPUColorTargetInfo world_color = {
            .texture = swapchain_tex,
            .clear_color = { 0.0f, 0.2f, 0.4f, 1.0f },
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPUDepthStencilTargetInfo depth_target = {
            .texture = state->depth_texture,
            .clear_depth = 1.0,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };

        SDL_GPURenderPass* worldPass = SDL_BeginGPURenderPass(
            cmd,
            &world_color,
            1,
            &depth_target
        );
        worldManager.draw(state, cmd, worldPass, worldUBO);
        entityManager.draw(cmd, worldPass, mat4Mul(state->projMat, viewMat));

        SDL_EndGPURenderPass(worldPass);

        SDL_GPUColorTargetInfo ui_color = {
            .texture = swapchain_tex,
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
        };

        SDL_GPURenderPass* uiPass = SDL_BeginGPURenderPass(cmd, &ui_color, 1, nullptr);
        ui.draw(uiPass);
        SDL_EndGPURenderPass(uiPass);
    }

    SDL_SubmitGPUCommandBuffer(cmd);

    return SDL_APP_CONTINUE;
}
