#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Data.h"

/*
struct SpawnData {
    float health = 1.0f;
    float maxHealth = 1.0f;
    float mass = 1.0f;
    bool  affectedByGravity = true;
};

// EntityData: per-instance MUTABLE state.
// Seeded from the type's SpawnData when the entity is spawned, then changes
// over the entity's lifetime (e.g. it takes damage). NOT shared between
// entities of the same type.
struct EntityData {
};*/

class EntityData : public Data {
public:
    EntityData() : Data(ENTITY) {};
    EntityData(float health, float maxHealth, float mass, bool affectedByGravity = true)
        : Data(ENTITY)
        , health(health)
        , maxHealth(maxHealth)
        , mass(mass)
        , affectedByGravity(affectedByGravity)
    {};

    float  health = 1.0f;
    float  maxHealth = 1.0f;
    bool   alive = true;
    float  mass = 1.0f;
    bool   affectedByGravity = true;
};