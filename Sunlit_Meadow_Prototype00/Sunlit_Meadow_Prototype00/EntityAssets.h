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

inline bool registerEntityAssets(AppState* state, EntityManager& em) {
    bool ok = true;

    // --- Player -------------------------------------------------------------
    // ~1.8 units tall, feet at the entity origin (center sits at z = 0.9).
    ok &= em.loadEntityType(
        state, "player",
        "Models/", "player.obj",
        "Textures/", "player.png",
        Hitbox{
            .offset = { 0.0f, 0.0f, 0.9f },
            .halfExtents = { 0.3f, 0.3f, 0.9f },
        },
        SpawnData{
            .health = 20.0f,
            .maxHealth = 20.0f,
            .mass = 1.0f,
            .affectedByGravity = true,
        }
        );

    // --- Goose --------------------------------------------------------------
    // Small and low to the ground (~0.8 units tall), lighter than the player.
    ok &= em.loadEntityType(
        state, "goose",
        "Models/", "goose.obj",
        "Textures/", "goose.png",
        Hitbox{
            .offset = { 0.0f, 0.0f, 0.4f },
            .halfExtents = { 0.25f, 0.25f, 0.4f },
        },
        SpawnData{
            .health = 10.0f,
            .maxHealth = 10.0f,
            .mass = 0.5f,
            .affectedByGravity = true,
        }
        );

    return ok;
}

// Optional: drop the initial entities into the world. Call after the assets
// above are registered. The player is normally driven by the camera, so a
// spawned "player" body is only needed for a visible third-person model —
// hence it is left commented out by default.
inline void spawnStartingEntities(EntityManager& em) {
    em.spawn("goose", { 264.0f, 264.0f, 70.0f });
    // em.spawn("player", { 264.0f, 264.0f, 70.0f });
}