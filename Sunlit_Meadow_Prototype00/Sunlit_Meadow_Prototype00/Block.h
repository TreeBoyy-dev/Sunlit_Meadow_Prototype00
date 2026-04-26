#pragma once
#include <string>
#include "Materials.h"

class Block {
public:
    uint16_t    id;
    std::string name;
    bool        transparent;
    bool        hasSlab;
    bool        hasStair;
    bool        hasWall;
    BlockModel  model;

    Block(
        uint8_t id,
        std::string name,
        BlockModel model,
        bool transparent = false,
        bool hasSlab = false,
        bool hasStair = false,
        bool hasWall = false
    );
};