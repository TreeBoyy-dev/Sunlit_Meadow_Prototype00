#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>

#include "StoneBlocks.h"
#include "Mat4.h"
#include "Materials.h"
#include "DataStructures.h"
#include "App_Render.h"
#include "App_Update.h"
#include "App_Init.h"
#include "Globals.h"
#include "InitPipeline.h"
#include "Vectors.h"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    AppState* state = (AppState*)SDL_calloc(1, sizeof(AppState));
    if (!state) { return SDL_APP_FAILURE; }
    *appstate = state;

    if (App_Init(state) != SDL_APP_CONTINUE) {
        SDL_Log("returned Error on App_Init");
        return SDL_APP_FAILURE;
    }

    int w, h;
    SDL_GetWindowSize(state->window, &w, &h);

    float fovY = 70.0f * (float)SDL_PI_F / 180.0f;
    float aspect = (float)w / (float)h;
    state->projMat = mat4Perspective(fovY, aspect, 0.0001f, 1000.0f);

    state->lastTicks = SDL_GetTicks();
    state->rotation = 0.0f;

    SDL_SetWindowRelativeMouseMode(state->window, true);

    return SDL_APP_CONTINUE; /* all good, proceed to the game loop */
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN &&
        event->key.scancode == SDL_SCANCODE_ESCAPE) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        mouseMovement.u += event->motion.xrel;
        mouseMovement.v += event->motion.yrel;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    AppState* state = (AppState*)appstate;

    SDL_AppResult r = App_Update(state);
    if (r != SDL_APP_CONTINUE) return r;

    r = App_Render(state);
    if (r != SDL_APP_CONTINUE) return r;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    AppState* state = (AppState*)appstate;
    if (state) {
        if (state->pipeline)
            SDL_ReleaseGPUGraphicsPipeline(state->gpu, state->pipeline);
        if (state->gpu)
            SDL_DestroyGPUDevice(state->gpu);
        if (state->window)
            SDL_DestroyWindow(state->window);
        SDL_free(state);
    }
    SDL_Quit();
}