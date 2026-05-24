#pragma once
#include <string>
#include <memory>
#include <SDL3/SDL.h>
#include "Vectors.h"
#include "DataStructures.h"
#include "EntityModel.h"
#include "WorldTypes.h"

struct EntityData {
    Uint32      id        = 0;
    std::string name;
    float       health    = 1.0f;
    float       maxHealth = 1.0f;
    bool        alive     = true;
};

struct PhysicsBody {
    Vec3  velocity          = { 0, 0, 0 };
    Vec3  acceleration      = { 0, 0, 0 };
    float mass              = 1.0f;
    bool  affectedByGravity = true;
    bool  onGround          = false;

    // Advance velocity/position one step (semi-implicit Euler). Collision
    // resolution against the world is handled separately by the Entity.
    void integrate(Vec3& position, float dt);
};

// ---------------------------------------------------------------------------
// Hitbox: axis-aligned bounding box for collision, relative to the entity.
// (Same AABB convention as Frustum.h: a min/max pair in world space.)
// ---------------------------------------------------------------------------
struct Hitbox {
    Vec3 offset;       // box center relative to the entity position
    Vec3 halfExtents;  // half-size along each axis

    Vec3 worldMin(Vec3 position) const;
    Vec3 worldMax(Vec3 position) const;
    bool intersects(Vec3 myPos, const Hitbox& other, Vec3 otherPos) const;
};

class Entity {
private:
    Vec3 position = { 0, 0, 0 };
    Vec3 rotation = { 0, 0, 0 };

    EntityData                   data;
    PhysicsBody                  physics;
    Hitbox                       hitbox;
    std::unique_ptr<EntityModel> model;

public:
    Entity(
        EntityData                   data,
        Hitbox                       hitbox,
        std::unique_ptr<EntityModel> model,
        Vec3                         position = { 0, 0, 0 }
    );
    virtual ~Entity() = default;

    // Per-frame step: run physics, then resolve world collisions, then any
    // per-type behaviour (override in subclasses).
    virtual void update(float dt);

    // Build this entity's model matrix (translate * rotate) for rendering.
    Mat4 getModelMatrix();

    // --- Accessors ---
    Vec3 getPosition() const;
    void setPosition(Vec3 p);
    Vec3 getRotation() const;
    void setRotation(Vec3 r);

    EntityData&  getData();
    PhysicsBody& getPhysics();
    const Hitbox& getHitbox() const;
    EntityModel*  getModel();

    bool isAlive() const;
};
