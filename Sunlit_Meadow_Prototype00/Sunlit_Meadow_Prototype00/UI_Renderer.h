#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <unordered_map>
#include <string>
#include <vector>

#include "Vectors.h"
#include "UITypes.h"

class ItemModel; // forward declaration; full definition in ItemModel.h

class UI_Renderer {
public:
    UI_Renderer();

    SDL_GPUGraphicsPipeline* pipeline;
    SDL_GPUBuffer* vertexBuffer;
    Uint32                   maxVertices;
    std::vector<UIVertex>    verts;

    SDL_GPUGraphicsPipeline* texPipeline;
    SDL_GPUBuffer* texVertexBuffer;
    std::vector<UITexBatch>  texBatches;

    // ---- 3D model rendering (rendered offscreen, composited as a UI quad) ----
    SDL_GPUGraphicsPipeline* modelPipeline = nullptr; // back-face culling on
    SDL_GPUGraphicsPipeline* modelPipelineNoCull = nullptr; // culling off
    SDL_GPUSampler* modelSampler = nullptr;
    SDL_GPUTextureFormat     modelDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    std::vector<PendingModelDraw> pendingModels;
    std::vector<SDL_GPUTexture*>  frameModelTargets; // offscreen RTs freed next frame

    // Offscreen color format for model previews (independent of the swapchain).
    static constexpr SDL_GPUTextureFormat MODEL_COLOR_FORMAT = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    TTF_Font* font = nullptr;
    SDL_GPUSampler* textSampler = nullptr;
    std::unordered_map<std::string, CachedText> textCache;
    std::vector<PendingTextDraw>                pendingText;


    float screenW = 1280.0f;
    float screenH = 780.0f;

    bool init(SDL_GPUDevice* gpu, SDL_GPUTextureFormat swapchainFormat);
    void destroy(SDL_GPUDevice* gpu);

    // Coordinate helpers: pixel -> NDC
    float ndcX(float px) const { return (px / screenW) * 2.0f - 1.0f; }
    float ndcY(float py) const { return 1.0f - (py / screenH) * 2.0f; }

    // color drawing
    void drawTriangle(UIVertex a, UIVertex b, UIVertex c);
    void drawRect(float px, float py, float w, float h,
        float r, float g, float b, float a);
    void drawLine(float x0, float y0, float x1, float y1,
        float thickness,
        float r, float g, float b, float a);
    void drawCircle(float cx, float cy, float radius,
        float r, float g, float b, float a,
        int segments = 24);
    void drawCrosshair(float cx, float cy,
        float lineLen, float lineThick, float circleRadius,
        float r, float g, float b, float a);

    // textured drawing
    void drawTexture(SDL_GPUTexture* texture,
        float x, float y, float w, float h,
        SDL_FColor tint = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 3D model drawing (queued; resolved in upload())
    void drawItemModel(ItemModel* itemModel,
        float panelX, float panelY, float panelW, float panelH,
        float pitch, float yaw, float roll,
        float scale = 1.0f,
        SDL_FColor tint = { 1.0f, 1.0f, 1.0f, 1.0f },
        bool cullBackFaces = true);

    // text drawing
    bool loadFont(const char* path, int pointSize);
    void drawText(const char* text, float x, float y, SDL_FColor color);
    void clearTextCache(SDL_GPUDevice* gpu);

    void upload(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd);
    void draw(SDL_GPURenderPass* pass);

    float getScreenW();
    float getScreenH();

private:
    // Shared by drawTexture() and the model compositor — queues a textured quad
    // using a specific sampler.
    void pushTexturedQuad(SDL_GPUTexture* texture, SDL_GPUSampler* sampler,
        float x, float y, float w, float h, SDL_FColor tint);

    // Builds the two model pipelines + sampler. Called from init().
    bool initModelPipeline(SDL_GPUDevice* gpu);

    // Renders one queued model to a fresh offscreen target and queues the
    // composite quad. Called from upload().
    void renderModelOffscreen(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd,
        const PendingModelDraw& pm);
};