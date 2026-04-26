#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Vectors.h"
#include "Mat4.h"

//typedef struct {
//    Vec3 position;
//    Vec3 normal;
//    Vec2 uv;
//    float materialIndex;
//}Vertex;

typedef struct {
    Vec3 position;
    Vec2 uv;
    SDL_FColor color;
    float materialIndex;
}VertexData;

typedef struct {
    Mat4 mvp;   /* model * view * projection, uploaded to the vertex shader */
} UBO;

typedef struct {
    SDL_Window* window;
    SDL_GPUDevice* gpu;
    SDL_GPUGraphicsPipeline* pipeline;
    SDL_GPUTexture* depth_texture;
    SDL_GPUSampler* sampler;

    SDL_GPUTexture* textureArray;

    Uint64  lastTicks;      /* timestamp at end of previous frame   */
    float   rotation;       /* current rotation angle (radians)     */
    Mat4    projMat;        /* projection matrix, computed at init  */
} AppState;

typedef struct {
    Vec3 position;
    Vec3 forward;
    Vec3 lookTarget;
    float yaw;
    float pitch;
} Camera;

