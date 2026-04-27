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

void BlockModel::addFace(
    std::vector<VertexData>& vertices,
    std::vector<uint16_t>&   indices,
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
    uint16_t base = static_cast<uint16_t>(vertices.size());

    for (int i = 0; i < 4; i++) {
        vertices.push_back({ corners[i], uvs[i], white, (float)materialIndex });
    }

    // Two triangles per quad: 0-1-2, 2-3-0
    indices.insert(indices.end(), {
        base,                 (uint16_t)(base + 1), (uint16_t)(base + 2),
        (uint16_t)(base + 2), (uint16_t)(base + 3), base
    });
}

void BlockModel::generateMesh(
    std::vector<VertexData>& vertices,
    std::vector<uint16_t>&   indices,
    AdjacencyInfo            adj)
{
    // Unit cube from (0,0,0) to (1,1,1)
    // Each face defined as 4 corners in CCW winding order

    if (!adj.top) {
        const Vec3 corners[4] = {
            { 0.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f },
            { 0.0f, 1.0f, 1.0f },
        };
        addFace(vertices, indices, corners, topMaterial);
    }

    if (!adj.bottom) {
        const Vec3 corners[4] = {
            { 0.0f, 0.0f, 1.0f },
            { 1.0f, 0.0f, 1.0f },
            { 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
        };
        addFace(vertices, indices, corners, bottomMaterial);
    }

    if (!adj.front) {  // +Z
        const Vec3 corners[4] = {
            { 0.0f, 0.0f, 1.0f },
            { 1.0f, 0.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f },
            { 0.0f, 1.0f, 1.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.back) {   // -Z
        const Vec3 corners[4] = {
            { 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.right) {  // +X
        const Vec3 corners[4] = {
            { 1.0f, 0.0f, 1.0f },
            { 1.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.left) {   // -X
        const Vec3 corners[4] = {
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f },
            { 0.0f, 1.0f, 1.0f },
            { 0.0f, 1.0f, 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }
}