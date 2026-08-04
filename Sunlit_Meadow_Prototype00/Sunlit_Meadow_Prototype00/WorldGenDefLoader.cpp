#include "WorldGenDefLoader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------------
//  Shared parse helpers
// ---------------------------------------------------------------------

bool readJsonFile(const fs::path& path, json& out, const std::string& where) {
    std::ifstream f(path);
    if (!f) {
        SDL_Log("[WorldGenDefLoader] %s: cannot open file", where.c_str());
        return false;
    }
    try {
        f >> out;
    }
    catch (const std::exception& e) {
        SDL_Log("[WorldGenDefLoader] %s: %s", where.c_str(), e.what());
        return false;
    }
    return true;
}

// Every def file needs a name and an explicit id. Registration-order ids
// would silently repoint saved worlds the moment the directory scan
// order changes — same reasoning as BlockDefLoader.
bool parseIdentity(const json& j, Uint16& id, std::string& name, const std::string& where) {
    if (!j.contains("name") || !j["name"].is_string()) {
        SDL_Log("[WorldGenDefLoader] %s: missing 'name'", where.c_str());
        return false;
    }
    name = j["name"].get<std::string>();
    if (!j.contains("id") || !j["id"].is_number_unsigned()) {
        SDL_Log("[WorldGenDefLoader] %s: missing mandatory numeric 'id'", where.c_str());
        return false;
    }
    id = (Uint16)j["id"].get<unsigned>();
    return true;
}

// A block palette is authored in one of three shapes:
//   "grass_block"
//   [ "grass_block", "podzol" ]                       (equal weights)
//   [ { "block": "grass_block_slab", "weight": 199 },
//     { "block": "cobble_stone_stair", "weight": 1 } ]
// Ids stay 0 here — WorldGenRegistry resolves the names once BlockManager
// is available.
bool parseBlockPalette(const json& j, BlockPalette& out, const std::string& where,
                       const char* field) {
    out.picks.clear();
    out.totalWeight = 0;

    auto addPick = [&](const json& entry) -> bool {
        BlockPick p;
        if (entry.is_string()) {
            p.name = entry.get<std::string>();
        }
        else if (entry.is_object()) {
            if (!entry.contains("block") || !entry["block"].is_string()) {
                SDL_Log("[WorldGenDefLoader] %s: '%s' entry is missing 'block'",
                        where.c_str(), field);
                return false;
            }
            p.name = entry["block"].get<std::string>();
            p.weight = entry.value("weight", 1);
        }
        else {
            SDL_Log("[WorldGenDefLoader] %s: '%s' entry must be a block name or "
                    "{block, weight}", where.c_str(), field);
            return false;
        }
        if (p.weight <= 0) {
            SDL_Log("[WorldGenDefLoader] %s: '%s' entry '%s' has weight %d — "
                    "clamped to 1", where.c_str(), field, p.name.c_str(), p.weight);
            p.weight = 1;
        }
        out.picks.push_back(std::move(p));
        return true;
    };

    if (j.is_array()) {
        for (const json& entry : j)
            if (!addPick(entry)) return false;
    }
    else if (!addPick(j)) {
        return false;
    }

    for (const BlockPick& p : out.picks) out.totalWeight += p.weight;
    return true;
}

// Optional palette: absent key = empty palette, which every generator
// reads as "place nothing".
bool parseOptionalPalette(const json& j, const char* key, BlockPalette& out,
                          const std::string& where) {
    if (!j.contains(key)) return true;
    return parseBlockPalette(j[key], out, where, key);
}

// A single block reference (no weights): "baseBlock": "cobble_stone".
bool parseBlockPick(const json& j, BlockPick& out, const std::string& where,
                    const char* field) {
    if (!j.is_string()) {
        SDL_Log("[WorldGenDefLoader] %s: '%s' must be a block name", where.c_str(), field);
        return false;
    }
    out.name = j.get<std::string>();
    out.weight = 1;
    return true;
}

