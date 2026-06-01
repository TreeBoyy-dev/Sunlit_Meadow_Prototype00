#include "Entity.h"
#include "Mat4.h"

bool sameBlock(Vec3 a, Vec3 b) {
    return floorf(a.x) == floorf(b.x)
        && floorf(a.y) == floorf(b.y)
        && floorf(a.z) == floorf(b.z);
}

Entity::Entity(
    const EntityAsset* asset,
    EntityData         data,
    PhysicsBody        physics,
    Vec3               position)
    : asset(asset),
    position(position),
    data(std::move(data)),
    physics(physics)
{
}

void Entity::update(float dt, WorldManager* worldManager) {
    Vec3 initialPos = position;

    if (physics.affectedByGravity)
        physics.acceleration.z += -9.81f;

    float downAccMult = worldManager->getBlockCollision(position);
    if (downAccMult == -1) {
        SDL_Log("[Entity] supporting block id not found");
        physics.acceleration.z = 0;
    }
    else {
        physics.acceleration.z = physics.acceleration.z * downAccMult;
        if (downAccMult == 0.0 && physics.velocity.z != 0)
            physics.velocity.z = 0.0;
    }
        
    physics.integrate(position, dt);
}

Mat4 Entity::getModelMatrix() {
    Mat4 t = mat4Translate(position.x, position.y, position.z);
    Mat4 r = mat4Rotate(rotation.x, rotation.y, rotation.z);
    return mat4Mul(t, r);
}

Vec3 Entity::getPosition() const { return position; }
void Entity::setPosition(Vec3 p) { position = p; }
Vec3 Entity::getRotation() const { return rotation; }
void Entity::setRotation(Vec3 r) { rotation = r; }

EntityData& Entity::getData() { return data; }
PhysicsBody& Entity::getPhysics() { return physics; }
const EntityAsset* Entity::getAsset() const { return asset; }
const Hitbox& Entity::getHitbox() const { return asset->hitbox; }

bool Entity::isAlive() const { return data.alive; }