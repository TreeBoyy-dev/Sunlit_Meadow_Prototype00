#include "BlockModel.h"
#include "Materials.h"
#include "ObjParser.h"

#include <unordered_map>
#include <algorithm>
#include <cmath>

static const char* baseModelPath = "Models/";

BlockModel::BlockModel(
    ModelFace topMaterial,
    ModelFace bottomMaterial,
    ModelFace sideMaterial)
    : topMaterial(topMaterial),
    bottomMaterial(bottomMaterial),
    sideMaterial(sideMaterial)
{
}

// ---------------------------------------------------------------------
//  Baking helpers.
//
//  Model space after obj_parse (MODEL_ROTATION already baked in):
//    X, Y in [-0.5, 0.5] (centered — the clean rotation pivot), Z in [0, 1].
//  The +0.5 placement offset is applied AFTER all transforms, exactly like
//  the old code path.
// ---------------------------------------------------------------------
namespace {

// 0 = side, 1 = top, 2 = bottom — which of the block's three materials a
// face uses. Captured from the PRE-transform normal so textures stay glued
// to their geometry when a variant is rotated (a sideways log keeps its
// rings on the ends). A Z-flip additionally swaps top/bottom so the face
// that ends up pointing up still shows the top material (per plan §5).
int materialCellForNormal(const Vec3& n) {
    if (n.z > 0.5f)  return 1;
    if (n.z < -0.5f) return 2;
    return 0;
}

// k * 90° counter-clockwise (looking down -Z) about the Z axis at the
// model's X/Y origin. Exact integer swaps — no trig error.
void rotZSteps(std::vector<ModelVertex>& verts, int k) {
    k = ((k % 4) + 4) % 4;
    for (int s = 0; s < k; ++s) {
        for (ModelVertex& v : verts) {
            float x = v.position.x, y = v.position.y;
            v.position.x = -y; v.position.y = x;
            float nx = v.normal.x, ny = v.normal.y;
            v.normal.x = -ny; v.normal.y = nx;
        }
    }
}

// k * 90° about the X axis through the block center (y = 0, z = 0.5).
void rotXSteps(std::vector<ModelVertex>& verts, int k) {
    k = ((k % 4) + 4) % 4;
    for (int s = 0; s < k; ++s) {
        for (ModelVertex& v : verts) {
            float y = v.position.y, z = v.position.z - 0.5f;
            v.position.y = -z; v.position.z = y + 0.5f;
            float ny = v.normal.y, nz = v.normal.z;
            v.normal.y = -nz; v.normal.z = ny;
        }
    }
}

// k * 90° about the Y axis through the block center (x = 0, z = 0.5).
void rotYSteps(std::vector<ModelVertex>& verts, int k) {
    k = ((k % 4) + 4) % 4;
    for (int s = 0; s < k; ++s) {
        for (ModelVertex& v : verts) {
            float x = v.position.x, z = v.position.z - 0.5f;
            v.position.x = z; v.position.z = -x + 0.5f;
            float nx = v.normal.x, nz = v.normal.z;
            v.normal.x = nz; v.normal.z = -nx;
        }
    }
}

// Mirror in Z (bottom <-> top). Mirrors flip triangle winding, so the caller
// must also reverse the index order per triangle.
void flipZ(std::vector<ModelVertex>& verts) {
    for (ModelVertex& v : verts) {
        v.position.z = 1.0f - v.position.z;
        v.normal.z = -v.normal.z;
    }
}
void reverseWinding(std::vector<Uint16>& indices) {
    for (size_t t = 0; t + 2 < indices.size(); t += 3)
        std::swap(indices[t + 1], indices[t + 2]);
}

// facing (rot4 / first 4 values of rot6): N=+Y, E=+X, S=-Y, W=-X.
// Models are authored facing north; east is one clockwise step = 3 CCW.
int facingToZSteps(Uint16 facing) {
    switch (facing & 3) {
    default:
    case 0: return 0; // north
    case 1: return 3; // east
    case 2: return 2; // south
    case 3: return 1; // west
    }
}

const char* shape5Name(Uint16 v) {
    switch (v) {
    default:
    case 0: return "straight";
    case 1: return "inner_l";
    case 2: return "inner_r";
    case 3: return "outer_l";
    case 4: return "outer_r";
    }
}

// Per-bake .obj cache so shared geometry (fence arms, alternate shapes) is
// parsed once.
struct ObjCache {
    std::unordered_map<std::string, std::pair<std::vector<ModelVertex>, std::vector<Uint16>>> map;

