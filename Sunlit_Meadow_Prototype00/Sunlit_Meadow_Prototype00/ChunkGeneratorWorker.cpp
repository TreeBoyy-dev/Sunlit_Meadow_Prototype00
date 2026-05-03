#include "ChunkGeneratorWorker.h"

ChunkGeneratorWorker::ChunkGeneratorWorker() : m_running(false) {}
ChunkGeneratorWorker::~ChunkGeneratorWorker() { stop(); }

void ChunkGeneratorWorker::start() {
    m_running = true;
    m_thread = std::thread(&ChunkGeneratorWorker::workerLoop, this);
}

void ChunkGeneratorWorker::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void ChunkGeneratorWorker::requestChunk(ChunkCoord coord) {
    m_inputQueue.push(coord);
}

// Returns a fully generated (but not yet mesh-init'd) Chunk, or nullopt
std::optional<std::unique_ptr<Chunk>> ChunkGeneratorWorker::tryGetChunk() {
    return m_outputQueue.try_pop();
}

void ChunkGeneratorWorker::workerLoop() {
    while (m_running) {
        if (auto coord = m_inputQueue.try_pop()) {
            auto chunk = std::make_unique<Chunk>(*coord);

            chunk->getChunkGenerated();
            chunk->createMeshes({});

            m_outputQueue.push(std::move(chunk));
        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
}