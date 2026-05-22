#include "Entity.h"
#include "Mat4.h"

void PhysicsBody::integrate(Vec3& position, float dt) {
    // Gravity along -Z (Z is up). Tune the constant later / move to Globals.
    if (affectedByGravity)
        acceleration.z += -9.81f;

    velocity = vec3Add(velocity, vec3Scale(acceleration, dt));
    position = vec3Add(position, vec3Scale(velocity, dt));

    acceleration = { 0, 0, 0 };
}

Vec3 Hitbox::worldMin(Vec3 position) const {
    return vec3Sub(vec3Add(position, offset), halfExtents);
}

Vec3 Hitbox::worldMax(Vec3 position) const {
    return vec3Add(vec3Add(position, offset), halfExtents);
}

bool Hitbox::intersects(Vec3 myPos, const Hitbox& other, Vec3 otherPos) const {
    Vec3 aMin = worldMin(myPos),    aMax = worldMax(myPos);
    Vec3 bMin = other.worldMin(otherPos), bMax = other.worldMax(otherPos);
    return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
           (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
           (aMin.z <= bMax.z && aMax.z >= bMin.z);
}

Entity::Entity(
    EntityData                   data,
    Hitbox                       hitbox,
    std::unique_ptr<EntityModel> model,
    Vec3                         position)
    : position(position),
      data(std::move(data)),
      hitbox(hitbox),
      model(std::move(model)) {
}

void Entity::update(float dt) {
    physics.integrate(position, dt);
    // TODO: resolve collisions against the world using hitbox + worldManager.
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

EntityData&   Entity::getData()    { return data; }
PhysicsBody&  Entity::getPhysics() { return physics; }
const Hitbox& Entity::getHitbox() const { return hitbox; }
EntityModel*  Entity::getModel()   { return model.get(); }

bool Entity::isAlive() const { return data.alive; }
