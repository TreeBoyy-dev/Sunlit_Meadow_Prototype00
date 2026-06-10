#include "ChunkGeneratorWorker.h"
#include "GenerateColumn.h"

ChunkGeneratorWorker::ChunkGeneratorWorker() : m_running(false) {}
ChunkGeneratorWorker::~ChunkGeneratorWorker() { stop(); }

void ChunkGeneratorWorker::start(BlockManager* blockManager, FastNoiseLite* standartNoise, int regionChunkZStart) {
    m_blockManager = blockManager;
    m_standartNoise = standartNoise;
    m_regionChunkZStart = regionChunkZStart;
    m_running = true;
    m_thread = std::thread(&ChunkGeneratorWorker::workerLoop, this);
}

void ChunkGeneratorWorker::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void ChunkGeneratorWorker::requestColumn(ColumnCoord  coord) {
    m_inputQueue.push(coord);
}

// Returns a fully generated (but not yet mesh-init'd) Chunk, or nullopt
std::optional<std::unique_ptr<Chunk>> ChunkGeneratorWorker::tryGetChunk() {
    return m_outputQueue.try_pop();
}

void ChunkGeneratorWorker::workerLoop() {
    while (m_running) {
        if (auto coord = m_inputQueue.try_pop()) {
            ColumnCoord columnCoord = coord.value();

            std::vector<GeneratedChunkData> column =
                generateColumn(columnCoord, m_regionChunkZStart, *m_blockManager, *m_standartNoise);

            // Turn each returned PalettedContainer into a Chunk and queue it.
            for (auto& chunkData : column) {
                auto chunk = std::make_unique<Chunk>(
                    chunkData.coordinates,
                    std::move(chunkData.storage)
                );
                chunk->createMeshes(*m_blockManager);

                m_outputQueue.push(std::move(chunk));
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
}

bool ChunkGeneratorWorker::cancelColumn(ColumnCoord  coord) {
    return m_inputQueue.remove_if(
        [&](const ColumnCoord& c) { return c == coord; }
    );
}