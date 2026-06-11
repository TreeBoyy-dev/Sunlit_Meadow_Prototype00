#include "BlockModel.h"
#include "Materials.h"
#include "ObjParser.h"

static const char* baseModelPath = "Models/";

BlockModel::BlockModel(
    Material topMaterial,
    Material bottomMaterial,
    Material sideMaterial)
    : topMaterial(topMaterial),
    bottomMaterial(bottomMaterial),
    sideMaterial(sideMaterial)
{
}
BlockModel::BlockModel(
    Material topBottomMaterial,
    Material sideMaterial)
    : topMaterial(topBottomMaterial),
    bottomMaterial(topBottomMaterial),
    sideMaterial(sideMaterial)
{
}
BlockModel::BlockModel(
    Material sideMaterial)
    : topMaterial(sideMaterial),
    bottomMaterial(sideMaterial),
    sideMaterial(sideMaterial)
{
}

Material BlockModel::materialForNormal(const Vec3& normal) const {
    if (normal.z > 0.5f)  return topMaterial;
    if (normal.z < -0.5f) return bottomMaterial;
    return sideMaterial;
}

bool BlockModel::init(const char* fileName) {
    std::vector<ModelVertex> modelVertices;
    std::vector<Uint16>      modelIndices;

    if (!obj_parse(BuildAbsolutePath(baseModelPath, fileName), modelVertices, modelIndices)) {
        return false;
        SDL_Log("[BlockModel] init: obj_parse retured false");
    }

    const float kPlacementOffsetX = 0.5f;
    const float kPlacementOffsetY = 0.5f;

    // Convert ModelVertex -> WorldVertex (adds the materialIndex field).
    vertices.clear();
    vertices.reserve(modelVertices.size());

    for (const ModelVertex& mv : modelVertices) {
        Material material = materialForNormal(mv.normal);
        vertices.push_back({
            { mv.position.x + kPlacementOffsetX,
              mv.position.y + kPlacementOffsetY,
              mv.position.z },
            mv.normal,
            mv.uv,
            mv.color,
            static_cast<float>(material)
            });
    }

    indices = std::move(modelIndices);
    return true;
}

void BlockModel::getMesh(
    std::vector<WorldVertex>& outVertices,
    std::vector<Uint32>& outIndices,
    int x, int y, int z)
{
    // Indices are relative to where this model's vertices land in the buffer.
    const Uint32 base = static_cast<Uint32>(outVertices.size());

    for (const WorldVertex& v : vertices) {
        WorldVertex moved = v;
        moved.position.x += static_cast<float>(x);
        moved.position.y += static_cast<float>(y);
        moved.position.z += static_cast<float>(z);
        outVertices.push_back(moved);
    }

    for (const Uint16 index : indices) {
        outIndices.push_back(base + index);
    }
}

Material BlockModel::getTopMaterial() {
    return topMaterial;
}
Material BlockModel::getBottomMaterial() {
    return bottomMaterial;
}
Material BlockModel::getSideMaterial() {
    return sideMaterial;
}