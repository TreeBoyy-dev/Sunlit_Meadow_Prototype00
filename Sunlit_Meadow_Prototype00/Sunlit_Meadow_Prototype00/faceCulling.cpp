#include "ChunkMesh.h"

#include <cmath>
#include <algorithm>
#include <unordered_map>

// One reconstructed axis-aligned rectangular face (a quad = 2 triangles).
struct Quad {
    int    axis;        // 0=X, 1=Y, 2=Z : the axis the normal lies on
    int    sign;        // +1 / -1 : which way the face points
    float  plane;       // coordinate along `axis` the face sits on
    float  uMin, uMax;  // extent on the first in-plane axis
    float  vMin, vMax;  // extent on the second in-plane axis
    Uint32 idx[6];      // original indices, so we can re-emit if it survives
    bool   removed;
};

constexpr float kEps = 1e-4f;

// For a normal pointing along `axis`, pick the two in-plane axes (u, v).
inline void planeAxes(int axis, int& u, int& v) {
    switch (axis) {
    case 0: u = 1; v = 2; break; // X-face spans Y,Z
    case 1: u = 0; v = 2; break; // Y-face spans X,Z
    default: u = 0; v = 1; break; // Z-face spans X,Y
    }
}

inline float comp(const Vec3& p, int axis) {
    return axis == 0 ? p.x : (axis == 1 ? p.y : p.z);
}

// Try to read indices [start, start+6) as one axis-aligned rectangle.
// Returns false if it isn't a clean quad; caller then leaves it untouched.
bool tryMakeQuad(const std::vector<WorldVertex>& verts,
    const std::vector<Uint32>& indices,
    size_t start, Quad& out)
{
    if (start + 6 > indices.size())
        return false;

    const Vec3& n = verts[indices[start]].normal;
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    int axis = (ax >= ay && ax >= az) ? 0 : (ay >= az ? 1 : 2);
    float nc = comp(n, axis);
    if (std::fabs(nc) < 0.5f)
        return false;                 // normal not axis-aligned -> skip
    int sign = (nc > 0.0f) ? 1 : -1;

    int u, v;
    planeAxes(axis, u, v);

    float plane = comp(verts[indices[start]].position, axis);
    float uMin = 1e30f, uMax = -1e30f, vMin = 1e30f, vMax = -1e30f;

    for (int k = 0; k < 6; ++k) {
        const WorldVertex& w = verts[indices[start + k]];
        if (std::fabs(comp(w.position, axis) - plane) > kEps)
            return false;             // not all on the same plane
        float wn = comp(w.normal, axis);
        if (std::fabs(wn) < 0.5f || ((wn > 0.0f) ? 1 : -1) != sign)
            return false;             // mixed normals -> not one flat quad
        float pu = comp(w.position, u);
        float pv = comp(w.position, v);
        uMin = std::min(uMin, pu); uMax = std::max(uMax, pu);
        vMin = std::min(vMin, pv); vMax = std::max(vMax, pv);
    }
    if (uMax - uMin < kEps || vMax - vMin < kEps)
        return false;                 // degenerate

    out.axis = axis; out.sign = sign; out.plane = plane;
    out.uMin = uMin; out.uMax = uMax; out.vMin = vMin; out.vMax = vMax;
    for (int k = 0; k < 6; ++k) out.idx[k] = indices[start + k];
    out.removed = false;
    return true;
}

// True if rect A fully covers rect B (B sits inside A).
inline bool contains(const Quad& a, const Quad& b) {
    return a.uMin <= b.uMin + kEps && a.uMax >= b.uMax - kEps &&
        a.vMin <= b.vMin + kEps && a.vMax >= b.vMax - kEps;
}

void ChunkMesh::faceCulling()
{
    if (indices.empty()) {
        //SDL_Log("[ChunkMesh] optimizeMesh: indices empty");
        return;
    }
    //else
    //    SDL_Log("[ChunkMesh] optimizeMesh: indices NOT empty");


    int initialindizes = indices.size();

    // 1. Reconstruct quads from the triangle list. Anything we can't read as a
    //    clean axis-aligned rectangle is kept untouched (passthrough tris).
    std::vector<Quad>   quads;
    std::vector<Uint32> passthrough;
    quads.reserve(indices.size() / 6 + 1);

    size_t i = 0;
    while (i < indices.size()) {
        Quad q;
        if (tryMakeQuad(vertices, indices, i, q)) {
            quads.push_back(q);
            i += 6;
        }
        else {
            for (int k = 0; k < 3 && i + k < indices.size(); ++k)
                passthrough.push_back(indices[i + k]);
            i += 3;
        }
    }

    // 2. Bucket quads by (axis, plane) so we only ever compare faces that could
    //    actually line up. The key packs axis + a quantized plane coordinate.
    auto keyOf = [](const Quad& q) -> uint64_t {
        uint64_t p = (uint64_t)(int64_t)(std::llround(q.plane * 1000.0f) + (1LL << 40));
        return ((uint64_t)q.axis << 56) | (p & 0xFFFFFFFFFFFFULL);
        };
    std::unordered_map<uint64_t, std::vector<int>> buckets;
    for (int qi = 0; qi < (int)quads.size(); ++qi)
        buckets[keyOf(quads[qi])].push_back(qi);

    // 3. A quad dies if an opposing-facing quad on the same plane fully covers
    //    it. Equal faces cover each other -> both go. A smaller face inside a
    //    bigger one -> only the smaller goes (the big one still shows the rest,
    //    e.g. a slab against a full block).
    int removedCount = 0;
    for (auto& kv : buckets) {
        std::vector<int>& group = kv.second;
        for (int a : group) {
            Quad& qa = quads[a];
            for (int b : group) {
                if (a == b) continue;
                Quad& qb = quads[b];
                if (qb.axis != qa.axis || qb.sign == qa.sign) continue;
                if (std::fabs(qb.plane - qa.plane) > kEps) continue;
                if (contains(qb, qa)) {
                    if (!qa.removed) { qa.removed = true; ++removedCount; }
                    break;
                }
            }
        }
    }

    if (removedCount == 0) {
        SDL_Log("[ChunkMesh] optimizeMesh: no quads removed");
        return;
    }

    // 4. Rebuild indices from surviving quads + the passthrough triangles.
    std::vector<Uint32> newIndices;
    newIndices.reserve(indices.size());
    for (const Quad& q : quads)
        if (!q.removed)
            for (int k = 0; k < 6; ++k)
                newIndices.push_back(q.idx[k]);
    for (Uint32 idx : passthrough)
        newIndices.push_back(idx);

    // 5. Compact the vertex buffer so we don't ship verts nothing references.
    std::vector<uint32_t>    remap(vertices.size(), UINT32_MAX);
    std::vector<WorldVertex> newVertices;
    newVertices.reserve(vertices.size());
    for (Uint32& idx : newIndices) {
        if (remap[idx] == UINT32_MAX) {
            remap[idx] = (uint32_t)newVertices.size();
            newVertices.push_back(vertices[idx]);
        }
        idx = (Uint32)remap[idx];
    }

    vertices.swap(newVertices);
    indices.swap(newIndices);
    numIndices = (uint32_t)indices.size();

    SDL_Log("[ChunkMesh] optimizeMesh: reduced indices from %d to %d", initialindizes, numIndices);
}