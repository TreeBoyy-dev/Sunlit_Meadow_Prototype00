#include "MinableBLock.h"

MinableBLock::MinableBLock(
    BlockCoordinates position,
    float health,
    BlockDamageType weakness,
    Material materialUP,
    Material materialDOWN,
    Material materialNORTH,
    Material materialEAST,
    Material materialSOUTH,
    Material materialWEST
)
    : Block(
        position,
        materialUP,
        materialDOWN,
        materialNORTH,
        materialEAST,
        materialSOUTH,
        materialWEST
    ),
    health(health),
    weakness(weakness)
{
}