    bool get(const std::string& file,
             std::vector<ModelVertex>& outVerts, std::vector<Uint16>& outIdx) {
        auto it = map.find(file);
        if (it == map.end()) {
            std::vector<ModelVertex> v;
            std::vector<Uint16> i;
            if (!obj_parse(BuildAbsolutePath(baseModelPath, file.c_str()), v, i)) {
                SDL_Log("[BlockModel] bake: obj_parse failed for '%s'", file.c_str());
                return false;
            }
            it = map.emplace(file, std::make_pair(std::move(v), std::move(i))).first;
        }
        outVerts = it->second.first;   // copies — transforms mutate them
        outIdx = it->second.second;
        return true;
    }
};

// =====================================================================
//  Phase 2: bake-time face classification.
//
//  Splits a baked mesh into the "always" bucket + six per-direction
//  boundary buckets, and computes coverMask (see BakedMesh.h). Runs once
//  per variant/part at init — cost is irrelevant, faithfulness is not.
// =====================================================================

// Strict on-plane tolerance — MUST match faceCulling's kEps, because
// coverMask claims "faceCulling would cull the face touching this plane".
constexpr float kCoverEps = 1e-4f;
// Loose boundary tolerance for bucket assignment: catches the 0.001-offset
// overlay copies (grass sides) that sit just off the plane. Anything a
// covering neighbor hides geometrically, even if faceCulling's exact-plane
// test never removed it (it was invisible behind the neighbor regardless).
constexpr float kBoundaryEps = 2.5e-3f;

inline float axisComp(const Vec3& p, int axis) {
    return axis == 0 ? p.x : (axis == 1 ? p.y : p.z);
}

// Classify one triangle: FaceDir index if it is an outward-facing,
// axis-aligned face lying on (or overlay-close to) a cell boundary plane
// and within the cell footprint; -1 -> "always" bucket.
int triBoundaryDir(const WorldVertex& a, const WorldVertex& b, const WorldVertex& c) {
    const WorldVertex* v[3] = { &a, &b, &c };

    const Vec3& n = a.normal;
    float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
    int axis = (ax >= ay && ax >= az) ? 0 : (ay >= az ? 1 : 2);
    float nc = axisComp(n, axis);
    if (std::fabs(nc) < 0.5f) return -1;
    int sign = (nc > 0.0f) ? 1 : -1;
    const float boundary = (sign > 0) ? 1.0f : 0.0f;

    int u = (axis == 0) ? 1 : 0;
    int w = (axis == 2) ? 1 : 2;

    for (int k = 0; k < 3; ++k) {
        float wn = axisComp(v[k]->normal, axis);
        if (std::fabs(wn) < 0.5f || ((wn > 0.0f) ? 1 : -1) != sign)
            return -1;                                   // mixed normals
        if (std::fabs(axisComp(v[k]->position, axis) - boundary) > kBoundaryEps)
            return -1;                                   // not on the plane
        float pu = axisComp(v[k]->position, u);
        float pw = axisComp(v[k]->position, w);
        if (pu < -kBoundaryEps || pu > 1.0f + kBoundaryEps ||
            pw < -kBoundaryEps || pw > 1.0f + kBoundaryEps)
            return -1;                                   // outside footprint
    }
    return axis * 2 + (sign > 0 ? 0 : 1);                // FaceDir index
}

// coverMask: walk the index stream EXACTLY like faceCulling::tryMakeQuad
// does (6-index rects, advance 6 on success / 3 on failure) and set bit d
// when a rect sits strictly on boundary plane d and spans the full [0,1]^2
// footprint. Partial faces and split faces set no bit — those pairs are
// left for the residual faceCulling pass, matching today's output.
Uint8 computeCoverMask(const std::vector<WorldVertex>& verts,
                       const std::vector<Uint16>& indices)
{
    Uint8 mask = 0;
    size_t i = 0;
    while (i + 6 <= indices.size()) {
        const Vec3& n = verts[indices[i]].normal;
        float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
        int axis = (ax >= ay && ax >= az) ? 0 : (ay >= az ? 1 : 2);
        float nc = axisComp(n, axis);
        if (std::fabs(nc) < 0.5f) { i += 3; continue; }
        int sign = (nc > 0.0f) ? 1 : -1;

        int u = (axis == 0) ? 1 : 0;
        int w = (axis == 2) ? 1 : 2;

        float plane = axisComp(verts[indices[i]].position, axis);
        float uMin = 1e30f, uMax = -1e30f, wMin = 1e30f, wMax = -1e30f;
        bool ok = true;
        for (int k = 0; k < 6 && ok; ++k) {
            const WorldVertex& v = verts[indices[i + k]];
            if (std::fabs(axisComp(v.position, axis) - plane) > kCoverEps) { ok = false; break; }
            float wn = axisComp(v.normal, axis);
            if (std::fabs(wn) < 0.5f || ((wn > 0.0f) ? 1 : -1) != sign) { ok = false; break; }
            float pu = axisComp(v.position, u);
            float pw = axisComp(v.position, w);
            uMin = std::min(uMin, pu); uMax = std::max(uMax, pu);
            wMin = std::min(wMin, pw); wMax = std::max(wMax, pw);
        }
        if (!ok || uMax - uMin < kCoverEps || wMax - wMin < kCoverEps) { i += 3; continue; }

        const float boundary = (sign > 0) ? 1.0f : 0.0f;
        if (std::fabs(plane - boundary) <= kCoverEps &&
            uMin <= kCoverEps && uMax >= 1.0f - kCoverEps &&
            wMin <= kCoverEps && wMax >= 1.0f - kCoverEps)
            mask |= (Uint8)(1u << (axis * 2 + (sign > 0 ? 0 : 1)));

        i += 6;   // consumed as one rect, same as faceCulling's walk
    }
    return mask;
}

// Split a freshly converted BakedMesh into the always bucket + six
// per-direction boundary buckets. Triangle order inside each bucket is
// preserved, so the 6-index quad structure greedyMeshing and faceCulling
// rely on survives the split (both tris of a coplanar quad classify alike).
void classifyBakedFaces(BakedMesh& m) {
    m.coverMask = computeCoverMask(m.vertices, m.indices);

    std::vector<WorldVertex> srcVerts;
    std::vector<Uint16>      srcIdx;
    srcVerts.swap(m.vertices);
    srcIdx.swap(m.indices);
    for (int d = 0; d < 6; ++d) {
        m.boundaryFaces[d].vertices.clear();
        m.boundaryFaces[d].indices.clear();
    }

    // remap[bucket][srcVertex] -> vertex index inside that bucket (7 = always)
    std::vector<int> remap[7];
    for (int b = 0; b < 7; ++b) remap[b].assign(srcVerts.size(), -1);

    auto bucketOf = [&](int dir) -> BakedMesh::FaceSet* {
        return dir < 0 ? nullptr : &m.boundaryFaces[dir];
    };

    for (size_t t = 0; t + 2 < srcIdx.size(); t += 3) {
        Uint16 ia = srcIdx[t], ib = srcIdx[t + 1], ic = srcIdx[t + 2];
        int dir = triBoundaryDir(srcVerts[ia], srcVerts[ib], srcVerts[ic]);

        std::vector<WorldVertex>* dstV;
        std::vector<Uint16>*      dstI;
        int bucket;
        if (BakedMesh::FaceSet* fs = bucketOf(dir)) {
            dstV = &fs->vertices; dstI = &fs->indices; bucket = dir;
        } else {
            dstV = &m.vertices;   dstI = &m.indices;   bucket = 6;
        }

        for (Uint16 src : { ia, ib, ic }) {
            int& r = remap[bucket][src];
            if (r < 0) { r = (int)dstV->size(); dstV->push_back(srcVerts[src]); }
            dstI->push_back((Uint16)r);
        }
    }
}

} // namespace

