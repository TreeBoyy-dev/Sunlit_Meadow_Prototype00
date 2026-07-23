#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>

#include "WorldTypes.h"
#include "PalettedGrid2D.h"

// =====================================================================
//  WorldGenTypes
//  Plain-data definitions for the layer / zone / biome hierarchy.
//
//   - Layer: every region sharing the same RegionCoord.z belongs to one
//     layer. A layer will later select an entirely distinct terrain
//     generator (surface meadow vs. underground vs. ...).
//   - Zone:  a large themed area (desert, volcanic, icy). Constrains
//     which biomes may appear inside it.
//   - Biome: determines most terrain features; always belongs to a zone.
//
//  These are pure definitions — the *storage* of "which zone/biome is
//  at cell (x, y)" lives in Region (PalettedGrid2D maps), and the
//  *decision* lives in WorldGenSampler.
// =====================================================================

struct LayerDef {
    Uint16              id;
    std::string         name;
    std::vector<Uint16> allowedZoneIds;
};

struct ZoneDef {
    Uint16              id;
    std::string         name;
    std::vector<Uint16> allowedBiomeIds;
};

struct BiomeDef {
    Uint16      id;
    std::string name;
    Uint16      zoneId;   // owning zone
};

// ---------------------------------------------------------------------
//  Map resolution. Both the zone map and the biome map use 4x4-block
//  cells (same resolution). Each region stores its own maps plus a one
//  chunk apron on every side, so feature generation near the region
//  edge can look "past" the edge without ever touching a neighbor
//  region (the apron is recomputed from the same pure sampler, so
//  neighbors always agree — see WorldGenSampler.h).
// ---------------------------------------------------------------------
#define BIOME_CELL 4                                        // blocks per map cell edge (both maps)
#define MAP_CELLS_PER_REGION ((CHUNK_SIZE * REGION_SIZE_YX) / BIOME_CELL)  // 128
#define MAP_APRON_CELLS (CHUNK_SIZE / BIOME_CELL)           // 4 cells = 1 chunk margin per side
#define MAP_GRID_SIZE (MAP_CELLS_PER_REGION + 2 * MAP_APRON_CELLS)         // 136

struct RegionShape {
    const LayerDef* m_layer;
    PalettedGrid2D  zoneMap{ MAP_GRID_SIZE };
    PalettedGrid2D  biomeMap{ MAP_GRID_SIZE };
};