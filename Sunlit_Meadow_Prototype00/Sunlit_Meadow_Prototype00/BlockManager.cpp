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

    Uint16 id = nextId++;

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
    registerBlock("air",            BlockModel(MATERIAL_COBBLESTONE), /*transparent=*/true);
    registerBlock("cobble_stone",   BlockModel(MATERIAL_COBBLESTONE), false, true, true, false);
    registerBlock("gneiss",         BlockModel(MATERIAL_GNEISS), false, true, true, false);
    registerBlock("marble",         BlockModel(MATERIAL_MARBLE), false, true, true, false);
    registerBlock("diorite",        BlockModel(MATERIAL_DIORITE), false, true, true, true);
    
    registerBlock("dirt",           BlockModel(MATERIAL_DIRT), false, true, true);
    registerBlock("grass_block",    BlockModel(MATERIAL_GRASS_BLOCK, MATERIAL_DIRT), false, true, true);

    registerBlock("birch_log",      BlockModel(MATERIAL_BIRCH_LOG_TOP, MATERIAL_BIRCH_LOG_SIDE), false, true, true);
    registerBlock("birch_leaves",   BlockModel(MATERIAL_BIRCH_LEAVES), false, true, true);

    registerBlock("chestnut_log",   BlockModel(MATERIAL_CHESTNUT_LOG_TOP, MATERIAL_CHESTNUT_LOG_SIDE), false, true, true);
    registerBlock("chestnut_leaves",BlockModel(MATERIAL_CHESTNUT_LEAVES), false, true, true);

    // --- Auto-generate variants ---
    // Snapshot current blocks (avoid mutating map while iterating)
    std::vector<Block> baseBlocks;
    for (auto& [id, block] : blocksById)
        baseBlocks.push_back(*block);

    for (Block& b : baseBlocks) {
        //if (b.getHasSlab())  registerBlock(b.getName() + "_Slab",  slabModelFrom(b.model));
        //if (b.getHasStair()) registerBlock(b.getName() + "_Stair", stairModelFrom(b.model));
        //if (b.getHasWall())  registerBlock(b.getName() + "_Wall",  wallModelFrom(b.model));
    }
}

Block* BlockManager::getById(Uint16 id) {
    auto it = blocksById.find(id);
    return it != blocksById.end() ? it->second : nullptr;
}

Block* BlockManager::getByName(const std::string& name) {
    auto it = blocksByName.find(name);
    return it != blocksByName.end() ? it->second : nullptr;
}