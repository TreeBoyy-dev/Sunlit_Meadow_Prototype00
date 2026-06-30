#include "ChunkMesh.h"

#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <vector>

// ============================================================================
// Greedy meshing.
//
// Runs AFTER optimizeMesh() (which has already deleted hidden/back-to-back
// faces). What is left is the set of visible faces. Greedy meshing merges
// adjacent, coplanar, same-direction faces that share the same material AND
// vertex color into a single large quad, so a flat 16x16 ground slab becomes
// one quad instead of 256.
//
// Why this is safe to do without touching the shader:
//   * The world sampler is created with a zero-initialised
//     SDL_GPUSamplerCreateInfo, so address_mode_* == SDL_GPU_SAMPLERADDRESSMODE_REPEAT.
//     A merged WxH quad gets UVs that run 0..W / 0..H, and the texture simply
//     tiles once per block -> looks identical to W*H individual faces.
//   * The world pipeline uses CULLMODE_NONE, so winding can't make a merged
//     quad vanish. We still emit a consistent winding and set per-vertex
//     normals from the face direction so the lighting term stays correct.
//
// Only full unit faces (1x1, sitting on integer u/v) are merged. Partial faces
// (slab sides, anything non-unit or off-grid) and any geometry that isn't a
// clean axis-aligned rectangle are passed through untouched. Slab TOPS/BOTTOMS
// are 1x1 on a half-integer plane and DO merge with each other, which is what
// we want.
// ============================================================================

namespace {

    constexpr float kEps = 1e-4f;

    // For a normal pointing along `axis`, pick the two in-plane axes (u, v).
    inline void planeAxesG(int axis, int& u, int& v) {
        switch (axis) {
        case 0: u = 1; v = 2; break; // X-face spans Y,Z
        case 1: u = 0; v = 2; break; // Y-face spans X,Z
        default: u = 0; v = 1; break; // Z-face spans X,Y
        }
    }

    inline float compG(const Vec3& p, int axis) {
        return axis == 0 ? p.x : (axis == 1 ? p.y : p.z);
    }

    inline bool nearInt(float f) {
        return std::fabs(f - std::round(f)) < kEps;
    }

    inline bool sameColor(const SDL_FColor& a, const SDL_FColor& b) {
        return std::fabs(a.r - b.r) < kEps && std::fabs(a.g - b.g) < kEps &&
            std::fabs(a.b - b.b) < kEps && std::fabs(a.a - b.a) < kEps;
    }

    inline bool sameVec2(const Vec2& a, const Vec2& b) {
        return std::fabs(a.x - b.x) < kEps && std::fabs(a.y - b.y) < kEps;
    }

    // A single 1x1 face that is a candidate for merging.
    struct Cell {
        int        axis;          // 0=X,1=Y,2=Z (normal axis)
        int        sign;          // +1 / -1
        long long  planeKey;      // quantised plane, distinguishes z vs z+0.5 etc.
        int        iu, iv;        // integer grid position of the min corner
        float      materialIndex;
        SDL_FColor color;
        // Affine UV mapping derived from the source face:
        //   uv(u,v) = uv00 + duvU*(u-iu) + duvV*(v-iv)
        Vec2       uv00;          // uv at the (uMin,vMin) corner
        Vec2       duvU;          // uv delta per +1 in u
        Vec2       duvV;          // uv delta per +1 in v
        bool       used = false;
    };

    // Bucket of cells that live on the same plane and face the same way.
    struct Bucket {
        std::unordered_map<long long, int> grid; // packed (iu,iv) -> index into cells
        std::vector<int>                   cellIndices;
    };

    inline long long packGrid(int iu, int iv) {
        // iu/iv are world block coords, can be negative. Bias into unsigned space.
        return ((long long)(iu + (1 << 20)) << 16) | (long long)(unsigned)(iv + (1 << 20));
    }

    inline long long bucketKey(int axis, int sign, long long planeKey) {
        // axis (2 bits) | sign bit | plane
        return ((long long)axis << 62) | ((long long)(sign > 0 ? 1 : 0) << 61) |
            (planeKey & 0x1FFFFFFFFFFFFFFFLL);
    }

