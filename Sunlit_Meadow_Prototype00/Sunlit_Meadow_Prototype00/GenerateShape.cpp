#include "Globals.h"
#include "BlockManager.h"
#include "GenerateShape.h"
#include "Materials.h"
#include "Spline.h"

#include <cmath>

// =====================================================================
//  Meadow shape tunables — TUNE HERE.
//  (Raw noise field frequencies/octaves live in WorldGenNoise.cpp.)
//
//  Everything below is a pure function of (world coords, world seed):
//  no rand(), no shared mutable state, no neighbor reads. That purity
//  is what keeps region aprons valid and columns byte-reproducible —
//  same contract as the zone/biome sampler (WorldGenSampler.h).
// =====================================================================

// Nominal sea level. No water yet — the deep-basin knot below dips
// under this so future lakes have somewhere to pool.
static const float MEADOW_SEA_LEVEL = 64.0f;

// Hard vertical bounds the shape must respect: surface stays within
// [10, COLUMN_HEIGHT - 10], caves within [1, COLUMN_HEIGHT - 10]
// (absolute world Z, ground-level region).
static const float SHAPE_MIN_Z = 10.0f;
static const float SHAPE_MAX_Z = (float)(COLUMN_HEIGHT - 10);
static const int   CAVE_FLOOR_Z = 1;                  // never carve below
static const int   CAVE_CEIL_Z = COLUMN_HEIGHT - 10; // never carve above

// Cave carve. We carve where |caveField| < threshold — a BAND around
// zero, not a one-sided cut. The zero-level-set of 3D noise is a
// connected sheet snaking through space, so a band around it gives
// connected wormy tunnels; "value > t" would give isolated blobs.
static const float CAVE_THRESHOLD = 0.06f;

// Partial surface taper: within TAPER blocks of the surface the
// threshold shrinks linearly toward TAPER_MIN_SCALE * CAVE_THRESHOLD.
// Not to zero — a strong tunnel can still punch through and make an
// occasional cave mouth, but the surface stops being swiss cheese.
static const float CAVE_SURFACE_TAPER_BLOCKS = 8.0f;
static const float CAVE_TAPER_MIN_SCALE = 0.35f;

// The meadow terrain shaper: control noise in ~[-1, 1] -> world Z.
// Knot .y fractional parts are deliberately >= .5 on the plateaus so
// generateFeatures_GrassAndDirt places FULL grass blocks there (its
// decimalPart >= 0.5 branch); cliff flanks sweep through all fractions
// and get the slab treatment, which reads nicely as broken terrain.
//
//   c=-1.0 .. -0.5 : deep basin plateau (~Z44, below sea level 64)
//   c=-0.5 .. -0.2 : rise out of the basin
//   c=-0.2 ..  0.35: WIDE flat plains plateau (~Z68) — the meadow
//   c= 0.35..  0.55: the cliff (dydx 0 at both ends -> smooth S-jump)
//   c= 0.55..  0.85: highland plateau (~Z130) drifting up
//   c= 0.85..  1.0 : peaks climbing toward ~Z190
static const Spline MEADOW_SHAPE_SPLINE = {
	{ -0.99f,    10.0f,   0.00f },
	{ -0.55f,    58.0f,   40.0f },
	{ -0.37f,    68.0f,   0.00f },
	{  0.07f,    78.0f,   0.00f },
	{  0.55f,   130.0f,   0.00f },
	{  0.76f,   256.0f,   20.0f },
	{  1.02f,   500.0f,  120.0f },
};

void generateShape(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager,
	const LayerDef& layer,
	const WorldGenNoise& worldGenNoise
) {
	// Dispatch on the layer: each layer OWNS its shape generation. Only
	// the meadow generator exists today — this switch is the seam where
	// underground/sky/etc. layers plug in later (switching on layer.id
	// for now; if the id<->generator mapping ever stops being 1:1, add
	// a shapeGenId to LayerDef and switch on that instead).
	switch (layer.id) {
	case 0:
		generateShape_Meadow(blockIDs, columnCoordinates, regionChunkZStart,
			heightmap, blockManager, worldGenNoise);
		break;
	default:
		SDL_Log("[generateShape] no shape generator for layer %u (%s), using meadow",
			layer.id, layer.name.c_str());
		generateShape_Meadow(blockIDs, columnCoordinates, regionChunkZStart,
			heightmap, blockManager, worldGenNoise);
		break;
	}
}

