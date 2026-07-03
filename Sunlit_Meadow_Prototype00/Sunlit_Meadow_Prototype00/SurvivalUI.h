#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "UI_Renderer.h"

#include <string>
#include <string_view>
#include <unordered_set>

void drawSurvivalUI(UI_Renderer* ui);

void drawHotbar(UI_Renderer* ui);