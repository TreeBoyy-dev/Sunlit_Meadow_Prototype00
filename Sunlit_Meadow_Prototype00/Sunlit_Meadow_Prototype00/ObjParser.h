#pragma once
#include <vector>
#include <string>

#include "DataStructures.h"
#include "EntityTypes.h"

bool obj_parse(
    std::string path,
    std::vector<EntityVertex>& outVertices,
    std::vector<Uint16>& outIndices
);