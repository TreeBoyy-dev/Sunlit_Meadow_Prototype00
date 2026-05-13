#pragma once

#include "DataStructures.h"
#include "Vectors.h"
#include "WorldManager.h"
#include "BlockManager.h"
#include "ChunkTypes.h"
#include "FastNoiseLite.h"

extern const float NEAR_PLANE;
extern const float FAR_PLANE;
extern float fovDeg;
extern float fovX;
extern float aspect;

extern bool renderDebugUI;
extern float fps;

// --- Camera ---
extern Camera camera;
extern Vec2 mouseMovement;
extern const float MAX_CAMERA_SPEED_MOVE;
extern const float MAX_CAMERA_SPEED_LOOK;

// --- Calc FPS ---
extern const int ARR_SIZE;
extern float lst_dt[];
extern int frame_count;
extern float sum;

// --- WorldManager ---
extern WorldManager worldManager;
extern const int RENDER_DISTANCE;

extern ChunkCoord prevPlayerChunkCoords;

// --- BlockManager ---
extern BlockManager blockManager;

// --- World generation related ---
extern FastNoiseLite standartNoise;