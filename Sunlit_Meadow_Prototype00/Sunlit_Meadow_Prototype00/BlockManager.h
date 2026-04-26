#pragma once
#include <unordered_map>

#include "Materials.h"
#include "Block.h"

class BlockManager {
private:
	std::unordered_map<> blocksById;
	std::unordered_map<> blocksByName;

public:
	void initBlockManager();

	Block getBlockById(Uint16 id);
	Block getBlockByName(std::string name);

};