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

    void start(BlockManager* blockManager, FastNoiseLite* standartNoise);
    void stop();

    void requestChunk(ChunkCoord coord);
    std::optional<std::unique_ptr<Chunk>> tryGetChunk();
    bool cancelRequest(ChunkCoord coord);

private:
    void workerLoop();

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    BlockManager* m_blockManager = nullptr;
    FastNoiseLite* m_standartNoise = nullptr;

    ThreadSafeQueue<ChunkCoord>              m_inputQueue;
    ThreadSafeQueue<std::unique_ptr<Chunk>>  m_outputQueue;
};