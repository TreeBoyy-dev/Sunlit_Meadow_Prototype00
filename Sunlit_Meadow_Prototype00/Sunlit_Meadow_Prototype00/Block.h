#pragma once
#include <string>
#include <memory>
#include <array>

#include "ObjParser.h"
#include "Materials.h"
#include "BlockModel.h"
#include "StateLayout.h"
#include "WorldTypes.h"

class Block {
private:
    Uint16      id;
    std::string name;
    std::string modelFileName;   // variant-0 geometry; used for the item icon
    bool        transparent;

    StateLayout layout;          // this block's Uint16 state bit layout
    Collision   collision;

    std::unique_ptr<BlockModel> model;   // fully baked at init
    std::array<bool, 6> obstructs;

public:
    Block(
        Uint16 id,
        std::string name,
        std::string modelFileName,
        std::unique_ptr<BlockModel> model,
        StateLayout layout,
        Collision collision,
        std::array<bool, 6> obstructs,
        bool transparent = false
    );

    // Appends the baked mesh variant for `state` (fluid bit is masked off
    // inside BlockModel) at (x, y, z).
    void generateMeshFromModel(
        std::vector<WorldVertex>& vertices,
        std::vector<Uint32>&   indices,
        int x, int y, int z, Uint16 state
    );

    // Builds a render-ready inventory-icon mesh for this block.
    // Re-parses the block's .obj (so MODEL_ROTATION is baked in, exactly like
    // the regular items) and remaps each face's UVs into a horizontal 3-cell
    // atlas [ side | top | bottom ], choosing the cell per-face from the normal
    // (same +Z up / -Z down convention as the world meshes).
    // Uses the default-state (variant 0) geometry so icons stay correct.
    // Pair the result with buildBlockIconTexture(), which builds the matching
    // atlas from getSideMaterial()/getTopMaterial()/getBottomMaterial().
    // Returns false if the .obj could not be parsed.
    bool buildItemModel(
        std::vector<ModelVertex>& outVertices,
        std::vector<Uint16>& outIndices
    );

    bool isTransparent();
    Collision& getCollision() { return collision; }
    const StateLayout& getStateLayout() const { return layout; }
    std::string getName();
    Uint16 getID();
    const char* getModelFileName() { return modelFileName.c_str(); }

    ModelFace getTopMaterial();
    ModelFace getBottomMaterial();
    ModelFace getSideMaterial();
    bool getObstructs(int faceIndex);

    SDL_GPUTexture* getIcon();
};
