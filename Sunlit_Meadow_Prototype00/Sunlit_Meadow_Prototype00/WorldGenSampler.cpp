#include "WorldGenSampler.h"

// TODO worldgen: replace both placeholder bodies with seeded-noise picks —
// sampleZoneId chooses from layer.allowedZoneIds, sampleBiomeId from
// zone.allowedBiomeIds (of the zone passed in). Must stay pure functions of
// the parameters only (no globals, no region state), or the apron guarantee
// in WorldGenSampler.h breaks.

Uint16 sampleZoneId(int worldCellX, int worldCellY, const LayerDef& layer, Uint64 worldSeed) {
    (void)worldCellX; (void)worldCellY; (void)layer; (void)worldSeed;
    return 0;   // placeholder: the single default zone, everywhere
}

Uint16 sampleBiomeId(int worldCellX, int worldCellY, const LayerDef& layer, Uint64 worldSeed,
                     Uint16 zoneId) {
    (void)worldCellX; (void)worldCellY; (void)layer; (void)worldSeed; (void)zoneId;
    return 0;   // placeholder: the single default biome, everywhere
}
