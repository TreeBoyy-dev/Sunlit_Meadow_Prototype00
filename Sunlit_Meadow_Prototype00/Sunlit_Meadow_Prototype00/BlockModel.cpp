#include "BlockModel.h"
#include "Materials.h"

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

void BlockModel::addFace(
    std::vector<VertexData>& vertices,
    std::vector<Uint16>&   indices,
    const Vec3               corners[4],
    Material                 materialIndex)
{
    // UV coordinates are the same for every face
    const Vec2 uvs[4] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
    };

    const SDL_FColor white = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Indices are relative to the current end of the vertex buffer
    Uint16 base = static_cast<Uint16>(vertices.size());

    for (int i = 0; i < 4; i++) {
        vertices.push_back({ corners[i], uvs[i], white, (float)materialIndex });
    }

    indices.insert(indices.end(), {
        base,                 (Uint16)(base + 1), (Uint16)(base + 2),
        (Uint16)(base + 2), (Uint16)(base + 3), base
    });
}

void BlockModel::generateMesh(
    std::vector<VertexData>& vertices,
    std::vector<Uint16>&     indices,
    AdjacencyInfo            adj,
    int x, int y, int z)
{
    if (!adj.top) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 1.0f },
            { x + 0.0f, y + 1.0f, z + 1.0f },
        };
        addFace(vertices, indices, corners, topMaterial);
    }

    if (!adj.bottom) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 0.0f, z + 1.0f },
            { x + 1.0f, y + 0.0f, z + 1.0f },
            { x + 1.0f, y + 0.0f, z + 0.0f },
            { x + 0.0f, y + 0.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, bottomMaterial);
    }

    if (!adj.front) {  // +Z
        const Vec3 corners[4] = {
            { x + 0.0f, y + 0.0f, z + 1.0f },
            { x + 1.0f, y + 0.0f, z + 1.0f },
            { x + 1.0f, y + 1.0f, z + 1.0f },
            { x + 0.0f, y + 1.0f, z + 1.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.back) {   // -Z
        const Vec3 corners[4] = {
            { x + 1.0f, y + 0.0f, z + 0.0f },
            { x + 0.0f, y + 0.0f, z + 0.0f },
            { x + 0.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.right) {  // +X
        const Vec3 corners[4] = {
            { x + 1.0f, y + 0.0f, z + 1.0f },
            { x + 1.0f, y + 0.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 1.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.left) {   // -X
        const Vec3 corners[4] = {
            { x + 0.0f, y + 0.0f, 0.0f },
            { x + 0.0f, y + 0.0f, 1.0f },
            { x + 0.0f, y + 1.0f, 1.0f },
            { x + 0.0f, y + 1.0f, 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }
}