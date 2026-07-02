#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include "ThreadSafeQueue.h"
#include "Chunk.h"
#include "WorldTypes.h"
#include "BlockManager.h"
#include "FastNoiseLite.h"

class ChunkGeneratorWorker {
public:
    ChunkGeneratorWorker();
    ~ChunkGeneratorWorker();

    void start(BlockManager* blockManager, FastNoiseLite* standartNoise, int regionChunkZStart);
    void stop();

    void requestColumn(ColumnCoord coord);
    std::optional<std::unique_ptr<Chunk>> tryGetChunk();
    bool cancelColumn(ColumnCoord coord);

private:
    void workerLoop();
    
    int totalChunkGenerated;
    Uint64 lastTicks;
    Uint64 times[100] = { 0 };
    int frame;
    float s;

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    int m_regionChunkZStart = 0;
    BlockManager* m_blockManager = nullptr;
    FastNoiseLite* m_standartNoise = nullptr;

    ThreadSafeQueue<ColumnCoord>              m_inputQueue;
    ThreadSafeQueue<std::unique_ptr<Chunk>>  m_outputQueue;
};