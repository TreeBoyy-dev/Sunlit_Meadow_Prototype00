#include "PlacementState.h"

#include <cmath>

#include "StateLayout.h"
#include "WorldManager.h"   // BlockFace enum (FACE_FRONT = +X, ...)

// ---------------------------------------------------------------------
//  Camera yaw -> horizontal facing.
//
//  World axes (see App_Update.h): yaw 0 looks down +X, yaw 90 looks down
//  +Y. Face enum: FACE_FRONT = +X, FACE_RIGHT = +Y.
//  rot4 values: 0 = north(+Y authoring dir... irrelevant here — what
//  matters is only that these numbers match facingToZSteps in the baker:
//  0=N 1=E 2=S 3=W, one clockwise step apart).
//
//  We quantize the yaw into one of 4 cardinal look directions, then flip
//  it so the block FACES the player (a placed stair's step points at you,
//  like Minecraft).
// ---------------------------------------------------------------------
static Uint16 yawToLookDirection(float yawDegrees) {
    // normalize to [0, 360)
    float yaw = std::fmod(yawDegrees, 360.0f);
    if (yaw < 0.0f) yaw += 360.0f;

    // +45 so each cardinal owns a 90-degree arc centered on it:
    //   [315,45) -> 0 (+X)   [45,135) -> 1 (+Y)
    //   [135,225)-> 2 (-X)   [225,315)-> 3 (-Y)
    return (Uint16)((int)((yaw + 45.0f) / 90.0f) & 3);
}

static Uint16 facingTowardPlayer(float yawDegrees) {
    // opposite cardinal of the look direction
    return (Uint16)((yawToLookDirection(yawDegrees) + 2) & 3);
}

// ---------------------------------------------------------------------
Uint16 computePlacementState(
    const Block* block,
    int face,
    float cameraYawDegrees,
    float hitZFrac
) {
    if (!block) return 0;

    const StateLayout& layout = block->getStateLayout();
    Uint16 state = 0;

    for (int i = 0; i < layout.propertyCount(); ++i) {
        const StateProperty& p = layout.property(i);
        switch (p.type) {

        case StatePropType::Axis3: {
            // The clicked face determines the axis the pillar runs along:
            // click a top -> upright log, click a side -> sideways log.
            Uint16 axis;
            switch (face) {
            case FACE_FRONT:
            case FACE_BACK:  axis = 1; break;   // X
            case FACE_RIGHT:
            case FACE_LEFT:  axis = 2; break;   // Y
            case FACE_UP:
            case FACE_DOWN:
            default:         axis = 0; break;   // Z (also the safe default)
            }
            state = layout.set(state, i, axis);
            break;
        }

        case StatePropType::Rot4: {
            // Horizontal facing comes from where the player looks, not from
            // the clicked face — so stairs placed on the ground still face
            // the right way.
            state = layout.set(state, i, facingTowardPlayer(cameraYawDegrees));
            break;
        }

        case StatePropType::Rot6: {
            // Piston-like: attaches to the clicked surface and points away
            // from it. Clicking the TOP of a block -> the new block faces
            // up; clicking the underside -> faces down; clicking a side ->
            // horizontal facing from yaw (same as rot4).
            Uint16 facing;
            switch (face) {
            case FACE_UP:   facing = 4; break;  // up
            case FACE_DOWN: facing = 5; break;  // down
            default:        facing = facingTowardPlayer(cameraYawDegrees); break;
            }
            state = layout.set(state, i, facing);
            break;
        }

        case StatePropType::Half: {
            // Prefer the exact hit position on the face when available;
            // otherwise fall back to the face itself.
            Uint16 half;
            if (hitZFrac >= 0.0f)       half = (hitZFrac > 0.5f) ? 1 : 0;
            else if (face == FACE_DOWN) half = 1;  // placed on an underside -> top half
            else                        half = 0;  // bottom half
            state = layout.set(state, i, half);
            break;
        }

                                // These are not decided at placement time:
                                //  - Connect4 / WallSide4 get written by neighbor-update logic
                                //    (Chunk::onNeighborChanged, still a stub — plan section 6)
                                //  - Shape5 (stair corners) is derived from neighboring stairs later
                                //  - Flag is block-specific (wall center post etc.)
        case StatePropType::Connect4:
        case StatePropType::WallSide4:
        case StatePropType::Shape5:
        case StatePropType::Flag:
        default:
            break;
        }
    }

    return state;
}