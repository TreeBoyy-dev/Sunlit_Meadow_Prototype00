#pragma once
#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "WorldGenTypes.h"

// =====================================================================
//  WorldGenDefLoader
//
//  Reads the worldgen definition tree — one file per def:
//
//      Assets/WorldGen/Layers/*.json     -> LayerDef
//      Assets/WorldGen/Zones/*.json      -> ZoneDef
//      Assets/WorldGen/Biomes/*.json     -> BiomeDef
//      Assets/WorldGen/Features/*.json   -> FeatureDef  (features AND structures)
//
//  Cross-references are authored by NAME ("zone": "temperate",
//  "allowedBiomes": [...], "features": [{ "feature": "birch_tree" }])
//  and resolved to ids in a second pass, so file load order never
//  matters and a typo is a logged error instead of a silent id shift.
//  BLOCK names are deliberately NOT resolved here — that needs the
//  BlockManager and happens in WorldGenRegistry::init.
//
//  Validation follows BlockDefLoader's rule: log and skip the offending
//  def, never crash. Loading is all-or-nothing only if the directories
//  themselves are missing.
//
//  Ids are explicit and mandatory in every file, and must be dense per
//  type (0, 1, 2, ...) because the registry indexes vectors by id.
//  Gaps and duplicates are reported here.
// =====================================================================

class WorldGenDefLoader {
public:
    // worldGenDir is the absolute path of the Assets/WorldGen folder.
    // Each `out` vector comes back sorted by id. Returns false only when
    // the directory tree is unusable (missing dirs, or no layers at all).
    bool loadAll(
        const std::string&       worldGenDir,
        std::vector<LayerDef>&   outLayers,
        std::vector<ZoneDef>&    outZones,
        std::vector<BiomeDef>&   outBiomes,
        std::vector<FeatureDef>& outFeatures
    );
};
