#pragma once

#include <vector>
#include <unordered_set>
#include <cstdint>

#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "WorldTypes.h"
#include "BlockManager.h"
#include "PalettedContainer.h"

class ChunkMesh
{
public:
    // Filled by buildMesh() / optimizeMesh(); read via stats(). One struct per
    // mesh (opaque / transparent), reset at the start of every buildMesh.
    struct BuildStats {
        double   buildMs      = 0.0;  // buildMesh wall time
        double   cullMs       = 0.0;  // faceCulling wall time
        double   greedyMs     = 0.0;  // greedyMeshing wall time
        uint32_t vertsEmitted = 0;    // vertices after buildMesh
        uint32_t indsEmitted  = 0;    // indices after buildMesh
        uint32_t cellsEmitted = 0;    // cells that reached generateMeshFromModel
        uint32_t emptyEmits   = 0;    // fully-visible cells that produced ZERO vertices
        uint32_t unknownIds   = 0;    // cells whose id getById() didn't know
        uint32_t quadsCulled  = 0;    // quads removed by (residual) faceCulling
        uint32_t facesHidden  = 0;    
    };
    const BuildStats& stats() const { return m_stats; }

    void destroy(AppState* state);

    void draw(
        AppState* state,
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const UBO& ubo
    );

    void buildMesh(
        PalettedContainer* storage,
        ChunkCoord chunkCoords,
        BlockManager& blockManager,
        bool isTranperent);
    void optimizeMesh();
    bool uploadToGPU(AppState* state, SDL_GPUTexture* textureArrayIn);

    SDL_GPUBuffer* getVertexBuffer() const { return vertexBuffer; }
    SDL_GPUBuffer* getIndexBuffer() const { return indexBuffer; }
    SDL_GPUTexture* getTextureArray() const { return textureArray; }
    uint32_t getNumIndices() const { return numIndices; }

private:
    void faceCulling();
    void greedyMeshing();

private:
    bool isTranperentMesh = false;

    std::vector<WorldVertex> vertices;
    std::vector<Uint32> indices;

    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    SDL_GPUTexture* textureArray = nullptr;
    ChunkCoord m_chunkCoord = { 0, 0, 0 };
    uint32_t numIndices = 0;

    BuildStats m_stats;
};