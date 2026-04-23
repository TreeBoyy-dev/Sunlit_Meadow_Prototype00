#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Materials.h"

struct BlockCoordinates {
	int x, y, z;
    bool operator==(const BlockCoordinates& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

class Block {
private:
    BlockCoordinates position;

    Material materialUP;
    Material materialDOWN;
    Material materialNORTH;
    Material materialEAST;
    Material materialSOUTH;
    Material materialWEST;

public:
    Block(
        BlockCoordinates position,
        Material materialUP,
        Material materialDOWN,
        Material materialNORTH,
        Material materialEAST,
        Material materialSOUTH,
        Material materialWEST
    );

    BlockCoordinates getPosition();

    Material getMaterialUP();
    Material getMaterialDOWN();
    Material getMaterialNORTH();
    Material getMaterialEAST();
    Material getMaterialSOUTH();
    Material getMaterialWEST();
};