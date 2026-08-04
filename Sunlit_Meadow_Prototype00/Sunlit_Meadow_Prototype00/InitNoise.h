#pragma once

#include "FastNoiseLite.h"

static const float BASE_FREQUENCY = 0.0005f;
static const int   BASE_OCTAVES = 3;

void initNoise_standard(FastNoiseLite* noise, int worldSeed)
{
    // -- General --
    // noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    // noise->SetRotationType3D(FastNoiseLite::RotationType3D_None);
    noise->SetSeed(worldSeed);
    // noise->SetFrequency(BASE_FREQUENCY);

    // -- Fractal --
    // noise->SetFractalType(FastNoiseLite::FractalType_None);
    // noise->SetFractalOctaves(3);
    // noise->SetFractalLacunarity(2.000f);
    // noise->SetFractalGain(0.500f);
    // noise->SetFractalWeightedStrength(0.000f);                         
    // noise->SetFractalPingPongStrength(2.000f);                         

    // -- Cellular --
    // noise->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_EuclideanSq); 
    // noise->SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);               
    // noise->SetCellularJitter(1.000f);                                  

    // -- Domain Warp --
    // Type "None" in the GUI has no library equivalent: warping only happens
    // if you actually call noise->DomainWarp(x, y). Leaving it uncalled = None.
    // noise->SetDomainWarpAmp(1.000f);                                   
    // Domain Warp Seed / Frequency reuse SetSeed / SetFrequency above.

    // -- Domain Warp Fractal --
    // Shares the fractal setters above; "None" == FractalType_None (default).
}

void initNoise_layer_0(FastNoiseLite* noise, int worldSeed)
{
    noise->SetSeed(worldSeed);
    noise->SetFrequency(BASE_FREQUENCY);

    noise->SetFractalType(FastNoiseLite::FractalType_FBm);
    noise->SetFractalOctaves(BASE_OCTAVES);
    noise->SetDomainWarpAmp(-3.500f);

    noise->SetFractalLacunarity(2.5f);
    noise->SetFractalGain(0.4f);
    noise->SetFractalWeightedStrength(0.1f);

}