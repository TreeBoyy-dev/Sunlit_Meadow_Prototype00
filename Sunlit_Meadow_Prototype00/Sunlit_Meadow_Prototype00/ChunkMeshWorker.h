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

    void requestChunk(Chunk* chunk, ChunkBorderAir borderAir);
    // Returns a fully generated (but not yet mesh-init'd) Chunk, or nullopt
    std::optional<ChunkCoord> tryGetChunk();
    bool cancelRequest(ChunkCoord coord);

private:
    void workerLoop();

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    ThreadSafeQueue_2T<Chunk*, ChunkBorderAir>  m_inputQueue;
    ThreadSafeQueue<ChunkCoord>                 m_outputQueue;
};