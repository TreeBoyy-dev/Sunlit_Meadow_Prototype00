#include "ChunkMesh.h"

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <vector> 

// ============================================================================
// ChunkMesh::greedyMeshing()
//
// Runs after optimizeMesh()'s faceCulling(), which has already removed
// hidden / back-to-back faces and compacted the buffers. What remains are the
// visible faces. This pass merges adjacent, coplanar, same-facing 1x1 block
// faces that share material, colour and texture orientation into single large
// quads, so a flat 16x16 slab of grass tops becomes 1 quad instead of 256.
//
// Assumptions (verified against the current renderer):
//   * The world sampler is created from a zero-initialised
//     SDL_GPUSamplerCreateInfo, so address modes are REPEAT. A merged WxH quad
//     gets UVs spanning W and H texture repeats, which tiles the block texture
//     once per cell -> pixel-identical to the unmerged faces.
//   * The opaque world pipeline has no rasterizer state set (CULLMODE_NONE),
//     but the transparent pipeline uses CULLMODE_FRONT with CCW front faces,
//     so triangle winding MATTERS. We therefore measure the winding of each
//     source face and emit merged quads with the same geometric orientation.
//   * Block models are Z-up after the obj loader's rotation, and full-block
//     faces land on integer world coordinates. Slab tops/bottoms are 1x1
//     cells on half-integer planes and merge with each other. Overlay faces
//     (e.g. grass sides) are pushed 0.001 along their normal; the quantised
//     plane key keeps them in their own merge layer.
//
// Anything that is not a clean, axis-aligned, on-grid, 1x1, single-material,
// single-colour rectangle (slab side strips, stairs, decorative models, odd
// triangles) is passed through completely untouched.
// ============================================================================

namespace {

    constexpr float kEps = 1e-4f;

    // In-plane axes (u, v) for a face whose normal lies on `axis`.
    inline void inPlaneAxes(int axis, int& u, int& v) {
        switch (axis) {
        case 0:  u = 1; v = 2; break;  // X-face spans Y,Z
        case 1:  u = 0; v = 2; break;  // Y-face spans X,Z
        default: u = 0; v = 1; break;  // Z-face spans X,Y
        }
    }

    inline float axisComp(const Vec3& p, int axis) {
        return axis == 0 ? p.x : (axis == 1 ? p.y : p.z);
    }

    inline bool nearInteger(float f) {
        return std::fabs(f - std::round(f)) < kEps;
    }

    inline bool sameColor(const SDL_FColor& a, const SDL_FColor& b) {
        return std::fabs(a.r - b.r) < kEps && std::fabs(a.g - b.g) < kEps &&
            std::fabs(a.b - b.b) < kEps && std::fabs(a.a - b.a) < kEps;
    }

    inline bool sameVec2(const Vec2& a, const Vec2& b) {
        return std::fabs(a.x - b.x) < kEps && std::fabs(a.y - b.y) < kEps;
    }

    // One mergeable 1x1 face.
    struct UnitFace {
        int        axis;        // 0=X, 1=Y, 2=Z (normal axis)
        int        sign;        // +1 / -1 along that axis
        int        crossSign;   // geometric winding of the source triangles:
        // +1 if edge-cross of tri 0 points along +axis
        float      plane;       // exact plane coordinate (kept as float)
        long long  planeKey;    // quantised plane, separates z / z+0.5 / z+0.001
        int        iu, iv;      // integer min-corner on the in-plane grid
        float      materialIndex;
        SDL_FColor color;
        // Affine UV map of the source face:
        //   uv(du, dv) = uv00 + duvU * du + duvV * dv   (du, dv in blocks)
        Vec2       uv00;        // uv at the (uMin, vMin) corner
        Vec2       duvU;        // uv change per +1 block along u
        Vec2       duvV;        // uv change per +1 block along v
        bool       used = false;
    };

    // All faces that live on one plane, face one way, and wind one way.
    struct Layer {
        std::unordered_map<long long, int> grid;  // packed (iu,iv) -> face index
        std::vector<int>                   faces;
    };

    inline long long packCoord(int iu, int iv) {
        // World block coords can be negative; bias into unsigned space.
        return (((long long)(iu + (1 << 24))) << 26) |
            ((long long)(iv + (1 << 24)));
    }

