#include "StoneBlocks.h"
#include "Materials.h"

Cobblestone_MinableBLock::Cobblestone_MinableBLock(
    BlockCoordinates position
)
    : MinableBLock(
        position,
        100.0f,
        PICKAXE,
        BLOCK_COBBLESTONE,
        BLOCK_COBBLESTONE,
        BLOCK_COBBLESTONE,
        BLOCK_COBBLESTONE,
        BLOCK_COBBLESTONE,
        BLOCK_COBBLESTONE
    )
{
}
Diorite_MinableBLock::Diorite_MinableBLock(
    BlockCoordinates position
)
    : MinableBLock(
        position,
        100.0f,
        PICKAXE,
        BLOCK_DIORITE,
        BLOCK_DIORITE,
        BLOCK_DIORITE,
        BLOCK_DIORITE,
        BLOCK_DIORITE,
        BLOCK_DIORITE
    )
{
}