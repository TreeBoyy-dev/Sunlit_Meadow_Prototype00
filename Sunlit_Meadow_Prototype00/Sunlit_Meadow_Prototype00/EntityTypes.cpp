#include "EntityTypes.h"

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
    Vec3 aMin = worldMin(myPos), aMax = worldMax(myPos);
    Vec3 bMin = other.worldMin(otherPos), bMax = other.worldMax(otherPos);
    return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
        (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
        (aMin.z <= bMax.z && aMax.z >= bMin.z);
}
