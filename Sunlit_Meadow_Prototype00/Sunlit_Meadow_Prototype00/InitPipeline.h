#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stddef.h>

#include "DataStructures.h"
#include "LoadShader.h"

extern const SDL_GPUTextureFormat depth_texture_format;

//Initializing the Graphics pipeline
SDL_AppResult App_InitPipeline(void* appstate);
