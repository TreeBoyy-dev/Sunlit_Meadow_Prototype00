#include "WorldGenNoise.h"

// ---------------------------------------------------------------------
//  Noise field configs — TUNE HERE.
//  (The shape spline + cave carve tunables live in GenerateChunk.cpp;
//  this block only owns the raw fields' character.)
// ---------------------------------------------------------------------

// Base shape ("continentalness"). Feature wavelength is roughly
// 1 / frequency, so 0.0025 gives plateaus/valleys on the order of
// ~400 blocks across ("medium" scale). Octaves add smaller detail on
// top without moving that headline scale.
static const float BASE_FREQUENCY = 0.0025f;
static const int   BASE_OCTAVES   = 4;

// Cave field. Higher frequency than the base shape — tunnels are
// tens of blocks, not hundreds. Two octaves make the tunnel band
// wiggle without exploding the per-sample cost (this field is sampled
// in FULL 3D per stone cell, it is the expensive one).
static const float CAVE_FREQUENCY = 0.018f;
static const int   CAVE_OCTAVES   = 2;

// Per-field seed salt. XOR-ing the world seed with a large odd
// constant (the 64-bit golden ratio, same spirit as splitmix64)
// decorrelates the fields: same world seed, unrelated noise. Without
// this, base and cave would share a seed and caves would visibly
// follow the terrain shape.
static const Uint64 CAVE_SEED_SALT = 0x9E3779B97F4A7C15ULL;

void WorldGenNoise::init(Uint64 worldSeed) {
	// FastNoiseLite seeds are int; truncating the 64-bit world seed is
	// fine — it just has to be deterministic, not collision-free.

	// -- m_base: 2D low-frequency FBm --------------------------------
	m_base.SetSeed((int)worldSeed);
	m_base.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_base.SetFractalType(FastNoiseLite::FractalType_FBm);
	m_base.SetFrequency(BASE_FREQUENCY);
	m_base.SetFractalOctaves(BASE_OCTAVES);
	m_base.SetFractalLacunarity(2.0f);
	m_base.SetFractalGain(0.5f);

	// -- m_cave: 3D FBm ----------------------------------------------
	m_cave.SetSeed((int)(worldSeed ^ CAVE_SEED_SALT));
	m_cave.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_cave.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes);
	m_cave.SetFractalType(FastNoiseLite::FractalType_FBm);
	m_cave.SetFrequency(CAVE_FREQUENCY);
	m_cave.SetFractalOctaves(CAVE_OCTAVES);
	m_cave.SetFractalLacunarity(2.0f);
	m_cave.SetFractalGain(0.5f);
}

float WorldGenNoise::baseControl(float wx, float wy) const {
	// Single field for now. When erosion / peaks&valleys arrive they
	// get combined here so callers keep seeing ONE control value.
	return m_base.GetNoise(wx, wy);
}

float WorldGenNoise::caveField(float wx, float wy, float wz) const {
	return m_cave.GetNoise(wx, wy, wz);
}
