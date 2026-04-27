#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include "Block.h"
#include "BlockModel.h"

class BlockManager {
private:
    // Single owner — all Block objects live here
    std::vector<std::unique_ptr<Block>> blocks;

    // Two indexes — both point into the vector above, no copies
    std::unordered_map<uint16_t,    Block*> blocksById;
    std::unordered_map<std::string, Block*> blocksByName;

    uint16_t nextId = 0;

    // Internal: register a block and auto-assign ID
    void registerBlock(
        const std::string& name,
        BlockModel model,
        bool transparent = false,
        bool hasSlab     = false,
        bool hasStair    = false,
        bool hasWall     = false
    );

public:
    void init();

    Block* getById(uint16_t id);
    Block* getByName(const std::string& name);
};