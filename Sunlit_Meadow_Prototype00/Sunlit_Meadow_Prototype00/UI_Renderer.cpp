#include "UI_Renderer.h"
#include <cmath>
#include <cstring>

#include "LoadShader.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

UI_Renderer::UI_Renderer()
    : maxVertices(MAX_UI_VERTECIES),
    pipeline(nullptr),
    vertexBuffer(nullptr),
    texPipeline(nullptr),
    texVertexBuffer(nullptr),
    font(nullptr),
    textSampler(nullptr)
{}

// ---------- pipeline init ----------
bool UI_Renderer::init(SDL_GPUDevice* gpu, SDL_GPUTextureFormat swapchainFormat)
{
    maxVertices = MAX_UI_VERTECIES;

    // ---- color pipeline  ----
    SDL_GPUShader* vert = loadShader(gpu, "ui.vert.spv", 0, 0);
    SDL_GPUShader* frag = loadShader(gpu, "ui.frag.spv", 0, 0);

    if (!vert || !frag) return false;

    SDL_GPUVertexAttribute attrs[2] = {
        {.location = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = offsetof(UIVertex, pos) },
        {.location = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
          .offset = offsetof(UIVertex, color) },
    };
    SDL_GPUVertexBufferDescription vbDesc = {
        .slot = 0,
        .pitch = sizeof(UIVertex),
    };
    SDL_GPUColorTargetBlendState blendState = {
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .enable_blend = true,
    };
    SDL_GPUColorTargetDescription colorDesc = {
        .format = swapchainFormat,
        .blend_state = blendState,
    };
    SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &vbDesc,
            .num_vertex_buffers = 1,
            .vertex_attributes = attrs,
            .num_vertex_attributes = 2,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        // No depth test for UI
        .depth_stencil_state = {.enable_depth_test = false, .enable_depth_write = false },
        .target_info = {
            .color_target_descriptions = &colorDesc,
            .num_color_targets = 1,
            .has_depth_stencil_target = false,
        },
    };

    pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeInfo);
    SDL_ReleaseGPUShader(gpu, vert);
    SDL_ReleaseGPUShader(gpu, frag);
    if (!pipeline) { SDL_Log("[UI] pipeline failed: %s", SDL_GetError()); return false; }

    Uint32 bufSize = maxVertices * (Uint32)sizeof(UIVertex);
    //SDL_Log("[UI] vertex buffer size: %u (maxVerts=%u, vertexSize=%u)",
    //    bufSize, maxVertices, (Uint32)sizeof(UIVertex));

    // Pre-allocate vertex buffer
    SDL_GPUBufferCreateInfo bufInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = maxVertices * sizeof(UIVertex),
    };
    vertexBuffer = SDL_CreateGPUBuffer(gpu, &bufInfo);

    // ---- textured pipeline ----
    SDL_GPUShader* tvs = loadShader(gpu, "ui_tex.vert.spv", 0, 0);
    SDL_GPUShader* tfs = loadShader(gpu, "ui_tex.frag.spv", 0, 1);

    if (!tvs || !tfs) return false;

    SDL_GPUVertexAttribute texAttrs[3] = {
        {.location = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = (Uint32)offsetof(UIVertexTextured, pos) },
        {.location = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = (Uint32)offsetof(UIVertexTextured, uv) },
        {.location = 2, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
          .offset = (Uint32)offsetof(UIVertexTextured, color) },
    };
    SDL_GPUVertexBufferDescription texVBDesc = {
        .slot = 0,
        .pitch = sizeof(UIVertexTextured),
    };
    blendState = {
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .enable_blend = true,
    };
    SDL_GPUColorTargetDescription texColorDesc = {
        .format = swapchainFormat,
        .blend_state = blendState,
    };
    SDL_GPUGraphicsPipelineCreateInfo texPipeInfo = {
        .vertex_shader = tvs,
        .fragment_shader = tfs,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &texVBDesc,
            .num_vertex_buffers = 1,
            .vertex_attributes = texAttrs,
            .num_vertex_attributes = 3,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .depth_stencil_state = {.enable_depth_test = false, .enable_depth_write = false },
        .target_info = {
            .color_target_descriptions = &texColorDesc,
            .num_color_targets = 1,
            .has_depth_stencil_target = false,
        },
    };

    texPipeline = SDL_CreateGPUGraphicsPipeline(gpu, &texPipeInfo);
    SDL_ReleaseGPUShader(gpu, tvs);
    SDL_ReleaseGPUShader(gpu, tfs);
    if (!texPipeline) { SDL_Log("UI tex pipeline failed: %s", SDL_GetError()); return false; }

    SDL_GPUBufferCreateInfo texBufInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = maxVertices * (Uint32)sizeof(UIVertexTextured),
    };
    texVertexBuffer = SDL_CreateGPUBuffer(gpu, &texBufInfo);

    // ---- text sampler  ----
    SDL_GPUSamplerCreateInfo textSamplerInfo = {
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    textSampler = SDL_CreateGPUSampler(gpu, &textSamplerInfo);
    //SDL_Log("[UI] textSampler created: %p", textSampler);

    return vertexBuffer != nullptr && texVertexBuffer != nullptr && textSampler != nullptr;
}

