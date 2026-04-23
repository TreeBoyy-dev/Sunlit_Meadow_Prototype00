#pragma once

#include "Block.h"

typedef enum {
	PICKAXE,
	AXE,
	SHOVEL,
	EXPLOSION,
}BlockDamageType;

class MinableBLock : public Block {
private:
	float health;
	BlockDamageType weakness;
public:
    MinableBLock(
        BlockCoordinates position,
        float health,
        BlockDamageType weakness,
        Material materialUP,
        Material materialDOWN,
        Material materialNORTH,
        Material materialEAST,
        Material materialSOUTH,
        Material materialWEST
    );
};