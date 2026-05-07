#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "Materials.h"

// Adjacency order: Top(+Z), Bottom(-Z), Front(+X), Back(-X), Right(+Y), Left(-Y)
struct AdjacencyInfo {
    bool front, back, right, left, top, bottom;
};

class BlockModel {
protected:
    Material topMaterial;
    Material bottomMaterial;
    Material sideMaterial;

    void addFace(
        std::vector<Vertex>& vertices,
        std::vector<Uint16>&   indices,
        const Vec3               corners[4],  // 4 corners of the quad, CCW
        Material                 materialIndex
    );

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

    virtual void generateMesh(
        std::vector<Vertex>& vertices,
        std::vector<Uint16>&   indices,
        AdjacencyInfo            adj,
        int x, int y, int z
    );

    Material getTopMaterial();
    Material getBottomMaterial();
    Material getSideMaterial();
};