#pragma once

#include <vector>
#include <unordered_set>
#include <cstdint>

#include <SDL3/SDL.h>
#include "DataStructures.h"
#include "Vectors.h"
#include "Block.h"

struct BlockCoordHasher
{
    std::size_t operator()(const BlockCoordinates& b) const
    {
        std::size_t hx = std::hash<int>{}(b.x);
        std::size_t hy = std::hash<int>{}(b.y);
        std::size_t hz = std::hash<int>{}(b.z);

        return hx ^ (hy << 1) ^ (hz << 2);
    }
};

class ChunkMesh
{
public:
    bool init(
        AppState* state,
        std::vector<Block>& blocks,
        SDL_GPUTexture* textureArray
    );

    void destroy(AppState* state);

    void draw(
        AppState* state,
        SDL_GPUCommandBuffer* cmd,
        SDL_GPURenderPass* pass,
        const UBO& ubo
    );

    SDL_GPUBuffer* getVertexBuffer() const { return vertexBuffer; }
    SDL_GPUBuffer* getIndexBuffer() const { return indexBuffer; }
    SDL_GPUTexture* getTextureArray() const { return textureArray; }
    uint32_t getNumIndices() const { return numIndices; }

private:
    void buildMesh(std::vector<Block>& blocks);
    bool uploadToGPU(AppState* state);
    bool hasBlock(int x, int y, int z) const;

    void addFace(
        const Vec3& v0,
        const Vec3& v1,
        const Vec3& v2,
        const Vec3& v3,
        int materialIndex
    );

private:
    std::vector<VertexData> vertices;
    std::vector<uint16_t> indices;
    std::unordered_set<BlockCoordinates, BlockCoordHasher> blockSet;

    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    SDL_GPUTexture* textureArray = nullptr;

    uint32_t numIndices = 0;
};