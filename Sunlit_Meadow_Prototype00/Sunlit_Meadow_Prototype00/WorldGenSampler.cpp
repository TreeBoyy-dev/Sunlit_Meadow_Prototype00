#include "WorldGenSampler.h"

#include <vector>

#include "WorldGenRandom.h"
#include "WorldGenRegistry.h"

// =====================================================================
//  Zone / biome selection.
//
//  Both picks are jittered-grid Voronoi: lay a coarse grid over the
//  world, put one randomly offset SITE inside each grid cell, and give
//  every map cell the id of its nearest site. The jitter is what makes
//  the borders irregular — a plain grid pick would tile the world with
//  visible squares. Each site draws its id from a WEIGHTED pick over the
//  allowed list, so a zone/biome's `weight` controls how much of the
//  world it covers.
//
//  This is deliberately not climate-based yet. BiomeDef carries
//  temperature/humidity for that: the eventual version samples two
//  low-frequency noise fields and picks the allowed biome whose
//  (temperature, humidity) is closest, which makes neighbouring biomes
//  related instead of random. Swapping that in only touches pickSite()
//  below — the purity contract above and every call site stay as-is.
// =====================================================================

namespace {

// Grid pitch in MAP CELLS (1 cell = BIOME_CELL blocks) at scale 1.0.
// 256 cells = 1024 blocks = a 2x2-region zone; 64 cells = 256 blocks
// for biomes inside it.
const int ZONE_GRID_CELLS  = 256;
const int BIOME_GRID_CELLS = 64;

// Salts keep the zone grid, the biome grid and their id rolls from
// marching in lockstep off the same coordinates.
const Uint64 SALT_ZONE_SITE  = 0x5A0Eull;
const Uint64 SALT_ZONE_ID    = 0x5A1Dull;
const Uint64 SALT_BIOME_SITE = 0xB10Eull;
const Uint64 SALT_BIOME_ID   = 0xB11Dull;

// Floor division — worldCellX goes negative, and C++ truncates toward
// zero, which would mirror the grid around the origin.
int floorDiv(int a, int b) {
    return (a >= 0) ? (a / b) : -(((-a) + b - 1) / b);
}

// The grid cell whose jittered site point is nearest to (cx, cy).
void nearestSite(int cx, int cy, int grid, Uint64 worldSeed, Uint64 salt,
                 int& outGX, int& outGY) {
    const int baseGX = floorDiv(cx, grid);
    const int baseGY = floorDiv(cy, grid);

    Sint64 bestDist = -1;
    outGX = baseGX;
    outGY = baseGY;

    // 3x3 is enough: a site never leaves its own grid cell, so nothing
    // outside the ring can be closer than the nearest of these nine.
    for (int dgx = -1; dgx <= 1; dgx++) {
        for (int dgy = -1; dgy <= 1; dgy++) {
            const int gx = baseGX + dgx;
            const int gy = baseGY + dgy;

            const Uint64 h = wgHash2D(worldSeed, gx, gy, salt);
            const int siteX = gx * grid + (int)(h % (Uint64)grid);
            const int siteY = gy * grid + (int)((h >> 32) % (Uint64)grid);

            const Sint64 dx = (Sint64)siteX - cx;
            const Sint64 dy = (Sint64)siteY - cy;
            const Sint64 dist = dx * dx + dy * dy;

            if (bestDist < 0 || dist < bestDist) {
                bestDist = dist;
                outGX = gx;
                outGY = gy;
            }
        }
    }
}

// Weighted pick over `ids`, using `weightOf` to score each one.
template <typename WeightFn>
Uint16 weightedPick(const std::vector<Uint16>& ids, WeightFn weightOf, Uint64 hash) {
    if (ids.empty()) return 0;
    if (ids.size() == 1) return ids[0];

    float total = 0.0f;
    for (Uint16 id : ids) {
        const float w = weightOf(id);
        if (w > 0.0f) total += w;
    }
    if (total <= 0.0f) return ids[0];

    // [0, 1) from the top 24 bits — the same conversion WorldGenRandom
    // uses, so both agree on what "a roll" means.
    const float roll = (float)((Uint32)(hash >> 40)) * (1.0f / 16777216.0f) * total;

    float acc = 0.0f;
    for (Uint16 id : ids) {
        const float w = weightOf(id);
        if (w <= 0.0f) continue;
        acc += w;
        if (roll < acc) return id;
    }
    return ids.back();
}

int scaledGrid(int base, float scale) {
    const int g = (int)(base * (scale > 0.0f ? scale : 1.0f));
    return g > 0 ? g : 1;
}

} // namespace

Uint16 sampleZoneId(int worldCellX, int worldCellY, const LayerDef& layer,
                    Uint64 worldSeed, const WorldGenRegistry& registry) {
    if (layer.allowedZoneIds.empty())
        return 0;   // layer allows nothing — the default zone covers it

    const int grid = scaledGrid(ZONE_GRID_CELLS, layer.zoneScale);

    int gx = 0, gy = 0;
    nearestSite(worldCellX, worldCellY, grid, worldSeed, SALT_ZONE_SITE, gx, gy);

    return weightedPick(
        layer.allowedZoneIds,
        [&registry](Uint16 id) {
            const ZoneDef* zone = registry.getZone(id);
            return zone ? zone->weight : 0.0f;
        },
        wgHash2D(worldSeed, gx, gy, SALT_ZONE_ID));
}

Uint16 sampleBiomeId(int worldCellX, int worldCellY, const LayerDef& layer,
                     Uint64 worldSeed, Uint16 zoneId,
                     const WorldGenRegistry& registry) {
    (void)layer;   // the biome grid is a property of the zone, not the layer

    // Guard before the lookup: getZone() logs on an unknown id, and this
    // runs once per map cell (~18k per region) — a world with no zone
    // defs would drown the log.
    if (zoneId >= registry.zoneCount())
        return 0;

    const ZoneDef* zone = registry.getZone(zoneId);
    if (zone == nullptr || zone->allowedBiomeIds.empty())
        return 0;

    const int grid = scaledGrid(BIOME_GRID_CELLS, zone->biomeScale);

    // The site salt is mixed with the zone id so two zones that happen to
    // share a border don't line their biome patches up across it.
    int gx = 0, gy = 0;
    nearestSite(worldCellX, worldCellY, grid, worldSeed,
                SALT_BIOME_SITE + zoneId, gx, gy);

    return weightedPick(
        zone->allowedBiomeIds,
        [&registry](Uint16 id) {
            const BiomeDef* biome = registry.getBiome(id);
            return biome ? biome->weight : 0.0f;
        },
        wgHash2D(worldSeed, gx, gy, SALT_BIOME_ID + zoneId));
}
