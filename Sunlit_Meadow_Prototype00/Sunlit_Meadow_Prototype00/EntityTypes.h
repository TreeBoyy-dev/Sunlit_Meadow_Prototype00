#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <string>

#include "Vectors.h"

// EntityAsset is the shared, per-type object (model + hitbox + GPU resources).
// It is defined in EntityManager.h and owned by the EntityManager; an Entity
// only needs a pointer to it, so a forward declaration is enough here.
struct EntityAsset;

struct PhysicsBody {
    Vec3  velocity = { 0, 0, 0 };
    Vec3  acceleration = { 0, 0, 0 };
    float mass = 1.0f;
    bool  affectedByGravity = true;
    bool  onGround = false;

    // Advance velocity/position one step (semi-implicit Euler). Collision
    // resolution against the world is handled separately by the Entity.
    void integrate(Vec3& position, float dt);
};

// Hitbox: axis-aligned bounding box for collision, relative to the entity.
// This is the SAME for every entity of a type, so it lives on the shared
// EntityAsset. The helpers take the instance position so each entity still
// tests in world space. (Same AABB convention as Frustum.h.)
struct Hitbox {
    Vec3 offset = { 0, 0, 0 };        // box center relative to the entity position
    Vec3 halfExtents = { 0.5f, 0.5f, 0.5f }; // half-size along each axis

    Vec3 worldMin(Vec3 position) const;
    Vec3 worldMax(Vec3 position) const;
    bool intersects(Vec3 myPos, const Hitbox& other, Vec3 otherPos) const;
};

// EntityAsset: the per-TYPE object. One of these exists for every kind of
// entity (one for "sheep", one for "cow", ...). It holds everything that is
// identical for every entity of that type: the shared GPU model (vertex/index
// buffers + texture, loaded from disk once) and the hitbox. This is the
// "asset list" entry, exactly analogous to a Block in the BlockManager.
struct EntityAsset {
    Uint16      id = 0;        // type id, assigned on registration
    std::string name;          // type name, e.g. "sheep"

    // --- Model: shared GPU resources for this entity type ---
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    SDL_GPUTexture* texture = nullptr;   // single 2D texture for this type
    Uint32          numIndices = 0;

    // --- Collision: same shape for every entity of this type ---
    Hitbox hitbox;
};