void generateShape_Meadow(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager,
	const WorldGenNoise& worldGenNoise
) {
	int columnZStartBlocks = regionChunkZStart * CHUNK_SIZE;

	// Look the two base blocks up once for the whole column.
	Block* cobble = blockManager.getByName("cobble_stone");
	Block* air = blockManager.getByName("air");
	if (cobble == nullptr || air == nullptr) {
		SDL_Log("Block = nullptr in Chunk generation!!!");
		return;
	}
	Uint16 cobbleID = cobble->getID();
	Uint16 airID = air->getID();

	(void)MEADOW_SEA_LEVEL; // referenced by design comments; water is a later pass

	for (int x = 0; x < CHUNK_SIZE; x++) {
		int xAbs = columnCoordinates.x * CHUNK_SIZE + x;
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int yAbs = columnCoordinates.y * CHUNK_SIZE + y;

			// --- 1) noise -> spline -> surface height ---------------
			// One control sample per (x, y); the spline reshapes it
			// into plateaus and cliffs (see Spline.h for the why).
			float c = worldGenNoise.baseControl((float)xAbs, (float)yAbs);
			float surfaceZ = MEADOW_SHAPE_SPLINE.eval(c);

			// Safety clamp to the layer's vertical budget. The spline
			// already lives inside these bounds; this guards future
			// knot edits, not the current math.
			if (surfaceZ < SHAPE_MIN_Z) surfaceZ = SHAPE_MIN_Z;
			if (surfaceZ > SHAPE_MAX_Z) surfaceZ = SHAPE_MAX_Z;

			// Absolute world Z — the existing heightmap contract that
			// generateFeatures_* reads back. Do not localize here.
			heightmap[x][y] = surfaceZ;

			// --- 2) fill air/stone ----------------------------------
			// NOTE for the future 3D-density refactor: only this fill
			// loop (and the carve below) changes when the spline output
			// becomes a bias fed into a 3D density field
			// (density > 0 -> stone). Spline/WorldGenNoise stay as-is.
			for (int z = 0; z < COLUMN_HEIGHT; z++) {
				int zAbs = columnZStartBlocks + z;
				blockIDs[x][y][z] = ((float)zAbs <= surfaceZ) ? cobbleID : airID;
			}

			// --- 3) cave carve --------------------------------------
			// Stone cells only, floored/ceiled, threshold tapered near
			// the surface. Pure function of world coords + seed, so
			// tunnels line up across chunk AND region borders with no
			// neighbor access.
			//
			// PERF: this samples 3D FBm at FULL resolution for every
			// stone cell (~surfaceZ * 256 samples per column). If
			// ms/chunk regresses badly, the lever is the Minecraft
			// Beta trick: sample the field on a coarse grid (e.g.
			// every 4th cell in x/y/z) and trilinearly interpolate —
			// caves tolerate the smoothing and it cuts samples ~64x.
			int zTop = (int)surfaceZ - columnZStartBlocks;
			if (zTop >= COLUMN_HEIGHT) zTop = COLUMN_HEIGHT - 1;
			for (int z = 0; z <= zTop; z++) {
				int zAbs = columnZStartBlocks + z;
				if (zAbs < CAVE_FLOOR_Z || zAbs > CAVE_CEIL_Z)
					continue;
				if (blockIDs[x][y][z] == airID)   // already air (above surface)
					continue;

				float threshold = CAVE_THRESHOLD;
				float depth = surfaceZ - (float)zAbs;
				if (depth < CAVE_SURFACE_TAPER_BLOCKS) {
					// Linear taper toward (not to!) zero at the surface,
					// so cave mouths stay possible but rare.
					float s = CAVE_TAPER_MIN_SCALE
						+ (1.0f - CAVE_TAPER_MIN_SCALE) * (depth / CAVE_SURFACE_TAPER_BLOCKS);
					threshold *= s;
				}

				float v = worldGenNoise.caveField((float)xAbs, (float)yAbs, (float)zAbs);
				if (fabsf(v) < threshold)
					blockIDs[x][y][z] = airID;
			}
		}
	}
}

#if 0
// ---------------------------------------------------------------------
//  Spline sanity test (acceptance check): call testMeadowSpline() once
//  from init, watch the log, then flip this back off.
// ---------------------------------------------------------------------
static void testMeadowSpline() {
	const Spline s = {
		{ -1.0f, 10.0f, 0.0f },
		{  0.0f, 10.0f, 0.0f },   // flat segment: -1..0 must hold 10
		{  1.0f, 50.0f, 0.0f },
	};

	// 1) reproduces each knot's y at its x
	SDL_assert(fabsf(s.eval(-1.0f) - 10.0f) < 0.001f);
	SDL_assert(fabsf(s.eval(0.0f) - 10.0f) < 0.001f);
	SDL_assert(fabsf(s.eval(1.0f) - 50.0f) < 0.001f);

	// 2) flat between two flat knots (dead plateau, no Catmull-Rom sag)
	SDL_assert(fabsf(s.eval(-0.5f) - 10.0f) < 0.001f);

	// 3) monotonically rising through the cliff segment
	float prev = s.eval(0.0f);
	for (float x = 0.05f; x <= 1.0f; x += 0.05f) {
		float v = s.eval(x);
		SDL_assert(v >= prev - 0.001f);
		prev = v;
	}

	// 4) clamps (holds end values) outside [x0, xn]
	SDL_assert(fabsf(s.eval(-5.0f) - 10.0f) < 0.001f);
	SDL_assert(fabsf(s.eval(5.0f) - 50.0f) < 0.001f);

	SDL_Log("[Spline] testMeadowSpline passed");
}
#endif
