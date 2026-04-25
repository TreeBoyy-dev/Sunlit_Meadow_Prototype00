#include "Block.h"

Block::Block(){
    transperent = false;
    isAir = true;
}

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
    transperent = false;
    isAir = false;
}

BlockCoordinates Block::getPosition()
{
    return position;
}
bool Block::getTransperency()
{
    return transperent;
}
bool Block::getIsAir()
{
    return isAir;
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