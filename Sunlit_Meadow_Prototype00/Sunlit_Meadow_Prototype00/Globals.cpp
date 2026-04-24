#include "DataStructures.h"
#include "Globals.h"
#include "Vectors.h"
#include "Chunk.h"

// --- Camera ---
Camera camera = {
	{0.0f, 0.0f, 5.0f},
	{0.0f, -0.0f, -5.0f}
};
Vec2 mouseMovement = {
	0.0f, 0.0f
};
const float MAX_CAMERA_SPEED_MOVE = 3.0f;
const float MAX_CAMERA_SPEED_LOOK = 0.03f;

// --- Calc FPS ---
const int ARR_SIZE = 120;
float lst_dt[ARR_SIZE] = { 0 };
int frame_count = 0;
float sum = 0.0;

// --- Chunks ---
Chunk testChunk({ 0, 0, 0 });