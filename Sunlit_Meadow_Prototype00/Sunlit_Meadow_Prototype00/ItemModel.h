#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <vector>

#include "Vectors.h"
#include "ObjParser.h"

class ItemModel {
private:
    std::vector<ModelVertex> vertices;
    std::vector<Uint16>       indices;
    SDL_GPUTexture* texture = nullptr;  // sampled in the fragment shader (space2)

    // GPU resources
    SDL_GPUBuffer* vbo = nullptr;
    SDL_GPUBuffer* ibo = nullptr;
    bool           uploaded = false;

    // Bounding sphere, used to auto-frame the model inside its panel.
    Vec3  center = { 0.0f, 0.0f, 0.0f };
    float radius = 1.0f;

    void computeBounds();

public:
    // ---- setup ----
    void setMesh(const std::vector<ModelVertex>& verts,
        const std::vector<Uint16>& inds);
    void setTexture(SDL_GPUTexture* tex) {
        texture = tex;
    }

    // Uploads the mesh to GPU buffers the first time it is called. Safe to call
    // every frame — it only does work once. Returns true if the model has valid
    // GPU buffers and can be drawn.
    bool ensureUploaded(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd);

    void destroy(SDL_GPUDevice* gpu);

    // ---- getters ----
    SDL_GPUBuffer*  getVertexBuffer() const { return vbo; }
    SDL_GPUBuffer*  getIndexBuffer()  const { return ibo; }
    SDL_GPUTexture* getTexture()      const { return texture; }
    Uint32          getIndexCount()   const { return (Uint32)indices.size(); }
    Vec3            getCenter()       const { return center; }
    float           getRadius()       const { return radius; }
    bool            isUploaded()      const { return uploaded; }
    bool            isEmpty()         const { return indices.empty(); }
};