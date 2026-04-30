#include "ChunkWorker.h"

ChunkWorker::ChunkWorker() : m_running(false) {}
ChunkWorker::~ChunkWorker() { stop(); }

void ChunkWorker::start() {
    m_running = true;
    m_thread = std::thread(&ChunkWorker::workerLoop, this);
}

void ChunkWorker::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void ChunkWorker::requestChunk(ChunkCoord coord) {
    m_inputQueue.push(coord);
}

// Returns a fully generated (but not yet mesh-init'd) Chunk, or nullopt
std::optional<std::unique_ptr<Chunk>> ChunkWorker::tryGetChunk() {
    return m_outputQueue.try_pop();
}

void ChunkWorker::workerLoop() {
    while (m_running) {
        if (auto coord = m_inputQueue.try_pop()) {
            auto chunk = std::make_unique<Chunk>(*coord);

            chunk->getChunkGenerated();
            chunk->createMeshes();

            m_outputQueue.push(std::move(chunk));
        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    }
}