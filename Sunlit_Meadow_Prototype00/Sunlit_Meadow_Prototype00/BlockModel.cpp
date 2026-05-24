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
    std::vector<WorldVertex>& vertices,
    std::vector<Uint16>& indices,
    const Vec3           corners[4],
    Material             materialIndex)
{
    // UV coordinates are the same for every face
    const Vec2 uvs[4] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
    };

    const SDL_FColor white = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Compute face normal from two edges (consistent with winding order)
    Vec3 edge1 = { corners[1].x - corners[0].x, corners[1].y - corners[0].y, corners[1].z - corners[0].z };
    Vec3 edge2 = { corners[3].x - corners[0].x, corners[3].y - corners[0].y, corners[3].z - corners[0].z };
    Vec3 normal = {
        edge1.y * edge2.z - edge1.z * edge2.y,
        edge1.z * edge2.x - edge1.x * edge2.z,
        edge1.x * edge2.y - edge1.y * edge2.x,
    };

    // Indices are relative to the current end of the vertex buffer
    Uint16 base = static_cast<Uint16>(vertices.size());

    for (int i = 0; i < 4; i++) {
        vertices.push_back({ corners[i], normal, uvs[i], white, (float)materialIndex });
    }

    indices.insert(indices.end(), {
        base, (Uint16)(base + 1), (Uint16)(base + 2),
        base, (Uint16)(base + 2), (Uint16)(base + 3)
    });
}

void BlockModel::generateMesh(
    std::vector<WorldVertex>& vertices,
    std::vector<Uint16>& indices,
    AdjacencyInfo        adj,
    int x, int y, int z)
{
    if (!adj.top) {
        const Vec3 corners[4] = {
            { x + 1.0f, y + 0.0f, z + 1.0f },
            { x + 1.0f, y + 1.0f, z + 1.0f },
            { x + 0.0f, y + 1.0f, z + 1.0f },
            { x + 0.0f, y + 0.0f, z + 1.0f },
        };
        addFace(vertices, indices, corners, topMaterial);
    }

    if (!adj.bottom) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 0.0f, z + 0.0f },
            { x + 0.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 0.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, bottomMaterial);
    }

    if (!adj.front) {
        const Vec3 corners[4] = {
            { x + 1.0f, y + 1.0f, z + 1.0f },
            { x + 1.0f, y + 0.0f, z + 1.0f },
            { x + 1.0f, y + 0.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.back) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 0.0f, z + 1.0f },
            { x + 0.0f, y + 1.0f, z + 1.0f },
            { x + 0.0f, y + 1.0f, z + 0.0f },
            { x + 0.0f, y + 0.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.right) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 1.0f, z + 1.0f },
            { x + 1.0f, y + 1.0f, z + 1.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
            { x + 0.0f, y + 1.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.left) {
        const Vec3 corners[4] = {
            { x + 1.0f, y + 0.0f, z + 1.0f },
            { x + 0.0f, y + 0.0f, z + 1.0f },
            { x + 0.0f, y + 0.0f, z + 0.0f },
            { x + 1.0f, y + 0.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
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