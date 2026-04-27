#pragma once
#include <string>
#include "Materials.h"
#include "BlockModel.h"
#include "DataStructures.h"

class Block {
private:
    Uint16    id;
    std::string name;
    bool        transparent;
    bool        hasSlab;
    bool        hasStair;
    bool        hasWall;
    BlockModel  model;

public:
    Block(
        Uint16 id,
        std::string name,
        BlockModel model,
        bool transparent = false,
        bool hasSlab = false,
        bool hasStair = false,
        bool hasWall = false
    );

    void generateMeshFromModel(
        std::vector<VertexData>& vertices,
        std::vector<Uint16>&   indices,
        AdjacencyInfo            adj,
        int x, int y, int z
    );

    bool isTransparent();
    bool getHasSlab();
    bool getHasStair();
    bool getHasWall();
    std::string getName();
    Uint16 getID();
};