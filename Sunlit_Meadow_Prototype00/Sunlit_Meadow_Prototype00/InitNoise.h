#pragma once

#include "FastNoiseLite.h"

void initNoise_standard(FastNoiseLite* noise, int worldSeed)
{
    // -- General --
    // noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    // noise->SetRotationType3D(FastNoiseLite::RotationType3D_None);
    noise->SetSeed(worldSeed);
    // noise->SetFrequency(0.010f);

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
    noise->SetFrequency(0.001f);

    noise->SetFractalType(FastNoiseLite::FractalType_FBm);
    noise->SetFractalOctaves(4);
    noise->SetDomainWarpAmp(-3.500f);
}