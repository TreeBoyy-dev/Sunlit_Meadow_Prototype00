#pragma once
#include <string>
#include <memory>
#include <array>

#include "ObjParser.h"
#include "Materials.h"
#include "BlockModel.h"
#include "WorldTypes.h"

class Block {
private:
    Uint16    id;
    std::string name;
    const char* modelFileName;
    bool        transparent;
    //multiplier for downward acceleration: 1 = no colission, 0.0 = full collision
    float       collision;
    bool        hasSlab, hasStair, hasPillar, hasWall, hasFence; //hasStep, hasCorner??

    std::unique_ptr<BlockModel> model;

    //obstructs visible surface at all sides:
    //front, back, right, left, up, down
    std::array<bool, 6> obstructs;

    bool modelInit;
public:
    Block(
        Uint16 id,
        std::string name,
        const char* modelFileName,
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
        std::vector<Uint32>&   indices,
        int x, int y, int z
    );

    // Builds a render-ready inventory-icon mesh for this block.
    // Re-parses the block's .obj (so MODEL_ROTATION is baked in, exactly like
    // the regular items) and remaps each face's UVs into a horizontal 3-cell
    // atlas [ side | top | bottom ], choosing the cell per-face from the normal
    // (same +Z up / -Z down convention as BlockModel::materialForNormal).
    // Pair the result with buildBlockIconTexture(), which builds the matching
    // atlas from getSideMaterial()/getTopMaterial()/getBottomMaterial().
    // Returns false if the .obj could not be parsed.
    bool buildItemModel(
        std::vector<ModelVertex>& outVertices,
        std::vector<Uint16>& outIndices
    );

    bool isTransparent();
    bool getHasSlab();
    bool getHasStair();
    bool getHasWall();
    float getCollision();
    std::string getName();
    Uint16 getID();
    const char* getModelFileName() { return modelFileName; }

    Material getTopMaterial();
    Material getBottomMaterial();
    Material getSideMaterial();
    bool getObstructs(int faceIndex);

    SDL_GPUTexture* getIcon();
};