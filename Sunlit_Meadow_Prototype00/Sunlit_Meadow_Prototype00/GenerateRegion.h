#pragma once
#include "WorldGenTypes.h"
#include "WorldGenRegistry.h"
#include "WorldGenSampler.h"

std::unique_ptr<RegionShape> generateRegion(Uint64 worldSeed, RegionCoord coord, WorldGenRegistry* worldGenRegistry) {
    auto shape = std::make_unique<RegionShape>();

    // Order matters here: layer, then maps, then workers. The workers get
    // raw pointers into m_layer/zoneMap/biomeMap, so those must be fully
    // built (and never mutated again) before the first worker thread runs.

    // 1) Resolve which layer this region belongs to (by region z).
    shape->m_layer = worldGenRegistry->getLayerForRegionZ(coord.z);

    // 2) Build both maps synchronously. Every cell — including the apron
    //    cells past the region edge — is computed by the same pure sampler
    //    (world cell coords + layer + seed), so neighboring regions agree
    //    on the overlap by construction. No neighbor-region access, ever.
    {
        std::vector<Uint16> denseZone(MAP_GRID_SIZE * MAP_GRID_SIZE);
        std::vector<Uint16> denseBiome(MAP_GRID_SIZE * MAP_GRID_SIZE);

        for (int gx = 0; gx < MAP_GRID_SIZE; gx++) {
            for (int gy = 0; gy < MAP_GRID_SIZE; gy++) {
                // grid index -> region-relative cell (apron = negative / >= 128)
                // -> world cell
                int worldCellX = coord.x * MAP_CELLS_PER_REGION + (gx - MAP_APRON_CELLS);
                int worldCellY = coord.y * MAP_CELLS_PER_REGION + (gy - MAP_APRON_CELLS);

                Uint16 zoneId  = sampleZoneId (worldCellX, worldCellY, *shape->m_layer,
                                               worldSeed, *worldGenRegistry);
                Uint16 biomeId = sampleBiomeId(worldCellX, worldCellY, *shape->m_layer,
                                               worldSeed, zoneId, *worldGenRegistry);

                int linear = gx * MAP_GRID_SIZE + gy;   // x-major, matches PalettedGrid2D
                denseZone[linear] = zoneId;
                denseBiome[linear] = biomeId;
            }
        }

        shape->zoneMap.fromDense(denseZone.data());
        shape->biomeMap.fromDense(denseBiome.data());
    }

	return shape;
}