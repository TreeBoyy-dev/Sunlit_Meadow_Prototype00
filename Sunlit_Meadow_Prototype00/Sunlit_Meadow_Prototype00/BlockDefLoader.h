#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "Materials.h"
#include "StateLayout.h"
#include "WorldTypes.h"

// =====================================================================
//  BlockDefLoader
//
//  Reads Assets/BlockTemplates/*.json, then Assets/Blocks/*.json (one file
//  per block), resolves each block against its template in memory, and
//  produces plain BlockDef structs the BlockManager consumes.
//
//  Validation (log + skip the block, never crash):
//   - unknown template name
//   - duplicate explicit id
//   - state bits overflowing past bit 14 (bit 15 = fluid)
//   - unknown material name
//  ID gaps produce a warning in BlockManager (they'd break ItemManager's
//  contiguous 1..N placeable-item loop), not an error here.
// =====================================================================

// One declared state property, e.g. { "name": "facing", "type": "rot4" }.
struct StatePropDef {
    std::string   name;
    StatePropType type = StatePropType::Invalid;
};

// One conditional part of a multipart model (fences, walls).
struct MultipartDef {
    std::string part;       // .obj file
    std::string whenProp;   // property name; "" = unconditional part
    std::string whenValue;  // meaning depends on the property type:
                            //   connect4 : "north"/"east"/"south"/"west" (bit set?)
                            //   wallSide4: "north:low", "east:tall", ...
                            //   flag     : "on"/"off"
                            //   enums    : value name equality ("east", "top", ...)
    int rotate = 0;         // baked rotation about Z in degrees (0/90/180/270)
};

// Fully resolved block definition = block JSON merged over its template.
struct BlockDef {
    Uint16      id = 0;
    std::string name;
    std::string templateName;

    std::vector<StatePropDef> states;   // declaration order = bit order

    // ---- variants mode (one mesh chosen/transformed by state) ----
    std::string geometry;               // base .obj (block JSON may override)
    std::string rotateBy;               // property driving baked Z-rotation ("" = none)
    std::string flipZBy;                // property driving baked Z-flip     ("" = none)
    std::vector<std::pair<std::string, std::string>> shapeGeometry; // shape value -> .obj

    // ---- multipart mode (mesh assembled from conditional parts) ----
    bool multipart = false;
    std::vector<MultipartDef> parts;

    // ---- materials ----
    ModelFace top{ MATERIAL_AIR };
    ModelFace bottom{ MATERIAL_AIR };
    ModelFace side{ MATERIAL_AIR };

    std::array<bool, 6> obstructs{ false, false, false, false, false, false };
    bool      transparent = false;
    Collision collision;
};

class BlockDefLoader {
public:
    // Scans the two directories (absolute paths) and fills `out`, sorted by
    // id. Returns false only if the directories themselves are unreadable;
    // individual bad files are logged and skipped.
    bool loadAll(
        const std::string& templatesDir,
        const std::string& blocksDir,
        std::vector<BlockDef>& out
    );
};