    inline long long layerKey(const UnitFace& f) {
        // 2 bits axis | 1 bit sign | 1 bit winding | quantised plane.
        return ((long long)f.axis << 61) |
            ((long long)(f.sign > 0 ? 1 : 0) << 60) |
            ((long long)(f.crossSign > 0 ? 1 : 0) << 59) |
            (f.planeKey & 0x07FFFFFFFFFFFFFFLL);
    }

    // Try to read indices [start, start+6) as one mergeable unit face.
    // Returns false for anything that should be passed through untouched.
    bool tryMakeUnitFace(const std::vector<WorldVertex>& verts,
        const std::vector<Uint32>& indices,
        size_t start, UnitFace& out)
    {
        if (start + 6 > indices.size())
            return false;

        // --- Normal must be axis-aligned and shared by all six vertices. ---
        const WorldVertex& v0 = verts[indices[start]];
        const Vec3& n = v0.normal;
        float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
        int axis = (ax >= ay && ax >= az) ? 0 : (ay >= az ? 1 : 2);
        float nAxis = axisComp(n, axis);
        if (std::fabs(nAxis) < 0.5f)
            return false;
        int sign = (nAxis > 0.0f) ? 1 : -1;

        int u, v;
        inPlaneAxes(axis, u, v);

        const float plane = axisComp(v0.position, axis);
        const float mat = v0.materialIndex;
        const SDL_FColor col = v0.color;

        float uMin = 1e30f, uMax = -1e30f, vMin = 1e30f, vMax = -1e30f;
        for (int k = 0; k < 6; ++k) {
            const WorldVertex& w = verts[indices[start + k]];
            if (std::fabs(axisComp(w.position, axis) - plane) > kEps)
                return false;                                       // off-plane
            float wn = axisComp(w.normal, axis);
            if (std::fabs(wn) < 0.5f || ((wn > 0.0f) ? 1 : -1) != sign)
                return false;                                       // mixed normals
            if (std::fabs(w.materialIndex - mat) > 0.5f)
                return false;                                       // mixed material
            if (!sameColor(w.color, col))
                return false;                                       // mixed colour
            float pu = axisComp(w.position, u);
            float pv = axisComp(w.position, v);
            uMin = std::min(uMin, pu); uMax = std::max(uMax, pu);
            vMin = std::min(vMin, pv); vMax = std::max(vMax, pv);
        }

        // --- Must be exactly 1x1 and sitting on the integer grid. ---
        if (std::fabs((uMax - uMin) - 1.0f) > kEps) return false;
        if (std::fabs((vMax - vMin) - 1.0f) > kEps) return false;
        if (!nearInteger(uMin) || !nearInteger(vMin)) return false;

        // --- Winding of the first source triangle (drives cull behaviour). ---
        {
            const Vec3& p0 = verts[indices[start + 0]].position;
            const Vec3& p1 = verts[indices[start + 1]].position;
            const Vec3& p2 = verts[indices[start + 2]].position;
            Vec3 e1 = vec3Sub(p1, p0);
            Vec3 e2 = vec3Sub(p2, p0);
            Vec3 cr = vec3Cross(e1, e2);
            float c = axisComp(cr, axis);
            if (std::fabs(c) < kEps)
                return false;                       // degenerate triangle
            out.crossSign = (c > 0.0f) ? 1 : -1;
        }

        // --- Recover the affine UV map from three of the four corners. ---
        bool got00 = false, gotU0 = false, gotV0 = false;
        Vec2 uv00{}, uvU0{}, uvV0{};
        for (int k = 0; k < 6; ++k) {
            const WorldVertex& w = verts[indices[start + k]];
            bool atUmin = std::fabs(axisComp(w.position, u) - uMin) < kEps;
            bool atVmin = std::fabs(axisComp(w.position, v) - vMin) < kEps;
            if (atUmin && atVmin) { uv00 = w.uv; got00 = true; }
            else if (!atUmin && atVmin) { uvU0 = w.uv; gotU0 = true; }
            else if (atUmin && !atVmin) { uvV0 = w.uv; gotV0 = true; }
        }
        if (!got00 || !gotU0 || !gotV0)
            return false;                           // corners unresolved

        out.axis = axis;
        out.sign = sign;
        out.plane = plane;
        out.planeKey = (long long)std::llround(plane * 1000.0f);
        out.iu = (int)std::lround(uMin);
        out.iv = (int)std::lround(vMin);
        out.materialIndex = mat;
        out.color = col;
        out.uv00 = uv00;
        out.duvU = { uvU0.x - uv00.x, uvU0.y - uv00.y };
        out.duvV = { uvV0.x - uv00.x, uvV0.y - uv00.y };
        out.used = false;
        return true;
    }

