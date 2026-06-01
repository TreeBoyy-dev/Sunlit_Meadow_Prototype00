#pragma once
#include <string>
#include <SDL3/SDL.h>
#include "Vectors.h"
#include "DataStructures.h"
#include "EntityTypes.h"
#include "BlockManager.h"
#include "WorldManager.h"

bool sameBlock(Vec3 a, Vec3 b);
// ---------------------------------------------------------------------------
// Entity: a live entity in the world.
// Holds a pointer to its shared, per-type EntityAsset (model + hitbox, owned by
// the EntityManager) plus its own per-instance data (position, rotation,
// EntityData, PhysicsBody). Many entities share one asset.
// ---------------------------------------------------------------------------
class Entity {
private:
    const EntityAsset* asset = nullptr;   // shared per-type data
    Uint16             supportingBlockID;

    Vec3 position = { 0, 0, 0 };
    Vec3 rotation = { 0, 0, 0 };

    EntityData  data;
    PhysicsBody physics;

public:
    Entity(
        const EntityAsset* asset,
        EntityData         data,
        PhysicsBody        physics,
        Vec3               position = { 0, 0, 0 }
    );
    virtual ~Entity() = default;

    // Per-frame step: run physics, then resolve world collisions, then any
    // per-type behaviour (override in subclasses).
    virtual void update(float dt, WorldManager* worldManager);

    // Build this entity's model matrix (translate * rotate) for rendering.
    Mat4 getModelMatrix();

    // --- Accessors ---
    Vec3 getPosition() const;
    void setPosition(Vec3 p);
    Vec3 getRotation() const;
    void setRotation(Vec3 r);

    EntityData& getData();
    PhysicsBody& getPhysics();
    const EntityAsset* getAsset() const;     // shared per-type asset
    const Hitbox& getHitbox() const;    // pulled from the shared asset

    bool isAlive() const;
};