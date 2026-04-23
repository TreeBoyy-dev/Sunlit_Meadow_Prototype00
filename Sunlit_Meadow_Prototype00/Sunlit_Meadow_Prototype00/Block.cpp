#include "Block.h"

Block::Block(
    BlockCoordinates position,
    Material materialUP,
    Material materialDOWN,
    Material materialNORTH,
    Material materialEAST,
    Material materialSOUTH,
    Material materialWEST
)
    : position(position),
    materialUP(materialUP),
    materialDOWN(materialDOWN),
    materialNORTH(materialNORTH),
    materialEAST(materialEAST),
    materialSOUTH(materialSOUTH),
    materialWEST(materialWEST)
{
}

BlockCoordinates Block::getPosition()
{
    return position;
}

Material Block::getMaterialUP()
{
    return materialUP;
}

Material Block::getMaterialDOWN()
{
    return materialDOWN;
}

Material Block::getMaterialNORTH()
{
    return materialNORTH;
}

Material Block::getMaterialEAST()
{
    return materialEAST;
}

Material Block::getMaterialSOUTH()
{
    return materialSOUTH;
}

Material Block::getMaterialWEST()
{
    return materialWEST;
}