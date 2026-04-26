#pragma once

#include "WorldManager.h"

bool updateCamera(float dt) {
    camera.yaw   -= mouseMovement.u * -MAX_CAMERA_SPEED_LOOK;
    camera.pitch += mouseMovement.v * -MAX_CAMERA_SPEED_LOOK;
    mouseMovement.u = 0;
    mouseMovement.v = 0;

    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    else if (camera.pitch < -89.0f) camera.pitch = -89.0f;
    if (camera.yaw >= 360.0f) camera.yaw -= 360.0f;
    else if (camera.yaw < 0.0f) camera.yaw += 360.0f;

    float yawRad = camera.yaw * (3.14159265f / 180.0f);
    float pitchRad = camera.pitch * (3.14159265f / 180.0f);

    camera.forward = {
        cosf(pitchRad) * cosf(yawRad),  // x
        cosf(pitchRad) * sinf(yawRad),  // y
        sinf(pitchRad)                  // z is up
    };

    camera.lookTarget = {
        camera.position.x + camera.forward.x,
        camera.position.y + camera.forward.y,
        camera.position.z + camera.forward.z
    };

    camera.forward = vec3Normalize(camera.forward);

    int numkeys;
    const bool* keyStates = SDL_GetKeyboardState(&numkeys);

    // Forward direction on ground plane only
    Vec3 flatForward = {
        camera.forward.x,
        camera.forward.y,
        0.0f,
    };
    flatForward = vec3Normalize(flatForward);

    // Right vector on ground plane
    Vec3 flatRight = {
        flatForward.y,
        -flatForward.x,
        0.0f
    };
    flatRight = vec3Normalize(flatRight);

    Vec3 movement = { 0, 0, 0 };

    if (keyStates[SDL_SCANCODE_W])     
    movement = {
        movement.x + flatForward.x,
        movement.y + flatForward.y,
        movement.z 
    };
    if (keyStates[SDL_SCANCODE_S])     
    movement = {
        movement.x - flatForward.x,
        movement.y - flatForward.y,
        movement.z
    };
    if (keyStates[SDL_SCANCODE_A])
    movement = {
        movement.x + flatRight.x,
        movement.y + flatRight.y,
        movement.z
    };
    if (keyStates[SDL_SCANCODE_D])
    movement = {
        movement.x - flatRight.x,
        movement.y - flatRight.y,
        movement.z
    };

    // Vertical movement stays world-relative
    if (keyStates[SDL_SCANCODE_SPACE])  movement.z +=  1.0f;
    if (keyStates[SDL_SCANCODE_LSHIFT]) movement.z += -1.0f;

    if (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f) {
        movement = vec3Normalize(movement);
    }

    float movementSpeed = MAX_CAMERA_SPEED_MOVE;
    if (keyStates[SDL_SCANCODE_LCTRL])  movementSpeed *= 1.5;

    camera.position = {
        camera.position.x += movement.x * movementSpeed * dt,
        camera.position.y += movement.y * movementSpeed * dt,
        camera.position.z += movement.z * movementSpeed * dt,
    };

    return true;
}

SDL_AppResult App_Update(void* appstate)
{
    AppState* state = (AppState*)appstate;

    Uint64 now = SDL_GetTicks();
    float  dt = (float)(now - state->lastTicks) / 1000.0f;
    state->lastTicks = now;

    sum += dt - lst_dt[frame_count];
    lst_dt[frame_count] = dt;

    float avrg = sum / (float)ARR_SIZE;
    float fps = 1 / avrg;

    frame_count++;
    frame_count = frame_count % ARR_SIZE;
    if (frame_count % 480 == 0) {
        SDL_Log("FPS: %.1f", fps);
    }

    float speed = 90.0f * (float)SDL_PI_F / 180.0f;   // deg/sec → rad/sec
    state->rotation += speed * dt;

    if (!updateCamera(dt))
        return SDL_APP_FAILURE;


    ChunkCoord playerChunkCoords = getPlayerChunkCoord(camera.position);

    if (playerChunkCoords != prevPlayerChunkCoords) {
        testManager.updateRenderList(camera.position, RENDER_DISTANCE, state, state->textureArray);
    }
    prevPlayerChunkCoords = playerChunkCoords;

    return SDL_APP_CONTINUE;
}

