#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "WorldGenTypes.h"

class BlockManager;

// =====================================================================
//  WorldGenRegistry
//  Owns every LayerDef / ZoneDef / BiomeDef / FeatureDef. Owned by
//  WorldManager as a plain member, populated once in WorldManager::init
//  from Assets/WorldGen/**.json and READ-ONLY afterwards — that
//  immutability is what lets regions and the generator workers hold raw
//  const pointers into it without locking (same ownership style as
//  BlockManager's registry-owned block defs).
//
//  Defs are stored in vectors indexed by id, so ids MUST stay dense
//  (0, 1, 2, ...). WorldGenDefLoader validates that and logs any gap.
//
//  Two layers are NOT loaded from JSON: the built-in fallbacks returned
//  for a region z no layer file claims. Below the defined layers the
//  world is solid, above it is empty — cheap, and it keeps every caller
//  free of null checks (getLayerForRegionZ never returns nullptr).
// =====================================================================

class WorldGenRegistry {
private:
    std::vector<LayerDef>   layers;
    std::vector<ZoneDef>    zones;
    std::vector<BiomeDef>   biomes;
    std::vector<FeatureDef> features;

    std::unordered_map<std::string, Uint16> layerByName;
    std::unordered_map<std::string, Uint16> zoneByName;
    std::unordered_map<std::string, Uint16> biomeByName;
    std::unordered_map<std::string, Uint16> featureByName;

    // Fallback layers for undefined region z (see the header comment).
    LayerDef defaultAbove;   // z above the defined layers -> all air
    LayerDef defaultBelow;   // z below                    -> all baseBlock

    bool loaded = false;

public:
    // Loads every def, then resolves the block NAMES in each palette to
    // block ids. blockManager must already be init'd (App_Init does that
    // first). Returns false if the def tree could not be loaded at all —
    // the fallback layers still work, so the world stays generatable.
    bool init(BlockManager* blockManager);

    // Layer for a region z. Never null: unclaimed z values get one of the
    // two built-in fallbacks.
    const LayerDef* getLayerForRegionZ(int regionZ) const;

    // Id lookups. nullptr + SDL_Log on an unknown id.
    const LayerDef*   getLayer  (Uint16 id) const;
    const ZoneDef*    getZone   (Uint16 id) const;
    const BiomeDef*   getBiome  (Uint16 id) const;
    const FeatureDef* getFeature(Uint16 id) const;

    // Name lookups. nullptr on unknown, WITHOUT logging — callers use
    // these to test for existence.
    const LayerDef*   findLayer  (const std::string& name) const;
    const ZoneDef*    findZone   (const std::string& name) const;
    const BiomeDef*   findBiome  (const std::string& name) const;
    const FeatureDef* findFeature(const std::string& name) const;

    size_t layerCount()   const { return layers.size(); }
    size_t zoneCount()    const { return zones.size(); }
    size_t biomeCount()   const { return biomes.size(); }
    size_t featureCount() const { return features.size(); }

private:
    // Drops picks whose block name BlockManager doesn't know (a bad name
    // would otherwise resolve to id 0 = air and quietly punch holes in
    // the terrain) and recomputes totalWeight.
    void resolvePalette(BlockPalette& palette, BlockManager& blockManager,
                        const char* owner, const char* field);
    void resolvePick(BlockPick& pick, BlockManager& blockManager,
                     const char* owner, const char* field);
    void buildNameIndexes();
    void buildFallbackLayers();
};
