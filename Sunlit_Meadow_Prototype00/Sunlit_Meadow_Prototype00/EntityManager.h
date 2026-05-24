#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

#include "Entity.h"
#include "DataStructures.h"
#include "Vectors.h"
#include "Mat4.h"
#include "WorldTypes.h"

// GPU resources for one mob *type*, loaded from files once and shared by every
// entity of that type. Mirrors how ChunkMesh holds a chunk's buffers, but keyed
// per mob instead of per chunk.
struct MobAsset {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    SDL_GPUTexture* texture = nullptr;   // single 2D texture for this mob
    Uint32          numIndices = 0;
};

// EntityManager owns its own graphics pipeline so it can draw mob models in a
// dedicated render pass, without going through WorldManager / the world pipeline.
class EntityManager {
private:
    // --- Own rendering pipeline (independent of WorldManager) ---
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUSampler* sampler = nullptr;

    // --- Shared, file-loaded GPU assets, keyed by mob name ---
    std::unordered_map<std::string, std::unique_ptr<MobAsset>> mobAssets;

    // --- Live entities + which asset each one renders with ---
    struct EntityInstance {
        std::unique_ptr<Entity> entity;
        MobAsset* asset;
    };
    std::vector<EntityInstance> entities;

    Uint32 nextEntityId = 0;

    // --- File loaders (fill these in) ---
    // Parse a model file into the engine Vertex/index format.
    bool loadModelFromFile(
        const char* filePath,
        const char* fileName,
        std::vector<WorldVertex>& outVertices,
        std::vector<Uint16>& outIndices
    );

    // Load a single 2D texture from disk (same idiom as Materials.cpp).
    SDL_GPUTexture* loadTextureFromFile(
        AppState* state,
        const char* filePath,
        const char* fileName
    );

    // Upload a CPU mesh into an asset's vertex/index buffers.
    bool uploadMeshToGPU(
        AppState* state,
        MobAsset* asset,
        const std::vector<WorldVertex>& vertices,
        const std::vector<Uint16>& indices
    );

public:
    EntityManager();

    // Build the entity pipeline + sampler. Call once after the GPU device exists.
    bool init(AppState* state);
    void destroy(AppState* state);

    // Load a mob's mesh + texture from disk into a shared asset, keyed by name.
    bool loadMob(
        AppState* state,
        const std::string& name,
        const char* modelPath, const char* modelFile,
        const char* texturePath, const char* textureFile
    );

    // Spawn an entity that renders with a previously-loaded mob asset.
    Entity* spawn(const std::string& mobName, Vec3 position);

    // Step physics/behaviour for all entities.
    void update(float dt);

    // Draw all entities in the manager's own pass.
    // viewProj = projection * view; per-entity MVP is viewProj * entityModel.
    void draw(
        //AppState* state,
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const Mat4& viewProj
    );
};