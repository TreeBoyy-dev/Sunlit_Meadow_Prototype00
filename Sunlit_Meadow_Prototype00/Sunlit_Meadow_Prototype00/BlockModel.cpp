#include "BlockModel.h"
#include "Materials.h"
#include "ObjParser.h"

static const char* baseModelPath = "Models/";

BlockModel::BlockModel(
    ModelFace topMaterial,
    ModelFace bottomMaterial,
    ModelFace sideMaterial)
    : topMaterial(topMaterial),
    bottomMaterial(bottomMaterial),
    sideMaterial(sideMaterial)
{
}
BlockModel::BlockModel(
    ModelFace topBottomMaterial,
    ModelFace sideMaterial)
    : topMaterial(topBottomMaterial),
    bottomMaterial(topBottomMaterial),
    sideMaterial(sideMaterial)
{
}
BlockModel::BlockModel(
    ModelFace sideMaterial)
    : topMaterial(sideMaterial),
    bottomMaterial(sideMaterial),
    sideMaterial(sideMaterial)
{
}

ModelFace BlockModel::materialForNormal(const Vec3& normal) const {
    if (normal.z > 0.5f)  return topMaterial;
    if (normal.z < -0.5f) return bottomMaterial;
    return sideMaterial;
}

bool BlockModel::init(const char* fileName) {
    std::vector<ModelVertex> modelVertices;
    std::vector<Uint16>      modelIndices;

    if (!obj_parse(BuildAbsolutePath(baseModelPath, fileName), modelVertices, modelIndices)) {
        SDL_Log("[BlockModel] init: obj_parse returned false");
        return false;
    }

    const float kPlacementOffsetX = 0.5f;
    const float kPlacementOffsetY = 0.5f;
    const float kOverlayEpsilon = 0.001f;

    vertices.clear();
    indices.clear();
    vertices.reserve(modelVertices.size());

    // overlayRemap[i] = index of the overlay copy of base vertex i, or -1 if it has none
    std::vector<int> overlayRemap(modelVertices.size(), -1);

    // 1) Base vertices
    for (const ModelVertex& mv : modelVertices) {
        ModelFace material = materialForNormal(mv.normal);
        vertices.push_back({
            { mv.position.x + kPlacementOffsetX,
              mv.position.y + kPlacementOffsetY,
              mv.position.z },
            mv.normal,
            mv.uv,
            mv.color,
            static_cast<float>(material.material)
            });
    }

    // 2) Overlay vertices
    for (size_t i = 0; i < modelVertices.size(); i++) {
        const ModelVertex& mv = modelVertices[i];
        ModelFace material = materialForNormal(mv.normal);
        if (material.overlayMaterial < 0) continue;

        overlayRemap[i] = (int)vertices.size();
        vertices.push_back({
            { mv.position.x + kPlacementOffsetX + mv.normal.x * kOverlayEpsilon,
              mv.position.y + kPlacementOffsetY + mv.normal.y * kOverlayEpsilon,
              mv.position.z + mv.normal.z * kOverlayEpsilon },
            mv.normal,
            mv.uv,
            mv.color,
            static_cast<float>(material.overlayMaterial)
            });
    }

    // 3) Base indices
    indices = modelIndices;

    // 4) Overlay indices — duplicate each triangle whose 3 verts all have overlays
    for (size_t t = 0; t + 2 < modelIndices.size(); t += 3) {
        Uint16 a = modelIndices[t + 0];
        Uint16 b = modelIndices[t + 1];
        Uint16 c = modelIndices[t + 2];
        if (overlayRemap[a] < 0 || overlayRemap[b] < 0 || overlayRemap[c] < 0)
            continue;
        indices.push_back((Uint16)overlayRemap[a]);
        indices.push_back((Uint16)overlayRemap[b]);
        indices.push_back((Uint16)overlayRemap[c]);
    }

    return true;
}

void BlockModel::getMesh(
    std::vector<WorldVertex>& outVertices,
    std::vector<Uint32>& outIndices,
    int x, int y, int z, Uint16 state)
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

ModelFace BlockModel::getTopMaterial() {
    return topMaterial;
}
ModelFace BlockModel::getBottomMaterial() {
    return bottomMaterial;
}
ModelFace BlockModel::getSideMaterial() {
    return sideMaterial;
}