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

void Entity::update(float dt, WorldManager* worldManager)
{
    // 1) forces -> velocity (semi-implicit Euler, like the old integrate but no move yet)
    if (physics.affectedByGravity)
        physics.acceleration.z += -9.81f;
    physics.velocity = vec3Add(physics.velocity, vec3Scale(physics.acceleration, dt));
    physics.acceleration = { 0, 0, 0 };

    // 2) medium drag: sample the block at the entity centre. slowdown = % REMOVED.
    const Collision* medium =
        worldManager->getBlockCollision(vec3Add(position, asset->hitbox.offset));
    if (medium && !medium->solid && medium->slowdown > 0) {
        float keep = 1.0f - (medium->slowdown / 100.0f);   // water 30 -> keep 0.7
        physics.velocity = vec3Scale(physics.velocity, keep);
    }

    // 3) move + resolve, one axis at a time
    Vec3 delta = vec3Scale(physics.velocity, dt);
    physics.onGround = false;
    collideAxis(0, delta.x, worldManager);
    collideAxis(1, delta.y, worldManager);
    bool hitZ = collideAxis(2, delta.z, worldManager);
    if (hitZ && delta.z < 0.0f) physics.onGround = true;   // blocked while descending = grounded
}
/*{
    Vec3 initialPos = position;

    if (physics.affectedByGravity)
        physics.acceleration.z += -9.81f;

    const Collision* col = worldManager->getBlockCollision(position);
    if (!col) {
        SDL_Log("[Entity] supporting block id not found");
        physics.acceleration.z = 0;
    }
    else if (col->solid) {
        physics.acceleration.z = 0;
        if (physics.velocity.z != 0) physics.velocity.z = 0;
    }
    else {
        // passable medium. Phase 1 keeps slowdown on ACCELERATION to match old feel
        // (air 100 -> *1.0, water 70 -> *0.7). Phase 2 moves this onto velocity.
        physics.acceleration.z *= 1.0f - (col->slowdown / 100.0f);
    }

    physics.integrate(position, dt);
}//*/

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

static float& comp(Vec3& v, int a)       { return a == 0 ? v.x : (a == 1 ? v.y : v.z); }
static float  comp(const Vec3& v, int a) { return a == 0 ? v.x : (a == 1 ? v.y : v.z); }

bool Entity::collideAxis(int axis, float delta, WorldManager* worldManager) {
    if (delta == 0.0f) return false;

    const Vec3& off = asset->hitbox.offset;
    const Vec3& he = asset->hitbox.halfExtents;

    Vec3 mn = vec3Sub(vec3Add(position, off), he);
    Vec3 mx = vec3Add(vec3Add(position, off), he);

    Vec3 smn = mn, smx = mx;                 // swept span on this axis
    comp(smn, axis) += (delta < 0.0f ? delta : 0.0f);
    comp(smx, axis) += (delta > 0.0f ? delta : 0.0f);

    const float EPS = 1e-4f;
    int x0 = (int)std::floor(smn.x + EPS), x1 = (int)std::floor(smx.x - EPS);
    int y0 = (int)std::floor(smn.y + EPS), y1 = (int)std::floor(smx.y - EPS);
    int z0 = (int)std::floor(smn.z + EPS), z1 = (int)std::floor(smx.z - EPS);

    const int a1 = (axis + 1) % 3, a2 = (axis + 2) % 3;
    static const AABB FULL_CUBE = { {0,0,0}, {1,1,1} };

    float allowed = delta;
    bool  blocked = false;

    for (int bx = x0; bx <= x1; ++bx)
        for (int by = y0; by <= y1; ++by)
            for (int bz = z0; bz <= z1; ++bz) {
                const Collision* col =
                    worldManager->getBlockCollision({ bx + 0.5f, by + 0.5f, bz + 0.5f });
                if (!col || !col->solid) continue;

                size_t count = col->boxes.empty() ? 1 : col->boxes.size();
                for (size_t i = 0; i < count; ++i) {
                    const AABB& b = col->boxes.empty() ? FULL_CUBE : col->boxes[i];
                    Vec3 wbMin = { bx + b.min.x, by + b.min.y, bz + b.min.z };
                    Vec3 wbMax = { bx + b.max.x, by + b.max.y, bz + b.max.z };

                    // need genuine overlap on the two perpendicular axes, else we slide past
                    if (comp(mx, a1) <= comp(wbMin, a1) + EPS || comp(mn, a1) >= comp(wbMax, a1) - EPS) continue;
                    if (comp(mx, a2) <= comp(wbMin, a2) + EPS || comp(mn, a2) >= comp(wbMax, a2) - EPS) continue;

                    if (delta > 0.0f) {
                        float gap = comp(wbMin, axis) - comp(mx, axis);
                        if (gap < -EPS) continue;   // box is behind the leading (top) face - not in our path
                        if (gap < allowed) { allowed = (gap > 0.0f ? gap : 0.0f); blocked = true; }
                    }
                    else {
                        float gap = comp(wbMax, axis) - comp(mn, axis);
                        if (gap > EPS) continue;    // box is ahead of the leading (bottom) face - not a floor we're landing on
                        if (gap > allowed) { allowed = (gap < 0.0f ? gap : 0.0f); blocked = true; }
                    }
                }
            }

    comp(position, axis) += allowed;
    if (blocked) comp(physics.velocity, axis) = 0.0f;
    return blocked;
}