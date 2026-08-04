#include "Globals.h"
#include "BlockManager.h"
#include "GenerateShape.h"
#include "Materials.h"

#include <algorithm>
#include <cmath>

// =====================================================================
//  Shape generation.
//
//  There are NO tunables in this file any more — spline knots, sea
//  level, vertical bounds, base block and every cave constant come off
//  the LayerDef, i.e. out of Assets/WorldGen/Layers/*.json. (Raw noise
//  field frequencies/octaves still live in WorldGenNoise.cpp.)
//
//  Everything below stays a pure function of (world coords, layer,
//  world seed): no rand(), no shared mutable state, no neighbor reads.
//  That purity is what keeps region aprons valid and columns
//  byte-reproducible — same contract as the zone/biome sampler
//  (WorldGenSampler.h).
//
//  Notes for whoever tunes the JSON:
//   - Knot .z fractional parts should be >= .5 on plateaus so the
//     surface pass places FULL blocks there (it switches to the biome's
//     surfaceSlab palette below .5); cliff flanks sweep through all
//     fractions and get the slab treatment, which reads nicely as
//     broken terrain.
//   - Caves are carved where |caveField| < threshold — a BAND around
//     zero, not a one-sided cut. The zero-level-set of 3D noise is a
//     connected sheet snaking through space, so a band around it gives
//     connected wormy tunnels; "value > t" would give isolated blobs.
// =====================================================================

void generateShape(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	BlockManager& blockManager,
	const LayerDef& layer,
	const WorldGenNoise& worldGenNoise
) {
	Block* air = blockManager.getByName("air");
	if (air == nullptr) {
		SDL_Log("[generateShape] 'air' block missing — cannot generate");
		return;
	}
	const Uint16 airID = air->getID();

	// Dispatch on the layer's declared shape generator. Which generator a
	// layer uses is data now (LayerDef::shape), so a new layer only needs
	// a JSON file — unless it needs a genuinely new algorithm, which is
	// what adding a ShapeGenerator value is for.
	switch (layer.shape) {
	case ShapeGenerator::SplineTerrain:
		generateShape_splineTerrain(blockIDs, columnCoordinates, regionChunkZStart,
			heightmap, airID, layer, worldGenNoise);
		break;

	case ShapeGenerator::SolidFill:
		generateShape_fill(blockIDs, heightmap, regionChunkZStart, layer.baseBlock.id);
		break;

	case ShapeGenerator::AirFill:
		generateShape_fill(blockIDs, heightmap, regionChunkZStart, airID);
		break;

	default:
		SDL_Log("[generateShape] layer %u ('%s') has no valid shape generator — "
			"filling with air", layer.id, layer.name.c_str());
		generateShape_fill(blockIDs, heightmap, regionChunkZStart, airID);
		break;
	}
}

void generateShape_splineTerrain(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	ColumnCoord columnCoordinates,
	int regionChunkZStart,
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	Uint16 airID,
	const LayerDef& layer,
	const WorldGenNoise& worldGenNoise
) {
	const int columnZStartBlocks = regionChunkZStart * CHUNK_SIZE;
	const Uint16 baseID = layer.baseBlock.id;

	if (layer.shapeSpline.empty()) {
		SDL_Log("[generateShape] layer '%s' has an empty spline — filling with air",
			layer.name.c_str());
		generateShape_fill(blockIDs, heightmap, regionChunkZStart, airID);
		return;
	}

	for (int x = 0; x < CHUNK_SIZE; x++) {
		int xAbs = columnCoordinates.x * CHUNK_SIZE + x;
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int yAbs = columnCoordinates.y * CHUNK_SIZE + y;

			// --- 1) noise -> spline -> surface height ---------------
			// One control sample per (x, y); the spline reshapes it
			// into plateaus and cliffs (see Spline.h for the why).
			float c = worldGenNoise.baseControl((float)xAbs, (float)yAbs);
			float surfaceZ = layer.shapeSpline.eval(c);

			// Safety clamp to the layer's vertical budget. A sane spline
			// already lives inside these bounds; this guards JSON edits,
			// not the current math.
			if (surfaceZ < layer.minSurfaceZ) surfaceZ = layer.minSurfaceZ;
			if (surfaceZ > layer.maxSurfaceZ) surfaceZ = layer.maxSurfaceZ;

			// Absolute world Z — the existing heightmap contract that
			// the feature pass reads back. Do not localize here.
			heightmap[x][y] = surfaceZ;

			// --- 2) fill air/base -----------------------------------
			// NOTE for the future 3D-density refactor: only this fill
			// loop (and the carve below) changes when the spline output
			// becomes a bias fed into a 3D density field
			// (density > 0 -> stone). Spline/WorldGenNoise stay as-is.
			for (int z = 0; z < COLUMN_HEIGHT; z++) {
				int zAbs = columnZStartBlocks + z;
				blockIDs[x][y][z] = ((float)zAbs <= surfaceZ) ? baseID : airID;
			}

			// --- 3) cave carve --------------------------------------
			// Solid cells only, floored/ceiled, threshold tapered near
			// the surface. Pure function of world coords + seed, so
			// tunnels line up across chunk AND region borders with no
			// neighbor access.
			//
			// PERF: this samples 3D FBm at FULL resolution for every
			// solid cell (~surfaceZ * 256 samples per column). If
			// ms/chunk regresses badly, the lever is the Minecraft
			// Beta trick: sample the field on a coarse grid (e.g.
			// every 4th cell in x/y/z) and trilinearly interpolate —
			// caves tolerate the smoothing and it cuts samples ~64x.
			if (!layer.caves)
				continue;

			int zTop = (int)surfaceZ - columnZStartBlocks;
			if (zTop >= COLUMN_HEIGHT) zTop = COLUMN_HEIGHT - 1;
			for (int z = 0; z <= zTop; z++) {
				int zAbs = columnZStartBlocks + z;
				if (zAbs < layer.caveFloorZ || zAbs > layer.caveCeilZ)
					continue;
				if (blockIDs[x][y][z] == airID)   // already air (above surface)
					continue;

				float threshold = layer.caveThreshold;
				float depth = surfaceZ - (float)zAbs;
				if (depth < layer.caveSurfaceTaper && layer.caveSurfaceTaper > 0.0f) {
					// Linear taper toward (not to!) zero at the surface,
					// so cave mouths stay possible but rare.
					float s = layer.caveTaperMinScale
						+ (1.0f - layer.caveTaperMinScale) * (depth / layer.caveSurfaceTaper);
					threshold *= s;
				}

				float v = worldGenNoise.caveField((float)xAbs, (float)yAbs, (float)zAbs);
				if (fabsf(v) < threshold)
					blockIDs[x][y][z] = airID;
			}
		}
	}
}

void generateShape_fill(
	Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][COLUMN_HEIGHT],
	float heightmap[CHUNK_SIZE][CHUNK_SIZE],
	int regionChunkZStart,
	Uint16 blockID
) {
	std::fill_n(&blockIDs[0][0][0], CHUNK_SIZE * CHUNK_SIZE * COLUMN_HEIGHT, blockID);

	// One block below the column: every feature placement is bounds-checked
	// against the column, so this reliably means "nothing to decorate here"
	// without needing a separate "has surface" flag.
	const float belowColumn = (float)(regionChunkZStart * CHUNK_SIZE) - 1.0f;
	std::fill_n(&heightmap[0][0], CHUNK_SIZE * CHUNK_SIZE, belowColumn);
}
