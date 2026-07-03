#include "UI_Renderer.h"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cctype>

#include "LoadShader.h"
#include "Mat4.h"
#include "EntityTypes.h"
#include "ItemModel.h"
#include "BuildAbsolutePath.h"
#include "LoadTextureFromFile.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char* kUITextureFolder = "Textures/UI/";

UI_Renderer::UI_Renderer()
    : maxVertices(MAX_UI_VERTECIES),
    pipeline(nullptr),
    vertexBuffer(nullptr),
    texPipeline(nullptr),
    texVertexBuffer(nullptr),
    font(nullptr),
    textSampler(nullptr)
{
}

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

    // ---- 3D model pipeline ----
    if (!initModelPipeline(gpu)) {
        SDL_Log("[UI] model pipeline failed: %s", SDL_GetError());
        return false;
    }

    if (!initUITexturtes(gpu))
        return false;

    return vertexBuffer != nullptr && texVertexBuffer != nullptr && textSampler != nullptr;
}

// Creates the two model pipelines (culling on/off) + the model sampler.
// Reuses the existing entity.*.spv shaders, which expect:
//   - vertex: column_major float4x4 mvp at register(b0, space1)
//   - fragment: Texture2D + SamplerState at space2
//   - input layout = ModelVertex (pos float3, normal float3, uv float2, color float4)
bool UI_Renderer::initModelPipeline(SDL_GPUDevice* gpu)
{
    // Pick a supported depth format (D16 is essentially universal; prefer D24 if present).
    if (SDL_GPUTextureSupportsFormat(gpu, SDL_GPU_TEXTUREFORMAT_D24_UNORM,
        SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        modelDepthFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    }
    else {
        modelDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    }

    SDL_GPUShader* mvs = loadShader(gpu, "entity.vert.spv", 1, 0); // 1 uniform buffer (mvp)
    SDL_GPUShader* mfs = loadShader(gpu, "entity.frag.spv", 0, 1); // 1 sampler
    if (!mvs || !mfs) return false;

    SDL_GPUVertexAttribute modelAttrs[4] = {
        {.location = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
          .offset = (Uint32)offsetof(ModelVertex, position) },
        {.location = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
          .offset = (Uint32)offsetof(ModelVertex, normal) },
        {.location = 2, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = (Uint32)offsetof(ModelVertex, uv) },
        {.location = 3, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
          .offset = (Uint32)offsetof(ModelVertex, color) },
    };
    SDL_GPUVertexBufferDescription modelVBDesc = {
        .slot = 0,
        .pitch = sizeof(ModelVertex),
    };

    // Opaque 3D render into the offscreen target. No blend: the clear color
    // alpha of 0 leaves untouched pixels transparent, and drawn pixels write
    // their own alpha. Transparency vs. the UI happens later, when the offscreen
    // texture is composited in the textured pass.
    SDL_GPUColorTargetDescription modelColorDesc = {
        .format = MODEL_COLOR_FORMAT,
    };

    SDL_GPUDepthStencilState modelDepth = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    // front_face / cull chosen to match the engine's entity & world pipelines,
    // so the same meshes look identical here. The projection flips Y for Vulkan
    // NDC, which inverts winding in screen space; culling FRONT keeps the
    // exterior faces. cullBackFaces=false selects the no-cull pipeline below.
    SDL_GPURasterizerState rasterCull = {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_FRONT,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
    };
    SDL_GPURasterizerState rasterNoCull = {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_NONE,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
    };

    SDL_GPUGraphicsPipelineCreateInfo modelPipeInfo = {
        .vertex_shader = mvs,
        .fragment_shader = mfs,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &modelVBDesc,
            .num_vertex_buffers = 1,
            .vertex_attributes = modelAttrs,
            .num_vertex_attributes = 4,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = rasterCull,
        .depth_stencil_state = modelDepth,
        .target_info = {
            .color_target_descriptions = &modelColorDesc,
            .num_color_targets = 1,
            .depth_stencil_format = modelDepthFormat,
            .has_depth_stencil_target = true,
        },
    };

    modelPipeline = SDL_CreateGPUGraphicsPipeline(gpu, &modelPipeInfo);

    modelPipeInfo.rasterizer_state = rasterNoCull;
    modelPipelineNoCull = SDL_CreateGPUGraphicsPipeline(gpu, &modelPipeInfo);

    SDL_ReleaseGPUShader(gpu, mvs);
    SDL_ReleaseGPUShader(gpu, mfs);

    if (!modelPipeline || !modelPipelineNoCull) return false;

    SDL_GPUSamplerCreateInfo modelSamplerInfo = {
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    modelSampler = SDL_CreateGPUSampler(gpu, &modelSamplerInfo);

    return modelSampler != nullptr;
}

bool UI_Renderer::initUITexturtes(SDL_GPUDevice* gpu) {
    std::string folder = BuildAbsolutePath(kUITextureFolder, "");
    if (folder.empty()) {
        SDL_Log("[SurvivalUI] could not resolve UI texture folder");
        return false;
    }

    std::error_code ec;
    std::filesystem::directory_iterator dirIt(folder, ec);
    if (ec) {
        SDL_Log("[SurvivalUI] cannot open '%s': %s", folder.c_str(), ec.message().c_str());
        return false;
    }

    bool allOk = true;

    for (const auto& entry : dirIt) {
        if (!entry.is_regular_file()) continue;

        const std::filesystem::path& p = entry.path();

        // Only .png (case-insensitive).
        std::string ext = p.extension().string();
        for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ext != ".png") continue;

        std::string name = p.stem().string();     // "heart"     from "heart.png"
        std::string fileName = p.filename().string();  // "heart.png"

        GPUTextureWH gpuTextureWH;
        if (!loadTextureFromFile(&gpuTextureWH, gpu, kUITextureFolder, fileName.c_str()))
        {
            SDL_Log("[SurvivalUI] failed to load UI texture '%s'", fileName.c_str());
            allOk = false;
            continue;
        }
        SDL_GPUTexture* tex = gpuTextureWH.texture;

        UITexture uiTex;
        uiTex.texture = tex;
        uiTex.w = (int)gpuTextureWH.width;
        uiTex.h = (int)gpuTextureWH.height;

        auto [it, inserted] = UITextureSet.emplace(std::move(name), std::move(uiTex));
        if (!inserted) {
            SDL_Log("[SurvivalUI] duplicate UI texture name '%s' (kept first)",
                it->first.c_str());
        }
    }

    SDL_Log("[SurvivalUI] loaded %zu UI texture(s) from '%s'",
        UITextureSet.size(), kUITextureFolder);
    return allOk;
}

void UI_Renderer::destroy(SDL_GPUDevice* gpu)
{
    if (vertexBuffer)    SDL_ReleaseGPUBuffer(gpu, vertexBuffer);
    if (texVertexBuffer) SDL_ReleaseGPUBuffer(gpu, texVertexBuffer);
    if (pipeline)        SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);
    if (texPipeline)     SDL_ReleaseGPUGraphicsPipeline(gpu, texPipeline);

    if (modelPipeline)       SDL_ReleaseGPUGraphicsPipeline(gpu, modelPipeline);
    if (modelPipelineNoCull) SDL_ReleaseGPUGraphicsPipeline(gpu, modelPipelineNoCull);
    if (modelSampler)        SDL_ReleaseGPUSampler(gpu, modelSampler);
    for (SDL_GPUTexture* t : frameModelTargets) SDL_ReleaseGPUTexture(gpu, t);
    frameModelTargets.clear();

    if (textSampler) SDL_ReleaseGPUSampler(gpu, textSampler);
    clearTextCache(gpu);
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
    UIVertex tl = { {x0, y0 }, { r, g, b, a } };
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

void UI_Renderer::pushTexturedQuad(SDL_GPUTexture* texture, SDL_GPUSampler* sampler,
    float x, float y, float w, float h, SDL_FColor tint)
{
    // find existing batch for this texture+sampler, or start a new one
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

void UI_Renderer::drawTexture(SDL_GPUTexture* texture,
    float x, float y, float w, float h, SDL_FColor tint)
{
    pushTexturedQuad(texture, textSampler, x, y, w, h, tint);
}

UITexture* UI_Renderer::FindUITexture(const std::string& name) {
    if (name.empty()) return nullptr;
    auto it = UITextureSet.find(name);
    return (it == UITextureSet.end()) ? nullptr : &it->second;
}


// ---------- 3D model drawing ----------
void UI_Renderer::drawItemModel(ItemModel* itemModel,
    float panelX, float panelY, float panelW, float panelH,
    float pitch, float yaw, float roll,
    float scale, SDL_FColor tint, bool cullBackFaces)
{
    if (!itemModel) return;
    pendingModels.push_back({
        itemModel, panelX, panelY, panelW, panelH,
        pitch, yaw, roll, scale, tint, cullBackFaces
        });
}

// Renders one queued model to a fresh offscreen RGBA+depth target, then queues
// a textured quad that composites that target onto the panel rect.
void UI_Renderer::renderModelOffscreen(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd,
    const PendingModelDraw& pm)
{
    ItemModel* model = pm.model;
    if (!model) {
        SDL_Log("[UI] renderModelOffscreen: no itemModel");
        return;
    }
    if (!model->ensureUploaded(gpu, cmd)) {
        SDL_Log("[UI] renderModelOffscreen: couldn't ensureUploaded()");
        return;
    }
    if (model->isEmpty()) {
        SDL_Log("[UI] renderModelOffscreen: ItemModel is empty");
        return;
    }
    if (!model->getTexture()) {
        SDL_Log("[UI] drawItemModel: model has no texture; the shader needs one. Skipping.");
        return;
    }

    Uint32 texW = (Uint32)SDL_max(1.0f, pm.panelW);
    Uint32 texH = (Uint32)SDL_max(1.0f, pm.panelH);

    SDL_GPUTextureCreateInfo colInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = MODEL_COLOR_FORMAT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = texW,
        .height = texH,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture* colorTex = SDL_CreateGPUTexture(gpu, &colInfo);

    SDL_GPUTextureCreateInfo depInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = modelDepthFormat,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = texW,
        .height = texH,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture* depthTex = SDL_CreateGPUTexture(gpu, &depInfo);

    if (!colorTex || !depthTex) {
        if (colorTex) SDL_ReleaseGPUTexture(gpu, colorTex);
        if (depthTex) SDL_ReleaseGPUTexture(gpu, depthTex);
        SDL_Log("[UI] drawItemModel: failed to create offscreen target: %s", SDL_GetError());
        return;
    }

    // ---- build MVP ----
    // model = R * S * T(-center): center the mesh on the origin, scale, rotate.
    Mat4 S = mat4Identity();
    S.m[0][0] = S.m[1][1] = S.m[2][2] = pm.scale;
    Vec3 c = model->getCenter();
    Mat4 T = mat4Translate(-c.x, -c.y, -c.z);
    Mat4 R = mat4Rotate(pm.pitch, pm.yaw, pm.roll);
    Mat4 modelMat = mat4Mul(R, mat4Mul(S, T));

    // Camera framed so the model's bounding sphere fits the panel.
    float effR = model->getRadius() * pm.scale;
    if (effR < 0.0001f) effR = 1.0f;
    const float fovY = 0.7853982f;                 // 45 degrees
    float aspect = (float)texW / (float)texH;
    float dist = effR / tanf(fovY * 0.5f);
    if (aspect < 1.0f) dist /= aspect;             // fit width on tall/narrow panels
    dist *= 1.3f;                                  // a little breathing room
    float nearP = SDL_max(0.01f, dist - effR * 2.0f);
    float farP = dist + effR * 2.0f;

    Mat4 view = mat4LookAt(Vec3{ 0.0f, 0.0f, dist },
        Vec3{ 0.0f, 0.0f, 0.0f },
        Vec3{ 0.0f, 1.0f, 0.0f });
    Mat4 proj = mat4Perspective(fovY, aspect, nearP, farP);
    Mat4 mvp = mat4Mul(proj, mat4Mul(view, modelMat));

    // ---- render the model into the offscreen target ----
    SDL_GPUColorTargetInfo colTarget = {
        .texture = colorTex,
        .clear_color = { 0.0f, 0.0f, 0.0f, 0.0f },  // transparent background
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPUDepthStencilTargetInfo depTarget = {
        .texture = depthTex,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,      // depth not needed after the pass
    };

    SDL_GPURenderPass* mp = SDL_BeginGPURenderPass(cmd, &colTarget, 1, &depTarget);
    SDL_BindGPUGraphicsPipeline(mp, pm.cullBackFaces ? modelPipeline : modelPipelineNoCull);

    SDL_GPUBufferBinding vb = { .buffer = model->getVertexBuffer(), .offset = 0 };
    SDL_BindGPUVertexBuffers(mp, 0, &vb, 1);
    SDL_GPUBufferBinding ib = { .buffer = model->getIndexBuffer(), .offset = 0 };
    SDL_BindGPUIndexBuffer(mp, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding sb = { .texture = model->getTexture(), .sampler = modelSampler };
    SDL_BindGPUFragmentSamplers(mp, 0, &sb, 1);

    SDL_PushGPUVertexUniformData(cmd, 0, &mvp, sizeof(Mat4));
    SDL_DrawGPUIndexedPrimitives(mp, model->getIndexCount(), 1, 0, 0, 0);
    SDL_EndGPURenderPass(mp);

    // Composite the rendered model as a normal UI quad (in the textured pass).
    pushTexturedQuad(colorTex, modelSampler, pm.panelX, pm.panelY, pm.panelW, pm.panelH, pm.tint);

    // Keep the targets alive until the frame is submitted; freed next upload().
    frameModelTargets.push_back(colorTex);
    frameModelTargets.push_back(depthTex);
}

bool UI_Renderer::loadFont(const char* path, int pointSize)
{
    font = TTF_OpenFont(path, (float)pointSize);
    return font != nullptr;
}

void UI_Renderer::drawText(const char* text, float x, float y, SDL_FColor color)
{
    pendingText.push_back({ std::string(text), x, y, color });
}

void UI_Renderer::clearTextCache(SDL_GPUDevice* gpu)
{
    for (auto& [key, entry] : textCache)
        SDL_ReleaseGPUTexture(gpu, entry.texture);
    textCache.clear();
}

void UI_Renderer::upload(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd)
{
    // Free last frame's offscreen model targets. By now that frame has been
    // submitted; SDL_gpu also defers the actual destruction until the GPU is
    // finished with them, so this is safe even if the GPU is still reading.
    for (SDL_GPUTexture* t : frameModelTargets) SDL_ReleaseGPUTexture(gpu, t);
    frameModelTargets.clear();

    // --- text ---
    for (auto& pending : pendingText)
    {
        // check cache first
        auto it = textCache.find(pending.text);
        if (it == textCache.end())
        {
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
        pushTexturedQuad(cached.texture, textSampler,
            pending.x, pending.y, cached.w, cached.h,
            pending.color);
    }
    pendingText.clear();

    // --- 3D models ---
    // Each model is rendered to its own offscreen target here and then queued
    // into texBatches as a textured quad, so it must run BEFORE the textured
    // vertex upload below.
    for (const auto& pm : pendingModels) {
        renderModelOffscreen(gpu, cmd, pm);
    }
    pendingModels.clear();

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
    // --- color draws ---
    if (!verts.empty()) {
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_GPUBufferBinding binding = { .buffer = vertexBuffer };
        SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
        SDL_DrawGPUPrimitives(pass, (Uint32)verts.size(), 1, 0, 0);
        verts.clear();
    }

    // --- textured draws (text + model previews) ---
    Uint32 offset = 0;
    if (!texBatches.empty()) {
        SDL_BindGPUGraphicsPipeline(pass, texPipeline);
        SDL_GPUBufferBinding binding = { .buffer = texVertexBuffer };
        SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);

        for (auto& batch : texBatches) {
            if (batch.verts.empty()) continue;
            SDL_GPUTextureSamplerBinding samplerBinding = {
                .texture = batch.texture,
                .sampler = batch.sampler,
            };
            SDL_BindGPUFragmentSamplers(pass, 0, &samplerBinding, 1);
            Uint32 firstVert = offset / (Uint32)sizeof(UIVertexTextured);
            SDL_DrawGPUPrimitives(pass, (Uint32)batch.verts.size(), 1, firstVert, 0);
            offset += (Uint32)(batch.verts.size() * sizeof(UIVertexTextured));
        }
        texBatches.clear();
    }
}

float UI_Renderer::getScreenW() {
    return screenW;
}
float UI_Renderer::getScreenH() {
    return screenH;
}