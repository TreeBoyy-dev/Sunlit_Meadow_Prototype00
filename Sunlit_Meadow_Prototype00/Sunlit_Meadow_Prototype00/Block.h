#pragma once
#include <string>
#include <memory>
#include <array>

#include "ObjParser.h"
#include "Materials.h"
#include "BlockModel.h"
#include "WorldTypes.h"
struct AABB {
    Vec3 min;   // local block space, 0..1 per axis, Z up
    Vec3 max;
};
struct Collision {
    bool solid = true;       // true  -> blocks movement
    int  slowdown = 0;       // % of speed removed per tick when !solid.
                             //0 = no slowing, 100 = frozen.
    std::vector<AABB> boxes; // local collision boxes. EMPTY = full unit cube [0,0,0]..[1,1,1].
                             // slab (bottom half) = {{0,0,0},{1,1,0.5}}
};

class Block {
private:
    Uint16    id;
    std::string name;
    const char* modelFileName;
    bool        transparent;
    bool        hasSlab, hasStair, hasPillar, hasWall, hasFence; //hasStep, hasCorner??

    Collision collision;

    std::unique_ptr<BlockModel> model;
    std::array<bool, 6> obstructs;

    bool modelInit;
public:
    Block(
        Uint16 id,
        std::string name,
        const char* modelFileName,
        std::unique_ptr<BlockModel> model,
        Collision collision,
        std::array<bool, 6> obstructs,
        bool transparent = false,
        bool hasSlab = false,
        bool hasStair = false,
        bool hasWall = false
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
    Collision& getCollision() { return collision; }
    std::string getName();
    Uint16 getID();
    const char* getModelFileName() { return modelFileName; }

    Material getTopMaterial();
    Material getBottomMaterial();
    Material getSideMaterial();
    bool getObstructs(int faceIndex);

    SDL_GPUTexture* getIcon();
};