// Spline knots: [ { "control": -0.55, "z": 58.0, "slope": 40.0 }, ... ].
// "control" is the noise value the knot sits at, "z" the absolute world
// Z it maps to, "slope" the Hermite derivative (0 = plateau).
bool parseSpline(const json& j, Spline& out, const std::string& where) {
    if (!j.is_array() || j.size() < 2) {
        SDL_Log("[WorldGenDefLoader] %s: 'spline' must be an array of at least 2 knots",
                where.c_str());
        return false;
    }
    std::vector<SplineKnot> knots;
    knots.reserve(j.size());
    for (const json& k : j) {
        if (!k.is_object() || !k.contains("control") || !k.contains("z")) {
            SDL_Log("[WorldGenDefLoader] %s: spline knot must be {control, z, slope}",
                    where.c_str());
            return false;
        }
        knots.push_back(SplineKnot{
            k["control"].get<float>(),
            k["z"].get<float>(),
            k.value("slope", 0.0f)
        });
    }
    // Sorted-ascending is Spline's contract; check it HERE so the log can
    // name the file (Spline itself only knows the knot index).
    for (size_t i = 1; i < knots.size(); i++) {
        if (knots[i].x <= knots[i - 1].x) {
            SDL_Log("[WorldGenDefLoader] %s: spline knots must be sorted ascending by "
                    "'control' (knot %zu: %f <= %f)",
                    where.c_str(), i, knots[i].x, knots[i - 1].x);
            return false;
        }
    }
    out = Spline(std::move(knots));
    return true;
}

// ---------------------------------------------------------------------
//  Per-type file parsers. Names are stored raw; resolveReferences()
//  turns them into ids afterwards.
// ---------------------------------------------------------------------

// Name references are kept out of the defs themselves (the defs only
// carry ids), so the parsers park them here until the resolve pass.
//
// Keyed by the DEF'S OWN NAME, not by index: files are parsed in
// directory order but come back sorted by id, so parallel arrays would
// silently pair each layer with a different layer's zone list. Names are
// unique per type (loadDir drops duplicates), so they are a safe key.
struct PendingRefs {
    std::unordered_map<std::string, std::vector<std::string>> layerZones;  // layer -> zones
    std::unordered_map<std::string, std::vector<std::string>> zoneBiomes;  // zone  -> biomes
    std::unordered_map<std::string, std::string>              biomeZone;   // biome -> zone
};

bool parseLayerFile(const fs::path& path, LayerDef& def, std::vector<std::string>& zoneNames) {
    const std::string where = "Layers/" + path.filename().string();
    json j;
    if (!readJsonFile(path, j, where)) return false;

    try {
        if (!parseIdentity(j, def.id, def.name, where)) return false;

        const std::string shapeName = j.value("shape", "");
        def.shape = shapeGeneratorFromName(shapeName);
        if (def.shape == ShapeGenerator::Invalid) {
            SDL_Log("[WorldGenDefLoader] %s: unknown 'shape' generator '%s' — layer skipped",
                    where.c_str(), shapeName.c_str());
            return false;
        }

        if (j.contains("baseBlock") && !parseBlockPick(j["baseBlock"], def.baseBlock, where, "baseBlock"))
            return false;
        if (def.baseBlock.name.empty() && def.shape != ShapeGenerator::AirFill) {
            SDL_Log("[WorldGenDefLoader] %s: shape '%s' needs a 'baseBlock' — layer skipped",
                    where.c_str(), shapeName.c_str());
            return false;
        }

        def.seaLevel    = j.value("seaLevel",    def.seaLevel);
        def.minSurfaceZ = j.value("minSurfaceZ", def.minSurfaceZ);
        def.maxSurfaceZ = j.value("maxSurfaceZ", def.maxSurfaceZ);

        if (def.shape == ShapeGenerator::SplineTerrain) {
            if (!j.contains("spline")) {
                SDL_Log("[WorldGenDefLoader] %s: shape 'spline_terrain' needs a 'spline' "
                        "— layer skipped", where.c_str());
                return false;
            }
            if (!parseSpline(j["spline"], def.shapeSpline, where)) return false;
        }

        if (j.contains("caves")) {
            const json& c = j["caves"];
            if (!c.is_object()) {
                SDL_Log("[WorldGenDefLoader] %s: 'caves' must be an object", where.c_str());
                return false;
            }
            def.caves             = c.value("enabled",            true);
            def.caveThreshold     = c.value("threshold",          def.caveThreshold);
            def.caveSurfaceTaper  = c.value("surfaceTaperBlocks", def.caveSurfaceTaper);
            def.caveTaperMinScale = c.value("taperMinScale",      def.caveTaperMinScale);
            def.caveFloorZ        = c.value("floorZ",             def.caveFloorZ);
            def.caveCeilZ         = c.value("ceilZ",              def.caveCeilZ);
        }

        def.zoneScale = j.value("zoneScale", def.zoneScale);

        zoneNames.clear();
        if (j.contains("allowedZones"))
            for (const json& z : j["allowedZones"])
                zoneNames.push_back(z.get<std::string>());
    }
    catch (const std::exception& e) {
        SDL_Log("[WorldGenDefLoader] %s: %s", where.c_str(), e.what());
        return false;
    }
    return true;
}

