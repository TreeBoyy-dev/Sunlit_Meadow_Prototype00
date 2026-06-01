#pragma once
#include <vector>
#include <string>

#include "DataStructures.h"
#include "EntityTypes.h"
#include "Vectors.h"

extern const Vec3 MODEL_ROTATION;

bool obj_parse(
    std::string path,
    std::vector<EntityVertex>& outVertices,
    std::vector<Uint16>& outIndices
);

void rotate_model(
    std::vector<EntityVertex>& vertices,
    Vec3 eulerDegrees
);