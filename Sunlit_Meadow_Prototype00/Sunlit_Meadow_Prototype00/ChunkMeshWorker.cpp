#include "ChunkMeshWorker.h"

ChunkMeshWorker::ChunkMeshWorker() : m_running(false) {}
ChunkMeshWorker::~ChunkMeshWorker() { stop(); }

void ChunkMeshWorker::start(BlockManager* blockManager) {
    m_blockManager = blockManager;
    m_running = true;
    m_thread = std::thread(&ChunkMeshWorker::workerLoop, this);
}

void ChunkMeshWorker::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void ChunkMeshWorker::requestChunk(Chunk chunk) {
    m_inputQueue.push(chunk);
}

// Returns a fully generated (but not yet mesh-init'd) Chunk, or nullopt
std::optional<Chunk> ChunkMeshWorker::tryGetChunk() {
    return m_outputQueue.try_pop();
}

void ChunkMeshWorker::workerLoop() {
    while (m_running) {
        if (auto chunk = m_inputQueue.try_pop()) {

            chunk->createMeshes(*m_blockManager);
            chunk->optimizeMeshes();

            m_outputQueue.push(*chunk);
        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
}

bool ChunkMeshWorker::cancelRequest(ChunkCoord coord) {
    return m_inputQueue.remove_if([&](Chunk chunk) 
    {
        return chunk.getChunkCoordinates() == coord;
    });
}