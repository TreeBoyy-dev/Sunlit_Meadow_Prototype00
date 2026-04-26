#pragma once

#include "MinableBlock.h"

class Cobblestone_MinableBlock : public MinableBlock {
public:
	Cobblestone_MinableBlock(BlockCoordinates position);
};

class Diorite_MinableBlock : public MinableBlock {
public:
	Diorite_MinableBlock(BlockCoordinates position);
};

class Dirt_MinableBlock : public MinableBlock {
public:
	Dirt_MinableBlock(BlockCoordinates position);
};
