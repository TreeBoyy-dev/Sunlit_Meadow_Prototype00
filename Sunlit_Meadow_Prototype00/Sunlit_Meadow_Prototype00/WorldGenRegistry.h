#pragma once
#include <SDL3/SDL.h>
#include <vector>

#include "WorldGenTypes.h"

// =====================================================================
//  WorldGenRegistry
//  Owns every LayerDef / ZoneDef / BiomeDef. Owned by WorldManager as a
//  plain member, populated once in WorldManager::init and READ-ONLY
//  afterwards — that immutability is what lets regions and the
//  generator worker hold raw const pointers into it without locking
//  (same ownership style as BlockManager's registry-owned block defs).
//
//  Defs are stored in vectors indexed by id, so ids MUST stay dense
//  (0, 1, 2, ...). That's trivially true for the hardcoded stubs; when
//  defs eventually move to JSON the loader has to keep the guarantee
//  (or this indexing switches to a map).
//
//  Hardcoded C++ stubs for now — no JSON, no file loading. Exactly one
//  layer / zone / biome exists, so every lookup lands on "meadow" /
//  "temperate" / "sunlit_meadow".
// =====================================================================

class WorldGenRegistry {
private:
    std::vector<LayerDef> layers;
    std::vector<ZoneDef>  zones;
    std::vector<BiomeDef> biomes;

public:
    void init();

    // Stub: always the meadow layer. Kept as a real mapping function (not
    // a constant) so a regionZ -> layer table can replace the body later
    // without touching any call site.
    const LayerDef* getLayerForRegionZ(int regionZ) const;

    const ZoneDef*  getZone (Uint16 id) const;   // nullptr + SDL_Log on unknown id
    const BiomeDef* getBiome(Uint16 id) const;   // nullptr + SDL_Log on unknown id
};
