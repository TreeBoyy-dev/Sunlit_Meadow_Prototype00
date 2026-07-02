#pragma once

#include "DataStructures.h"
#include "Vectors.h"
#include "WorldManager.h"
#include "BlockManager.h"
#include "WorldTypes.h"
#include "FastNoiseLite.h"
#include "Skybox.h"
#include "EntityManager.h"
#include "ItemManager.h"
#include "MenuManager.h"

extern const float NEAR_PLANE;
extern const float FAR_PLANE;
extern float fovDeg;
extern float fovX;
extern float aspect;

extern bool renderDebugUI;
extern float fps;

extern int selectedSlot; //HotbarSlot

extern Skybox skybox;

// --- Camera ---
extern Camera camera;
extern Vec2 mouseMovement;
extern const float MAX_CAMERA_SPEED_MOVE;
extern const float MAX_CAMERA_SPEED_LOOK;

// --- Player ---
extern Entity* playerEntity;
extern const float EYE_HEIGHT;   // feet -> eyes (player is 1.8 tall)
extern const float PLAYER_WALK_SPEED;    // blocks per second
extern const float SPRINT_MULTIPLIER;    // LCTRL
extern const float JUMP_SPEED;   // sqrt(2 * 9.81 * 1.25) -> ~1.25 block jump

// --- Calc FPS ---
const int ARR_SIZE = 120;
extern float lst_dt[ARR_SIZE];
extern int frame_count;
extern float sum;

// --- Calc FPS ---
extern const int ARR_SIZE;
extern float lst_dt[];
extern int frame_count;
extern float sum;

// --- Managers ---
extern EntityManager entityManager;

extern BlockManager blockManager;

extern WorldManager worldManager;
extern const int RENDER_DISTANCE;
extern bool doRemeshingSeperately;

extern ChunkCoord prevPlayerChunkCoords;

extern UI_Renderer ui;
extern MenuManager menuManager;

extern ItemManager itemManager;