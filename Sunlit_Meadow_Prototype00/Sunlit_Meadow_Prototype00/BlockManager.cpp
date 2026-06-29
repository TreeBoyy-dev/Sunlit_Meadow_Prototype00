#include "BlockManager.h"
#include "BlockModel.h"
#include <stdexcept>
#include <vector>
#include <memory>

void BlockManager::registerBlock(
    const std::string& name,
    std::unique_ptr<BlockModel> model,
    Collision collision,
    std::array<bool, 6> obstructs,
    bool transparent,
    bool hasSlab,
    bool hasStair,
    bool hasWall,
    const char* modelFileName
){
    Uint16 id = nextId++;

    auto newBlock = std::make_unique<Block>(
        id,
        name,
        modelFileName,
        std::move(model),
        collision,
        obstructs,
        transparent,
        hasSlab,
        hasStair,
        hasWall
    );
    Block* ptr = newBlock.get();

    blocksById[id] = ptr;
    blocksByName[name] = ptr;

    blocks.push_back(std::move(newBlock));
}

Collision fullBlockcollision = {
    true,
    0,
    { AABB{ {0,0,0}, {1,1,1} } }
};

void BlockManager::init() {
    // --- base blocks ---
    registerBlock("air",            std::make_unique<BlockModel>(MATERIAL_AIR),
        { false, 0 },
        { false, false, false, false, false, false },
        true, false, false, false
    );
    registerBlock("cobble_stone",   std::make_unique<BlockModel>(MATERIAL_COBBLESTONE),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true, false
    );
    registerBlock("gneiss",         std::make_unique<BlockModel>(MATERIAL_GNEISS),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true, false
    );
    registerBlock("chalk",          std::make_unique<BlockModel>(MATERIAL_CHALK),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true, false
    );
    registerBlock("marble",         std::make_unique<BlockModel>(MATERIAL_MARBLE),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true, false
    );
    registerBlock("diorite",        std::make_unique<BlockModel>(MATERIAL_DIORITE),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true, true
    );
    // --- dirt/grass related ---
    registerBlock("dirt",           std::make_unique<BlockModel>(MATERIAL_DIRT),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true
    );
    registerBlock("grass_block",    std::make_unique<BlockModel>(MATERIAL_GRASS_BLOCK, MATERIAL_DIRT, MATERIAL_DIRT),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true
    );
    // --- wood related ---
    registerBlock("birch_log",      std::make_unique<BlockModel>(MATERIAL_BIRCH_LOG_TOP, MATERIAL_BIRCH_LOG_SIDE),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true
    );
    registerBlock("birch_leaves",   std::make_unique<BlockModel>(MATERIAL_BIRCH_LEAVES),
        fullBlockcollision,
        { false, false, false, false, false, false },
        false, true, true
    );

    registerBlock("chestnut_log",   std::make_unique<BlockModel>(MATERIAL_CHESTNUT_LOG_TOP, MATERIAL_CHESTNUT_LOG_SIDE),
        fullBlockcollision,
        { true, true, true, true, true, true },
        false, true, true
    );
    registerBlock("chestnut_leaves",std::make_unique<BlockModel>(MATERIAL_CHESTNUT_LEAVES),
        fullBlockcollision,
        { false, false, false, false, false, false },
        false, true, true
    );


    size_t baseCount = blocks.size();
    for (size_t i = 0; i < baseCount; i++) {
        Block* b = blocks[i].get();
        
        if (b->getHasSlab()) {
            Collision slabCol = b->getCollision();
            slabCol.boxes = { AABB{ {0,0,0}, {1,1,0.5f} } };
            registerBlock(b->getName() + "_slab",
                std::make_unique<BlockModel>(b->getTopMaterial(), b->getBottomMaterial(), b->getSideMaterial()),
                slabCol,
                { false, false, false, false, false, true },
                false, false, false, false,
                "slab.obj"
            );
        }

        //if (b.getHasStair()) registerBlock(b.getName() + "_stair", stairModelFrom(b.model));
        //if (b.getHasWall())  registerBlock(b.getName() + "_Wall",  wallModelFrom(b.model));
    }

    registerBlock("flower_alpine_quill", std::make_unique<BlockModel>(MATERIAL_BIRCH_LEAVES),
        { false , 0 },
        { false, false, false, false, false, false },
        false, false, false, false,
        "flower_alpine_quill.obj"
    );
}

Block* BlockManager::getById(Uint16 id) {
    auto it = blocksById.find(id);
    return it != blocksById.end() ? it->second : nullptr;
}

Collision* BlockManager::getCollissionById(Uint16 id) {
    Block* b = getById(id);
    if (!b) return nullptr;
    return &b->getCollision();   // address of a member of a Block that lives for the program
}

Block* BlockManager::getByName(const std::string& name) {
    auto it = blocksByName.find(name);
    return it != blocksByName.end() ? it->second : nullptr;
}

int BlockManager::getNumberOfBlocks() {
    return blocks.size();
}