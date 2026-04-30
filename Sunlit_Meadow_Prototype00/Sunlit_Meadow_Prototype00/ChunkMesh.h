#pragma once

#include <vector>
#include <unordered_set>
#include <cstdint>

#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "ChunkTypes.h"

class ChunkMesh
{
public:
    void destroy(AppState* state);

    void draw(
        AppState* state,
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const UBO& ubo
    );


    void buildMesh(std::vector<LocationalBlockID>& blocks);
    bool uploadToGPU(AppState* state, SDL_GPUTexture* textureArrayIn);

    SDL_GPUBuffer* getVertexBuffer() const { return vertexBuffer; }
    SDL_GPUBuffer* getIndexBuffer() const { return indexBuffer; }
    SDL_GPUTexture* getTextureArray() const { return textureArray; }
    uint32_t getNumIndices() const { return numIndices; }

private:
    bool hasBlock(int x, int y, int z) const;

private:
    std::vector<VertexData> vertices;
    std::vector<Uint16> indices;
    std::unordered_set<LocationalBlockID, LocationalBlockIDHash> blockSet;

    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    SDL_GPUTexture* textureArray = nullptr;

    uint32_t numIndices = 0;
};