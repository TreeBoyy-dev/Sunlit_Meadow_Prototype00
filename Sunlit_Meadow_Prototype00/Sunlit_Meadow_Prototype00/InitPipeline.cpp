#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stddef.h>

#include "InitPipeline.h"
#include "DataStructures.h"

const SDL_GPUTextureFormat depth_texture_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;

static SDL_GPUShader* loadShader(SDL_GPUDevice* gpu,
    const char* path,
    SDL_GPUShaderStage stage,
    Uint32 num_uniform_buffers,
    Uint32 num_samplers)
{
    size_t code_size = 0;
    void* code = SDL_LoadFile(path, &code_size);
    if (!code) {
        SDL_Log("loadShader: could not read '%s': %s", path, SDL_GetError());
        return NULL;
    }

    SDL_GPUShaderCreateInfo info = {
        .code_size = code_size,
        .code = (const Uint8*)code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = num_samplers,
        .num_uniform_buffers = num_uniform_buffers,
    };

    SDL_GPUShader* shader = SDL_CreateGPUShader(gpu, &info);
    SDL_free(code);

    if (!shader) {
        SDL_Log("loadShader: SDL_CreateGPUShader failed for '%s': %s",
            path, SDL_GetError());
    }
    return shader;
}

SDL_AppResult App_InitPipeline(void* appstate)
{
    AppState* state = (AppState*)appstate;

    SDL_GPUShader* vert = loadShader(state->gpu,
        "shader.spv.vert",
        SDL_GPU_SHADERSTAGE_VERTEX,
        1,
        0
    );
    SDL_GPUShader* frag = loadShader(state->gpu,
        "shader.spv.frag",
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        0,
        1
    );
    if (!vert || !frag) { return SDL_APP_FAILURE; }

    SDL_GPUVertexAttribute vertex_attrs[4] = {
    {
        .location = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = (Uint32)offsetof(VertexData, position),
    },
    {
        .location = 1,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        .offset = (Uint32)offsetof(VertexData, uv),
    },
    {
        .location = 2,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
        .offset = (Uint32)offsetof(VertexData, color),
    },
    {
        .location = 3,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
        .offset = (Uint32)offsetof(VertexData, materialIndex),
    },
    };

    SDL_GPUColorTargetDescription color_target_desc = {
        .format = SDL_GetGPUSwapchainTextureFormat(state->gpu, state->window),
    };
    SDL_GPUVertexBufferDescription vertex_buffer_descriptions = {
        .slot = 0,
        .pitch = sizeof(VertexData),
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
