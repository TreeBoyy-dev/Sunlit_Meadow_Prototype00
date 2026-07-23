#include "ChunkGeneratorWorker.h"
#include "GenerateColumn.h"
#include "Globals.h"

ChunkGeneratorWorker::ChunkGeneratorWorker() : m_running(false), totalChunksGenerated(0){}
ChunkGeneratorWorker::~ChunkGeneratorWorker() { stop(); }

void ChunkGeneratorWorker::start(BlockManager* blockManager, FastNoiseLite* standartNoise, int regionChunkZStart,
    const LayerDef* layer, const PalettedGrid2D* zoneMap, const PalettedGrid2D* biomeMap) {
    m_blockManager = blockManager;
    m_standartNoise = standartNoise;
    m_regionChunkZStart = regionChunkZStart;
    m_layer = layer;
    m_zoneMap = zoneMap;
    m_biomeMap = biomeMap;
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
                generateColumn(columnCoord, m_regionChunkZStart,
                    *m_layer, *m_zoneMap, *m_biomeMap,   // lock-free: immutable, region outlives worker
                    *m_blockManager, *m_standartNoise);

            // Turn each returned PalettedContainer into a Chunk and queue it.
            for (auto& chunkData : column) {
                Uint64 lastTicks = SDL_GetTicks();

                auto chunk = std::make_unique<Chunk>(
                    chunkData.coordinates,
                    std::move(chunkData.storage)
                );
                chunk->createMeshes(*m_blockManager);
                if(!doRemeshingSeperately)
                    chunk->optimizeMeshes();
                totalChunksGenerated++;

                m_outputQueue.push(std::move(chunk));

                //calc avr time per chunk
                Uint64 now = SDL_GetTicks();
                float  dt = (float)(now - lastTicks) / 1000.0f;

                s += dt - times[totalChunksGenerated % 100];
                times[totalChunksGenerated % 100] = dt;

                float mspc = s / 100.0f;

                SDL_Log("[ChunkGeneratorWorker] chunk %6d generated: %3d|%3d|%3d (avr time/chunk: %2.4fms)",
                    totalChunksGenerated,
                    chunkData.coordinates.x, chunkData.coordinates.y, chunkData.coordinates.z,
                    mspc);
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
