#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Materials.h"

class Block {
private:
    Uint16 id;
    std::string name;
    bool transperent;

    bool hasSlab;

    BlockModel model;

public:
    Block(id, name, model, transperent, hasSlab);

    bool getTransperency();
    void getModel(pointerToVertecies, pointertoIndecies, Texture);
};