#define _CRT_SECURE_NO_WARNINGS
#define SDL_MAIN_HANDLED
#include "ObjParser.h"
#include "DataStructures.h"
#include "Vectors.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <cstdio>

const Vec3 MODEL_ROTATION = { 90.0f, 0.0f, -90.0f };

void rotate_model(std::vector<EntityVertex>& vertices, Vec3 eulerDegrees)
{
    const float deg2rad = 3.14159265358979323846f / 180.0f;
    float ax = eulerDegrees.x * deg2rad;
    float ay = eulerDegrees.y * deg2rad;
    float az = eulerDegrees.z * deg2rad;

    float sx = std::sin(ax), cx = std::cos(ax);
    float sy = std::sin(ay), cy = std::cos(ay);
    float sz = std::sin(az), cz = std::cos(az);

    for (EntityVertex& v : vertices) {
        v.position = vec3rotate(v.position, sx, cx, sy, cy, sz, cz);
        v.normal = vec3rotate(v.normal, sx, cx, sy, cy, sz, cz);
    }
}

bool obj_parse(
    std::string path,
    std::vector<EntityVertex>& outVertices,
    std::vector<Uint16>& outIndices)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        SDL_Log("obj_parse: failed to open '%s'", path);
        return false;
    }

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // Map packed (posIdx, uvIdx, normIdx) -> output vertex index
    std::unordered_map<uint64_t, Uint16> vertexCache;

    auto packKey = [](int p, int t, int n) -> uint64_t {
        // +1 so that -1 (missing) becomes 0 and stays distinct
        uint64_t pp = (uint32_t)(p + 1);
        uint64_t tt = (uint32_t)(t + 1);
        uint64_t nn = (uint32_t)(n + 1);
        return pp | (tt << 21) | (nn << 42);
        };

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            Vec3 p{};
            iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (prefix == "vn") {
            Vec3 n{};
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (prefix == "vt") {
            Vec2 t{};
            iss >> t.x >> t.y;
            t.y = 1.0f - t.y;
            uvs.push_back(t);
        }
        else if (prefix == "f") {
            std::vector<Uint16> faceIdx;
            faceIdx.reserve(4);

            std::string token;
            while (iss >> token) {
                int vi = 0, ti = 0, ni = 0;

                // Try formats in order: v/t/n, v//n, v/t, v
                if (std::sscanf(token.c_str(), "%d/%d/%d", &vi, &ti, &ni) == 3) {
                }
                else if (std::sscanf(token.c_str(), "%d//%d", &vi, &ni) == 2) {
                    ti = 0;
                }
                else if (std::sscanf(token.c_str(), "%d/%d", &vi, &ti) == 2) {
                    ni = 0;
                }
                else if (std::sscanf(token.c_str(), "%d", &vi) == 1) {
                    ti = 0; ni = 0;
                }
                else {
                    continue; // malformed token
                }

                // Resolve indices: OBJ is 1-based; negative is relative-from-end
                int pIdx = (vi > 0) ? vi - 1 : (int)positions.size() + vi;
                int tIdx = (ti > 0) ? ti - 1 : (ti < 0 ? (int)uvs.size() + ti : -1);
                int nIdx = (ni > 0) ? ni - 1 : (ni < 0 ? (int)normals.size() + ni : -1);

                if (pIdx < 0 || pIdx >= (int)positions.size()) {
                    SDL_Log("obj_parse: bad position index in '%s'", token.c_str());
                    return false;
                }

                uint64_t key = packKey(pIdx, tIdx, nIdx);
                auto it = vertexCache.find(key);
                Uint16 outIdx;

                if (it != vertexCache.end()) {
                    outIdx = it->second;
                }
                else {
                    if (outVertices.size() >= 65535) {
                        SDL_Log("obj_parse: '%s' exceeds Uint16 index limit (65535 verts)", path);
                        return false;
                    }

                    EntityVertex v{};
                    v.position = positions[pIdx];
                    v.normal = (nIdx >= 0) ? normals[nIdx] : Vec3{ 0.0f, 1.0f, 0.0f };
                    v.uv = (tIdx >= 0) ? uvs[tIdx] : Vec2{ 0.0f, 0.0f };
                    v.color = SDL_FColor{ 1.0f, 1.0f, 1.0f, 1.0f };

                    outIdx = (Uint16)outVertices.size();
                    outVertices.push_back(v);
                    vertexCache.emplace(key, outIdx);
                }

                faceIdx.push_back(outIdx);
            }

            // Fan-triangulate (handles tris, quads, n-gons; assumes convex & planar)
            for (size_t i = 1; i + 1 < faceIdx.size(); ++i) {
                outIndices.push_back(faceIdx[0]);
                outIndices.push_back(faceIdx[i]);
                outIndices.push_back(faceIdx[i + 1]);
            }
        }
        // ignored: o, g, s, mtllib, usemtl (material comes from the parameter)
    }
    // Bake the orientation fix into the loaded geometry (Blockbench Y-up -> game space).
    rotate_model(outVertices, MODEL_ROTATION);

    return true;
}