// ---------------------------------------------------------------------
//  ModelVertex -> WorldVertex conversion. Same behavior as the old init():
//  +0.5 placement offset in X/Y, overlay vertices duplicated with a small
//  offset along the normal, overlay triangles appended after the base ones.
//  `materialCells` holds the pre-transform material pick per vertex;
//  `swapTopBottom` compensates a Z-flip.
// ---------------------------------------------------------------------
static void convertToBaked(
    const std::vector<ModelVertex>& modelVertices,
    const std::vector<Uint16>& modelIndices,
    const std::vector<int>& materialCells,
    bool swapTopBottom,
    ModelFace top, ModelFace bottom, ModelFace side,
    BakedMesh& out
) {
    const float kPlacementOffsetX = 0.5f;
    const float kPlacementOffsetY = 0.5f;
    const float kOverlayEpsilon = 0.001f;

    auto faceFor = [&](int cell) -> ModelFace {
        if (cell == 1) return swapTopBottom ? bottom : top;
        if (cell == 2) return swapTopBottom ? top : bottom;
        return side;
    };

    out.vertices.clear();
    out.indices.clear();
    out.vertices.reserve(modelVertices.size());

    // overlayRemap[i] = index of the overlay copy of base vertex i, or -1
    std::vector<int> overlayRemap(modelVertices.size(), -1);

    // 1) Base vertices
    for (size_t i = 0; i < modelVertices.size(); ++i) {
        const ModelVertex& mv = modelVertices[i];
        ModelFace material = faceFor(materialCells[i]);
        out.vertices.push_back({
            { mv.position.x + kPlacementOffsetX,
              mv.position.y + kPlacementOffsetY,
              mv.position.z },
            mv.normal,
            mv.uv,
            mv.color,
            static_cast<float>(material.material)
            });
    }

    // 2) Overlay vertices
    for (size_t i = 0; i < modelVertices.size(); ++i) {
        const ModelVertex& mv = modelVertices[i];
        ModelFace material = faceFor(materialCells[i]);
        if (material.overlayMaterial < 0) continue;

        overlayRemap[i] = (int)out.vertices.size();
        out.vertices.push_back({
            { mv.position.x + kPlacementOffsetX + mv.normal.x * kOverlayEpsilon,
              mv.position.y + kPlacementOffsetY + mv.normal.y * kOverlayEpsilon,
              mv.position.z + mv.normal.z * kOverlayEpsilon },
            mv.normal,
            mv.uv,
            mv.color,
            static_cast<float>(material.overlayMaterial)
            });
    }

    // 3) Base indices
    out.indices = modelIndices;

    // 4) Overlay indices — duplicate each triangle whose 3 verts all have overlays
    for (size_t t = 0; t + 2 < modelIndices.size(); t += 3) {
        Uint16 a = modelIndices[t + 0];
        Uint16 b = modelIndices[t + 1];
        Uint16 c = modelIndices[t + 2];
        if (overlayRemap[a] < 0 || overlayRemap[b] < 0 || overlayRemap[c] < 0)
            continue;
        out.indices.push_back((Uint16)overlayRemap[a]);
        out.indices.push_back((Uint16)overlayRemap[b]);
        out.indices.push_back((Uint16)overlayRemap[c]);
    }
}

