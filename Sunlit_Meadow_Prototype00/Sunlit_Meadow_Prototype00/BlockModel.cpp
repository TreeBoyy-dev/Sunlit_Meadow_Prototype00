#include "BlockModel.h"
#include "Materials.h"
#include "ObjParser.h"

#include <unordered_map>

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
    }

    return true;
}

// ---------------------------------------------------------------------
static void appendBaked(
    const BakedMesh& mesh,
    std::vector<WorldVertex>& outVertices,
    std::vector<Uint32>& outIndices,
    int x, int y, int z
) {
    // Indices are relative to where this mesh's vertices land in the buffer.
    const Uint32 base = static_cast<Uint32>(outVertices.size());

    for (const WorldVertex& v : mesh.vertices) {
        WorldVertex moved = v;
        moved.position.x += static_cast<float>(x);
        moved.position.y += static_cast<float>(y);
        moved.position.z += static_cast<float>(z);
        outVertices.push_back(moved);
    }
    for (const Uint16 index : mesh.indices) {
        outIndices.push_back(base + index);
    }
}

void BlockModel::getMesh(
    std::vector<WorldVertex>& outVertices,
    std::vector<Uint32>& outIndices,
    int x, int y, int z, Uint16 state) const
{
    if (multipart) {
        // Emit every part whose condition matches. 2^n combos are assembled
        // here from a handful of baked parts — appends are cheap.
        for (const Part& p : parts) {
            if ((state & p.conditionMask) == p.conditionValue)
                appendBaked(p.mesh, outVertices, outIndices, x, y, z);
        }
        return;
    }

    // Variants: strip the fluid bit (and anything else above the template
    // bits) and index straight into the baked set.
    const Uint16 modelState = (Uint16)(state & modelMask);
    if (modelState < variants.size())
        appendBaked(variants[modelState], outVertices, outIndices, x, y, z);
    else if (!variants.empty())
        appendBaked(variants[0], outVertices, outIndices, x, y, z);
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