void UI_Renderer::destroy(SDL_GPUDevice* gpu)
{
    if (vertexBuffer) SDL_ReleaseGPUBuffer(gpu, vertexBuffer);
    if (pipeline)     SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);
}

// ---------- geometry helpers ----------

void UI_Renderer::drawTriangle(UIVertex a, UIVertex b, UIVertex c)
{
    verts.push_back(a);
    verts.push_back(b);
    verts.push_back(c);
}

void UI_Renderer::drawRect(float px, float py, float w, float h,
    float r, float g, float b, float a)
{
    float x0 = ndcX(px), y0 = ndcY(py);
    float x1 = ndcX(px + w), y1 = ndcY(py + h);
    UIVertex tl = { {x0, y0 }, { r, g, b, a }};
    UIVertex tr = { {x1, y0 }, { r, g, b, a } };
    UIVertex bl = { {x0, y1 }, { r, g, b, a } };
    UIVertex br = { {x1, y1 }, { r, g, b, a } };
    drawTriangle(tl, tr, bl);
    drawTriangle(tr, br, bl);
}

void UI_Renderer::drawLine(float x0, float y0, float x1, float y1,
    float thickness,
    float r, float g, float b, float a)
{
    // Build a thick line as a quad, then convert to NDC
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    float nx = (-dy / len) * (thickness * 0.5f);
    float ny = (dx / len) * (thickness * 0.5f);

    auto v = [&](float px, float py) -> UIVertex {
        return { {ndcX(px), ndcY(py)}, {r, g, b, a} };
        };
    drawTriangle(v(x0 - nx, y0 - ny), v(x0 + nx, y0 + ny), v(x1 - nx, y1 - ny));
    drawTriangle(v(x0 + nx, y0 + ny), v(x1 + nx, y1 + ny), v(x1 - nx, y1 - ny));
}

void UI_Renderer::drawCircle(float cx, float cy, float radius,
    float r, float g, float b, float a,
    int segments)
{
    UIVertex center = { ndcX(cx), ndcY(cy), r, g, b, a };
    for (int i = 0; i < segments; ++i) {
        float a0 = (float)i / segments * 2.0f * (float)M_PI;
        float a1 = (float)(i + 1) / segments * 2.0f * (float)M_PI;
        UIVertex v0 = { ndcX(cx + cosf(a0) * radius), ndcY(cy + sinf(a0) * radius), r, g, b, a };
        UIVertex v1 = { ndcX(cx + cosf(a1) * radius), ndcY(cy + sinf(a1) * radius), r, g, b, a };
        drawTriangle(center, v0, v1);
    }
}

void UI_Renderer::drawCrosshair(float cx, float cy,
    float lineLen, float lineThick, float circleRadius,
    float r, float g, float b, float a)
{
    // 4 lines: up, down, left, right (gap = circleRadius)
    float gap = circleRadius + 2.0f;
    drawLine(cx, cy - gap, cx, cy - gap - lineLen, lineThick, r, g, b, a);
    drawLine(cx, cy + gap, cx, cy + gap + lineLen, lineThick, r, g, b, a);
    drawLine(cx - gap, cy, cx - gap - lineLen, cy, lineThick, r, g, b, a);
    drawLine(cx + gap, cy, cx + gap + lineLen, cy, lineThick, r, g, b, a);
    drawCircle(cx, cy, circleRadius, r, g, b, a);
}