// ---------------------------------------------------------------------
bool BlockModel::bake(const BlockDef& def, const StateLayout& layout) {
    variants.clear();
    parts.clear();
    multipart = def.multipart;
    modelMask = layout.modelMask();

    ObjCache cache;

    // =====================  MULTIPART MODE  ==========================
    if (multipart) {
        for (const MultipartDef& mp : def.parts) {
            Part part;

            std::vector<ModelVertex> verts;
            std::vector<Uint16> idx;
            if (!cache.get(mp.part, verts, idx)) return false;

            // material pick from the un-rotated normals
            std::vector<int> cells(verts.size());
            for (size_t i = 0; i < verts.size(); ++i)
                cells[i] = materialCellForNormal(verts[i].normal);

            if (mp.rotate % 90 != 0)
                SDL_Log("[BlockModel] '%s': multipart rotate %d is not a "
                        "multiple of 90 — rounding down", def.name.c_str(), mp.rotate);
            rotZSteps(verts, mp.rotate / 90);

            convertToBaked(verts, idx, cells, false,
                           topMaterial, bottomMaterial, sideMaterial, part.mesh);
            classifyBakedFaces(part.mesh);

            // ---- compile the "when" condition into (mask, value) ----
            if (!mp.whenProp.empty()) {
                int pi = layout.indexOf(mp.whenProp);
                if (pi < 0) {
                    SDL_Log("[BlockModel] '%s': multipart condition names unknown "
                            "property '%s'", def.name.c_str(), mp.whenProp.c_str());
                    return false;
                }
                const StateProperty& p = layout.property(pi);
                Uint16 fieldMask = (Uint16)(((1u << p.bitCount) - 1u) << p.bitOffset);

                auto sideBit = [](const std::string& s) -> int {
                    if (s == "north") return 0;
                    if (s == "east")  return 1;
                    if (s == "south") return 2;
                    if (s == "west")  return 3;
                    return -1;
                };

                switch (p.type) {
                case StatePropType::Connect4: {
                    int bit = sideBit(mp.whenValue);
                    if (bit < 0) {
                        SDL_Log("[BlockModel] '%s': bad connect4 value '%s'",
                                def.name.c_str(), mp.whenValue.c_str());
                        return false;
                    }
                    part.conditionMask = (Uint16)(1u << (p.bitOffset + bit));
                    part.conditionValue = part.conditionMask;
                    break;
                }
                case StatePropType::WallSide4: {
                    // "north:low" / "east:tall" / "south:none"
                    size_t colon = mp.whenValue.find(':');
                    int bit = colon == std::string::npos ? -1
                              : sideBit(mp.whenValue.substr(0, colon));
                    std::string h = colon == std::string::npos ? ""
                                    : mp.whenValue.substr(colon + 1);
                    int code = h == "none" ? 0 : h == "low" ? 1 : h == "tall" ? 2 : -1;
                    if (bit < 0 || code < 0) {
                        SDL_Log("[BlockModel] '%s': bad wallSide4 value '%s' "
                                "(expected e.g. 'north:low')",
                                def.name.c_str(), mp.whenValue.c_str());
                        return false;
                    }
                    part.conditionMask = (Uint16)(3u << (p.bitOffset + 2 * bit));
                    part.conditionValue = (Uint16)((Uint16)code << (p.bitOffset + 2 * bit));
                    break;
                }
                case StatePropType::Flag: {
                    part.conditionMask = (Uint16)(1u << p.bitOffset);
                    part.conditionValue = mp.whenValue == "on" ? part.conditionMask : 0;
                    break;
                }
                default: {
                    // enum equality: rot4/axis3/rot6/half/shape5
                    auto enumIndex = [&]() -> int {
                        const std::string& v = mp.whenValue;
                        switch (p.type) {
                        case StatePropType::Rot4:
                        case StatePropType::Rot6:
                            if (v == "north") return 0;
                            if (v == "east")  return 1;
                            if (v == "south") return 2;
                            if (v == "west")  return 3;
                            if (v == "up")    return 4;
                            if (v == "down")  return 5;
                            return -1;
                        case StatePropType::Axis3:
                            if (v == "z") return 0;
                            if (v == "x") return 1;
                            if (v == "y") return 2;
                            return -1;
                        case StatePropType::Half:
                            if (v == "bottom") return 0;
                            if (v == "top")    return 1;
                            return -1;
                        case StatePropType::Shape5:
                            for (int s = 0; s < 5; ++s)
                                if (v == shape5Name((Uint16)s)) return s;
                            return -1;
                        default: return -1;
                        }
                    }();
                    if (enumIndex < 0) {
                        SDL_Log("[BlockModel] '%s': bad condition value '%s' for "
                                "property '%s'", def.name.c_str(),
                                mp.whenValue.c_str(), mp.whenProp.c_str());
                        return false;
                    }
                    part.conditionMask = fieldMask;
                    part.conditionValue = (Uint16)((Uint16)enumIndex << p.bitOffset);
                    break;
                }
                }
            }

            parts.push_back(std::move(part));
        }
        return true;
    }

    // =====================  VARIANTS MODE  ===========================
    const int totalBits = layout.totalBits();
    if (totalBits > 12) {
        SDL_Log("[BlockModel] '%s': %d state bits would bake %d variants — "
                "use multipart for combinatorial states", def.name.c_str(),
                totalBits, 1 << totalBits);
        return false;
    }
    const int comboCount = 1 << totalBits;

    // Which properties drive which baked transform.
    const int rotateIdx = def.rotateBy.empty() ? -1 : layout.indexOf(def.rotateBy);
    const int flipIdx = def.flipZBy.empty() ? -1 : layout.indexOf(def.flipZBy);
    // shapeGeometry is driven by the (first) shape5 property, if any.
    int shapeIdx = -1;
    if (!def.shapeGeometry.empty())
        for (int i = 0; i < layout.propertyCount(); ++i)
            if (layout.property(i).type == StatePropType::Shape5) { shapeIdx = i; break; }

    variants.resize(comboCount);

    for (int combo = 0; combo < comboCount; ++combo) {
        Uint16 state = (Uint16)combo;

        // ---- pick geometry (alternate shape meshes slot in here) ----
        std::string file = def.geometry;
        if (shapeIdx >= 0) {
            const char* wanted = shape5Name(layout.get(state, shapeIdx));
            for (const auto& [shapeName, objFile] : def.shapeGeometry)
                if (shapeName == wanted) { file = objFile; break; }
            // shapes without a mesh yet (corners) fall back to the base
            // geometry, so corner OBJs slot in later without code changes
        }

        std::vector<ModelVertex> verts;
        std::vector<Uint16> idx;
        if (!cache.get(file, verts, idx)) return false;

        // material pick BEFORE any transform (see materialCellForNormal)
        std::vector<int> cells(verts.size());
        for (size_t i = 0; i < verts.size(); ++i)
            cells[i] = materialCellForNormal(verts[i].normal);

        // ---- Z-flip (half = top) ----
        bool flipped = false;
        if (flipIdx >= 0 && layout.get(state, flipIdx) != 0) {
            flipZ(verts);
            reverseWinding(idx);
            flipped = true;
        }

        // ---- rotation ----
        if (rotateIdx >= 0) {
            Uint16 value = layout.get(state, rotateIdx);
            switch (layout.property(rotateIdx).type) {
            case StatePropType::Rot4:
                rotZSteps(verts, facingToZSteps(value));
                break;
            case StatePropType::Axis3:
                // 0 = Z (authored orientation), 1 = X, 2 = Y
                if (value == 1)      rotYSteps(verts, 1);
                else if (value == 2) rotXSteps(verts, 1);
                break;
            case StatePropType::Rot6:
                // 0-3 horizontal like rot4; 4 = up (authored), 5 = down
                if (value <= 3) {
                    rotXSteps(verts, 3); // up -> north
                    rotZSteps(verts, facingToZSteps(value));
                }
                else if (value == 5) rotXSteps(verts, 2); // down
                break;
            default:
                SDL_Log("[BlockModel] '%s': rotateBy property '%s' has a "
                        "non-rotational type", def.name.c_str(), def.rotateBy.c_str());
                return false;
            }
        }

        convertToBaked(verts, idx, cells, flipped,
                       topMaterial, bottomMaterial, sideMaterial, variants[combo]);
        classifyBakedFaces(variants[combo]);
    }

    return true;
}

