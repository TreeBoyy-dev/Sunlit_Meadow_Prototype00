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

// 3D model queued for display inside a UI panel. Resolved in UI_Renderer::upload(),
// where the model is rendered to an offscreen texture and then composited as a
// normal UI quad in the existing textured pass.
class ItemModel; // defined in ItemModel.h

struct PendingModelDraw {
    ItemModel* model;
    float      panelX, panelY, panelW, panelH;
    float      pitch, yaw, roll;
    float      scale;
    SDL_FColor tint;
    bool       cullBackFaces;
};
