#include "Block.h"

Block::Block(
    uint8_t id,
    std::string name,
    BlockModel model,
    bool transparent = false,
    bool hasSlab = false,
    bool hasStair = false,
    bool hasWall = false)
    : id(id), name(std::move(name)), model(model),
    transparent(transparent),
    hasSlab(hasSlab), hasStair(hasStair), hasWall(hasWall) {
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
