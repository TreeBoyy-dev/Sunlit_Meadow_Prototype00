#include "BlockManager.h"
#include "BlockModel.h"
#include <stdexcept>
#include <vector>

void BlockManager::registerBlock(
    const std::string& name,
    BlockModel model,
    bool transparent,
    bool hasSlab,
    bool hasStair,
    bool hasWall
){

    uint16_t id = nextId++;

    auto newBlock = std::make_unique<Block>(
        id,
        name,
        model,
        transparent,
        hasSlab,
        hasStair,
        hasWall
    );
    //auto newBlock = std::make_unique<Block>;
    Block* ptr = newBlock.get();

    blocksById[id] = ptr;
    blocksByName[name] = ptr;

    blocks.push_back(std::move(newBlock));
}

void BlockManager::init() {
    // --- Register base blocks ---
    registerBlock("Air", {}, /*transparent=*/true);
    registerBlock("Grass", BlockModel(), false, /*hasSlab=*/true, /*hasStair=*/true, /*hasWall=*/false);
    registerBlock("Stone", stoneModel, false, true, true, true);
    registerBlock("Wood",  woodModel,  false, true, true);

    // --- Auto-generate variants ---
    // Snapshot current blocks (avoid mutating map while iterating)
    std::vector<Block> baseBlocks;
    for (auto& [id, block] : blocksById)
        baseBlocks.push_back(*block);

    for (Block& b : baseBlocks) {
        if (b.getHasSlab())  registerBlock(b.getName() + "_Slab",  slabModelFrom(b.model));
        if (b.getHasStair()) registerBlock(b.getName() + "_Stair", stairModelFrom(b.model));
        if (b.getHasWall())  registerBlock(b.getName() + "_Wall",  wallModelFrom(b.model));
    }
}

Block* BlockManager::getById(uint16_t id) {
    auto it = blocksById.find(id);
    return it != blocksById.end() ? it->second : nullptr;
}

Block* BlockManager::getByName(const std::string& name) {
    auto it = blocksByName.find(name);
    return it != blocksByName.end() ? it->second : nullptr;
}