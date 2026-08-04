#include "GenerateFeatures.h"

#include <cmath>
#include <cstdlib>

#include "Globals.h"
#include "BlockManager.h"
#include "Materials.h"
#include "WorldGenRandom.h"
#include "WorldGenRegistry.h"

// =====================================================================
//  Feature generation.
//
//  The old version hardcoded one meadow: grass over dirt, a 1-in-2 tree,
//  a 1-in-5 boulder, with the block names spelled out in C++. All of
//  that is now data — BiomeDef supplies the surface palettes and the
//  list of features, FeatureDef supplies each generator's blocks and
//  dimensions, and this file only knows how to BUILD the shapes.
//
//  Determinism: every roll comes from a stream seeded with
//  (worldSeed, columnX, columnY). rand() was both a data race across the
//  region workers and non-reproducible on reload — see WorldGenRandom.h.
// =====================================================================

namespace {

// Salts: each pass gets its own stream so adding a feature to a biome
// doesn't reshuffle the surface, and vice versa. The values are
// arbitrary — only their uniqueness matters.
const Uint64 SALT_SURFACE  = 0x5A9Full;
const Uint64 SALT_FEATURES = 0xFEA7ull;

// Region-local block coordinate -> biome map cell, with the apron offset
// applied. Mirrors Region::getBiomeIdLocal, which we can't call here:
// the worker only has the map, not the Region.
int cellIndex(int localBlock) {
    const int cell = (localBlock >= 0)
        ? (localBlock / BIOME_CELL)
        : ((localBlock - (BIOME_CELL - 1)) / BIOME_CELL);
    return cell + MAP_APRON_CELLS;
}

// Writes one block if the target is inside the column buffer.
inline void put(Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
                int x, int y, int z, Uint16 id) {
    if (x < 0 || x >= CHUNK_SIZE) return;
    if (y < 0 || y >= CHUNK_SIZE) return;
    if (z < 0 || z >= COLUMN_HEIGHT) return;
    blockIDs[x][y][z] = id;
}

// ---------------------------------------------------------------------
//  1) Surface cover — the biome's own blocks laid over the heightmap.
// ---------------------------------------------------------------------
void generateSurface(
    Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
    int regionChunkZStart,
    const float heightmap[CHUNK_SIZE][CHUNK_SIZE],
    const PalettedGrid2D& biomeMap,
    int localBlockX0, int localBlockY0,
    const WorldGenRegistry& registry,
    WorldGenRandom& rng
) {
    const int columnZStartBlocks = regionChunkZStart * CHUNK_SIZE;

    // getBiome() logs on an unknown id and this loop runs 256 times per
    // column — bail out once instead of flooding when no biomes loaded.
    if (registry.biomeCount() == 0)
        return;

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            const float zGround = heightmap[x][y] - columnZStartBlocks;
            if (zGround < 0 || zGround >= COLUMN_HEIGHT)
                continue;

            // Surface blocks are per CELL, not per column: a 16x16 chunk
            // spans 4x4 biome cells, so a biome border cuts through a
            // chunk exactly where the map says it does.
            const Uint16 biomeId = biomeMap.get(cellIndex(localBlockX0 + x),
                                                cellIndex(localBlockY0 + y));
            const BiomeDef* biome = registry.getBiome(biomeId);
            if (biome == nullptr) continue;

            // A surface Z landing on a half step gets the slab palette;
            // the shape spline's plateaus are authored to sit at >= .5 so
            // flat ground stays full blocks (see GenerateShape.cpp).
            const float fraction = zGround - floorf(zGround);
            const BlockPalette& top = (fraction >= 0.5f) ? biome->surface : biome->surfaceSlab;

            if (!top.empty())
                blockIDs[x][y][(int)zGround] = top.pick(rng.next32());

            if (!biome->filler.empty()) {
                for (int d = 1; d <= biome->fillerDepth; d++) {
                    const int z = (int)zGround - d;
                    if (z <= 0) break;
                    blockIDs[x][y][z] = biome->filler.pick(rng.next32());
                }
            }
        }
    }
}

// ---------------------------------------------------------------------
//  2) The feature generators. Each one builds its shape around an origin
//     that the caller has already rolled and range-checked.
// ---------------------------------------------------------------------

