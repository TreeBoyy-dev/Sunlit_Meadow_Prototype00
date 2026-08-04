#include "WorldGenTypes.h"

// =====================================================================
//  Enum <-> JSON name tables.
//  Kept next to each other so adding a generator is one edit here plus
//  one case in the matching generate*() switch.
// =====================================================================

namespace {

struct ShapeGenRow { ShapeGenerator g; const char* name; };
const ShapeGenRow kShapeGens[] = {
    { ShapeGenerator::SplineTerrain, "spline_terrain" },
    { ShapeGenerator::SolidFill,     "solid_fill"     },
    { ShapeGenerator::AirFill,       "air_fill"       },
};

struct FeatureGenRow { FeatureGenerator g; const char* name; };
const FeatureGenRow kFeatureGens[] = {
    { FeatureGenerator::Scatter, "scatter" },
    { FeatureGenerator::Tree,    "tree"    },
    { FeatureGenerator::Blob,    "blob"    },
    { FeatureGenerator::Prefab,  "prefab"  },
};

} // namespace

ShapeGenerator shapeGeneratorFromName(const std::string& name) {
    for (const ShapeGenRow& r : kShapeGens)
        if (name == r.name) return r.g;
    return ShapeGenerator::Invalid;
}

const char* shapeGeneratorName(ShapeGenerator g) {
    for (const ShapeGenRow& r : kShapeGens)
        if (g == r.g) return r.name;
    return "invalid";
}

FeatureGenerator featureGeneratorFromName(const std::string& name) {
    for (const FeatureGenRow& r : kFeatureGens)
        if (name == r.name) return r.g;
    return FeatureGenerator::Invalid;
}

const char* featureGeneratorName(FeatureGenerator g) {
    for (const FeatureGenRow& r : kFeatureGens)
        if (g == r.g) return r.name;
    return "invalid";
}

FeatureKind kindFromName(const std::string& name) {
    return name == "structure" ? FeatureKind::Structure : FeatureKind::Feature;
}

const char* kindName(FeatureKind k) {
    return k == FeatureKind::Structure ? "structure" : "feature";
}
