#include "DataStructures.h"
#include "Globals.h"
#include "Vectors.h"
#include "WorldManager.h"

// --- Camera ---
Camera camera = {
	{264.0f, 264.0f, 22.0f},
	{0.0f, -0.0f, -5.0f}
};
Vec2 mouseMovement = {
	0.0f, 0.0f
};
const float MAX_CAMERA_SPEED_MOVE = 24.0f;
const float MAX_CAMERA_SPEED_LOOK = 0.03f;

// --- Calc FPS ---
const int ARR_SIZE = 120;
float lst_dt[ARR_SIZE] = { 0 };
int frame_count = 0;
float sum = 0.0;

// --- WorldManager ---
WorldManager testManager;
extern const int RENDER_DISTANCE = 8; // !!hardcoded to 5 in z-axis in WorldManager.cpp in calcVisibleChunksList

extern ChunkCoord prevPlayerChunkCoords = { 0,0,0 };

// --- BlockManager ---
BlockManager blockManager;

// --- World generation related ---
FastNoiseLite standartNoise;