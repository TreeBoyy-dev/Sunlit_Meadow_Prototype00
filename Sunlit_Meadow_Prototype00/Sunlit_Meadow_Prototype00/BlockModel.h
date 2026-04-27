#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "Materials.h"

// Adjacency order: Top, Bottom, Front(+Z), Back(-Z), Right(+X), Left(-X)
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
        std::vector<uint16_t>&   indices,
        const Vec3               corners[4],  // 4 corners of the quad, CCW
        Material                 materialIndex
    );

public:
    BlockModel(Material topMaterial, Material bottomMaterial, Material sideMaterial);

    void generateMesh(
        std::vector<VertexData>& vertices,
        std::vector<uint16_t>&   indices,
        AdjacencyInfo            adj
    );
};