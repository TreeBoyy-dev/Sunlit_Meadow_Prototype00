#pragma once
#include <SDL3/SDL.h>

// =====================================================================
//  WorldGenRandom
//  Deterministic hashing + RNG for world generation.
//
//  Worldgen runs on one ChunkGeneratorWorker thread PER REGION, and the
//  same column gets regenerated whenever a region is unloaded and comes
//  back. rand() satisfies neither case: it is process-global mutable
//  state (a data race between the workers) and its sequence depends on
//  how many columns happened to be generated before this one, so a
//  reloaded column comes back with different trees.
//
//  Everything here is a pure function of its arguments. Seed a stream
//  from (worldSeed, x, y, salt) and that column always rolls the same
//  features, on any thread, in any order — the same purity contract the
//  shape pass and the zone/biome sampler already hold to.
// =====================================================================

// SplitMix64's finalizer, with the golden-ratio increment folded in so
// `state = wgMix64(state)` is one full SplitMix64 step. Cheap, and its
// avalanche is good enough that adjacent coordinates give completely
// uncorrelated streams.
inline Uint64 wgMix64(Uint64 x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

// Stream seed for a 2D location. `salt` separates independent uses of
// the same coordinate (features vs. zone pick vs. biome pick) so they
// don't march in lockstep.
inline Uint64 wgHash2D(Uint64 worldSeed, Sint64 x, Sint64 y, Uint64 salt) {
    Uint64 h = worldSeed ^ (salt * 0xD6E8FEB86659FD93ull);
    h = wgMix64(h ^ ((Uint64)x * 0x9E3779B97F4A7C15ull));
    h = wgMix64(h ^ ((Uint64)y * 0xC2B2AE3D27D4EB4Full));
    return h;
}

struct WorldGenRandom {
    Uint64 state;

    explicit WorldGenRandom(Uint64 seed)
        : state(seed ? seed : 0x853C49E6748FEA9Bull) {}   // 0 would be a fixed point

    WorldGenRandom(Uint64 worldSeed, Sint64 x, Sint64 y, Uint64 salt)
        : WorldGenRandom(wgHash2D(worldSeed, x, y, salt)) {}

    Uint64 next()   { state = wgMix64(state); return state; }
    Uint32 next32() { return (Uint32)(next() >> 32); }   // high bits: best mixed

    // Uniform in [0, bound). Returns 0 for bound <= 0.
    int nextInt(int bound) { return bound > 0 ? (int)(next32() % (Uint32)bound) : 0; }

    // Uniform in [lo, hi] — INCLUSIVE on both ends.
    int nextIntRange(int lo, int hi) { return hi > lo ? lo + nextInt(hi - lo + 1) : lo; }

    float nextFloat() { return (float)(next32() >> 8) * (1.0f / 16777216.0f); }  // [0, 1)

    // Consumes a roll only when the outcome is actually in doubt, so a
    // chance of 1.0 (or 0.0) doesn't shift the stream for later rolls.
    bool chance(float p) {
        if (p >= 1.0f) return true;
        if (p <= 0.0f) return false;
        return nextFloat() < p;
    }
};
