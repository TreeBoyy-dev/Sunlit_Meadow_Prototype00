#include "WorldGenRegistry.h"

void WorldGenRegistry::init() {
    layers.clear();
    zones.clear();
    biomes.clear();

    // Hardcoded stub content. Ids must stay dense (vector index == id).
    layers.push_back(LayerDef{ 0, "meadow",        { 0 } });   // allowedZoneIds
    zones .push_back(ZoneDef { 0, "temperate",     { 0 } });   // allowedBiomeIds
    biomes.push_back(BiomeDef{ 0, "sunlit_meadow",   0   });   // zoneId
}

const LayerDef* WorldGenRegistry::getLayerForRegionZ(int regionZ) const {
    // Stub: every region z is the meadow layer. Later this becomes a
    // lookup table (e.g. z < 0 -> underground layer), which is why the
    // parameter already exists.
    (void)regionZ;
    return &layers[0];
}

const ZoneDef* WorldGenRegistry::getZone(Uint16 id) const {
    if (id >= zones.size()) {
        SDL_Log("[WorldGenRegistry] unknown zone id: %u", id);
        return nullptr;
    }
    return &zones[id];
}

const BiomeDef* WorldGenRegistry::getBiome(Uint16 id) const {
    if (id >= biomes.size()) {
        SDL_Log("[WorldGenRegistry] unknown biome id: %u", id);
        return nullptr;
    }
    return &biomes[id];
}
