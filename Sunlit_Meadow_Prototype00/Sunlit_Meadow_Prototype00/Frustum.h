#pragma once

#include "Vectors.h"
#include "DataStructures.h"

struct Frustum {
    Vec3 planes[6];   // normal (pointing inward)
    float dists[6];   // dot(normal, point_on_plane)
};

Frustum buildFrustum(Camera& cam, float fovY, float aspect, float zNear, float zFar) {
    Vec3 f = vec3Normalize(cam.forward);
    Vec3 r = vec3Normalize(vec3Cross({ 0, 0, 1 }, f));
    Vec3 u = vec3Cross(f, r);

    float halfVtan = tanf(fovY * 0.5f);
    float halfHtan = halfVtan * aspect;

    Frustum out;

    out.planes[0] = f;
    out.dists[0] = vec3Dot(f, cam.position) + zNear;

    out.planes[1] = vec3Negate(f);
    out.dists[1] = vec3Dot(vec3Negate(f), cam.position) - zFar;

    Vec3 rNear = vec3Add(f, vec3Scale(r, halfHtan));
    Vec3 lNear = vec3Add(f, vec3Scale(r, -halfHtan));
    Vec3 tNear = vec3Add(f, vec3Scale(u, halfVtan));
    Vec3 bNear = vec3Add(f, vec3Scale(u, -halfVtan));

    out.planes[2] = vec3Normalize(vec3Cross(rNear, u));     // right
    out.planes[3] = vec3Normalize(vec3Cross(u, lNear));     // left
    out.planes[4] = vec3Normalize(vec3Cross(rNear, tNear)); // top
    out.planes[5] = vec3Normalize(vec3Cross(lNear, bNear)); // bottom

    for (int i = 2; i < 6; i++)
        out.dists[i] = vec3Dot(out.planes[i], cam.position);

    return out;
}

bool aabbInsideFrustum(const Frustum& f, Vec3 chunkMin, Vec3 chunkMax) {
    for (int i = 0; i < 6; i++) {
        const Vec3& n = f.planes[i];
        // Positive vertex: pick the corner furthest along n
        Vec3 pv = {
            n.x >= 0 ? chunkMax.x : chunkMin.x,
            n.y >= 0 ? chunkMax.y : chunkMin.y,
            n.z >= 0 ? chunkMax.z : chunkMin.z
        };
        if (vec3Dot(n, pv) < f.dists[i])
            return false;   // fully outside this plane → cull
    }
    return true;
}