    // Faces may merge only when the merged, tiled quad renders pixel-identical
    // to the separate faces: same material, colour and texture orientation.
    inline bool mergeable(const UnitFace& a, const UnitFace& b) {
        return std::fabs(a.materialIndex - b.materialIndex) < 0.5f &&
            sameColor(a.color, b.color) &&
            sameVec2(a.uv00, b.uv00) &&
            sameVec2(a.duvU, b.duvU) &&
            sameVec2(a.duvV, b.duvV);
    }

    inline Vec3 buildPos(int axis, float plane, int u, int v,
        float uVal, float vVal) {
        Vec3 p{ 0.0f, 0.0f, 0.0f };
        float* c[3] = { &p.x, &p.y, &p.z };
        *c[axis] = plane;
        *c[u] = uVal;
        *c[v] = vVal;
        return p;
    }

} // namespace

void ChunkMesh::greedyMeshing()
{
    if (indices.empty())
        return;

    const int initialIndices = (int)indices.size();

    // ---- 1. Split geometry into mergeable unit faces and passthrough. ----
    std::vector<UnitFace> faces;
    std::vector<Uint32>   passthrough;      // original indices, kept verbatim
    faces.reserve(indices.size() / 6 + 1);

    size_t i = 0;
    while (i < indices.size()) {
        UnitFace f;
        if (tryMakeUnitFace(vertices, indices, i, f)) {
            faces.push_back(f);
            i += 6;
        }
        else {
            // Keep one triangle and re-inspect at the next one, so a stray
            // triangle can't knock a following valid quad out of alignment.
            for (int k = 0; k < 3 && i + k < indices.size(); ++k)
                passthrough.push_back(indices[i + k]);
            i += 3;
        }
    }

    if (faces.empty())
        return;                              // nothing mergeable at all

    // ---- 2. Bucket faces into layers: same plane, facing, winding. ----
    std::unordered_map<long long, Layer> layers;
    for (int fi = 0; fi < (int)faces.size(); ++fi) {
        const UnitFace& f = faces[fi];
        Layer& layer = layers[layerKey(f)];
        layer.grid[packCoord(f.iu, f.iv)] = fi;
        layer.faces.push_back(fi);
    }

    // ---- 3. Greedy rectangle expansion inside each layer. ----
    struct MergedQuad {
        int axis, sign, crossSign;
        float plane;
        int iu, iv, w, h;
        float materialIndex;
        SDL_FColor color;
        Vec2 uv00, duvU, duvV;
    };
    std::vector<MergedQuad> merged;
    merged.reserve(faces.size() / 2);

    for (auto& kv : layers) {
        Layer& layer = kv.second;

        // Deterministic sweep order: row by row (v), then along u.
        std::sort(layer.faces.begin(), layer.faces.end(),
            [&](int a, int b) {
                const UnitFace& fa = faces[a];
                const UnitFace& fb = faces[b];
                return fa.iv != fb.iv ? fa.iv < fb.iv : fa.iu < fb.iu;
            });

        for (int fi : layer.faces) {
            UnitFace& base = faces[fi];
            if (base.used) continue;

            // Grow the run along u as far as compatible neighbours exist.
            int w = 1;
            for (;;) {
                auto it = layer.grid.find(packCoord(base.iu + w, base.iv));
                if (it == layer.grid.end()) break;
                UnitFace& nb = faces[it->second];
                if (nb.used || !mergeable(base, nb)) break;
                ++w;
            }

            // Grow along v: every cell of the next row [iu .. iu+w) must fit.
            int h = 1;
            for (;;) {
                bool rowOk = true;
                for (int du = 0; du < w; ++du) {
                    auto it = layer.grid.find(packCoord(base.iu + du, base.iv + h));
                    if (it == layer.grid.end()) { rowOk = false; break; }
                    UnitFace& nb = faces[it->second];
                    if (nb.used || !mergeable(base, nb)) { rowOk = false; break; }
                }
                if (!rowOk) break;
                ++h;
            }

            // Claim the whole w x h rectangle.
            for (int dv = 0; dv < h; ++dv)
                for (int du = 0; du < w; ++du) {
                    auto it = layer.grid.find(packCoord(base.iu + du, base.iv + dv));
                    if (it != layer.grid.end())
                        faces[it->second].used = true;
                }

            MergedQuad q;
            q.axis = base.axis; q.sign = base.sign; q.crossSign = base.crossSign;
            q.plane = base.plane;
            q.iu = base.iu; q.iv = base.iv; q.w = w; q.h = h;
            q.materialIndex = base.materialIndex;
            q.color = base.color;
            q.uv00 = base.uv00; q.duvU = base.duvU; q.duvV = base.duvV;
            merged.push_back(q);
        }
    }

    // ---- 4. Rebuild the vertex / index buffers. ----
    std::vector<WorldVertex> newVertices;
    std::vector<Uint32>      newIndices;
    newVertices.reserve(merged.size() * 4 + passthrough.size());
    newIndices.reserve(merged.size() * 6 + passthrough.size());

    // 4a. Passthrough geometry first, with its vertices compacted.
    std::vector<uint32_t> remap(vertices.size(), UINT32_MAX);
    for (Uint32 oldIdx : passthrough) {
        if (remap[oldIdx] == UINT32_MAX) {
            remap[oldIdx] = (uint32_t)newVertices.size();
            newVertices.push_back(vertices[oldIdx]);
        }
        newIndices.push_back(remap[oldIdx]);
    }

    // 4b. Fresh geometry for every merged quad.
    for (const MergedQuad& q : merged) {
        int u, v;
        inPlaneAxes(q.axis, u, v);

        const float u0 = (float)q.iu;
        const float v0 = (float)q.iv;
        const float u1 = (float)(q.iu + q.w);
        const float v1 = (float)(q.iv + q.h);

        Vec3 normal{ 0.0f, 0.0f, 0.0f };
        float* nc[3] = { &normal.x, &normal.y, &normal.z };
        *nc[q.axis] = (float)q.sign;

        auto uvAt = [&](int du, int dv) -> Vec2 {
            return { q.uv00.x + q.duvU.x * du + q.duvV.x * dv,
                     q.uv00.y + q.duvU.y * du + q.duvV.y * dv };
            };

        WorldVertex c00{ buildPos(q.axis, q.plane, u, v, u0, v0), normal, uvAt(0,   0),   q.color, q.materialIndex };
        WorldVertex c10{ buildPos(q.axis, q.plane, u, v, u1, v0), normal, uvAt(q.w, 0),   q.color, q.materialIndex };
        WorldVertex c11{ buildPos(q.axis, q.plane, u, v, u1, v1), normal, uvAt(q.w, q.h), q.color, q.materialIndex };
        WorldVertex c01{ buildPos(q.axis, q.plane, u, v, u0, v1), normal, uvAt(0,   q.h), q.color, q.materialIndex };

        const Uint32 base = (Uint32)newVertices.size();
        newVertices.push_back(c00);
        newVertices.push_back(c10);
        newVertices.push_back(c11);
        newVertices.push_back(c01);

        // Match the source winding. The order (c00, c10, c11) has an edge
        // cross of +u x +v, which points along +axis for X and Z faces and
        // along -axis for Y faces. Flip if that disagrees with the source.
        const int defaultCross = (q.axis == 1) ? -1 : 1;
        if (q.crossSign == defaultCross) {
            newIndices.push_back(base + 0); newIndices.push_back(base + 1); newIndices.push_back(base + 2);
            newIndices.push_back(base + 0); newIndices.push_back(base + 2); newIndices.push_back(base + 3);
        }
        else {
            newIndices.push_back(base + 0); newIndices.push_back(base + 2); newIndices.push_back(base + 1);
            newIndices.push_back(base + 0); newIndices.push_back(base + 3); newIndices.push_back(base + 2);
        }
    }

    vertices.swap(newVertices);
    indices.swap(newIndices);
    numIndices = (uint32_t)indices.size();

    //SDL_Log("[ChunkMesh] greedyMeshing: indices %d -> %u (%zu merged quads, %zu passthrough indices)",
    //    initialIndices, numIndices, merged.size(), passthrough.size());
}