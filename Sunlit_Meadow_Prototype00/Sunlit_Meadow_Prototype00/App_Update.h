#pragma once

#include "WorldManager.h"
#include "Globals.h"

// --- 1) Mouse-look only ------------------------------------------------------
// Turn accumulated mouse motion into yaw/pitch and rebuild camera.forward.
// The camera's POSITION is no longer set here - it follows the player's head
// in syncCameraToPlayer(), after collision resolution.
void updateCameraLook() {
    camera.yaw -= mouseMovement.x * -MAX_CAMERA_SPEED_LOOK;
    camera.pitch += mouseMovement.y * -MAX_CAMERA_SPEED_LOOK;
    mouseMovement.x = 0;
    mouseMovement.y = 0;

    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    else if (camera.pitch < -89.0f) camera.pitch = -89.0f;
    if (camera.yaw >= 360.0f) camera.yaw -= 360.0f;
    else if (camera.yaw < 0.0f) camera.yaw += 360.0f;

    float yawRad = camera.yaw * (3.14159265f / 180.0f);
    float pitchRad = camera.pitch * (3.14159265f / 180.0f);

    camera.forward = vec3Normalize({
        cosf(pitchRad) * cosf(yawRad),  // x
        cosf(pitchRad) * sinf(yawRad),  // y
        sinf(pitchRad)                  // z is up
        });
}

// --- 2) WASD + jump onto the player's physics body ---------------------------
// Writes velocity ONLY. Entity::update + collideAxis remain the only thing
// that moves the entity, so wall/floor collision works for free. Note that
// Entity::update applies medium drag (water) to the full velocity afterwards,
// including what we set here - that is intended (water slows you down).
void applyPlayerInput(float dt) {
    if (playerEntity == nullptr) return;

    int numkeys;
    const bool* keyStates = SDL_GetKeyboardState(&numkeys);

    // Movement direction from camera yaw, projected onto the ground plane
    Vec3 flatForward = vec3Normalize({
        camera.forward.x,
        camera.forward.y,
        0.0f
        });
    Vec3 flatRight = rightVector(flatForward);

    Vec3 dir = { 0, 0, 0 };
    if (keyStates[SDL_SCANCODE_W]) { dir.x += flatForward.x; dir.y += flatForward.y; }
    if (keyStates[SDL_SCANCODE_S]) { dir.x -= flatForward.x; dir.y -= flatForward.y; }
    if (keyStates[SDL_SCANCODE_A]) { dir.x += flatRight.x;   dir.y += flatRight.y; }
    if (keyStates[SDL_SCANCODE_D]) { dir.x -= flatRight.x;   dir.y -= flatRight.y; }

    if (dir.x != 0.0f || dir.y != 0.0f)
        dir = vec3Normalize(dir);

    if (!renderDebugUI) {
        float speed = PLAYER_WALK_SPEED;
        if (keyStates[SDL_SCANCODE_LCTRL]) speed *= SPRINT_MULTIPLIER;

        PhysicsBody& physics = playerEntity->getPhysics();

        // Horizontal velocity is set directly each frame (zeroed when no key is
        // held). velocity.z is left alone - gravity and jumping own it.
        physics.velocity.x = dir.x * speed;
        physics.velocity.y = dir.y * speed;

        // Jump: only from the ground (onGround is set by collideAxis when the
        // player was blocked while descending last frame). No double jump.
        if (keyStates[SDL_SCANCODE_SPACE] && physics.onGround)
            physics.velocity.z = JUMP_SPEED;
    }
    else {
        float movementSpeed = MAX_CAMERA_SPEED_MOVE;
        if (keyStates[SDL_SCANCODE_LCTRL])  movementSpeed *= 1.5;

        float zMovement = 0;
        if (keyStates[SDL_SCANCODE_SPACE])  zMovement += 1.0f;
        if (keyStates[SDL_SCANCODE_LSHIFT]) zMovement += -1.0f;

        camera.position = {
            camera.position.x += dir.x * movementSpeed * dt,
            camera.position.y += dir.y * movementSpeed * dt,
            camera.position.z += zMovement * movementSpeed * dt,
        };
    }
}

// --- 3) Camera follows the player's head, post-collision ---------------------
void syncCameraToPlayer() {
    if (playerEntity != nullptr && !renderDebugUI) {
        Vec3 p = playerEntity->getPosition();          // entity position = feet
        camera.position = { p.x, p.y, p.z + EYE_HEIGHT };

        // Model yaw follows the camera (mat4Rotate's 3rd arg = rotation about
        // Z, in radians) so a future third-person view faces the right way.
        float yawRad = camera.yaw * (3.14159265f / 180.0f);
        playerEntity->setRotation({ 0.0f, 0.0f, yawRad });
    }

    // lookTarget is always derived from position + forward, even if the
    // player is missing (camera then just stays where it is, look-only).
    camera.lookTarget = vec3Add(camera.position, camera.forward);
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
    fps = 1 / avrg;

    frame_count++;
    frame_count = frame_count % ARR_SIZE;

    // 1) input: mouse-look, then WASD/jump written into the player's physics
    updateCameraLook();
    applyPlayerInput(dt);

    // 2) physics + collision moves the player (and every other entity)
    entityManager.update(dt, &worldManager);

    // 3) camera snaps to the player's head using the post-collision position
    syncCameraToPlayer();

    // 4) chunk streaming around the player (camera.position IS the player's
    //    head now, so this explicitly tracks the entity)
    worldManager.update(state, camera.position);

    return SDL_APP_CONTINUE;
}