// ---------------------------------------------------------------------
static void appendGeometry(
    const std::vector<WorldVertex>& srcVertices,
    const std::vector<Uint16>& srcIndices,
    std::vector<WorldVertex>& outVertices,
    std::vector<Uint32>& outIndices,
    int x, int y, int z
) {
    if (srcIndices.empty()) return;

    // Indices are relative to where these vertices land in the buffer.
    const Uint32 base = static_cast<Uint32>(outVertices.size());

    for (const WorldVertex& v : srcVertices) {
        WorldVertex moved = v;
        moved.position.x += static_cast<float>(x);
        moved.position.y += static_cast<float>(y);
        moved.position.z += static_cast<float>(z);
        outVertices.push_back(moved);
    }
    for (const Uint16 index : srcIndices) {
        outIndices.push_back(base + index);
    }
}

// emit the always bucket plus only the VISIBLE boundary buckets.
static void appendBaked(
    const BakedMesh& mesh,
    std::vector<WorldVertex>& outVertices,
    std::vector<Uint32>& outIndices,
    int x, int y, int z, Uint8 visMask
) {
    appendGeometry(mesh.vertices, mesh.indices, outVertices, outIndices, x, y, z);
    for (int d = 0; d < 6; ++d)
        if (visMask & (1u << d))
            appendGeometry(mesh.boundaryFaces[d].vertices, mesh.boundaryFaces[d].indices,
                           outVertices, outIndices, x, y, z);
}

