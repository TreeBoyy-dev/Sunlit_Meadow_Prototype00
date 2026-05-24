#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <string>

std::string BuildAbsolutePath(
    const char* filePath,
    const char* fileName
);