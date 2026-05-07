#pragma once

#include "BlockModel.h"

class SlabBlockModel : public BlockModel {

public:
    SlabBlockModel(
        Material topMaterial,
        Material bottomMaterial,
        Material sideMaterial
    );
    SlabBlockModel(
        Material topBottomMaterial,
        Material sideMaterial
    );
    SlabBlockModel(
        Material sideMaterial
    );

    void generateMesh(
        std::vector<Vertex>& vertices,
        std::vector<Uint16>& indices,
        AdjacencyInfo            adj,
        int x, int y, int z
    ) override;
};