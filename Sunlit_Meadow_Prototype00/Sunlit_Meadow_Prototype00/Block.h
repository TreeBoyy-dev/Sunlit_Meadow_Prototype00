#pragma once
#include <string>
#include <memory>
#include "Materials.h"
#include "BlockModel.h"
#include "WorldTypes.h"
#include <array>

class Block {
private:
    Uint16    id;
    std::string name;
    bool        transparent;
    //multiplier for downward acceleration: 1 = no colission, 0.0 = full collision
    float       collision;
    bool        hasSlab, hasStair, hasPillar, hasWall, hasFence; //hasStep, hasCorner??

    std::unique_ptr<BlockModel> model;

    //obstructs visible surface at all sides:
    //front, back, right, left, up, down
    std::array<bool, 6> obstructs;

public:
    Block(
        Uint16 id,
        std::string name,
        std::unique_ptr<BlockModel> model,
        std::array<bool, 6> obstructs,
        bool transparent = false,
        bool hasSlab = false,
        bool hasStair = false,
        bool hasWall = false,
        float collision = 0.0f
    );

    void generateMeshFromModel(
        std::vector<WorldVertex>& vertices,
        std::vector<Uint16>&   indices,
        AdjacencyInfo            adj,
        int x, int y, int z
    );

    bool isTransparent();
    bool getHasSlab();
    bool getHasStair();
    bool getHasWall();
    float getCollision();
    std::string getName();
    Uint16 getID();

    Material getTopMaterial();
    Material getBottomMaterial();
    Material getSideMaterial();
    bool getObstructs(int faceIndex);
};