bool parseZoneFile(const fs::path& path, ZoneDef& def, std::vector<std::string>& biomeNames) {
    const std::string where = "Zones/" + path.filename().string();
    json j;
    if (!readJsonFile(path, j, where)) return false;

    try {
        if (!parseIdentity(j, def.id, def.name, where)) return false;
        def.weight     = j.value("weight",     def.weight);
        def.biomeScale = j.value("biomeScale", def.biomeScale);

        biomeNames.clear();
        if (j.contains("allowedBiomes"))
            for (const json& b : j["allowedBiomes"])
                biomeNames.push_back(b.get<std::string>());
    }
    catch (const std::exception& e) {
        SDL_Log("[WorldGenDefLoader] %s: %s", where.c_str(), e.what());
        return false;
    }
    return true;
}

// "features": [ "birch_tree", { "feature": "boulder", "chance": 0.2 } ]
bool parseBiomeFeatureList(const json& j, std::vector<BiomeFeature>& out,
                           const std::string& where, const char* field) {
    if (!j.is_array()) {
        SDL_Log("[WorldGenDefLoader] %s: '%s' must be an array", where.c_str(), field);
        return false;
    }
    for (const json& e : j) {
        BiomeFeature bf;
        if (e.is_string()) {
            bf.featureName = e.get<std::string>();
        }
        else if (e.is_object()) {
            if (!e.contains("feature") || !e["feature"].is_string()) {
                SDL_Log("[WorldGenDefLoader] %s: '%s' entry is missing 'feature'",
                        where.c_str(), field);
                return false;
            }
            bf.featureName = e["feature"].get<std::string>();
            bf.chance   = e.value("chance",   -1.0f);   // < 0 = inherit the def's
            bf.attempts = e.value("attempts", -1);
        }
        else {
            SDL_Log("[WorldGenDefLoader] %s: '%s' entry must be a feature name or "
                    "{feature, chance, attempts}", where.c_str(), field);
            return false;
        }
        out.push_back(std::move(bf));
    }
    return true;
}

bool parseBiomeFile(const fs::path& path, BiomeDef& def, std::string& zoneName) {
    const std::string where = "Biomes/" + path.filename().string();
    json j;
    if (!readJsonFile(path, j, where)) return false;

    try {
        if (!parseIdentity(j, def.id, def.name, where)) return false;

        if (!j.contains("zone") || !j["zone"].is_string()) {
            SDL_Log("[WorldGenDefLoader] %s: missing 'zone' — biome skipped", where.c_str());
            return false;
        }
        zoneName = j["zone"].get<std::string>();

        def.weight       = j.value("weight",       def.weight);
        def.temperature  = j.value("temperature",  def.temperature);
        def.humidity     = j.value("humidity",     def.humidity);
        def.heightOffset = j.value("heightOffset", def.heightOffset);
        def.heightScale  = j.value("heightScale",  def.heightScale);
        def.fillerDepth  = j.value("fillerDepth",  def.fillerDepth);

        if (!parseOptionalPalette(j, "surface",     def.surface,     where)) return false;
        if (!parseOptionalPalette(j, "surfaceSlab", def.surfaceSlab, where)) return false;
        if (!parseOptionalPalette(j, "filler",      def.filler,      where)) return false;
        // A biome with no surfaceSlab still has to cover half-step
        // surfaces with SOMETHING, or the terrain gets holes on every
        // slope: fall back to the full-block surface palette.
        if (def.surfaceSlab.empty()) def.surfaceSlab = def.surface;

        if (j.contains("features") &&
            !parseBiomeFeatureList(j["features"], def.features, where, "features"))
            return false;
        if (j.contains("structures") &&
            !parseBiomeFeatureList(j["structures"], def.structures, where, "structures"))
            return false;
    }
    catch (const std::exception& e) {
        SDL_Log("[WorldGenDefLoader] %s: %s", where.c_str(), e.what());
        return false;
    }
    return true;
}

