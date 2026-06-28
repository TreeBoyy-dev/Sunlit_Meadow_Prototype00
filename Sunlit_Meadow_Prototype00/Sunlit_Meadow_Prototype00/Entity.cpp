#include "Entity.h"
#include "Mat4.h"

bool sameBlock(Vec3 a, Vec3 b) {
    return floorf(a.x) == floorf(b.x)
        && floorf(a.y) == floorf(b.y)
        && floorf(a.z) == floorf(b.z);
}

Entity::Entity(
    Uint16 id,
    const EntityAsset* asset,
    std::vector<std::unique_ptr<Data>> dataIn,
    PhysicsBody        physics,
    Vec3               position)
    : id(id),
    asset(asset),
    position(position),
    data(std::move(dataIn)),
    physics(physics),
    supportingBlockID(0)
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

Data* Entity::getData(Datatype type)
{
    return findByType(&data, type);
}

Uint16 Entity::getId() { return id; }
Vec3 Entity::getPosition() const { return position; }
void Entity::setPosition(Vec3 p) { position = p; }
Vec3 Entity::getRotation() const { return rotation; }
void Entity::setRotation(Vec3 r) { rotation = r; }

PhysicsBody& Entity::getPhysics() { return physics; }
const EntityAsset* Entity::getAsset() const { return asset; }
const Hitbox& Entity::getHitbox() const { return asset->hitbox; }

bool Entity::isAlive() {
    EntityData* entityData;
    Data* found = findByType(&data, ENTITY);
    if (found == nullptr)
        return false;
    else
        entityData = static_cast<EntityData*>(found);

    return entityData->alive;
}