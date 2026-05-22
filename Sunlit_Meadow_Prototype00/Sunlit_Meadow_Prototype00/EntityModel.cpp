#include "EntityModel.h"

EntityModel::EntityModel(Material material)
    : material(material) {
}

void EntityModel::generateMesh(
    std::vector<Vertex>& vertices,
    std::vector<Uint16>& indices
) {
    // TODO: emit a default box mesh in local space (origin at feet, +Z up).
    (void)vertices;
    (void)indices;
}

Material EntityModel::getMaterial() {
    return material;
}
