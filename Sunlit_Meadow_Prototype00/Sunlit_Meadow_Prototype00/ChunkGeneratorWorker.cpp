#include "ChunkGeneratorWorker.h"
#include "GenerateColumn.h"
#include "Globals.h"

ChunkGeneratorWorker::ChunkGeneratorWorker() : m_running(false), totalChunksGenerated(0){}
ChunkGeneratorWorker::~ChunkGeneratorWorker() { stop(); }

void ChunkGeneratorWorker::start(BlockManager* blockManager, FastNoiseLite* standartNoise,
    const WorldGenNoise* worldGenNoise, int regionChunkZStart,
    const LayerDef* layer, const PalettedGrid2D* zoneMap, const PalettedGrid2D* biomeMap) {
    m_blockManager = blockManager;
    m_standartNoise = standartNoise;
    m_worldGenNoise = worldGenNoise;
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
            Uint64 lastTicks = SDL_GetTicks();

            std::vector<GeneratedChunkData> column =
                generateColumn(columnCoord, m_regionChunkZStart,
                    *m_layer, *m_zoneMap, *m_biomeMap,   // lock-free: immutable, region outlives worker
                    *m_blockManager, *m_worldGenNoise);  // lock-free: init-once, WorldManager outlives worker

            // Turn each returned PalettedContainer into a Chunk and queue it.
            for (auto& chunkData : column) {

                auto chunk = std::make_unique<Chunk>(
                    chunkData.coordinates,
                    std::move(chunkData.storage)
                );
                chunk->createMeshes(*m_blockManager);
                if(!doRemeshingSeperately)
                    chunk->optimizeMeshes();
                totalChunksGenerated++;

                m_outputQueue.push(std::move(chunk));
            }

            //calc avr time per chunk
            Uint64 now = SDL_GetTicks();
            float  dt = (float)(now - lastTicks) / 1000.0f;

            s += dt - times[totalChunksGenerated % 100];
            times[totalChunksGenerated % 100] = dt;

            float mspc = s / 100.0f;

            SDL_Log("[ChunkGeneratorWorker] collumn %6d generated: %3d|%3d (avr time/collumn: %2.4fms)",
                totalChunksGenerated,
                columnCoord.x, columnCoord.y, mspc);

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