void UI_Renderer::drawTexture(SDL_GPUTexture* texture, SDL_GPUSampler* sampler,
    float x, float y, float w, float h, SDL_FColor tint)
{
    // find existing batch for this texture, or start a new one
    UITexBatch* batch = nullptr;
    for (auto& b : texBatches) {
        if (b.texture == texture && b.sampler == sampler) {
            batch = &b;
            break;
        }
    }
    if (!batch) {
        texBatches.push_back({ texture, sampler, {} });
        batch = &texBatches.back();
    }

    float x0 = ndcX(x), y0 = ndcY(y);
    float x1 = ndcX(x + w), y1 = ndcY(y + h);

    UIVertexTextured tl = { {x0, y0}, {0, 0}, tint };
    UIVertexTextured tr = { {x1, y0}, {1, 0}, tint };
    UIVertexTextured bl = { {x0, y1}, {0, 1}, tint };
    UIVertexTextured br = { {x1, y1}, {1, 1}, tint };

    batch->verts.insert(batch->verts.end(), { tl, tr, bl, tr, br, bl });
}

bool UI_Renderer::loadFont(const char* path, int pointSize)
{
    font = TTF_OpenFont(path, (float)pointSize);
    //SDL_Log("[UI] loadFont '%s' size=%d font=%p error=%s",
    //    path, pointSize, font, SDL_GetError());
    return font != nullptr;
}

void UI_Renderer::drawText(const char* text, float x, float y, SDL_FColor color)
{
    pendingText.push_back({ std::string(text), x, y, color });

    //SDL_Log("[UI] draw text");
}

void UI_Renderer::clearTextCache(SDL_GPUDevice* gpu)
{
    for (auto& [key, entry] : textCache)
        SDL_ReleaseGPUTexture(gpu, entry.texture);
    textCache.clear();

    //SDL_Log("[UI] clear text cache %d", textCache.size());
}

void UI_Renderer::upload(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd)
{
    //SDL_Log("[UI] upload called — verts:%d pending:%d batches:%d",
    //    (int)verts.size(), (int)pendingText.size(), (int)texBatches.size());

    // --- text ---
    for (auto& pending : pendingText)
    {
        // check cache first
        auto it = textCache.find(pending.text);
        if (it == textCache.end())
        {
            //SDL_Log("[UI] rendering new string: '%s'", pending.text.c_str());
            if (!font) {
                SDL_Log("[UI] ERROR: font is null! Did you call loadFont()?");
                continue;
            }
            // render to surface
            SDL_Color fg = {
                (Uint8)(pending.color.r * 255),
                (Uint8)(pending.color.g * 255),
                (Uint8)(pending.color.b * 255),
                (Uint8)(pending.color.a * 255),
            };
            SDL_Surface* surf = TTF_RenderText_Blended(font, pending.text.c_str(), 0, fg);
            if (!surf) {
                SDL_Log("[UI] TTF_RenderText_Blended failed: %s", SDL_GetError());
                continue;
            }
            //SDL_Log("[UI] surface created: %dx%d", surf->w, surf->h);

            // ensure RGBA format
            SDL_Surface* rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(surf);
            if (!rgba) continue;

            // create GPU texture
            SDL_GPUTextureCreateInfo texInfo = {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                .width = (Uint32)rgba->w,
                .height = (Uint32)rgba->h,
                .layer_count_or_depth = 1,
                .num_levels = 1,
            };
            SDL_GPUTexture* tex = SDL_CreateGPUTexture(gpu, &texInfo);

            // upload pixels
            Uint32 uploadSize = (Uint32)(rgba->w * rgba->h * 4);
            SDL_GPUTransferBufferCreateInfo tbInfo = {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = uploadSize,
            };
            SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(gpu, &tbInfo);
            void* mapped = SDL_MapGPUTransferBuffer(gpu, tb, false);

            // copy row by row in case pitch != w*4
            Uint8* dst = (Uint8*)mapped;
            Uint8* src = (Uint8*)rgba->pixels;
            for (int row = 0; row < rgba->h; row++) {
                memcpy(dst + row * rgba->w * 4, src + row * rgba->pitch, rgba->w * 4);
            }
            SDL_UnmapGPUTransferBuffer(gpu, tb);

            SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);
            SDL_GPUTextureTransferInfo tsrc = {
                .transfer_buffer = tb,
                .pixels_per_row = (Uint32)rgba->w,
            };
            SDL_GPUTextureRegion tdst = {
                .texture = tex,
                .w = (Uint32)rgba->w,
                .h = (Uint32)rgba->h,
                .d = 1,
            };
            SDL_UploadToGPUTexture(cp, &tsrc, &tdst, false);
            SDL_EndGPUCopyPass(cp);
            SDL_ReleaseGPUTransferBuffer(gpu, tb);

            CachedText cached = { tex, textSampler, (float)rgba->w, (float)rgba->h };
            SDL_DestroySurface(rgba);

            textCache[pending.text] = cached;
            it = textCache.find(pending.text);
        }

        // queue a textured quad using the cached texture
        const CachedText& cached = it->second;
        drawTexture(cached.texture, cached.sampler,
            pending.x, pending.y, cached.w, cached.h,
            pending.color);
    }
    pendingText.clear();

    // --- color verts ---
    if (!verts.empty()) {
        Uint32 uploadSize = (Uint32)(verts.size() * sizeof(UIVertex));
        SDL_GPUTransferBufferCreateInfo tbInfo = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = uploadSize };
        SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(gpu, &tbInfo);
        memcpy(SDL_MapGPUTransferBuffer(gpu, tb, false), verts.data(), uploadSize);
        SDL_UnmapGPUTransferBuffer(gpu, tb);
        SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTransferBufferLocation src = { .transfer_buffer = tb };
        SDL_GPUBufferRegion dst = { .buffer = vertexBuffer, .size = uploadSize };
        SDL_UploadToGPUBuffer(cp, &src, &dst, false);
        SDL_EndGPUCopyPass(cp);
        SDL_ReleaseGPUTransferBuffer(gpu, tb);
    }

    // --- textured verts ---
    Uint32 offset = 0;
    for (auto& batch : texBatches) {
        if (batch.verts.empty()) continue;
        Uint32 uploadSize = (Uint32)(batch.verts.size() * sizeof(UIVertexTextured));
        SDL_GPUTransferBufferCreateInfo tbInfo = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = uploadSize };
        SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(gpu, &tbInfo);
        memcpy(SDL_MapGPUTransferBuffer(gpu, tb, false), batch.verts.data(), uploadSize);
        SDL_UnmapGPUTransferBuffer(gpu, tb);
        SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTransferBufferLocation src = { .transfer_buffer = tb };
        SDL_GPUBufferRegion dst = { .buffer = texVertexBuffer, .offset = offset, .size = uploadSize };
        SDL_UploadToGPUBuffer(cp, &src, &dst, false);
        SDL_EndGPUCopyPass(cp);
        SDL_ReleaseGPUTransferBuffer(gpu, tb);
        offset += uploadSize;
    }
}

