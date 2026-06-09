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
    std::vector<Uint16>& indices,
    int x, int y, int z
) {
    if (!modelInit) {
        model->init(modelFileName);
        modelInit = true;
    }
    model->getMesh(vertices, indices, x, y, z);
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
