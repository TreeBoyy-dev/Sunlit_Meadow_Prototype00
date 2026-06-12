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
#include "EntityTypes.h"
#include "LoadTextureFromFile.h"
#include "ObjParser.h"
#include "EntityData.h"

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
    std::vector<EntityData> spawnDefaults;

    // === LIVE ENTITIES =====================================================
    std::vector<std::unique_ptr<Entity>> entities;

    Uint16 nextTypeId = 0;
    Uint32 nextEntityId = 0;

    // --- File loaders ---
    bool loadModelFromFile(
        const char* filePath,
        const char* fileName,
        std::vector<ModelVertex>& outVertices,
        std::vector<Uint16>& outIndices
    );
    bool uploadMeshToGPU(
        AppState* state,
        EntityAsset* asset,
        const std::vector<ModelVertex>& vertices,
        const std::vector<Uint16>& indices
    );

    // Internal: take a fully built asset + its spawn defaults, assign a type id,
    // and store both in their respective lists
    EntityAsset* registerType(
        std::unique_ptr<EntityAsset> asset,
        EntityData                   defaults
    );

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
        Hitbox     hitbox,
        EntityData spawnDefaults
    );

    Entity* spawn(const std::string& typeName, Vec3 position, std::vector<Data*> data = {});

    void update(float dt, WorldManager* worldManager);

    void draw(
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const Mat4& viewProj
    );

    // --- Type-asset lookups ---
    EntityAsset* getAssetById(Uint16 id);
    EntityAsset* getAssetByName(const std::string& name);
};