bool parseFeatureFile(const fs::path& path, FeatureDef& def) {
    const std::string where = "Features/" + path.filename().string();
    json j;
    if (!readJsonFile(path, j, where)) return false;

    try {
        if (!parseIdentity(j, def.id, def.name, where)) return false;

        def.kind = kindFromName(j.value("kind", "feature"));

        const std::string genName = j.value("generator", "");
        def.generator = featureGeneratorFromName(genName);
        if (def.generator == FeatureGenerator::Invalid) {
            SDL_Log("[WorldGenDefLoader] %s: unknown 'generator' '%s' — feature skipped",
                    where.c_str(), genName.c_str());
            return false;
        }

        def.chance   = j.value("chance",   def.chance);
        def.attempts = j.value("attempts", def.attempts);
        def.minZ     = j.value("minZ",     def.minZ);
        def.maxZ     = j.value("maxZ",     def.maxZ);

        // "height": 7 or "height": [5, 8]
        if (j.contains("height")) {
            const json& h = j["height"];
            if (h.is_array() && h.size() == 2) {
                def.minHeight = h[0].get<int>();
                def.maxHeight = h[1].get<int>();
            }
            else if (h.is_number_integer()) {
                def.minHeight = def.maxHeight = h.get<int>();
            }
            else {
                SDL_Log("[WorldGenDefLoader] %s: 'height' must be an int or [min, max]",
                        where.c_str());
                return false;
            }
            if (def.maxHeight < def.minHeight) std::swap(def.minHeight, def.maxHeight);
        }
        def.radius       = j.value("radius",       def.radius);
        def.canopyHeight = j.value("canopyHeight", def.canopyHeight);

        if (!parseOptionalPalette(j, "body",    def.body,    where)) return false;
        if (!parseOptionalPalette(j, "foliage", def.foliage, where)) return false;
        if (!parseOptionalPalette(j, "accent",  def.accent,  where)) return false;
        def.accentChance = j.value("accentChance", def.accent.empty() ? 0.0f : 1.0f);

        def.prefab = j.value("prefab", "");

        // Cheap authoring checks — a feature with nothing to place is
        // always a mistake, and it would silently do nothing at runtime.
        if (def.generator != FeatureGenerator::Prefab && def.body.empty()) {
            SDL_Log("[WorldGenDefLoader] %s: generator '%s' needs a 'body' palette — "
                    "feature skipped", where.c_str(), genName.c_str());
            return false;
        }
        if (def.generator == FeatureGenerator::Tree && def.foliage.empty()) {
            SDL_Log("[WorldGenDefLoader] %s: generator 'tree' needs a 'foliage' palette — "
                    "feature skipped", where.c_str());
            return false;
        }
    }
    catch (const std::exception& e) {
        SDL_Log("[WorldGenDefLoader] %s: %s", where.c_str(), e.what());
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------
//  Directory scan + id validation
// ---------------------------------------------------------------------

// Reads every *.json in `dir` through `parseOne`, drops duplicates, and
// sorts by id. Returns false only when the directory is missing.
template <typename Def, typename ParseFn>
bool loadDir(const fs::path& dir, const char* label,
             std::vector<Def>& out, ParseFn parseOne) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        SDL_Log("[WorldGenDefLoader] %s directory missing: '%s'", label, dir.string().c_str());
        return false;
    }

    std::unordered_set<Uint16>      seenIds;
    std::unordered_set<std::string> seenNames;
    for (const fs::directory_entry& e : fs::directory_iterator(dir, ec)) {
        if (!e.is_regular_file() || e.path().extension() != ".json") continue;

        Def def;
        if (!parseOne(e.path(), def)) continue;

        if (seenIds.count(def.id)) {
            SDL_Log("[WorldGenDefLoader] duplicate %s id %u ('%s') — skipped",
                    label, def.id, def.name.c_str());
            continue;
        }
        if (seenNames.count(def.name)) {
            SDL_Log("[WorldGenDefLoader] duplicate %s name '%s' — skipped",
                    label, def.name.c_str());
            continue;
        }
        seenIds.insert(def.id);
        seenNames.insert(def.name);
        out.push_back(std::move(def));
    }

    std::sort(out.begin(), out.end(),
              [](const Def& a, const Def& b) { return a.id < b.id; });
    return true;
}

