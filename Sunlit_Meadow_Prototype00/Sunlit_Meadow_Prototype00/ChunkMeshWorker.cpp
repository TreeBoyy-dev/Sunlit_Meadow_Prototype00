#include "ChunkMeshWorker.h"

ChunkMeshWorker::ChunkMeshWorker() : m_running(false) {}
ChunkMeshWorker::~ChunkMeshWorker() { stop(); }

void ChunkMeshWorker::start() {
    m_running = true;
    m_thread = std::thread(&ChunkMeshWorker::workerLoop, this);
}

void ChunkMeshWorker::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void ChunkMeshWorker::requestChunk(Chunk* chunk, ChunkBorderAir borderAir) {
    m_inputQueue.push(chunk, borderAir);
}

// Returns a fully generated (but not yet mesh-init'd) Chunk, or nullopt
std::optional<ChunkCoord> ChunkMeshWorker::tryGetChunk() {
    return m_outputQueue.try_pop();
}

void ChunkMeshWorker::workerLoop() {
    while (m_running) {
        if (auto pair = m_inputQueue.try_pop()) {
            auto [chunk, borderAir] = std::move(*pair);

            chunk->createMeshes(borderAir);

            m_outputQueue.push(chunk->getChunkCoordinates());
        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
}

bool ChunkMeshWorker::cancelRequest(ChunkCoord coord) {
    return m_inputQueue.remove_if([&](Chunk* chunk, ChunkBorderAir&) 
    {
        return chunk->getChunkCoordinates() == coord;
    });
}