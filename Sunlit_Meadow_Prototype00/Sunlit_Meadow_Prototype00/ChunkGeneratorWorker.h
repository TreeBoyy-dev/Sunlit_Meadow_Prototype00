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

class WorldGenRegistry;

class ChunkGeneratorWorker {
public:
    ChunkGeneratorWorker();
    ~ChunkGeneratorWorker();

    // layer/zoneMap/biomeMap point into the OWNING Region. They are
    // immutable after Region::setShape and the region stops this worker
    // before they die, so reading them from the worker thread without a
    // lock is safe (see Region.h).
    // worldGenNoise and worldGenRegistry are WorldManager-owned,
    // init-once/read-only — same lock-free sharing contract as the maps
    // (see WorldGenNoise.h / WorldGenRegistry.h).
    // regionChunkStart is the region's bottom-left-bottom chunk coord: z
    // offsets the column, x/y turn absolute column coords into the
    // region-local ones the biome map is indexed by.
    void start(BlockManager* blockManager, FastNoiseLite* standartNoise,
        const WorldGenNoise* worldGenNoise, const WorldGenRegistry* worldGenRegistry,
        ChunkCoord regionChunkStart, Uint64 worldSeed,
        const LayerDef* layer, const PalettedGrid2D* zoneMap, const PalettedGrid2D* biomeMap);
    void stop();

    void requestColumn(ColumnCoord coord);
    std::optional<std::unique_ptr<Chunk>> tryGetChunk();
    bool cancelColumn(ColumnCoord coord);

private:
    void workerLoop();
    
    int totalChunksGenerated;
    // the old average stored dt in SECONDS, divided by a fixed
    // 100, and indexed the ring by totalChunksGenerated (which jumps +32 per
    // column) — the printed "ms" was wrong in unit AND window. Now: samples
    // are per-COLUMN milliseconds, ring indexed by columns generated, divided
    // by how many samples the ring actually holds.
    int   totalColumnsGenerated = 0;
    float times[100] = { 0.0f };
    float s = 0.0f;

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    ChunkCoord m_regionChunkStart = { 0, 0, 0 };
    Uint64 m_worldSeed = 0;
    BlockManager* m_blockManager = nullptr;
    // standartNoise no longer feeds any worldgen pass: the shape pass uses
    // WorldGenNoise and the feature pass uses WorldGenRandom (seeded from
    // m_worldSeed). Kept plumbed for the callers that still pass it.
    FastNoiseLite* m_standartNoise = nullptr;
    const WorldGenNoise* m_worldGenNoise = nullptr;
    const WorldGenRegistry* m_worldGenRegistry = nullptr;

    // Read-only worldgen context, owned by the Region (see start()).
    const LayerDef*       m_layer = nullptr;
    const PalettedGrid2D* m_zoneMap = nullptr;
    const PalettedGrid2D* m_biomeMap = nullptr;

    ThreadSafeQueue<ColumnCoord>              m_inputQueue;
    ThreadSafeQueue<std::unique_ptr<Chunk>>  m_outputQueue;
};
