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
    std::unordered_map<Uint16,      Block*> blocksById;
    std::unordered_map<std::string, Block*> blocksByName;

    Uint16 nextId = 0;

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

    Block* getById(Uint16 id);
    Block* getByName(const std::string& name);
};