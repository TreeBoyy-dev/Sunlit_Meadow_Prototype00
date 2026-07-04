#include "BlockDefLoader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------
// Internal: a parsed template file, before any block references it.
// ---------------------------------------------------------------------
namespace {

struct TemplateDef {
    std::string name;
    std::vector<StatePropDef> states;

    // variants mode
    bool hasVariants = false;
    std::string geometry;
    std::string rotateBy;
    std::string flipZBy;
    std::vector<std::pair<std::string, std::string>> shapeGeometry;

    // multipart mode
    bool hasMultipart = false;
    std::vector<MultipartDef> parts;

    // defaults a block may override
    bool hasObstructs = false;
    std::array<bool, 6> obstructs{ false, false, false, false, false, false };
    bool hasCollision = false;
    Collision collision;
    bool hasTransparent = false;
    bool transparent = false;
};

bool parseObstructsArray(const json& j, std::array<bool, 6>& out, const char* where) {
    if (!j.is_array() || j.size() != 6) {
        SDL_Log("[BlockDefLoader] %s: 'obstructs' must be an array of 6 bools", where);
        return false;
    }
    for (int i = 0; i < 6; ++i) out[i] = j[i].get<bool>();
    return true;
}

bool parseCollision(const json& j, Collision& out, const char* where) {
    if (!j.is_object()) {
        SDL_Log("[BlockDefLoader] %s: 'collision' must be an object", where);
        return false;
    }
    out.solid = j.value("solid", true);
    out.slowdown = j.value("slowdown", 0);
    out.boxes.clear();
    if (j.contains("boxes")) {
        for (const json& b : j["boxes"]) {
            if (!b.is_array() || b.size() != 6) {
                SDL_Log("[BlockDefLoader] %s: collision box must be "
                        "[x0,y0,z0,x1,y1,z1]", where);
                return false;
            }
            AABB box;
            box.min = { b[0].get<float>(), b[1].get<float>(), b[2].get<float>() };
            box.max = { b[3].get<float>(), b[4].get<float>(), b[5].get<float>() };
            out.boxes.push_back(box);
        }
    }
    return true;
}

// A material value is either "COBBLESTONE" or
// { "material": "GRASS_BLOCK_SIDE", "overlay": "GRASS_BLOCK_SIDE_OV" }.
bool parseModelFace(const json& j, ModelFace& out, const char* where) {
    std::string matName;
    std::string overlayName;
    if (j.is_string()) {
        matName = j.get<std::string>();
    }
    else if (j.is_object()) {
        matName = j.value("material", "");
        overlayName = j.value("overlay", "");
    }
    else {
        SDL_Log("[BlockDefLoader] %s: material must be a string or "
                "{material, overlay} object", where);
        return false;
    }

    Material m = materialFromName(matName);
    if (m == MATERIAL_COUNT) {
        SDL_Log("[BlockDefLoader] %s: unknown material name '%s'", where, matName.c_str());
        return false;
    }
    out.material = m;
    out.overlayMaterial = -1;
    if (!overlayName.empty()) {
        Material ov = materialFromName(overlayName);
        if (ov == MATERIAL_COUNT) {
            SDL_Log("[BlockDefLoader] %s: unknown overlay material '%s'",
                    where, overlayName.c_str());
            return false;
        }
        out.overlayMaterial = (int)ov;
    }
    return true;
}

// "materials" is either one string (all three faces) or an object with any
// of top/bottom/side. Missing keys fall back: top -> side, bottom -> top.
bool parseMaterials(const json& j, BlockDef& def, const char* where) {
    if (j.is_string()) {
        ModelFace f;
        if (!parseModelFace(j, f, where)) return false;
        def.top = def.bottom = def.side = f;
        return true;
    }
    if (!j.is_object()) {
        SDL_Log("[BlockDefLoader] %s: 'materials' must be a string or object", where);
        return false;
    }

    bool haveSide = j.contains("side");
    bool haveTop = j.contains("top");
    bool haveBottom = j.contains("bottom");
    if (!haveSide && !haveTop && !haveBottom) {
        SDL_Log("[BlockDefLoader] %s: 'materials' object is empty", where);
        return false;
    }

    if (haveSide && !parseModelFace(j["side"], def.side, where)) return false;
    if (haveTop) { if (!parseModelFace(j["top"], def.top, where)) return false; }
    else def.top = def.side;
    if (haveBottom) { if (!parseModelFace(j["bottom"], def.bottom, where)) return false; }
    else def.bottom = def.top;
    if (!haveSide) def.side = def.top;
    return true;
}

bool parseStates(const json& j, std::vector<StatePropDef>& out, const char* where) {
    if (!j.is_array()) {
        SDL_Log("[BlockDefLoader] %s: 'states' must be an array", where);
        return false;
    }
    Uint32 totalBits = 0;
    for (const json& s : j) {
        StatePropDef p;
        p.name = s.value("name", "");
        p.type = statePropTypeFromName(s.value("type", ""));
        if (p.name.empty() || p.type == StatePropType::Invalid) {
            SDL_Log("[BlockDefLoader] %s: bad state entry (name '%s', type '%s')",
                    where, p.name.c_str(), s.value("type", "").c_str());
            return false;
        }
        totalBits += statePropTypeBits(p.type);
        out.push_back(p);
    }
    // Bits 0..14 belong to the template; bit 15 is the fluid bit.
    if (totalBits > 15) {
        SDL_Log("[BlockDefLoader] %s: states need %u bits, only 15 are available "
                "(bit 15 is the fluid bit)", where, totalBits);
        return false;
    }
    return true;
}

bool parseTemplateFile(const fs::path& path, TemplateDef& t) {
    std::ifstream file(path);
    if (!file.is_open()) {
        SDL_Log("[BlockDefLoader] cannot open template '%s'", path.string().c_str());
        return false;
    }
    json j = json::parse(file, nullptr, false);
    if (j.is_discarded()) {
        SDL_Log("[BlockDefLoader] template '%s' is not valid JSON", path.string().c_str());
        return false;
    }
    const std::string where = path.filename().string();
    t.name = path.stem().string();

    try {
        if (j.contains("states") && !parseStates(j["states"], t.states, where.c_str()))
            return false;

        if (j.contains("variants")) {
            const json& v = j["variants"];
            t.hasVariants = true;
            t.geometry = v.value("geometry", "");
            t.rotateBy = v.value("rotateBy", "");
            t.flipZBy = v.value("flipZBy", "");
            if (v.contains("shapeGeometry"))
                for (auto& [key, val] : v["shapeGeometry"].items())
                    t.shapeGeometry.emplace_back(key, val.get<std::string>());
        }

        if (j.contains("multipart")) {
            t.hasMultipart = true;
            for (const json& p : j["multipart"]) {
                MultipartDef mp;
                mp.part = p.value("part", "");
                mp.rotate = p.value("rotate", 0);
                if (p.contains("when")) {
                    const json& w = p["when"];
                    if (!w.is_object() || w.size() != 1) {
                        SDL_Log("[BlockDefLoader] %s: 'when' must be an object with "
                                "exactly one { property: value } pair", where.c_str());
                        return false;
                    }
                    auto it = w.items().begin();
                    mp.whenProp = it.key();
                    mp.whenValue = it.value().get<std::string>();
                }
                if (mp.part.empty()) {
                    SDL_Log("[BlockDefLoader] %s: multipart entry without 'part'", where.c_str());
                    return false;
                }
                t.parts.push_back(std::move(mp));
            }
        }

        if (t.hasVariants && t.hasMultipart) {
            SDL_Log("[BlockDefLoader] %s: a template uses either 'variants' or "
                    "'multipart', not both", where.c_str());
            return false;
        }
        if (!t.hasVariants && !t.hasMultipart) {
            SDL_Log("[BlockDefLoader] %s: template has neither 'variants' nor "
                    "'multipart'", where.c_str());
            return false;
        }

        if (j.contains("obstructs")) {
            const json& o = j["obstructs"];
            // Templates use { "default": [...] } so per-variant overrides can
            // slot in later without a schema change.
            const json& arr = o.is_object() ? o["default"] : o;
            if (!parseObstructsArray(arr, t.obstructs, where.c_str())) return false;
            t.hasObstructs = true;
        }
        if (j.contains("collision")) {
            if (!parseCollision(j["collision"], t.collision, where.c_str())) return false;
            t.hasCollision = true;
        }
        if (j.contains("transparent")) {
            t.transparent = j["transparent"].get<bool>();
            t.hasTransparent = true;
        }
    }
    catch (const std::exception& e) {
        SDL_Log("[BlockDefLoader] %s: %s", where.c_str(), e.what());
        return false;
    }
    return true;
}

bool parseBlockFile(
    const fs::path& path,
    const std::unordered_map<std::string, TemplateDef>& templates,
    BlockDef& def
) {
    std::ifstream file(path);
    if (!file.is_open()) {
        SDL_Log("[BlockDefLoader] cannot open block '%s'", path.string().c_str());
        return false;
    }
    json j = json::parse(file, nullptr, false);
    if (j.is_discarded()) {
        SDL_Log("[BlockDefLoader] block '%s' is not valid JSON", path.string().c_str());
        return false;
    }
    const std::string where = path.filename().string();

    try {
        def.name = j.value("name", "");
        if (def.name.empty()) {
            SDL_Log("[BlockDefLoader] %s: missing 'name'", where.c_str());
            return false;
        }
        // Explicit, mandatory id: registration-order ids would silently
        // corrupt saved worlds once the directory scan order changes.
        if (!j.contains("id") || !j["id"].is_number_unsigned()) {
            SDL_Log("[BlockDefLoader] %s: missing mandatory numeric 'id'", where.c_str());
            return false;
        }
        def.id = (Uint16)j["id"].get<unsigned>();

        def.templateName = j.value("template", "");
        auto it = templates.find(def.templateName);
        if (it == templates.end()) {
            SDL_Log("[BlockDefLoader] %s: unknown template '%s' — block skipped",
                    where.c_str(), def.templateName.c_str());
            return false;
        }
        const TemplateDef& t = it->second;

        // ---- merge template (resolved in memory, never copied to disk) ----
        def.states = t.states;
        def.multipart = t.hasMultipart;
        def.parts = t.parts;
        def.rotateBy = t.rotateBy;
        def.flipZBy = t.flipZBy;
        def.shapeGeometry = t.shapeGeometry;
        // The block may override the template geometry (e.g. flowers reusing
        // the 'cross' template with their own .obj).
        def.geometry = j.value("geometry", t.geometry);
        if (!def.multipart && def.geometry.empty()) {
            SDL_Log("[BlockDefLoader] %s: no geometry (neither template '%s' nor "
                    "the block provide one)", where.c_str(), def.templateName.c_str());
            return false;
        }

        // obstructs: block > template > conservative default. For templates
        // with states the safe default is "obstructs nothing" (correct
        // rendering, a few more faces drawn).
        if (j.contains("obstructs")) {
            if (!parseObstructsArray(j["obstructs"], def.obstructs, where.c_str()))
                return false;
        }
        else if (t.hasObstructs) def.obstructs = t.obstructs;
        else def.obstructs = { false, false, false, false, false, false };

        if (j.contains("collision")) {
            if (!parseCollision(j["collision"], def.collision, where.c_str()))
                return false;
        }
        else if (t.hasCollision) def.collision = t.collision;
        else def.collision = Collision{}; // solid full cube

        if (j.contains("transparent")) def.transparent = j["transparent"].get<bool>();
        else if (t.hasTransparent)     def.transparent = t.transparent;

        if (!j.contains("materials")) {
            SDL_Log("[BlockDefLoader] %s: missing 'materials'", where.c_str());
            return false;
        }
        if (!parseMaterials(j["materials"], def, where.c_str())) return false;

        // rotateBy / flipZBy must name declared properties.
        auto declared = [&](const std::string& n) {
            for (const StatePropDef& p : def.states) if (p.name == n) return true;
            return false;
        };
        if (!def.rotateBy.empty() && !declared(def.rotateBy)) {
            SDL_Log("[BlockDefLoader] %s: rotateBy names unknown property '%s'",
                    where.c_str(), def.rotateBy.c_str());
            return false;
        }
        if (!def.flipZBy.empty() && !declared(def.flipZBy)) {
            SDL_Log("[BlockDefLoader] %s: flipZBy names unknown property '%s'",
                    where.c_str(), def.flipZBy.c_str());
            return false;
        }
    }
    catch (const std::exception& e) {
        SDL_Log("[BlockDefLoader] %s: %s", where.c_str(), e.what());
        return false;
    }
    return true;
}

} // namespace

