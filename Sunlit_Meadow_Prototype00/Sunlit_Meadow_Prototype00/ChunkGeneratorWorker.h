#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include "ThreadSafeQueue.h"
#include "Chunk.h"
#include "ChunkTypes.h"

class ChunkGeneratorWorker {
public:
    ChunkGeneratorWorker();
    ~ChunkGeneratorWorker();

    void start();
    void stop();

    void requestChunk(ChunkCoord coord);
    // Returns a fully generated (but not yet mesh-init'd) Chunk, or nullopt
    std::optional<std::unique_ptr<Chunk>> tryGetChunk();

private:
    void workerLoop();

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    ThreadSafeQueue<ChunkCoord>              m_inputQueue;
    ThreadSafeQueue<std::unique_ptr<Chunk>>  m_outputQueue;
};