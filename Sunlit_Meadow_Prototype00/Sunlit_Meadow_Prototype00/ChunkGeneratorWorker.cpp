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

                if (logMeshStats)
                    chunk->logMeshStats();

                m_outputQueue.push(std::move(chunk));
            }

            // Rolling average, per COLUMN, in MILLISECONDS
            Uint64 now = SDL_GetTicks();
            float  dtMs = (float)(now - lastTicks);

            totalColumnsGenerated++;
            const int slot = totalColumnsGenerated % 100;
            s += dtMs - times[slot];
            times[slot] = dtMs;

            const int   window = totalColumnsGenerated < 100 ? totalColumnsGenerated : 100;
            const float msPerColumn = s / (float)window;

            //SDL_Log("[ChunkGeneratorWorker] column %6d generated: %3d|%3d (avg time/column: %.2fms)",
            //    totalColumnsGenerated,
            //    columnCoord.x, columnCoord.y, msPerColumn);

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
