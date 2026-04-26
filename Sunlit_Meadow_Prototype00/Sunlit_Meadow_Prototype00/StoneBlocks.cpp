#include "StoneBlocks.h"
#include "Materials.h"

Cobblestone_MinableBlock::Cobblestone_MinableBlock(
    BlockCoordinates position
)
    : MinableBlock(
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
Diorite_MinableBlock::Diorite_MinableBlock(
    BlockCoordinates position
)
    : MinableBlock(
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
Dirt_MinableBlock::Dirt_MinableBlock(
    BlockCoordinates position
)
    : MinableBlock(
        position,
        100.0f,
        PICKAXE,
        BLOCK_DIRT,
        BLOCK_DIRT,
        BLOCK_DIRT,
        BLOCK_DIRT,
        BLOCK_DIRT,
        BLOCK_DIRT
    )
{
}