// The registry indexes vectors by id, so a gap would put the wrong def
// at that index. Report it here rather than letting lookups lie.
template <typename Def>
void checkDenseIds(const std::vector<Def>& defs, const char* label) {
    for (size_t i = 0; i < defs.size(); i++) {
        if (defs[i].id != (Uint16)i) {
            SDL_Log("[WorldGenDefLoader] %s id gap — expected %zu, got %u ('%s'). "
                    "Ids must be dense (0, 1, 2, ...)!",
                    label, i, defs[i].id, defs[i].name.c_str());
            return;
        }
    }
}

template <typename Def>
std::unordered_map<std::string, Uint16> buildNameIndex(const std::vector<Def>& defs) {
    std::unordered_map<std::string, Uint16> index;
    index.reserve(defs.size());
    for (const Def& d : defs) index[d.name] = d.id;
    return index;
}

// Resolves one name against an index; logs and returns false on a typo.
bool resolveName(const std::unordered_map<std::string, Uint16>& index,
                 const std::string& name, Uint16& outId,
                 const char* kind, const std::string& owner) {
    auto it = index.find(name);
    if (it == index.end()) {
        SDL_Log("[WorldGenDefLoader] '%s' references unknown %s '%s' — dropped",
                owner.c_str(), kind, name.c_str());
        return false;
    }
    outId = it->second;
    return true;
}

} // namespace

