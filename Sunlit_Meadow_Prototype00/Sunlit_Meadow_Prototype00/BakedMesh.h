#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <vector>

#include "WorldTypes.h"

// One fully transformed, render-ready copy of a block model's mesh.
// Positions are in local block space (X/Y already at the +0.5 placement
// offset, Z in 0..1); overlay vertices/triangles are already appended.
// Baked once at BlockManager::init(), read-only afterwards — safe to share
// across the mesh worker threads.
struct BakedMesh {
    std::vector<WorldVertex> vertices;
    std::vector<Uint16>      indices;
};
