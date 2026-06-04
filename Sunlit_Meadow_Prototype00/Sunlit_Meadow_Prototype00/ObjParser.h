#pragma once
#include <vector>
#include <string>

#include "DataStructures.h"
#include "EntityTypes.h"
#include "Vectors.h"

extern const Vec3 MODEL_ROTATION;

typedef struct {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    SDL_FColor color;
}ModelVertex;

bool obj_parse(
    std::string path,
    std::vector<ModelVertex>& outVertices,
    std::vector<Uint16>& outIndices
);

void rotate_model(
    std::vector<ModelVertex>& vertices,
    Vec3 eulerDegrees
);