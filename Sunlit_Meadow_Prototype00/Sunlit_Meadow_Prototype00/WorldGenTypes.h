#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>

#include "WorldTypes.h"
#include "PalettedGrid2D.h"
#include "Spline.h"

// =====================================================================
//  WorldGenTypes
//  Plain-data definitions for the layer / zone / biome hierarchy and the
//  features / structures a biome places.
//
//   - Layer: every region sharing the same RegionCoord.z belongs to one
//     layer. A layer owns the TERRAIN SHAPE: which shape generator runs,
//     its spline, its cave settings, its base block.
//   - Zone:  a large themed area (desert, volcanic, icy). Constrains
//     which biomes may appear inside it.
//   - Biome: owns everything that happens ON the shape — the surface
//     blocks and the list of features / structures that generate there.
//
//  These are pure definitions. The *storage* of "which zone/biome is at
//  cell (x, y)" lives in Region (PalettedGrid2D maps), the *decision*
//  lives in WorldGenSampler, and every def here is loaded from
//  Assets/WorldGen/**.json by WorldGenDefLoader and owned by
//  WorldGenRegistry (read-only after load — see WorldGenRegistry.h).
//
//  Ids are explicit in the JSON and must stay DENSE per type (0, 1,
//  2, ...): the registry stores each kind in a vector indexed by id.
// =====================================================================

// ---------------------------------------------------------------------
//  Block palettes
//
//  Every block a def refers to is authored as a NAME; the registry
//  resolves the names to block ids ONCE at load (BlockManager is fully
//  populated by then — App_Init runs blockManager.init() before
//  worldManager.init()). Resolving up front keeps the per-column
//  generators off the name->Block hash lookup entirely.
// ---------------------------------------------------------------------
struct BlockPick {
    std::string name;         // as authored in JSON
    Uint16      id = 0;       // resolved at load; unknown names are dropped
    int         weight = 1;   // relative pick weight inside its palette
};

// A weighted list of block choices. An EMPTY palette means "this
// generator has nothing to place here" — generators must skip, never
// fall back to id 0 (that would punch air into the terrain).
struct BlockPalette {
    std::vector<BlockPick> picks;
    int totalWeight = 0;      // sum of picks[].weight, recomputed by the loader

    bool empty() const { return picks.empty(); }

    // Weighted pick. `roll` is any random 32-bit value — the caller owns
    // the RNG so the choice stays reproducible (see WorldGenRandom.h).
    Uint16 pick(Uint32 roll) const {
        if (picks.empty()) return 0;
        if (totalWeight <= 0) return picks[0].id;
        int r = (int)(roll % (Uint32)totalWeight);
        for (const BlockPick& p : picks) {
            r -= p.weight;
            if (r < 0) return p.id;
        }
        return picks.back().id;   // only reachable on a weight/total mismatch
    }
};

// ---------------------------------------------------------------------
//  Shape generators — layer level.
//  Adding one: add an enum value, a row in the name table
//  (WorldGenTypes.cpp), and a case in generateShape().
// ---------------------------------------------------------------------
enum class ShapeGenerator : Uint16 {
    Invalid = 0,
    SplineTerrain,   // control noise -> spline -> surface Z, then a 3D cave carve
    SolidFill,       // whole column = baseBlock (the default below-world layer)
    AirFill,         // whole column = air       (the default above-world layer)
};

ShapeGenerator shapeGeneratorFromName(const std::string& name);  // Invalid on unknown
const char*    shapeGeneratorName(ShapeGenerator g);

// ---------------------------------------------------------------------
//  Features & structures — biome level.
// ---------------------------------------------------------------------

// Feature vs. structure is about WHEN placement runs, not how big the
// thing is: features are rolled per chunk column during column
// generation, structures need a region-level placement pass so one
// building can span many columns. Biomes list them separately.
enum class FeatureKind : Uint8 {
    Feature,
    Structure,
};

FeatureKind kindFromName(const std::string& name);   // defaults to Feature
const char* kindName(FeatureKind k);

// Which C++ generator builds the thing. Adding one: add an enum value,
// a row in the name table, and a case in placeFeature()
// (GenerateFeatures.cpp).
enum class FeatureGenerator : Uint16 {
    Invalid = 0,
    Scatter,   // loose blocks strewn on the surface (flowers, tall grass)
    Tree,      // vertical trunk + a canopy blob on top
    Blob,      // rounded lump of one block (boulders, ore pockets)
    Prefab,    // structures: stamp a saved block layout — NOT IMPLEMENTED YET
};

FeatureGenerator featureGeneratorFromName(const std::string& name);  // Invalid on unknown
const char*      featureGeneratorName(FeatureGenerator g);

// One feature / structure definition. Flat on purpose (same shape as
// BlockDef): which fields a generator reads is documented per field, and
// the ones it doesn't read simply stay at their defaults.
struct FeatureDef {
    Uint16           id = 0;
    std::string      name;
    FeatureKind      kind      = FeatureKind::Feature;
    FeatureGenerator generator = FeatureGenerator::Invalid;

