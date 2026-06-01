#include "BlockManager.h"
#include "BlockModel.h"
#include "SlabBlockModel.h"
#include <stdexcept>
#include <vector>
#include <memory>

void BlockManager::registerBlock(
    const std::string& name,
    std::unique_ptr<BlockModel> model,
    std::array<bool, 6> obstructs,
    bool transparent,
    bool hasSlab,
    bool hasStair,
    bool hasWall,
    float collision
){
    Uint16 id = nextId++;

    auto newBlock = std::make_unique<Block>(
        id,
        name,
        std::move(model),
        obstructs,
        transparent,
        hasSlab,
        hasStair,
        hasWall,
        collision
    );
    Block* ptr = newBlock.get();

    blocksById[id] = ptr;
    blocksByName[name] = ptr;

    blocks.push_back(std::move(newBlock));
}

void BlockManager::init() {
    // --- Register base blocks ---
    registerBlock("air", std::make_unique<BlockModel>(MATERIAL_COBBLESTONE), { false, false, false, false, false, false }, /*transparent=*/true, false, false, false, 1.0);
    registerBlock("cobble_stone",   std::make_unique<BlockModel>(MATERIAL_COBBLESTONE), { true, true, true, true, true, true }, false, true, true, false);
    registerBlock("gneiss",         std::make_unique<BlockModel>(MATERIAL_GNEISS), { true, true, true, true, true, true }, false, true, true, false);
    registerBlock("chalk",          std::make_unique<BlockModel>(MATERIAL_CHALK), { true, true, true, true, true, true }, false, true, true, false);
    registerBlock("marble",         std::make_unique<BlockModel>(MATERIAL_MARBLE), { true, true, true, true, true, true }, false, true, true, false);
    registerBlock("diorite",        std::make_unique<BlockModel>(MATERIAL_DIORITE), { true, true, true, true, true, true }, false, true, true, true);
    
    registerBlock("dirt",           std::make_unique<BlockModel>(MATERIAL_DIRT), { true, true, true, true, true, true }, false, true, true);
    registerBlock("grass_block",    std::make_unique<BlockModel>(MATERIAL_GRASS_BLOCK, MATERIAL_DIRT, MATERIAL_DIRT), { true, true, true, true, true, true }, false, true, true);

    registerBlock("birch_log",      std::make_unique<BlockModel>(MATERIAL_BIRCH_LOG_TOP, MATERIAL_BIRCH_LOG_SIDE), { true, true, true, true, true, true }, false, true, true);
    registerBlock("birch_leaves",   std::make_unique<BlockModel>(MATERIAL_BIRCH_LEAVES), { false, false, false, false, false, false }, false, true, true);

    registerBlock("chestnut_log",   std::make_unique<BlockModel>(MATERIAL_CHESTNUT_LOG_TOP, MATERIAL_CHESTNUT_LOG_SIDE), { true, true, true, true, true, true }, false, true, true);
    registerBlock("chestnut_leaves",std::make_unique<BlockModel>(MATERIAL_CHESTNUT_LEAVES), { false, false, false, false, false, false }, false, true, true);


    size_t baseCount = blocks.size();
    for (size_t i = 0; i < baseCount; i++) {
        Block* b = blocks[i].get();
        
        if (b->getHasSlab())  registerBlock(b->getName() + "_slab", std::make_unique<SlabBlockModel>(b->getTopMaterial(), b->getBottomMaterial(), b->getSideMaterial()), { false, false, false, false, false, true });
        //if (b.getHasStair()) registerBlock(b.getName() + "_stair", stairModelFrom(b.model));
        //if (b.getHasWall())  registerBlock(b.getName() + "_Wall",  wallModelFrom(b.model));
    }
}

Block* BlockManager::getById(Uint16 id) {
    auto it = blocksById.find(id);
    return it != blocksById.end() ? it->second : nullptr;
}

float BlockManager::getCollissionById(Uint16 id) {
    auto it = blocksById.find(id);
    if (it == blocksById.end())
        return -1;
    else
    {
        Block* block = it->second;
        return block->getCollision();
    }
}

Block* BlockManager::getByName(const std::string& name) {
    auto it = blocksByName.find(name);
    return it != blocksByName.end() ? it->second : nullptr;
}