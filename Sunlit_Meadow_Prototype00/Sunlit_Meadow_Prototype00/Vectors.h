#pragma once

#include "math.h"

typedef struct {
    float x, y, z;
}Vec3;

typedef struct {
    float u, v;
}Vec2;

Vec3 vec3Sub(Vec3 a, Vec3 b);
float vec3Dot(Vec3 a, Vec3 b);
Vec3 vec3Cross(Vec3 a, Vec3 b);
Vec3 vec3Normalize(Vec3 v);