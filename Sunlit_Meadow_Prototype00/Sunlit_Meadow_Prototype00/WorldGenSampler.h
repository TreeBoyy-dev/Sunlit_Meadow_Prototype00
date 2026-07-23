#pragma once
#include <SDL3/SDL.h>

#include "WorldGenTypes.h"

// =====================================================================
//  WorldGenSampler
//  Pure, deterministic functions of (world cell coords, layer, seed).
//
//  The zone/biome maps stored per region are a CACHE, not the source of
//  truth — these functions are. Because two neighboring regions
//  evaluating the same world cell always get the same answer, each
//  region can fill its apron cells (the ones past its own edge) by just
//  calling the sampler again. No neighbor-region access, no ordering
//  dependency, no seams — the apron is valid by construction.
//
//  worldCellX/Y are in CELL units (world block / BIOME_CELL), so the
//  functions are region-agnostic.
// =====================================================================

Uint16 sampleZoneId (int worldCellX, int worldCellY, const LayerDef& layer, Uint64 worldSeed);
Uint16 sampleBiomeId(int worldCellX, int worldCellY, const LayerDef& layer, Uint64 worldSeed,
                     Uint16 zoneId /* result of sampleZoneId for this cell */);
