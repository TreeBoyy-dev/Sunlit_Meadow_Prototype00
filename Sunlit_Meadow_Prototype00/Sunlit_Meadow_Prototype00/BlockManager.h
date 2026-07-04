#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include "Block.h"
#include "BlockModel.h"
#include "BlockDefLoader.h"

class BlockManager {
private:
    // Single owner — all Block objects live here
    std::vector<std::unique_ptr<Block>> blocks;

    // Two indexes — both point into the vector above, no copies
    std::unordered_map<Uint16,      Block*> blocksById;
    std::unordered_map<std::string, Block*> blocksByName;

    // Internal: turn one resolved JSON definition into a live Block
    // (build StateLayout -> construct BlockModel -> bake -> index).
    bool registerBlock(const BlockDef& def);

public:
    // Loads Assets/BlockTemplates/*.json + Assets/Blocks/*.json and bakes
    // every block's model variants. Blocks are no longer hardcoded here —
    // adding a block means adding a JSON file, not touching this class.
    void init();

    Block* getById(Uint16 id);
    Collision* getCollissionById(Uint16 id);
    Block* getByName(const std::string& name);

    int getNumberOfBlocks();
};
