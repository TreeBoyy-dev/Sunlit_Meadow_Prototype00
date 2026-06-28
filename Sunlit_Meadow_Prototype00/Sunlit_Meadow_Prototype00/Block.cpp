#include "Block.h"
#include "BlockModel.h"

Block::Block(
    Uint16 id,
    std::string name,
    const char* modelFileName,
    std::unique_ptr<BlockModel> model,
    std::array<bool, 6> obstructs,
    bool transparent,
    bool hasSlab,
    bool hasStair,
    bool hasWall,
    float collision)
    : id(id),
    name(std::move(name)),
    modelFileName(modelFileName),
    model(std::move(model)),
    obstructs(obstructs),
    transparent(transparent),
    hasSlab(hasSlab), hasStair(hasStair), hasWall(hasWall),
    collision(collision),
    modelInit(false)
{
}

void Block::generateMeshFromModel(
    std::vector<WorldVertex>& vertices,
    std::vector<Uint32>& indices,
    int x, int y, int z
) {
    if (!modelInit) {
        model->init(modelFileName);
        modelInit = true;
    }
    model->getMesh(vertices, indices, x, y, z);
}

bool Block::buildItemModel(
    std::vector<ModelVertex>& outVertices,
    std::vector<Uint16>& outIndices
) {
    const char* file = modelFileName;

    std::vector<ModelVertex> verts;
    std::vector<Uint16>      idx;
    if (!obj_parse(BuildAbsolutePath("Models", file), verts, idx)) {
        SDL_Log("[Block] buildItemModel: obj_parse failed for '%s'", file);
        return false;
    }

    // Horizontal 3-cell atlas laid out [ side | top | bottom ]; cell order must
    // match buildBlockIconTexture().
    constexpr float cellW = 1.0f / 3.0f;
    // Nudge UVs away from the cell seams so the sampler can't bleed one cell
    // into the next. ~half a texel for 16px tiles; lower it toward 0 if you use
    // a NEAREST sampler or larger textures, raise it if seams still leak under
    // the current LINEAR model sampler.
    constexpr float inset = 1.0f / 32.0f;

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
bool Block::getHasSlab() {
    return hasSlab;
}
bool Block::getHasStair() {
    return hasStair;
}
bool Block::getHasWall() {
    return hasWall;
}
float Block::getCollision() {
    return collision;
}
std::string Block::getName() {
    return name;
}
Uint16 Block::getID() {
    return id;
}

Material Block::getTopMaterial() { return model->getTopMaterial(); }
Material Block::getBottomMaterial() { return model->getBottomMaterial(); }
Material Block::getSideMaterial() { return model->getSideMaterial(); }
bool Block::getObstructs(int faceIndex) { return obstructs[faceIndex]; }

SDL_GPUTexture* Block::getIcon() {
    return nullptr;
}
