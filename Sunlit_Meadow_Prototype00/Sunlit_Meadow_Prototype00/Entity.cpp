#include "Entity.h"
#include "Mat4.h"

Entity::Entity(
    const EntityAsset* asset,
    EntityData         data,
    PhysicsBody        physics,
    Vec3               position)
    : asset(asset),
    position(position),
    data(std::move(data)),
    physics(physics) {
}

void Entity::update(float dt) {
    physics.integrate(position, dt);
    // TODO: resolve collisions against the world using getHitbox() + worldManager.
    (void)dt;
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