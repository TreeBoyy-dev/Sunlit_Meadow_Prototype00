#include "Block.h"
#include "BlockModel.h"

Block::Block(
    Uint16 id,
    std::string name,
    std::string modelFileName,
    std::unique_ptr<BlockModel> model,
    StateLayout layout,
    Collision collision,
    std::array<bool, 6> obstructs,
    bool transparent)
    : id(id),
    name(std::move(name)),
    modelFileName(std::move(modelFileName)),
    model(std::move(model)),
    layout(std::move(layout)),
    obstructs(obstructs),
    transparent(transparent),
    collision(collision)
{
}

void Block::generateMeshFromModel(
    std::vector<WorldVertex>& vertices,
    std::vector<Uint32>& indices,
    int x, int y, int z, Uint16 state
) {
    // All variants were baked at BlockManager::init() (on the main thread,
    // before the mesh workers spin up) — this is a pure read-only lookup and
    // safe to call from any thread.
    model->getMesh(vertices, indices, x, y, z, state);
}

bool Block::buildItemModel(
    std::vector<ModelVertex>& outVertices,
    std::vector<Uint16>& outIndices
) {
    const char* file = modelFileName.c_str();

    std::vector<ModelVertex> verts;
    std::vector<Uint16>      idx;
    if (!obj_parse(BuildAbsolutePath("Models", file), verts, idx)) {
        SDL_Log("[Block] buildItemModel: obj_parse failed for '%s'", file);
        return false;
    }
    constexpr float cellW = 1.0f / 3.0f;
    constexpr float inset = 1.0f / 16.0f;

    for (ModelVertex& v : verts) {
        int cell;                              // 0 side, 1 top, 2 bottom
        if (v.normal.z > 0.5f)       cell = 1; // +Z up   -> top
        else if (v.normal.z < -0.5f) cell = 2; // -Z down -> bottom
        else                         cell = 0; // sides

        const float u = v.uv.x;
        const float w = v.uv.y;
        v.uv.x = (cell + inset + u * (1.0f - 2.0f * inset)) * cellW;
        v.uv.y = inset + w * (1.0f - 2.0f * inset);
    }

    outVertices = std::move(verts);
    outIndices = std::move(idx);
    return true;
}

bool Block::isTransparent() {
    return transparent;
}
std::string Block::getName() {
    return name;
}
Uint16 Block::getID() {
    return id;
}

ModelFace Block::getTopMaterial() { return model->getTopMaterial(); }
ModelFace Block::getBottomMaterial() { return model->getBottomMaterial(); }
ModelFace Block::getSideMaterial() { return model->getSideMaterial(); }
bool Block::getObstructs(int faceIndex) { return obstructs[faceIndex]; }

SDL_GPUTexture* Block::getIcon() {
    return nullptr;
}
