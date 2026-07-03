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

// --- Player ---
Entity* playerEntity = nullptr;
const float EYE_HEIGHT = 1.62f;   // feet -> eyes (player is 1.8 tall)
const float PLAYER_WALK_SPEED = 4.3f;    // blocks per second
const float SPRINT_MULTIPLIER = 1.5f;    // LCTRL
const float JUMP_SPEED = 4.95f;   // sqrt(2 * 9.81 * 1.25) -> ~1.25 block jump

// --- Calc FPS ---
float lst_dt[ARR_SIZE] = { 0 };
int frame_count = 0;
float sum = 0.0;

// --- Managers ---
EntityManager entityManager;

BlockManager blockManager;

WorldManager worldManager;
const int RENDER_DISTANCE = 2;
bool doRemeshingSeperately = false;

ChunkCoord prevPlayerChunkCoords = { 0,0,0 };

UI_Renderer ui;
MenuManager menuManager;

ItemManager itemManager;