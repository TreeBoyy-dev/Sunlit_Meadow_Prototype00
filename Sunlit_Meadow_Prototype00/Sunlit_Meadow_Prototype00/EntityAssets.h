#pragma once
#include "EntityManager.h"
#include "EntityTypes.h"

// Central place that registers every entity TYPE (its shared model + hitbox +
// default spawn parameters). This is to the EntityManager what BlockManager::init
// is to blocks, just pulled out into its own helper file so the manager stays
// generic and adding a new creature is a one-block edit here.
//
// Call registerEntityAssets() once, AFTER EntityManager::init() (the GPU device
// and the entity pipeline must already exist). Paths are relative to the same
// base BuildAbsolutePath() uses for the rest of the engine's assets.
const char* baseModelPath   = "Models/";
const char* baseTexturePathEntities = "Textures/Entities";


inline void registerEntityAssets(AppState* state, EntityManager& em) {
    bool ok = true;

    // --- Player -------------------------------------------------------------
    ok &= em.loadEntityType(
        state, "player",
        baseModelPath, "player.obj",
        baseTexturePathEntities, "player.png",
        Hitbox{
            .offset = { 0.0f, 0.0f, 0.9f },
            .halfExtents = { 0.4f, 0.4f, 0.9f },
        },
        SpawnData{
            .health = 20.0f,
            .maxHealth = 20.0f,
            .mass = 1.0f,
            .affectedByGravity = true,
        }
        );

    // --- Rubber Duck --------------------------------------------------------------
    ok &= em.loadEntityType(
        state, "rubber_duck",
        baseModelPath, "rubber_duck.obj",
        baseTexturePathEntities, "rubber_duck.png",
        Hitbox{
            .offset = { 0.0f, 0.0f, 0.3f },
            .halfExtents = { 0.25f, 0.25f, 0.4f },
        },
        SpawnData{
            .health = 10.0f,
            .maxHealth = 10.0f,
            .mass = 0.5f,
            .affectedByGravity = true,
        }
        );

    if (ok)
        SDL_Log("all Entity assets loaded");
    else
        SDL_Log("failed loading Entity assets");

    return;
}

// Optional: drop the initial entities into the world. Call after the assets
// above are registered. The player is normally driven by the camera, so a
// spawned "player" body is only needed for a visible third-person model —
// hence it is left commented out by default.
inline void spawnStartingEntities(EntityManager& em) {
    em.spawn("rubber_duck", { 261.0f, 266.0f, 110.0f });
    em.spawn("player", { 264.0f, 264.0f, 70.0f });
}