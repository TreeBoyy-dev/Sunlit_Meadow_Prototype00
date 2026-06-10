#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

#include "WorldTypes.h"
#include "Chunk.h"
#include "ChunkGeneratorWorker.h"
#include "ChunkMeshWorker.h"

class Region {
private:
    RegionCoord regionCoordinates;

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;

    std::unordered_set<ColumnCoord, ColumnCoordHash>    requestedColumns;
    std::unordered_set<ChunkCoord,  ChunkCoordHash>     pendingMeshChunks;
    std::unordered_set<ChunkCoord,  ChunkCoordHash>     meshedChunks;

    ChunkGeneratorWorker g_worker;
    ChunkMeshWorker m_worker;

    BlockManager* m_blockManager = nullptr;
    FastNoiseLite* m_standartNoise = nullptr;
public:
    Region(
        RegionCoord regionCoordinates,
        BlockManager* blockManager,
        FastNoiseLite* standartNoise);
    ~Region();

    Chunk* getChunk(ChunkCoord chunkCoordinates);

    void requestChunkGeneration(ChunkCoord chunkCoordinates);
    void cancelChunkGeneration(ChunkCoord chunkCoordinates);

    bool update(AppState* state,
        SDL_GPUTexture* textureArray,
        std::vector<ChunkCoord>& outNewlyReady);

    RegionCoord getCoordinates();
    void destroyRegion(AppState* state);

private:
    void queueMeshUpdate(ChunkCoord coord);
    bool collectMeshResults(AppState* state,
        SDL_GPUTexture* textureArray,
        std::vector<ChunkCoord>& outNewlyReady);
};