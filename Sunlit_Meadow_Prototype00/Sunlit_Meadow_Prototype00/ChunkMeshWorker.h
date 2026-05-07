#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include "ThreadSafeQueue.h"
#include "Chunk.h"
#include "ChunkTypes.h"

class ChunkMeshWorker {
public:
    ChunkMeshWorker();
    ~ChunkMeshWorker();

    void start();
    void stop();

    void requestChunk(Chunk chunk, ChunkBorderAir borderAir);
    std::optional<Chunk> tryGetChunk();
    bool cancelRequest(ChunkCoord coord);

private:
    void workerLoop();

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    ThreadSafeQueue_2T<Chunk, ChunkBorderAir>  m_inputQueue;
    ThreadSafeQueue<Chunk>                     m_outputQueue;
};