#include "WorldGenRegistry.h"

#include "BlockManager.h"
#include "BuildAbsolutePath.h"
#include "WorldGenDefLoader.h"

bool WorldGenRegistry::init(BlockManager* blockManager) {
    layers.clear();
    zones.clear();
    biomes.clear();
    features.clear();
    loaded = false;

    buildFallbackLayers();

    WorldGenDefLoader loader;
    if (!loader.loadAll(BuildAbsolutePath("Assets", "WorldGen"),
                        layers, zones, biomes, features)) {
        SDL_Log("[WorldGenRegistry] could not load worldgen definitions — every region "
                "will fall back to the built-in air/solid layers");
        return false;
    }

    buildNameIndexes();

    // ---- resolve block names -> block ids ----------------------------
    // Done here rather than in the loader because it needs BlockManager,
    // which is fully populated by now (App_Init runs blockManager.init()
    // before worldManager.init()).
    if (blockManager != nullptr) {
        for (LayerDef& layer : layers)
            resolvePick(layer.baseBlock, *blockManager, layer.name.c_str(), "baseBlock");

        for (BiomeDef& biome : biomes) {
            resolvePalette(biome.surface,     *blockManager, biome.name.c_str(), "surface");
            resolvePalette(biome.surfaceSlab, *blockManager, biome.name.c_str(), "surfaceSlab");
            resolvePalette(biome.filler,      *blockManager, biome.name.c_str(), "filler");
        }

        for (FeatureDef& feature : features) {
            resolvePalette(feature.body,    *blockManager, feature.name.c_str(), "body");
            resolvePalette(feature.foliage, *blockManager, feature.name.c_str(), "foliage");
            resolvePalette(feature.accent,  *blockManager, feature.name.c_str(), "accent");
        }

        resolvePick(defaultBelow.baseBlock, *blockManager, defaultBelow.name.c_str(), "baseBlock");
    }
    else {
        SDL_Log("[WorldGenRegistry] no BlockManager — every block reference stays "
                "unresolved and nothing will generate");
    }

    loaded = true;
    SDL_Log("[WorldGenRegistry] ready: %zu layers, %zu zones, %zu biomes, %zu features",
            layers.size(), zones.size(), biomes.size(), features.size());
    return true;
}

void WorldGenRegistry::buildFallbackLayers() {
    // Ids are deliberately outside the JSON id space (which is dense from
    // 0): nothing looks these up by id, they are only ever returned by
    // getLayerForRegionZ, and a distinct id keeps them obvious in a log.
    defaultAbove = LayerDef{};
    defaultAbove.id    = 0xFFFF;
    defaultAbove.name  = "default_sky";
    defaultAbove.shape = ShapeGenerator::AirFill;

    defaultBelow = LayerDef{};
    defaultBelow.id    = 0xFFFE;
    defaultBelow.name  = "default_underground";
    defaultBelow.shape = ShapeGenerator::SolidFill;
    defaultBelow.baseBlock.name = "cobble_stone";
}

void WorldGenRegistry::buildNameIndexes() {
    layerByName.clear();
    zoneByName.clear();
    biomeByName.clear();
    featureByName.clear();

    for (const LayerDef&   d : layers)   layerByName[d.name]   = d.id;
    for (const ZoneDef&    d : zones)    zoneByName[d.name]    = d.id;
    for (const BiomeDef&   d : biomes)   biomeByName[d.name]   = d.id;
    for (const FeatureDef& d : features) featureByName[d.name] = d.id;
}

void WorldGenRegistry::resolvePick(BlockPick& pick, BlockManager& blockManager,
                                   const char* owner, const char* field) {
    if (pick.name.empty()) return;
    Block* block = blockManager.getByName(pick.name);
    if (block == nullptr) {
        SDL_Log("[WorldGenRegistry] '%s'.%s: unknown block '%s'",
                owner, field, pick.name.c_str());
        pick.id = 0;
        return;
    }
    pick.id = block->getID();
}

void WorldGenRegistry::resolvePalette(BlockPalette& palette, BlockManager& blockManager,
                                      const char* owner, const char* field) {
    std::vector<BlockPick> kept;
    kept.reserve(palette.picks.size());

    for (BlockPick& pick : palette.picks) {
        Block* block = blockManager.getByName(pick.name);
        if (block == nullptr) {
            // Dropped, not zeroed: id 0 is air, and an air pick inside a
            // surface or blob palette would punch holes into the terrain.
            SDL_Log("[WorldGenRegistry] '%s'.%s: unknown block '%s' — pick dropped",
                    owner, field, pick.name.c_str());
            continue;
        }
        pick.id = block->getID();
        kept.push_back(std::move(pick));
    }

    palette.picks = std::move(kept);
    palette.totalWeight = 0;
    for (const BlockPick& p : palette.picks) palette.totalWeight += p.weight;
}

const LayerDef* WorldGenRegistry::getLayerForRegionZ(int regionZ) const {
    // Layer id == region z, so the dense id space maps straight onto the
    // stack of region layers. Anything the JSON doesn't claim gets a
    // fallback: solid below the defined world, empty above it.
    if (regionZ >= 0 && (size_t)regionZ < layers.size())
        return &layers[(size_t)regionZ];
    return regionZ < 0 ? &defaultBelow : &defaultAbove;
}

const LayerDef* WorldGenRegistry::getLayer(Uint16 id) const {
    if (id >= layers.size()) {
        SDL_Log("[WorldGenRegistry] unknown layer id: %u", id);
        return nullptr;
    }
    return &layers[id];
}

const ZoneDef* WorldGenRegistry::getZone(Uint16 id) const {
    if (id >= zones.size()) {
        SDL_Log("[WorldGenRegistry] unknown zone id: %u", id);
        return nullptr;
    }
    return &zones[id];
}

const BiomeDef* WorldGenRegistry::getBiome(Uint16 id) const {
    if (id >= biomes.size()) {
        SDL_Log("[WorldGenRegistry] unknown biome id: %u", id);
        return nullptr;
    }
    return &biomes[id];
}

const FeatureDef* WorldGenRegistry::getFeature(Uint16 id) const {
    if (id >= features.size()) {
        SDL_Log("[WorldGenRegistry] unknown feature id: %u", id);
        return nullptr;
    }
    return &features[id];
}

const LayerDef* WorldGenRegistry::findLayer(const std::string& name) const {
    auto it = layerByName.find(name);
    return it != layerByName.end() ? &layers[it->second] : nullptr;
}

const ZoneDef* WorldGenRegistry::findZone(const std::string& name) const {
    auto it = zoneByName.find(name);
    return it != zoneByName.end() ? &zones[it->second] : nullptr;
}

const BiomeDef* WorldGenRegistry::findBiome(const std::string& name) const {
    auto it = biomeByName.find(name);
    return it != biomeByName.end() ? &biomes[it->second] : nullptr;
}

const FeatureDef* WorldGenRegistry::findFeature(const std::string& name) const {
    auto it = featureByName.find(name);
    return it != featureByName.end() ? &features[it->second] : nullptr;
}
