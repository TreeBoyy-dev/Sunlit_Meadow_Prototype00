#pragma once

#include <SDL3/SDL.h>
#include "FastNoiseLite.h"

// =====================================================================
//  WorldGenNoise
//  The configured bundle of noise fields shape generation samples from.
//
//  Owned by WorldManager as a plain member, init()'d ONCE from
//  m_worldSeed, and strictly read-only afterwards. That immutability is
//  the whole thread-safety story: FastNoiseLite::GetNoise() is const
//  and touches no mutable state, so every ChunkGeneratorWorker thread
//  can sample the same instance lock-free — the exact contract the
//  zone/biome maps already rely on (immutable after build, owner
//  outlives every reader; see Region.h / WorldGenRegistry.h).
//
//  Each field gets its own SALTED seed derived from the world seed so
//  the fields don't correlate (identical seeds would make caves hug the
//  terrain shape). Structured so more 2D control fields (erosion,
//  peaks&valleys) can be added later without touching call sites: add a
//  member, configure it in init(), fold it into baseControl().
//
//  Field configs (type / frequency / octaves) live at the top of
//  WorldGenNoise.cpp in one labeled tune-block.
// =====================================================================

class WorldGenNoise {
public:
	// Configure every field once. Read-only after this returns.
	void init(Uint64 worldSeed);

	// --- sampling helpers -------------------------------------------
	// All const: safe to call from any worker thread (see above).
	// Coordinates are ABSOLUTE world block coordinates.

	// Combined 2D control value for the base terrain shape
	// ("continentalness"), roughly [-1, 1]. This is what gets fed into
	// the shape spline. Later control fields get summed/blended here.
	float baseControl(float wx, float wy) const;

	// 3D cave field, roughly [-1, 1]. Terrain is carved where |value|
	// falls inside a narrow band around 0 (see generateShape_Meadow).
	float caveField(float wx, float wy, float wz) const;

private:
	FastNoiseLite m_base;   // 2D low-freq FBm — base shape / continentalness
	FastNoiseLite m_cave;   // 3D FBm — cave carving field
};