// Trunk of `height` blocks with a canopy blob on top. The canopy spans
// the top `canopyHeight` levels, its corners are cut, and its topmost
// level is pulled in one further so the crown reads as rounded.
void placeTree(
    Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
    const FeatureDef& def, int bx, int by, int zGround, WorldGenRandom& rng
) {
    const int height = rng.nextIntRange(def.minHeight, def.maxHeight);
    const int radius = def.radius;
    const int canopyBottom = height - def.canopyHeight + 1;

    const Uint16 logID = def.body.pick(rng.next32());
    const Uint16 leafID = def.foliage.pick(rng.next32());

    for (int dz = 0; dz <= height; dz++) {
        const bool inCanopy = dz >= canopyBottom;
        const bool topLevel = dz == height;

        for (int dx = -radius; dx <= radius; dx++) {
            for (int dy = -radius; dy <= radius; dy++) {
                const bool isTrunk = (dx == 0 && dy == 0);

                // Below the canopy only the trunk exists.
                if (!inCanopy && !isTrunk) continue;

                if (inCanopy && !isTrunk) {
                    // Cut the vertical corner columns, and pull the top
                    // level in by one more ring.
                    if (abs(dx) == radius && abs(dy) == radius) continue;
                    if (topLevel && (abs(dx) == radius || abs(dy) == radius)) continue;
                }

                // The trunk stops one short of the top so the crown
                // closes over it.
                const Uint16 id = (isTrunk && !topLevel) ? logID : leafID;
                put(blockIDs, bx + dx, by + dy, zGround + dz, id);
            }
        }
    }
}

// Rounded lump centred ON the surface, with an optional accent block
// (a flower, a crystal) sitting one above its top.
void placeBlob(
    Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
    const FeatureDef& def, int bx, int by, int zGround, WorldGenRandom& rng
) {
    const int radius = def.radius;

    for (int dz = -radius; dz <= radius; dz++) {
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dy = -radius; dy <= radius; dy++) {
                // Cut every corner of the cube — the three "two axes at
                // full radius" cases — to round it off.
                if (abs(dx) == radius && abs(dy) == radius) continue;
                if (abs(dz) == radius && (abs(dx) == radius || abs(dy) == radius)) continue;

                put(blockIDs, bx + dx, by + dy, zGround + dz, def.body.pick(rng.next32()));
            }
        }
    }

    if (!def.accent.empty() && rng.chance(def.accentChance))
        put(blockIDs, bx, by, zGround + radius + 1, def.accent.pick(rng.next32()));
}

// Loose blocks scattered around the origin, each dropped onto its own
// column's surface so they follow the terrain instead of floating.
void placeScatter(
    Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
    const FeatureDef& def, int bx, int by, int regionChunkZStart,
    const float heightmap[CHUNK_SIZE][CHUNK_SIZE], WorldGenRandom& rng
) {
    const int columnZStartBlocks = regionChunkZStart * CHUNK_SIZE;
    const int spread = def.radius;
    const int count = (2 * spread + 1) * (2 * spread + 1);

    for (int i = 0; i < count; i++) {
        const int x = bx + rng.nextIntRange(-spread, spread);
        const int y = by + rng.nextIntRange(-spread, spread);
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE) continue;

        const int zGround = (int)heightmap[x][y] - columnZStartBlocks;
        const int stack = rng.nextIntRange(def.minHeight, def.maxHeight);
        for (int dz = 1; dz <= stack; dz++)
            put(blockIDs, x, y, zGround + dz, def.body.pick(rng.next32()));
    }
}

