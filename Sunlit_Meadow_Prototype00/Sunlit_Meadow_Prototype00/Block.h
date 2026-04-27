#pragma once
#include <string>
#include "Materials.h"

class Block {
private:
    uint16_t    id;
    std::string name;
    bool        transparent;
    bool        hasSlab;
    bool        hasStair;
    bool        hasWall;
    BlockModel  model;

public:
    Block(
        uint8_t id,
        std::string name,
        BlockModel model,
        bool transparent = false,
        bool hasSlab = false,
        bool hasStair = false,
        bool hasWall = false
    );

    bool isTransparent();
    bool getHasSlab();
    bool getHasStair();
    bool getHasWall();
    std::string getName();
};