#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include "ThreadSafeQueue.h"
#include "Chunk.h"
#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "PalettedGrid2D.h"
#include "BlockManager.h"
#include "FastNoiseLite.h"
#include "WorldGenNoise.h"

class ChunkGeneratorWorker {
public:
    ChunkGeneratorWorker();
    ~ChunkGeneratorWorker();

    // layer/zoneMap/biomeMap point into the OWNING Region. They are
    // immutable after Region's constructor and the region stops this
    // worker before they die, so reading them from the worker thread
    // without a lock is safe (see Region.h).
    // worldGenNoise is WorldManager-owned, init-once/read-only — same
    // lock-free sharing contract as the maps (see WorldGenNoise.h).
    void start(BlockManager* blockManager, FastNoiseLite* standartNoise,
        const WorldGenNoise* worldGenNoise, int regionChunkZStart,
        const LayerDef* layer, const PalettedGrid2D* zoneMap, const PalettedGrid2D* biomeMap);
    void stop();

    void requestColumn(ColumnCoord coord);
    std::optional<std::unique_ptr<Chunk>> tryGetChunk();
    bool cancelColumn(ColumnCoord coord);

private:
    void workerLoop();
    
    int totalChunksGenerated;
    Uint64 times[100] = { 0 };
    float s;

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    int m_regionChunkZStart = 0;
    BlockManager* m_blockManager = nullptr;
    // standartNoise no longer feeds the shape pass (WorldGenNoise does);
    // kept plumbed for the upcoming features determinism pass.
    FastNoiseLite* m_standartNoise = nullptr;
    const WorldGenNoise* m_worldGenNoise = nullptr;

    // Read-only worldgen context, owned by the Region (see start()).
    const LayerDef*       m_layer = nullptr;
    const PalettedGrid2D* m_zoneMap = nullptr;
    const PalettedGrid2D* m_biomeMap = nullptr;

    ThreadSafeQueue<ColumnCoord>              m_inputQueue;
    ThreadSafeQueue<std::unique_ptr<Chunk>>  m_outputQueue;
};
