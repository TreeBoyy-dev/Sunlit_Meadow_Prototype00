#include "Vectors.h"
#include "math.h"

const Vec3 WORLD_UP = { 0.0f, 0.0f, 1.0f };

Vec3 rightVector(Vec3 forward) {
    return vec3Normalize(vec3Cross(forward, WORLD_UP));
}

Vec3 vec3Add(Vec3 a, Vec3 b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3 vec3Scale(Vec3 v, float s) {
    return { v.x * s, v.y * s, v.z * s };
}

Vec3 vec3Negate(Vec3 v) {
    return { -v.x, -v.y, -v.z };
}

Vec3 vec3Sub(Vec3 a, Vec3 b){
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

float vec3Dot(Vec3 a, Vec3 b){
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3Cross(Vec3 a, Vec3 b){
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 vec3Normalize(Vec3 v){
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    if (len <= 0.000001f) {
        return { 0.0f, 0.0f, 0.0f };
    }

    return { v.x / len, v.y / len, v.z / len };
}