// Dispatch one placement. Returns false when nothing was placed, so the
// caller can tell "rolled and skipped" from "rolled and built".
bool placeFeature(
    Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
    const FeatureDef& def,
    int regionChunkZStart,
    const float heightmap[CHUNK_SIZE][CHUNK_SIZE],
    WorldGenRandom& rng
) {
    const int columnZStartBlocks = regionChunkZStart * CHUNK_SIZE;

    // Roll an origin that keeps the whole footprint inside this chunk
    // where possible. Features still get clipped at chunk borders (put()
    // drops out-of-range writes) — placing across borders needs a
    // region-level pass, the same one structures are waiting on.
    const int margin = def.radius + 1;
    const int span = CHUNK_SIZE - 2 * margin;
    const int bx = (span > 0) ? margin + rng.nextInt(span) : CHUNK_SIZE / 2;
    const int by = (span > 0) ? margin + rng.nextInt(span) : CHUNK_SIZE / 2;

    // Absolute world Z of the ground under the origin, checked against
    // the feature's own placement window.
    const float surfaceAbs = heightmap[bx][by];
    if (surfaceAbs < (float)def.minZ || surfaceAbs > (float)def.maxZ)
        return false;

    const int zGround = (int)surfaceAbs - columnZStartBlocks;
    if (zGround < 0 || zGround >= COLUMN_HEIGHT)
        return false;

    switch (def.generator) {
    case FeatureGenerator::Tree:
        placeTree(blockIDs, def, bx, by, zGround, rng);
        return true;

    case FeatureGenerator::Blob:
        placeBlob(blockIDs, def, bx, by, zGround, rng);
        return true;

    case FeatureGenerator::Scatter:
        placeScatter(blockIDs, def, bx, by, regionChunkZStart, heightmap, rng);
        return true;

    case FeatureGenerator::Prefab:
        // Structures need a region-level placement pass (one building
        // spans many columns, and every column it touches has to agree
        // on where it sits) plus a prefab file format. Neither exists
        // yet; the data path down to here does, so wiring it up is the
        // only work left.
        return false;

    default:
        return false;
    }
}

} // namespace

// ---------------------------------------------------------------------
void generateFeatures(
    Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
    ColumnCoord columnCoordinates,
    ColumnCoord regionColumnStart,
    int regionChunkZStart,
    float heightmap[CHUNK_SIZE][CHUNK_SIZE],
    BlockManager& blockManager,
    const WorldGenRegistry& registry,
    const PalettedGrid2D& biomeMap,
    Uint64 worldSeed
) {
    // Region-local block coordinates of this column's (0, 0) corner —
    // what the biome map is indexed by.
    const int localBlockX0 = (columnCoordinates.x - regionColumnStart.x) * CHUNK_SIZE;
    const int localBlockY0 = (columnCoordinates.y - regionColumnStart.y) * CHUNK_SIZE;

    // ---- 1) surface cover ----
    WorldGenRandom surfaceRng(worldSeed, columnCoordinates.x, columnCoordinates.y, SALT_SURFACE);
    generateSurface(blockIDs, regionChunkZStart, heightmap, biomeMap,
                    localBlockX0, localBlockY0, registry, surfaceRng);

    // ---- 2) features ----
    // Feature rolls use the biome at the column's CENTRE. A feature is a
    // single object, so it belongs to one biome even when the column
    // straddles a border — unlike the surface, which is resolved per cell.
    const Uint16 biomeId = biomeMap.get(cellIndex(localBlockX0 + CHUNK_SIZE / 2),
                                        cellIndex(localBlockY0 + CHUNK_SIZE / 2));
    const BiomeDef* biome = (registry.biomeCount() > 0) ? registry.getBiome(biomeId) : nullptr;

    if (biome != nullptr) {
        WorldGenRandom rng(worldSeed, columnCoordinates.x, columnCoordinates.y, SALT_FEATURES);

        for (const BiomeFeature& entry : biome->features) {
            const FeatureDef* def = registry.getFeature(entry.featureId);
            if (def == nullptr) continue;

            // The biome's overrides win; a negative value means "inherit".
            const float chance = (entry.chance   >= 0.0f) ? entry.chance   : def->chance;
            const int attempts = (entry.attempts >= 0)    ? entry.attempts : def->attempts;

            for (int i = 0; i < attempts; i++) {
                if (!rng.chance(chance)) continue;
                placeFeature(blockIDs, *def, regionChunkZStart, heightmap, rng);
            }
        }

        // Structures are parsed, validated and carried on the biome, but
        // placing them needs the region-level pass described in
        // placeFeature(). Deliberately silent — logging per column would
        // flood, and the biome list is the honest record of intent.
    }

    // ---- 3) debug ----
    generateFeatures_BlockPallette(blockIDs, columnCoordinates, regionChunkZStart, blockManager);
}

void generateFeatures_BlockPallette(
    Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
    ColumnCoord columnCoordinates,
    int regionChunkZStart,
    BlockManager& blockManager
) {
    if (columnCoordinates.x < 0 || columnCoordinates.y != 0)
        return;

    const int z = 80 - regionChunkZStart * CHUNK_SIZE;
    if (z < 0 || z >= COLUMN_HEIGHT)
        return;

    int blocks = columnCoordinates.x * 256;   // last id written by the previous column

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            if (++blocks >= blockManager.getNumberOfBlocks())
                return;
            blockIDs[x][y][z] = (Uint16)blocks;
        }
    }
}
