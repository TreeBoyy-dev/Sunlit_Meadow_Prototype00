#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "Materials.h"
#include "WorldTypes.h"

class EntityModel {
protected:
    Material material;

public:
    EntityModel(Material material);
    virtual ~EntityModel() = default;

    // Fills local-space geometry for this entity. Override per model type.
    virtual void generateMesh(
        std::vector<WorldVertex>& vertices,
        std::vector<Uint16>& indices
    );

    Material getMaterial();
};