void UI_Renderer::draw(SDL_GPURenderPass* pass)
{
    //SDL_Log("[UI] draw — verts:%d batches:%d",
    //    (int)verts.size(), (int)texBatches.size());

    // --- color draws ---
    if (!verts.empty()) {
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_GPUBufferBinding binding = { .buffer = vertexBuffer };
        SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
        SDL_DrawGPUPrimitives(pass, (Uint32)verts.size(), 1, 0, 0);
        verts.clear();
    }

    // --- textured draws ---
    //SDL_Log("[UI] texBatches count: %d", (int)texBatches.size());
    Uint32 offset = 0;
    if (!texBatches.empty()) {
        SDL_BindGPUGraphicsPipeline(pass, texPipeline);
        SDL_GPUBufferBinding binding = { .buffer = texVertexBuffer };
        SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);

        for (auto& batch : texBatches) {
            //SDL_Log("[UI] drawing batch: tex=%p verts:%d",
            //    batch.texture, (int)batch.verts.size());

            if (batch.verts.empty()) continue;
            SDL_GPUTextureSamplerBinding samplerBinding = {
                .texture = batch.texture,
                .sampler = batch.sampler,
            };
            SDL_BindGPUFragmentSamplers(pass, 0, &samplerBinding, 1);
            Uint32 firstVert = offset / (Uint32)sizeof(UIVertexTextured);
            //SDL_Log("[UI] DrawPrimitives: count=%d firstVert=%d offset=%d",
            //    (int)batch.verts.size(), firstVert, offset);
            SDL_DrawGPUPrimitives(pass, (Uint32)batch.verts.size(), 1, firstVert, 0);
            offset += (Uint32)(batch.verts.size() * sizeof(UIVertexTextured));
        }
        texBatches.clear();
    }
}