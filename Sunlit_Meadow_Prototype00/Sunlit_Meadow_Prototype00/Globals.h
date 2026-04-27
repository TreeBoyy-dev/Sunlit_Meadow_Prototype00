#pragma once

#include "DataStructures.h"
#include "Vectors.h"
#include "WorldManager.h"
#include "BlockManager.h"
#include "ChunkTypes.h"

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
extern WorldManager testManager;
extern const int RENDER_DISTANCE;

extern ChunkCoord prevPlayerChunkCoords;

// --- BlockManager ---
extern BlockManager blockManager;