// ---------------------------------------------------------------------
bool WorldGenDefLoader::loadAll(
    const std::string&       worldGenDir,
    std::vector<LayerDef>&   outLayers,
    std::vector<ZoneDef>&    outZones,
    std::vector<BiomeDef>&   outBiomes,
    std::vector<FeatureDef>& outFeatures
) {
    const fs::path root(worldGenDir);
    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
        SDL_Log("[WorldGenDefLoader] worldgen directory missing: '%s'", worldGenDir.c_str());
        return false;
    }

    // ---- 1) parse every file, parking name references ----------------
    PendingRefs refs;

    bool ok = true;
    ok &= loadDir(root / "Features", "feature", outFeatures,
                  [](const fs::path& p, FeatureDef& d) { return parseFeatureFile(p, d); });
    ok &= loadDir(root / "Biomes", "biome", outBiomes,
                  [&refs](const fs::path& p, BiomeDef& d) {
                      std::string zoneName;
                      if (!parseBiomeFile(p, d, zoneName)) return false;
                      refs.biomeZone.emplace(d.name, std::move(zoneName));
                      return true;
                  });
    ok &= loadDir(root / "Zones", "zone", outZones,
                  [&refs](const fs::path& p, ZoneDef& d) {
                      std::vector<std::string> biomeNames;
                      if (!parseZoneFile(p, d, biomeNames)) return false;
                      refs.zoneBiomes.emplace(d.name, std::move(biomeNames));
                      return true;
                  });
    ok &= loadDir(root / "Layers", "layer", outLayers,
                  [&refs](const fs::path& p, LayerDef& d) {
                      std::vector<std::string> zoneNames;
                      if (!parseLayerFile(p, d, zoneNames)) return false;
                      refs.layerZones.emplace(d.name, std::move(zoneNames));
                      return true;
                  });
    if (!ok) return false;

    // ---- 2) resolve name references to ids ---------------------------
    auto zoneIndex    = buildNameIndex(outZones);
    auto biomeIndex   = buildNameIndex(outBiomes);
    auto featureIndex = buildNameIndex(outFeatures);

    // Layers -> zones
    for (LayerDef& layer : outLayers) {
        layer.allowedZoneIds.clear();
        auto parked = refs.layerZones.find(layer.name);
        if (parked != refs.layerZones.end()) {
            for (const std::string& zoneName : parked->second) {
                Uint16 zoneId = 0;
                if (resolveName(zoneIndex, zoneName, zoneId, "zone", layer.name))
                    layer.allowedZoneIds.push_back(zoneId);
            }
        }
        if (layer.allowedZoneIds.empty())
            SDL_Log("[WorldGenDefLoader] layer '%s' has no valid zones — every cell in it "
                    "will fall back to zone 0", layer.name.c_str());
    }

    // Zones -> biomes
    for (ZoneDef& zone : outZones) {
        zone.allowedBiomeIds.clear();
        auto parked = refs.zoneBiomes.find(zone.name);
        if (parked != refs.zoneBiomes.end()) {
            for (const std::string& biomeName : parked->second) {
                Uint16 biomeId = 0;
                if (resolveName(biomeIndex, biomeName, biomeId, "biome", zone.name))
                    zone.allowedBiomeIds.push_back(biomeId);
            }
        }
        if (zone.allowedBiomeIds.empty())
            SDL_Log("[WorldGenDefLoader] zone '%s' has no valid biomes — every cell in it "
                    "will fall back to biome 0", zone.name.c_str());
    }

    // Biomes -> owning zone, and -> features / structures.
    // Looked up by id rather than by index: checkDenseIds() only WARNS
    // about gaps, so outFeatures[id] would be the wrong def in exactly
    // the case we are trying to report.
    auto featureById = [&outFeatures](Uint16 id) -> const FeatureDef* {
        auto it = std::lower_bound(outFeatures.begin(), outFeatures.end(), id,
            [](const FeatureDef& d, Uint16 v) { return d.id < v; });
        return (it != outFeatures.end() && it->id == id) ? &*it : nullptr;
    };

    for (BiomeDef& biome : outBiomes) {
        auto parked = refs.biomeZone.find(biome.name);
        if (parked != refs.biomeZone.end())
            resolveName(zoneIndex, parked->second, biome.zoneId, "zone", biome.name);

        auto resolveList = [&](std::vector<BiomeFeature>& list, FeatureKind expected) {
            std::vector<BiomeFeature> kept;
            kept.reserve(list.size());
            for (BiomeFeature& bf : list) {
                if (!resolveName(featureIndex, bf.featureName, bf.featureId, "feature", biome.name))
                    continue;
                const FeatureDef* def = featureById(bf.featureId);
                if (def == nullptr) continue;   // resolveName already logged
                // A structure listed under "features" would be rolled per
                // column instead of once per region — catch the mix-up now.
                const FeatureKind actual = def->kind;
                if (actual != expected) {
                    SDL_Log("[WorldGenDefLoader] biome '%s' lists '%s' as a %s, but it is "
                            "defined as a %s — dropped", biome.name.c_str(),
                            bf.featureName.c_str(), kindName(expected), kindName(actual));
                    continue;
                }
                kept.push_back(std::move(bf));
            }
            list = std::move(kept);
        };
        resolveList(biome.features,   FeatureKind::Feature);
        resolveList(biome.structures, FeatureKind::Structure);
    }

    // ---- 3) id density (the registry indexes by id) ------------------
    checkDenseIds(outLayers,   "layer");
    checkDenseIds(outZones,    "zone");
    checkDenseIds(outBiomes,   "biome");
    checkDenseIds(outFeatures, "feature");

    if (outLayers.empty()) {
        SDL_Log("[WorldGenDefLoader] no usable layers loaded from '%s'", worldGenDir.c_str());
        return false;
    }

    SDL_Log("[WorldGenDefLoader] loaded %zu layers, %zu zones, %zu biomes, %zu features",
            outLayers.size(), outZones.size(), outBiomes.size(), outFeatures.size());
    return true;
}
