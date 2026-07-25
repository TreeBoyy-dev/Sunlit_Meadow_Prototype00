#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <vector>

#include "WorldTypes.h"

// ---------------------------------------------------------------------
//  Face direction convention shared by baking, meshing, and visibility.
//  Chosen so that opposite(d) is a single XOR.
// ---------------------------------------------------------------------
namespace FaceDir {
    constexpr int PosX = 0, NegX = 1;
    constexpr int PosY = 2, NegY = 3;
    constexpr int PosZ = 4, NegZ = 5;          // Z is up in this engine
    constexpr Uint8 AllVisible = 0x3F;
    inline constexpr int opposite(int d) { return d ^ 1; }
}

// One fully transformed, render-ready copy of a block model's mesh.
// Positions are in local block space (X/Y already at the +0.5 placement
// offset, Z in 0..1); overlay vertices/triangles are already appended.
// Baked once at BlockManager::init(), read-only afterwards — safe to share
// across the mesh worker threads.
//
// the geometry is SPLIT at bake time (classifyBakedFaces):
//   - vertices/indices ("always"): everything that does not lie on a cell
//     boundary plane — interior faces, slab mid-planes, cross plants, ...
//     Emitted unconditionally, exactly as before.
//   - boundaryFaces[d]: faces sitting on boundary plane d, facing outward
//     (incl. their 0.001-offset overlay copies). Emitted only when face d
//     of the cell is visible, i.e. NOT hidden by a covering neighbor.
//   - coverMask bit d: this mesh has a full 1x1 rect EXACTLY on boundary
//     plane d — the precise condition under which today's faceCulling
//     would cull the touching neighbor face. Detected with the same
//     quad-walk faceCulling uses, so build-time visibility reproduces its
//     decisions 1:1 (partial coverers set no bit; the residual faceCulling
//     pass still handles those rare pairs).
struct BakedMesh {
    std::vector<WorldVertex> vertices;   // "always" bucket
    std::vector<Uint16>      indices;

    struct FaceSet {
        std::vector<WorldVertex> vertices;
        std::vector<Uint16>      indices;
    };
    FaceSet boundaryFaces[6];            // indexed by FaceDir

    Uint8 coverMask = 0;                 // bit d: fully covers boundary plane d
};
