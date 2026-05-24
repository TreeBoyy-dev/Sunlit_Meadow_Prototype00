#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Vectors.h"
#include "Mat4.h"
#include "UI_Renderer.h"
/*
typedef struct {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    SDL_FColor color;
    float materialIndex;
}Vertex;
*/
const SDL_GPUTextureFormat depth_texture_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;

typedef struct {
    Mat4 mvp;   /* model * view * projection, uploaded to the vertex shader */
} UBO;

typedef struct {
    SDL_Window* window;
    SDL_GPUDevice* gpu;
    SDL_GPUTexture* depth_texture;
    SDL_GPUSampler* sampler;

    Uint64  lastTicks;      /* timestamp at end of previous frame   */
    Mat4    projMat;        /* projection matrix, computed at init  */
} AppState;

typedef struct {
    Vec3 position;
    Vec3 forward;
    Vec3 lookTarget;
    float yaw;
    float pitch;
} Camera;
