#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include "ThreadSafeQueue.h"
#include "WorldTypes.h"
#include "WorldGenTypes.h"
#include "WorldGenRegistry.h"
#include "FastNoiseLite.h"

struct GeneratedRegion {
    RegionCoord coord;
    std::unique_ptr<RegionShape> shape;
};

class RegionGeneratorWorker {
public:
    RegionGeneratorWorker();
    ~RegionGeneratorWorker();

    void start(Uint64 worldSeedin, FastNoiseLite* standartNoise, WorldGenRegistry* worldGen);
    void stop();

    void requestRegion(RegionCoord coord);
    std::optional<GeneratedRegion> tryGetRegionShape();
    bool cancelRegion(RegionCoord coord);

private:
    void workerLoop();

    int totalRegionsGenerated;
    Uint64 times[100] = { 0 };
    float s;

    std::atomic<bool>  m_running;
    std::thread        m_thread;

    FastNoiseLite* m_standartNoise = nullptr;
    WorldGenRegistry* worldGenRegistry = nullptr;
    Uint64 worldSeed = 0;

    ThreadSafeQueue<RegionCoord>  m_inputQueue;
    ThreadSafeQueue<GeneratedRegion> m_outputQueue;
};
