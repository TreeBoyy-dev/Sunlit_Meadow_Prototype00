#pragma once
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "Materials.h"
#include "WorldTypes.h"
#include "BakedMesh.h"
#include "StateLayout.h"
#include "BlockDefLoader.h"

// =====================================================================
//  BlockModel
//
//  Everything is BAKED at BlockManager::init() and only LOOKED UP at mesh
//  time — no math per block per rebuild beyond an array index.
//
//  Two model modes, mirroring Minecraft but simpler:
//   - variants:  ONE mesh chosen/transformed by state (cubes, stairs, logs,
//                slabs). One baked copy exists for every combination of the
//                template's state bits; the copy for a given cell is
//                variants[state & layout.modelMask()].
//   - multipart: mesh ASSEMBLED from conditional parts (fences, walls).
//                Each part is baked once per needed rotation; at mesh time
//                the matching parts are appended. The 2^n connection combos
//                are never individually baked — assembly is just appends.
// =====================================================================

class BlockModel {
protected:
    ModelFace topMaterial;
    ModelFace bottomMaterial;
    ModelFace sideMaterial;

    bool   multipart = false;
    Uint16 modelMask = 0;   // layout.modelMask() at bake time

    // variants mode: indexed directly by (state & modelMask)
    std::vector<BakedMesh> variants;

    // multipart mode: parts with a precompiled (mask, value) condition over
    // the state. conditionMask == 0 means "always emit".
    struct Part {
        BakedMesh mesh;
        Uint16 conditionMask = 0;
        Uint16 conditionValue = 0;
    };
    std::vector<Part> parts;

public:
    BlockModel(
        ModelFace topMaterial,
        ModelFace bottomMaterial,
        ModelFace sideMaterial
    );

    // Bakes all variants (or multipart parts) for the resolved block
    // definition. Parses each referenced .obj once, applies the state-driven
    // transforms (rotation / Z-flip / alternate shape geometry), re-picks
    // materials per baked copy, and converts to WorldVertex (placement
    // offset + overlay duplication).
    //
    // Must be called once, on the main thread, before any chunk meshing —
    // the mesh workers only ever read the baked data afterwards.
    bool bake(const BlockDef& def, const StateLayout& layout);

    // Appends the baked mesh for `state` into the given buffers, offset to
    // (x, y, z). The fluid bit (and any future non-model bits) are stripped
    // by the modelMask.
    // visMask (FaceDir bits) selects which boundary-face buckets
    // are emitted; hidden faces are simply never appended. Pass
    // FaceDir::AllVisible for the old emit-everything behavior.
    void getMesh(
        std::vector<WorldVertex>& outVertices,
        std::vector<Uint32>& outIndices,
        int x, int y, int z, Uint16 state, Uint8 visMask
    ) const;

    // coverMask of the variant selected by `state` (0 for
    // multipart models). Bit d set means this variant fully covers its own
    // boundary plane d and therefore hides the touching neighbor face.
    Uint8 getCoverMask(Uint16 state) const;

    ModelFace getTopMaterial();
    ModelFace getBottomMaterial();
    ModelFace getSideMaterial();
};
