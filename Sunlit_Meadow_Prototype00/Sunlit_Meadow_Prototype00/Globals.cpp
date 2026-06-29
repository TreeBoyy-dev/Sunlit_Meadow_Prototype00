#include "Globals.h"

const float NEAR_PLANE = 0.5f;
const float FAR_PLANE = 3000.0f;
float fovDeg = 70.0f;
float fovX = 0.0f;
float aspect = 1.0f;

int selectedSlot = 0; //HotbarSlot

bool renderDebugUI = true;
float fps = 0;

Skybox skybox;

// --- Camera ---
Camera camera = {
	{264.0f, 264.0f, 70.0f},
	{0.0f,   0.0f,  -5.0f}
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

// --- EntityManager ---
EntityManager entityManager;

// --- BlockManager ---
BlockManager blockManager;

// --- WorldManager ---
WorldManager worldManager;
const int RENDER_DISTANCE = 8;

ChunkCoord prevPlayerChunkCoords = { 0,0,0 };

// --- UI ---
UI_Renderer ui;

// --- Items ---
ItemManager itemManager;