#pragma once

#include "math.h"

typedef struct {
    float x, y, z;
}Vec3;

typedef struct {
    float x, y;
}Vec2;

extern const Vec3 WORLD_UP;

Vec3 rightVector(Vec3 forward);

Vec3 vec3Add(Vec3 a, Vec3 b);
Vec3 vec3Scale(Vec3 v, float s);
Vec3 vec3Negate(Vec3 v);
Vec3 vec3rotate(Vec3 v,
    float sx, float cx,
    float sy, float cy,
    float sz, float cz
);

Vec3 vec3Sub(Vec3 a, Vec3 b);
float vec3Dot(Vec3 a, Vec3 b);
Vec3 vec3Cross(Vec3 a, Vec3 b);
Vec3 vec3Normalize(Vec3 v);