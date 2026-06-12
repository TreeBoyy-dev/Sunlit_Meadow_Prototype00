#pragma once
#include <string>
#include <SDL3/SDL.h>
#include "Vectors.h"
#include "DataStructures.h"
#include "EntityTypes.h"
#include "BlockManager.h"
#include "WorldManager.h"
#include "EntityData.h"

bool sameBlock(Vec3 a, Vec3 b);

class Entity {
private:
    const EntityAsset* asset = nullptr;   // shared per-type data
    Uint16             supportingBlockID;

    Vec3 position = { 0, 0, 0 };
    Vec3 rotation = { 0, 0, 0 };

    std::vector<Data*>  data;
    PhysicsBody         physics;

public:
    Entity(
        const EntityAsset* asset,
        std::vector<Data*> data,
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
    Data* getData(Datatype type);

    Vec3 getPosition() const;
    void setPosition(Vec3 p);
    Vec3 getRotation() const;
    void setRotation(Vec3 r);

    PhysicsBody& getPhysics();
    const EntityAsset* getAsset() const;     // shared per-type asset
    const Hitbox& getHitbox() const;    // pulled from the shared asset

    bool isAlive();
};