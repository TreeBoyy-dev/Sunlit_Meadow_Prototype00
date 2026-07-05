#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include "Block.h"

// =====================================================================
//  computePlacementState
//
//  Turns "which face did the player click + which way are they looking"
//  into the initial blockstate for a freshly placed block.
//
//  It only sets the properties the block actually declares — it walks the
//  block's own StateLayout, so a plain cube gets state 0 (unchanged
//  behavior), a log gets an axis, a stair gets a facing and a half, etc.
//  The fluid bit (bit 15) is never touched here.
//
//  Value conventions match BlockModel's baker exactly:
//    rot4  facing : 0=N 1=E 2=S 3=W          (from camera yaw — the block
//                                             turns to face the player)
//    rot6  facing : 0-3 as rot4, 4=up 5=down (up/down from the clicked
//                                             face, otherwise from yaw)
//    axis3 axis   : 0=Z 1=X 2=Y              (from the clicked face's axis)
//    half         : 0=bottom 1=top           (from the clicked face; see
//                                             hitZFrac below)
//
//  `face` is a BlockFace value from getBlockLookingAt (FACE_FRONT etc.).
//
//  `hitZFrac` is optional: where on the clicked face the ray landed,
//   0..1 along the Z axis. getBlockLookingAt doesn't provide this yet, so
//  the default -1 makes `half` fall back to the face (clicked the
//  underside of a block -> top half, anything else -> bottom half). Once
//  the raycast reports the hit point, pass its fractional Z here and
//  aiming at the upper part of a side face will place top halves.
// =====================================================================
Uint16 computePlacementState(
    const Block* block,
    int face,
    float cameraYawDegrees,
    float hitZFrac = -1.0f
);