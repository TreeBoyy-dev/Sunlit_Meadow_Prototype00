#pragma once

#include "Vectors.h"
#include "ChunkTypes.h"
#include "DataStructures.h"

bool inCameraView(Camera cam, ChunkCoord coord, float FOV) {

    // Vector from camera to chunk (fixed .y and .z)
    Vec3 chunkV;
    chunkV.x = (coord.x * CHUNK_SIZE + CHUNK_SIZE/2) - cam.position.x;
    chunkV.y = (coord.y * CHUNK_SIZE + CHUNK_SIZE / 2) - cam.position.y;
    chunkV.z = (coord.z * CHUNK_SIZE + CHUNK_SIZE / 2) - cam.position.z;

    // check if it is the chunk the player is in
    float abs = (float)sqrt(chunkV.x * chunkV.x + chunkV.y * chunkV.y + chunkV.z * chunkV.z);
    if (abs < CHUNK_SIZE*1.25)
        return true;

    // Normalize chunkV
    Vec3 chunkVnorm = vec3Normalize(chunkV);

    // Dot product with the camera forward vector (assumed normalized)
    float dot = vec3Dot(chunkVnorm, cam.forward);

    // Chunk is visible if the angle is less than FOV/2
    // i.e. dot > cos(FOV/2)  (FOV in radians)
    return dot > cosf(FOV * 0.5f * (3.14159265359 / 180.0f));
}