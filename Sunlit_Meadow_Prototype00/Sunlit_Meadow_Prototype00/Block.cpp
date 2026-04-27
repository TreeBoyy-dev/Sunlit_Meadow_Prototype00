#include "Block.h"
#include "BlockModel.h"

Block::Block(
    Uint16 id,
    std::string name,
    BlockModel model,
    bool transparent,
    bool hasSlab,
    bool hasStair,
    bool hasWall)
    : id(id), name(std::move(name)), model(model),
    transparent(transparent),
    hasSlab(hasSlab), hasStair(hasStair), hasWall(hasWall) {
}

void Block::generateMeshFromModel(
    std::vector<VertexData>& vertices,
    std::vector<Uint16>& indices,
    AdjacencyInfo            adj,
    int x, int y, int z
) {
    model.generateMesh(vertices, indices, adj, x, y, z);
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
