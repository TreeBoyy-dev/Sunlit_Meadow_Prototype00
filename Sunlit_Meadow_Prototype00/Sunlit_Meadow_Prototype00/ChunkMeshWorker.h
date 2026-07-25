#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include "ThreadSafeQueue.h"
#include "Chunk.h"
#include "WorldTypes.h"
#include "BlockManager.h"

class ChunkMeshWorker {
public:
    ChunkMeshWorker();
    ~ChunkMeshWorker();

    void start(BlockManager* blockManager);
    void stop();

    // chunks travel as unique_ptr. The old by-value API copied the
    // whole chunk (both PalettedContainers + mesh vectors) THREE times per
    // remesh request: into the queue, out of the queue, into the output.
    // Now the caller makes exactly ONE snapshot copy and ownership moves.
    void requestChunk(std::unique_ptr<Chunk> chunk);
    std::optional<std::unique_ptr<Chunk>> tryGetChunk();
    bool cancelRequest(ChunkCoord coord);

private:
    void workerLoop();

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    BlockManager* m_blockManager = nullptr;

    ThreadSafeQueue<std::unique_ptr<Chunk>> m_inputQueue;
    ThreadSafeQueue<std::unique_ptr<Chunk>> m_outputQueue;
};