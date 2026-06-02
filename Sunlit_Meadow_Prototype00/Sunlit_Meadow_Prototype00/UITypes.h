#pragma once
#include "Vectors.h"

#define MAX_UI_VERTECIES 8192

struct UIVertex {
    Vec2     pos;
    SDL_FColor color;
};

struct UIVertexTextured {
    Vec2       pos;
    Vec2       uv;
    SDL_FColor color; // tint — use {1,1,1,1} for no tint
};

// One batch per texture used in a frame
struct UITexBatch {
    SDL_GPUTexture* texture;
    SDL_GPUSampler* sampler;
    std::vector<UIVertexTextured> verts;
};

struct CachedText {
    SDL_GPUTexture* texture;
    SDL_GPUSampler* sampler;
    float w, h;
};

struct PendingTextDraw {
    std::string text;
    float x, y;
    SDL_FColor color;
};
