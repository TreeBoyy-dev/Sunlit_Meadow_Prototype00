#pragma once
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "Materials.h"
#include "WorldTypes.h"

class BlockModel {
protected:
    Material topMaterial;
    Material bottomMaterial;
    Material sideMaterial;

    // Mesh loaded from the .obj, already converted to WorldVertex.
    // Positions are in local block space (origin at 0,0,0).
    std::vector<WorldVertex> vertices;
    std::vector<Uint16>      indices;

    // Picks a material for a vertex from its normal direction.
    // Z is up: +Z -> top, -Z -> bottom, everything else -> side.
    Material materialForNormal(const Vec3& normal) const;

public:
    BlockModel(
        Material topMaterial,
        Material bottomMaterial,
        Material sideMaterial
    );
    BlockModel(
        Material topBottomMaterial,
        Material sideMaterial
    );
    BlockModel(
        Material sideMaterial
    );

    // Loads the mesh from a .obj file and converts ModelVertex -> WorldVertex.
    // Returns false if the file could not be parsed.
    bool init(
        const char* fileName
    );

    // Appends this model's mesh into the given buffers, offset to (x, y, z).
    // No longer takes adjacency info: the whole obj mesh is always emitted.
    virtual void getMesh(
        std::vector<WorldVertex>& outVertices,
        std::vector<Uint16>& outIndices,
        int x, int y, int z
    );

    Material getTopMaterial();
    Material getBottomMaterial();
    Material getSideMaterial();
};