    // Reconstruct indices [start, start+6) as one mergeable unit face.
    // Returns false (caller passes it through) for anything that isn't a clean,
    // axis-aligned, 1x1, on-grid, single-material, single-colour rectangle.
    bool makeUnitCell(const std::vector<WorldVertex>& verts,
        const std::vector<Uint32>& indices,
        size_t start, Cell& out)
    {
        if (start + 6 > indices.size())
            return false;

        const WorldVertex& v0 = verts[indices[start]];
        const Vec3& n = v0.normal;
        float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
        int axis = (ax >= ay && ax >= az) ? 0 : (ay >= az ? 1 : 2);
        float nc = compG(n, axis);
        if (std::fabs(nc) < 0.5f)
            return false;
        int sign = (nc > 0.0f) ? 1 : -1;

        int u, v;
        planeAxesG(axis, u, v);

        float plane = compG(v0.position, axis);
        float mat = v0.materialIndex;
        SDL_FColor col = v0.color;

        float uMin = 1e30f, uMax = -1e30f, vMin = 1e30f, vMax = -1e30f;
        for (int k = 0; k < 6; ++k) {
            const WorldVertex& w = verts[indices[start + k]];
            if (std::fabs(compG(w.position, axis) - plane) > kEps) return false; // off-plane
            float wn = compG(w.normal, axis);
            if (std::fabs(wn) < 0.5f || ((wn > 0.0f) ? 1 : -1) != sign) return false; // mixed normals
            if (std::fabs(w.materialIndex - mat) > 0.5f) return false;               // mixed material
            if (!sameColor(w.color, col)) return false;                             // mixed colour
            float pu = compG(w.position, u);
            float pv = compG(w.position, v);
            uMin = std::min(uMin, pu); uMax = std::max(uMax, pu);
            vMin = std::min(vMin, pv); vMax = std::max(vMax, pv);
        }

        // Must be a unit face sitting on the integer grid.
        if (std::fabs((uMax - uMin) - 1.0f) > kEps) return false;
        if (std::fabs((vMax - vMin) - 1.0f) > kEps) return false;
        if (!nearInt(uMin) || !nearInt(vMin)) return false;

        // Pull the UV at each of the four corners so we can rebuild the affine map.
        bool got00 = false, gotU0 = false, gotV0 = false;
        Vec2 uv00{}, uvU0{}, uvV0{};
        for (int k = 0; k < 6; ++k) {
            const WorldVertex& w = verts[indices[start + k]];
            bool atUmin = std::fabs(compG(w.position, u) - uMin) < kEps;
            bool atVmin = std::fabs(compG(w.position, v) - vMin) < kEps;
            if (atUmin && atVmin) { uv00 = w.uv; got00 = true; }
            else if (!atUmin && atVmin) { uvU0 = w.uv; gotU0 = true; }
            else if (atUmin && !atVmin) { uvV0 = w.uv; gotV0 = true; }
        }
        if (!got00 || !gotU0 || !gotV0)
            return false; // couldn't resolve corners -> leave it alone

        out.axis = axis;
        out.sign = sign;
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

    // Two cells may be merged only if texturing/material/colour is identical, so
    // the merged-and-tiled quad is pixel-for-pixel the same as the separate faces.
    inline bool compatible(const Cell& a, const Cell& b) {
        return std::fabs(a.materialIndex - b.materialIndex) < 0.5f &&
            sameColor(a.color, b.color) &&
            sameVec2(a.uv00, b.uv00) &&
            sameVec2(a.duvU, b.duvU) &&
            sameVec2(a.duvV, b.duvV);
    }

    inline Vec3 makePos(int axis, float plane, int u, int v, float uVal, float vVal) {
        Vec3 p{ 0, 0, 0 };
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

    // ---- 1. Reconstruct faces: mergeable unit cells vs. everything else. ----
    std::vector<Cell>   cells;
    std::vector<Uint32> passthrough; // original indices we won't touch
    cells.reserve(indices.size() / 6 + 1);

    size_t i = 0;
    while (i < indices.size()) {
        Cell c;
        if (makeUnitCell(vertices, indices, i, c)) {
            cells.push_back(c);
            i += 6;
        }
        else if (i + 6 <= indices.size()) {
            // Not mergeable but still a (presumably) valid quad -> keep its 6.
            for (int k = 0; k < 6; ++k) passthrough.push_back(indices[i + k]);
            i += 6;
        }
        else {
            // Tail that isn't a full quad -> keep whatever triangle is there.
            for (int k = 0; k < 3 && i + k < indices.size(); ++k)
                passthrough.push_back(indices[i + k]);
            i += 3;
        }
    }

    // ---- 2. Bucket cells by (axis, sign, plane) and index them on a grid. ----
    std::unordered_map<long long, Bucket> buckets;
    for (int ci = 0; ci < (int)cells.size(); ++ci) {
        const Cell& c = cells[ci];
        Bucket& bk = buckets[bucketKey(c.axis, c.sign, c.planeKey)];
        bk.grid[packGrid(c.iu, c.iv)] = ci;
        bk.cellIndices.push_back(ci);
    }

    // ---- 3. Greedy-merge each bucket into rectangles. ----
    struct MergedQuad {
        int axis, sign, u, v;
        float plane;
        int iu, iv, w, h;
        float materialIndex;
        SDL_FColor color;
        Vec2 uv00, duvU, duvV;
    };
    std::vector<MergedQuad> merged;

    for (auto& kv : buckets) {
        Bucket& bk = kv.second;
        // Deterministic order: by v then u.
        std::sort(bk.cellIndices.begin(), bk.cellIndices.end(),
            [&](int a, int b) {
                const Cell& ca = cells[a];
                const Cell& cb = cells[b];
                return ca.iv != cb.iv ? ca.iv < cb.iv : ca.iu < cb.iu;
            });

        for (int ci : bk.cellIndices) {
            Cell& base = cells[ci];
            if (base.used) continue;

            int u, v;
            planeAxesG(base.axis, u, v);

            // Grow along u.
            int w = 1;
            while (true) {
                auto it = bk.grid.find(packGrid(base.iu + w, base.iv));
                if (it == bk.grid.end()) break;
                Cell& nb = cells[it->second];
                if (nb.used || !compatible(base, nb)) break;
                ++w;
            }

            // Grow along v: an entire row [iu..iu+w) must be present & compatible.
            int h = 1;
            while (true) {
                bool rowOk = true;
                for (int du = 0; du < w; ++du) {
                    auto it = bk.grid.find(packGrid(base.iu + du, base.iv + h));
                    if (it == bk.grid.end()) { rowOk = false; break; }
                    Cell& nb = cells[it->second];
                    if (nb.used || !compatible(base, nb)) { rowOk = false; break; }
                }
                if (!rowOk) break;
                ++h;
            }

            // Claim the whole rectangle.
            for (int dv = 0; dv < h; ++dv)
                for (int du = 0; du < w; ++du) {
                    auto it = bk.grid.find(packGrid(base.iu + du, base.iv + dv));
                    if (it != bk.grid.end()) cells[it->second].used = true;
                }

            MergedQuad mq;
            mq.axis = base.axis; mq.sign = base.sign; mq.u = u; mq.v = v;
            mq.plane = (float)base.planeKey / 1000.0f;
            mq.iu = base.iu; mq.iv = base.iv; mq.w = w; mq.h = h;
            mq.materialIndex = base.materialIndex;
            mq.color = base.color;
            mq.uv00 = base.uv00; mq.duvU = base.duvU; mq.duvV = base.duvV;
            merged.push_back(mq);
        }
    }

    // ---- 4. Rebuild vertex/index buffers. ----
    std::vector<WorldVertex> newVertices;
    std::vector<Uint32>      newIndices;
    newVertices.reserve(merged.size() * 4 + passthrough.size());
    newIndices.reserve(merged.size() * 6 + passthrough.size());

    // 4a. Carry passthrough geometry over, compacting the vertices it uses.
    std::vector<uint32_t> remap(vertices.size(), UINT32_MAX);
    for (Uint32 oldIdx : passthrough) {
        if (remap[oldIdx] == UINT32_MAX) {
            remap[oldIdx] = (uint32_t)newVertices.size();
            newVertices.push_back(vertices[oldIdx]);
        }
        newIndices.push_back(remap[oldIdx]);
    }

    // 4b. Emit the merged quads as fresh geometry.
    for (const MergedQuad& mq : merged) {
        float u0 = (float)mq.iu;
        float v0 = (float)mq.iv;
        float u1 = (float)(mq.iu + mq.w);
        float v1 = (float)(mq.iv + mq.h);

        Vec3 normal{ 0, 0, 0 };
        float* nc[3] = { &normal.x, &normal.y, &normal.z };
        *nc[mq.axis] = (float)mq.sign;

        auto uvAt = [&](int du, int dv) -> Vec2 {
            return { mq.uv00.x + mq.duvU.x * du + mq.duvV.x * dv,
                     mq.uv00.y + mq.duvU.y * du + mq.duvV.y * dv };
            };

        WorldVertex c00{ makePos(mq.axis, mq.plane, mq.u, mq.v, u0, v0), normal, uvAt(0,      0),      mq.color, mq.materialIndex };
        WorldVertex c10{ makePos(mq.axis, mq.plane, mq.u, mq.v, u1, v0), normal, uvAt(mq.w,   0),      mq.color, mq.materialIndex };
        WorldVertex c11{ makePos(mq.axis, mq.plane, mq.u, mq.v, u1, v1), normal, uvAt(mq.w,   mq.h),   mq.color, mq.materialIndex };
        WorldVertex c01{ makePos(mq.axis, mq.plane, mq.u, mq.v, u0, v1), normal, uvAt(0,      mq.h),   mq.color, mq.materialIndex };

        Uint32 base = (Uint32)newVertices.size();
        newVertices.push_back(c00);
        newVertices.push_back(c10);
        newVertices.push_back(c11);
        newVertices.push_back(c01);

        // Winding: keep the geometric normal pointing the same way as `sign`.
        // For axes X(0) and Z(2) the (u x v) cross is +axis; for Y(1) it's -axis.
        bool flip = (mq.axis == 1) ? (mq.sign > 0) : (mq.sign < 0);
        if (!flip) {
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

    SDL_Log("[ChunkMesh] greedyMesh: reduced indices from %d to %d (%zu merged quads)",
        initialIndices, numIndices, merged.size());
}