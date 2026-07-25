#include "ChunkMeshWorker.h"
#include "Globals.h"   // logMeshStats

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

void ChunkMeshWorker::requestChunk(std::unique_ptr<Chunk> chunk) {
    m_inputQueue.push(std::move(chunk));
}

// Returns a freshly re-meshed Chunk snapshot, or nullopt
std::optional<std::unique_ptr<Chunk>> ChunkMeshWorker::tryGetChunk() {
    return m_outputQueue.try_pop();
}

void ChunkMeshWorker::workerLoop() {
    while (m_running) {
        if (auto chunk = m_inputQueue.try_pop()) {

            (*chunk)->createMeshes(*m_blockManager);
            (*chunk)->optimizeMeshes();

            if (logMeshStats)
                (*chunk)->logMeshStats();

            m_outputQueue.push(std::move(*chunk));
        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
}

bool ChunkMeshWorker::cancelRequest(ChunkCoord coord) {
    // the old predicate took Chunk BY VALUE — a full deep copy
    // of every queued chunk just to compare coordinates.
    return m_inputQueue.remove_if([&](std::unique_ptr<Chunk>& chunk)
    {
        return chunk->getChunkCoordinates() == coord;
    });
}