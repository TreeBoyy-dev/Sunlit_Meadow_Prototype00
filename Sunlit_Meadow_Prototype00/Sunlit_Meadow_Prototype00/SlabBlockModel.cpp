
#include "SlabBlockModel.h"

SlabBlockModel::SlabBlockModel(
    Material topMaterial,
    Material bottomMaterial,
    Material sideMaterial
)
    : BlockModel(topMaterial, bottomMaterial, sideMaterial)
{
}

SlabBlockModel::SlabBlockModel(
    Material topBottomMaterial,
    Material sideMaterial
)
    : BlockModel(topBottomMaterial, sideMaterial)
{
}

SlabBlockModel::SlabBlockModel(
    Material sideMaterial
)
    : BlockModel(sideMaterial)
{
}

void SlabBlockModel::generateMesh(
    std::vector<WorldVertex>& vertices,
    std::vector<Uint16>& indices,
    AdjacencyInfo adj,
    int x, int y, int z
) 
{
    if (!adj.top || !adj.front || !adj.back || !adj.left || !adj.right) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 0.0f, z + 0.5f },
            { x + 1.0f, y + 0.0f, z + 0.5f },
            { x + 1.0f, y + 1.0f, z + 0.5f },
            { x + 0.0f, y + 1.0f, z + 0.5f },

        };
        addFace(vertices, indices, corners, topMaterial);
    }

    if (!adj.bottom) {
        const Vec3 corners[4] = {
            { x + 1.0f, y + 0.0f, z + 0.0f },
            { x + 0.0f, y + 0.0f, z + 0.0f },
            { x + 0.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, bottomMaterial);
    }

    if (!adj.front) {
        const Vec3 corners[4] = {
            { x + 1.0f, y + 0.0f, z + 0.5f },
            { x + 1.0f, y + 0.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.5f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.back) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 0.0f, z + 0.0f },
            { x + 0.0f, y + 0.0f, z + 0.5f },
            { x + 0.0f, y + 1.0f, z + 0.5f },
            { x + 0.0f, y + 1.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.right) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.0f },
            { x + 1.0f, y + 1.0f, z + 0.5f },
            { x + 0.0f, y + 1.0f, z + 0.5f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }

    if (!adj.left) {
        const Vec3 corners[4] = {
            { x + 0.0f, y + 0.0f, z + 0.5f },
            { x + 1.0f, y + 0.0f, z + 0.5f },
            { x + 1.0f, y + 0.0f, z + 0.0f },
            { x + 0.0f, y + 0.0f, z + 0.0f },
        };
        addFace(vertices, indices, corners, sideMaterial);
    }
}