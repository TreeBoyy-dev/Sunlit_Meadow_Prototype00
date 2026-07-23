#include "RegionGeneratorWorker.h"
#include "Globals.h"
#include "GenerateRegion.h"

RegionGeneratorWorker::RegionGeneratorWorker() : m_running(false), totalRegionsGenerated(0) {}
RegionGeneratorWorker::~RegionGeneratorWorker() { stop(); }

void RegionGeneratorWorker::start(Uint64 worldSeedin, FastNoiseLite* standartNoise, WorldGenRegistry* worldGen) {
    worldSeed = worldSeedin;
    worldGenRegistry = worldGen;
    m_running = true;
    m_thread = std::thread(&RegionGeneratorWorker::workerLoop, this);
}

void RegionGeneratorWorker::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void RegionGeneratorWorker::requestRegion(RegionCoord  coord) {
    m_inputQueue.push(coord);
}

// Returns a fully generated (but not yet mesh-init'd) Chunk, or nullopt
std::optional<GeneratedRegion> RegionGeneratorWorker::tryGetRegionShape() {
    return m_outputQueue.try_pop();
}

void RegionGeneratorWorker::workerLoop() {
    while (m_running) {
        if (auto coord = m_inputQueue.try_pop()) {
            Uint64 lastTicks = SDL_GetTicks();
                
            RegionCoord regionCoord = coord.value();
            auto regionShape = generateRegion(worldSeed, regionCoord, worldGenRegistry);

            m_outputQueue.push(GeneratedRegion{ regionCoord, std::move(regionShape) });

            //calc avr time per region
            Uint64 now = SDL_GetTicks();
            float  dt = (float)(now - lastTicks) / 1000.0f;

            s += dt - times[totalRegionsGenerated % 100];
            times[totalRegionsGenerated % 100] = dt;

            float mspc = s / 100.0f;

            SDL_Log("[RegionGeneratorWorker] region %6d generated: %3d|%3d|%3d (avr time/Region: %2.4fms)",
                totalRegionsGenerated, regionCoord.x, regionCoord.y, regionCoord.z, mspc);
            
        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
}

bool RegionGeneratorWorker::cancelRegion(RegionCoord  coord) {
    return m_inputQueue.remove_if(
        [&](const RegionCoord& c) { return c == coord; }
    );
}
