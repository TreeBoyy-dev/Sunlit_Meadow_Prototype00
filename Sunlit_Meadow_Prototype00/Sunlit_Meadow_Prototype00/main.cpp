#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>

#include "Mat4.h"
#include "Materials.h"
#include "DataStructures.h"
#include "App_Render.h"
#include "App_Update.h"
#include "App_Init.h"
#include "App_Event.h"
#include "Globals.h"
#include "Vectors.h"
#include "MenuManager.h"
#include "MenuFactory.h"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    AppState* state = new AppState();
    if (!state) { return SDL_APP_FAILURE; }
    *appstate = state;

    if (App_Init(state) != SDL_APP_CONTINUE) {
        SDL_Log("returned Error on App_Init");
        return SDL_APP_FAILURE;
    }

    int w, h;
    SDL_GetWindowSize(state->window, &w, &h);

    fovX = fovDeg * (float)SDL_PI_F / 180.0f;
    aspect = (float)w / (float)h;
    state->projMat = mat4Perspective(fovX, aspect, NEAR_PLANE, FAR_PLANE);

    state->lastTicks = SDL_GetTicks();

    SDL_SetWindowRelativeMouseMode(state->window, true);
    menuManager.setWindow(state->window);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    return App_Event(appstate, event);
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
        worldManager.destroy(state);

        if (state->gpu)
            SDL_DestroyGPUDevice(state->gpu);
        if (state->window)
            SDL_DestroyWindow(state->window);
        SDL_free(state);
    }
    SDL_Quit();
}