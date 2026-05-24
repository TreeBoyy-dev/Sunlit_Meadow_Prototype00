#include "Block.h"
#include "BlockModel.h"

Block::Block(
    Uint16 id,
    std::string name,
    std::unique_ptr<BlockModel> model,
    std::array<bool, 6> obstructs,
    bool transparent,
    bool hasSlab,
    bool hasStair,
    bool hasWall)
    : id(id),
    name(std::move(name)),
    model(std::move(model)),
    obstructs(obstructs),
    transparent(transparent),
    hasSlab(hasSlab), hasStair(hasStair), hasWall(hasWall) {
}

void Block::generateMeshFromModel(
    std::vector<WorldVertex>& vertices,
    std::vector<Uint16>& indices,
    AdjacencyInfo            adj,
    int x, int y, int z
) {
    model->generateMesh(vertices, indices, adj, x, y, z);
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