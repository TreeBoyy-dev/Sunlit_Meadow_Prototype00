#include "ChunkGeneratorWorker.h"
#include "GenerateColumn.h"
#include "Globals.h"

ChunkGeneratorWorker::ChunkGeneratorWorker() : m_running(false), totalChunkGenerated(0){}
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
                if(!doRemeshingSeperately)
                    chunk->optimizeMeshes();
                totalChunkGenerated++;

                m_outputQueue.push(std::move(chunk));

                //calc avr time per chunk
                Uint64 now = SDL_GetTicks();
                float  dt = (float)(now - lastTicks) / 1000.0f;
                lastTicks = now;

                s += dt - times[frame];
                times[frame] = dt;

                float avrg = s / 100.0f;
                float cps = 1 / avrg;

                frame++; frame = frame % 100;

                SDL_Log("[GeneratorWorker] chunk %6d generated: %3d|%3d|%3d (avr time/chunk: %4.2fms)",
                    totalChunkGenerated,
                    chunkData.coordinates.x, chunkData.coordinates.y, chunkData.coordinates.z,
                    cps);
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