    // ---- default placement rate (a biome entry may override both) ----
    float chance   = 1.0f;   // probability that ONE attempt places anything, 0..1
    int   attempts = 1;      // attempts per chunk column

    // ---- where it may sit: ABSOLUTE world Z of the surface it lands on --
    int   minZ = 0;
    int   maxZ = 1 << 20;

    // ---- size, in blocks ----
    //   Tree    : minHeight/maxHeight = trunk height, radius = canopy
    //             radius, canopyHeight = how many levels the canopy spans
    //   Blob    : radius = lump radius (heights unused)
    //   Scatter : minHeight/maxHeight = stack height, radius = how far
    //             from the rolled origin the blocks may land
    int   minHeight    = 1;
    int   maxHeight    = 1;
    int   radius       = 1;
    int   canopyHeight = 1;

    // ---- block palettes ----
    //   Tree    : body = log,   foliage = leaves, accent unused
    //   Blob    : body = lump,  foliage unused,   accent sits on top
    //   Scatter : body = block, foliage unused,   accent unused
    BlockPalette body;
    BlockPalette foliage;
    BlockPalette accent;
    float        accentChance = 0.0f;   // 0..1 probability the accent is placed

    // ---- Prefab only ----
    std::string prefab;   // file under Assets/WorldGen/Prefabs (unimplemented)
};

// One entry in a biome's feature / structure list: WHICH feature, and
// how often it runs HERE. The overrides are the point — they let a
// single "chestnut_tree" def be dense in a forest and rare in a meadow
// without duplicating the def.
struct BiomeFeature {
    std::string featureName;       // as authored
    Uint16      featureId = 0;     // resolved at load
    float       chance   = -1.0f;  // < 0 -> inherit FeatureDef::chance
    int         attempts = -1;     // < 0 -> inherit FeatureDef::attempts
};

// ---------------------------------------------------------------------
//  The hierarchy
// ---------------------------------------------------------------------
struct LayerDef {
    Uint16              id = 0;          // same as RegionCoord.z
    std::string         name;
    std::vector<Uint16> allowedZoneIds;  // from "allowedZones": [names]

    // ---- terrain shape ----
    ShapeGenerator shape = ShapeGenerator::AirFill;
    BlockPick      baseBlock;            // what SplineTerrain / SolidFill fill with
    Spline         shapeSpline;          // control noise (~[-1, 1]) -> absolute world Z
    float          seaLevel    = 64.0f;  // no water yet; basins dip below it on purpose
    float          minSurfaceZ = 10.0f;  // hard clamp on the spline output — guards
    float          maxSurfaceZ = (float)(COLUMN_HEIGHT - 10);  // future knot edits

    // ---- caves (SplineTerrain only) ----
    // Carved where |caveField| < threshold — a BAND around zero, because
    // the zero-level-set of 3D noise is a connected sheet (wormy tunnels);
    // "value > t" would give isolated blobs instead.
    bool  caves             = false;
    float caveThreshold     = 0.06f;
    float caveSurfaceTaper  = 8.0f;   // blocks below the surface the taper spans
    float caveTaperMinScale = 0.35f;  // threshold scale AT the surface — deliberately
                                      // not 0, so cave mouths stay possible but rare
    int   caveFloorZ        = 1;                  // never carve below
    int   caveCeilZ         = COLUMN_HEIGHT - 10; // never carve above

    // ---- zone selection ----
    float zoneScale = 1.0f;   // > 1 = larger zones (see WorldGenSampler)
};

struct ZoneDef {
    Uint16              id = 0;
    std::string         name;
    std::vector<Uint16> allowedBiomeIds;   // from "allowedBiomes": [names]

    float weight     = 1.0f;   // relative pick weight among its layer's zones
    float biomeScale = 1.0f;   // > 1 = larger biomes inside this zone
};

struct BiomeDef {
    Uint16      id = 0;
    std::string name;
    Uint16      zoneId = 0;    // owning zone, from "zone": <name>

    // ---- selection, inside the owning zone ----
    float weight      = 1.0f;  // relative pick weight among the zone's biomes
    float temperature = 0.5f;  // 0..1 climate coordinates. Reserved for the
    float humidity    = 0.5f;  // climate-noise sampler (see WorldGenSampler.cpp).

    // ---- terrain tweaks on top of the layer's shape ----
    // NOT APPLIED YET: a raw per-cell offset would cut a vertical wall at
    // every biome border. Whoever wires these up owes the surface pass a
    // blend across the biome map first (weighted average over the
    // neighbouring cells), which is exactly why the region maps carry an
    // apron (WorldGenSampler.h).
    float heightOffset = 0.0f; // blocks added to the layer's surface Z
    float heightScale  = 1.0f; // scales the surface's deviation from seaLevel

    // ---- surface cover (runs before any feature) ----
    BlockPalette surface;      // the block AT the surface Z
    BlockPalette surfaceSlab;  // used where the surface Z lands on a half step
    BlockPalette filler;       // the layers directly beneath the surface
    int          fillerDepth = 3;

    // ---- what generates here ----
    std::vector<BiomeFeature> features;     // rolled per chunk column
    std::vector<BiomeFeature> structures;   // region-level pass (unimplemented)
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