// ---------------------------------------------------------------------
bool BlockDefLoader::loadAll(
    const std::string& templatesDir,
    const std::string& blocksDir,
    std::vector<BlockDef>& out
) {
    std::error_code ec;

    // ---- 1) templates ----
    std::unordered_map<std::string, TemplateDef> templates;
    if (!fs::is_directory(templatesDir, ec)) {
        SDL_Log("[BlockDefLoader] template directory missing: '%s'", templatesDir.c_str());
        return false;
    }
    for (const fs::directory_entry& e : fs::directory_iterator(templatesDir, ec)) {
        if (!e.is_regular_file() || e.path().extension() != ".json") continue;
        TemplateDef t;
        if (parseTemplateFile(e.path(), t))
            templates[t.name] = std::move(t);
    }
    SDL_Log("[BlockDefLoader] loaded %zu block templates", templates.size());

    // ---- 2) blocks ----
    if (!fs::is_directory(blocksDir, ec)) {
        SDL_Log("[BlockDefLoader] block directory missing: '%s'", blocksDir.c_str());
        return false;
    }
    std::unordered_set<Uint16> seenIds;
    std::unordered_set<std::string> seenNames;
    for (const fs::directory_entry& e : fs::directory_iterator(blocksDir, ec)) {
        if (!e.is_regular_file() || e.path().extension() != ".json") continue;
        BlockDef def;
        if (!parseBlockFile(e.path(), templates, def)) continue;

        if (seenIds.count(def.id)) {
            SDL_Log("[BlockDefLoader] duplicate block id %u ('%s') — block skipped",
                    def.id, def.name.c_str());
            continue;
        }
        if (seenNames.count(def.name)) {
            SDL_Log("[BlockDefLoader] duplicate block name '%s' — block skipped",
                    def.name.c_str());
            continue;
        }
        seenIds.insert(def.id);
        seenNames.insert(def.name);
        out.push_back(std::move(def));
    }

    // Deterministic order regardless of filesystem enumeration order.
    std::sort(out.begin(), out.end(),
              [](const BlockDef& a, const BlockDef& b) { return a.id < b.id; });

    SDL_Log("[BlockDefLoader] loaded %zu block definitions", out.size());
    return true;
}