void BlockModel::getMesh(
    std::vector<WorldVertex>& outVertices,
    std::vector<Uint32>& outIndices,
    int x, int y, int z, Uint16 state, Uint8 visMask) const
{
    if (multipart) {
        // Emit every part whose condition matches. 2^n combos are assembled
        // here from a handful of baked parts — appends are cheap.
        for (const Part& p : parts) {
            if ((state & p.conditionMask) == p.conditionValue)
                appendBaked(p.mesh, outVertices, outIndices, x, y, z, visMask);
        }
        return;
    }

    // Variants: strip the fluid bit (and anything else above the template
    // bits) and index straight into the baked set.
    const Uint16 modelState = (Uint16)(state & modelMask);
    if (modelState < variants.size())
        appendBaked(variants[modelState], outVertices, outIndices, x, y, z, visMask);
    else if (!variants.empty())
        appendBaked(variants[0], outVertices, outIndices, x, y, z, visMask);
}

// which boundary planes this block's CURRENT variant fully covers
// (see BakedMesh::coverMask). Multipart models return 0 — their parts are
// conditional and none (fences, walls) have full-cell faces, so claiming no
// coverage keeps every neighbor face, exactly like today.
Uint8 BlockModel::getCoverMask(Uint16 state) const {
    if (multipart) return 0;
    const Uint16 modelState = (Uint16)(state & modelMask);
    if (modelState < variants.size()) return variants[modelState].coverMask;
    return variants.empty() ? 0 : variants[0].coverMask;
}

ModelFace BlockModel::getTopMaterial() {
    return topMaterial;
}
ModelFace BlockModel::getBottomMaterial() {
    return bottomMaterial;
}
ModelFace BlockModel::getSideMaterial() {
    return sideMaterial;
}
