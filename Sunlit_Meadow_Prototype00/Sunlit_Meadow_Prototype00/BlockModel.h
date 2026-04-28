#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "Materials.h"

// Adjacency order: Top(+Z), Bottom(-Z), Front(+X), Back(-X), Right(+Y), Left(-Y)
struct AdjacencyInfo {
    bool top, bottom, front, back, right, left;
};

class BlockModel {
private:
    Material topMaterial;
    Material bottomMaterial;
    Material sideMaterial;

    void addFace(
        std::vector<VertexData>& vertices,
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

    void generateMesh(
        std::vector<VertexData>& vertices,
        std::vector<Uint16>&   indices,
        AdjacencyInfo            adj,
        int x, int y, int z
    );
};