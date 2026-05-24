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
#include "EntityTypes.h"

class EntityManager {
private:
    // --- Own rendering pipeline (independent of WorldManager) ---
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
    SDL_GPUSampler* sampler = nullptr;

    // === ASSET LIST ========================================================
    std::vector<std::unique_ptr<EntityAsset>> assets;
    std::unordered_map<Uint16, EntityAsset*> assetsById;
    std::unordered_map<std::string, EntityAsset*> assetsByName;

    // === SPAWN PARAMETERS ==================================================
    std::vector<SpawnData> spawnDefaults;

    // === LIVE ENTITIES =====================================================
    std::vector<std::unique_ptr<Entity>> entities;

    Uint16 nextTypeId = 0;
    Uint32 nextEntityId = 0;

    // --- File loaders ---
    bool loadModelFromFile(
        const char* filePath,
        const char* fileName,
        std::vector<WorldVertex>& outVertices,
        std::vector<Uint16>& outIndices
    );
    SDL_GPUTexture* loadTextureFromFile(
        AppState* state,
        const char* filePath,
        const char* fileName
    );
    bool uploadMeshToGPU(
        AppState* state,
        EntityAsset* asset,
        const std::vector<WorldVertex>& vertices,
        const std::vector<Uint16>& indices
    );

    // Internal: take a fully built asset + its spawn defaults, assign a type id,
    // and store both in their respective lists
    EntityAsset* registerType(
        std::unique_ptr<EntityAsset> asset,
        SpawnData                    defaults
    );

    // Shared spawn path used by both public spawn overloads
    Entity* spawnInternal(EntityAsset* asset, Vec3 position, const SpawnData& sd);

public:
    EntityManager();

    bool init(AppState* state);
    void destroy(AppState* state);

    // Register an entity TYPE: load its model + texture from disk into a shared
    // asset, and store its default spawn parameters. Keyed by name.
    bool loadEntityType(
        AppState* state,
        const std::string& name,
        const char* modelPath, const char* modelFile,
        const char* texturePath, const char* textureFile,
        Hitbox    hitbox,
        SpawnData spawnDefaults
    );

    Entity* spawn(const std::string& typeName, Vec3 position);
    Entity* spawn(const std::string& typeName, Vec3 position, SpawnData overrideData);

    void update(float dt);

    void draw(
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const Mat4& viewProj
    );

    // --- Type-asset lookups ---
    EntityAsset* getAssetById(Uint16 id);
    EntityAsset* getAssetByName(const std::string& name);
};