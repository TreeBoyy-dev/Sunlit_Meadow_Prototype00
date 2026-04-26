#include "BlockManager.h"
#include <stdexcept>

void BlockManager::registerBlock(
    const std::string& name,
    BlockModel model,
    bool transparent,
    bool hasSlab,
    bool hasStair,
    bool hasWall
){

    uint16_t id = nextId++;

    auto newBlock = std::make_unique<Block>(id, name, model, transparent, hasSlab, hasStair, hasWall);
    Block* ptr = newBlock.get();

    blocksById[id] = ptr;
    blocksByName[name] = ptr;

    blocks.push_back(std::move(newBlock));
}

void BlockManager::init() {
    // --- Register base blocks ---
    registerBlock("Air", {}, /*transparent=*/true);
    registerBlock("Grass", grassModel, false, /*hasSlab=*/true, /*hasStair=*/true, /*hasWall=*/false);
    registerBlock("Stone", stoneModel, false, true, true, true);
    registerBlock("Wood",  woodModel,  false, true, true);

    // --- Auto-generate variants ---
    // Snapshot current blocks (avoid mutating map while iterating)
    std::vector<Block> baseBlocks;
    for (auto& [id, block] : blocksById)
        baseBlocks.push_back(block);

    for (const Block& b : baseBlocks) {
        if (b.hasSlab)  registerBlock(b.name + "_Slab",  slabModelFrom (b.model));
        if (b.hasStair) registerBlock(b.name + "_Stair", stairModelFrom(b.model));
        if (b.hasWall)  registerBlock(b.name + "_Wall",  wallModelFrom (b.model));
    }
}

const Block& BlockManager::getById(uint8_t id) const {
    auto it = blocksById.find(id);
    if (it == blocksById.end()) throw std::runtime_error("Unknown block id");
    return it->second;
}

const Block& BlockManager::getByName(const std::string& name) const {
    auto it = blocksByName.find(name);
    if (it == blocksByName.end()) throw std::runtime_error("Unknown block name");
    return blocksById.at(it->second);
}