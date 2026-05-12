#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stddef.h>

#include "InitPipeline.h"
#include "DataStructures.h"
#include "LoadShader.h"

const SDL_GPUTextureFormat depth_texture_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;

SDL_AppResult App_InitPipeline(void* appstate)
{
    AppState* state = (AppState*)appstate;

    SDL_GPUShader* vert = loadShader(state->gpu, "shader.vert.spv", 1, 0);
    SDL_GPUShader* frag = loadShader(state->gpu, "shader.frag.spv", 0, 1);
    
    if (!vert || !frag) { return SDL_APP_FAILURE; }

    SDL_GPUVertexAttribute vertex_attrs[5] = {
    {
        .location = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = (Uint32)offsetof(Vertex, position),
    },
    {
        .location = 1,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = (Uint32)offsetof(Vertex, normal),
    },
    {
        .location = 2,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        .offset = (Uint32)offsetof(Vertex, uv),
    },
    {
        .location = 3,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
        .offset = (Uint32)offsetof(Vertex, color),
    },
    {
        .location = 4,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
        .offset = (Uint32)offsetof(Vertex, materialIndex),
    },
    };

    SDL_GPUColorTargetDescription color_target_desc = {
        .format = SDL_GetGPUSwapchainTextureFormat(state->gpu, state->window),
    };
    SDL_GPUVertexBufferDescription vertex_buffer_descriptions = {
        .slot = 0,
        .pitch = sizeof(Vertex),
    };
    SDL_GPUDepthStencilState depth_stencil_state = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
    .vertex_shader = vert,
    .fragment_shader = frag,
    .vertex_input_state = {
        .vertex_buffer_descriptions = &vertex_buffer_descriptions,
        .num_vertex_buffers = 1,
        .vertex_attributes = vertex_attrs,
        .num_vertex_attributes = SDL_arraysize(vertex_attrs),
    },
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .depth_stencil_state = depth_stencil_state,
    .target_info = {
        .color_target_descriptions = &color_target_desc,
        .num_color_targets = 1,
        .depth_stencil_format = depth_texture_format,
        .has_depth_stencil_target = true,
    },
    };

    state->pipeline = SDL_CreateGPUGraphicsPipeline(state->gpu, &pipeline_info);

    if (!state->pipeline) {
        SDL_Log("SDL_CreateGPUGraphicsPipeline failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ReleaseGPUShader(state->gpu, vert);
    SDL_ReleaseGPUShader(state->gpu, frag);

    return SDL_APP_CONTINUE;
}
