#pragma once

#include "FastNoiseLite.h"

void initNoise(FastNoiseLite* noise)
{
    // -- General --
    noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise->SetRotationType3D(FastNoiseLite::RotationType3D_None);
    noise->SetSeed(1337);
    noise->SetFrequency(0.010f);

    // -- Fractal --
    noise->SetFractalType(FastNoiseLite::FractalType_None);
    noise->SetFractalOctaves(3);
    noise->SetFractalLacunarity(2.000f);
    noise->SetFractalGain(0.500f);
    noise->SetFractalWeightedStrength(0.000f);
    noise->SetFractalPingPongStrength(2.000f);

    // -- Cellular --
    noise->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_EuclideanSq);
    noise->SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    noise->SetCellularJitter(1.000f);

    // -- Domain Warp --
    // Note: There is no DomainWarpType_None — omit SetDomainWarpType() to leave warping inactive.
    // Note: Domain Warp Seed and Frequency share SetSeed/SetFrequency above (1337, 0.010).
    noise->SetDomainWarpAmp(1.000f);

    // -- Domain Warp Fractal --
    // Uses the same fractal setters above; "None" here means FractalType_None (already set).
    // Octaves (3), Lacunarity (2.000), and Gain (0.500) are also already set above.
}