#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>

#include "Mat4.h"
#include "Materials.h"
#include "DataStructures.h"
#include "App_Render.h"
#include "App_Update.h"
#include "App_Init.h"
#include "Globals.h"
#include "Vectors.h"
#include "MenuManager.h"
#include "MenuFactory.h"

// NEW: map SDL button index to the menu system's enum
static MouseButton toMouseButton(Uint8 b) {
    switch (b) {
    case 1:  return MouseButton::Left;
    case 3:  return MouseButton::Right;
    default: return MouseButton::Middle;
    }
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    AppState* state = new AppState();
    if (!state) { return SDL_APP_FAILURE; }
    *appstate = state;

    if (App_Init(state) != SDL_APP_CONTINUE) {
        SDL_Log("returned Error on App_Init");
        return SDL_APP_FAILURE;
    }

    int w, h;
    SDL_GetWindowSize(state->window, &w, &h);

    fovX = fovDeg * (float)SDL_PI_F / 180.0f;
    aspect = (float)w / (float)h;
    state->projMat = mat4Perspective(fovX, aspect, NEAR_PLANE, FAR_PLANE);

    state->lastTicks = SDL_GetTicks();

    SDL_SetWindowRelativeMouseMode(state->window, true);
    menuManager.setWindow(state->window);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    AppState* state = (AppState*)appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        switch (event->key.scancode)
        {
        case SDL_SCANCODE_ESCAPE:
            if (menuManager.anyOpen()) {
                menuManager.closeTop();
                break;
            }
            return SDL_APP_SUCCESS;

        case SDL_SCANCODE_E:
            MenuFactory::togglePlayerInventory(menuManager);
            break;

        case SDL_SCANCODE_F3:
            if (renderDebugUI) {
                renderDebugUI = false;
                playerEntity->setPosition(camera.position);
            }
            else
                renderDebugUI = true;
            break;

        default:
            break;
        }
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        if (menuManager.anyOpen()) {
            menuManager.handleMouseMotion(event->motion.x, event->motion.y);
        }
        else {
            mouseMovement.x += event->motion.xrel;
            mouseMovement.y += event->motion.yrel;
        }
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (menuManager.anyOpen())
            menuManager.handleMouseUp(toMouseButton(event->button.button),
                event->button.x, event->button.y);
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        SDL_Log("mouse button down");
        if (menuManager.anyOpen() &&
            menuManager.handleMouseDown(toMouseButton(event->button.button),
                event->button.x, event->button.y)) {
            SDL_Log("continue after mouse down casue menu");
            return SDL_APP_CONTINUE;
        }

        switch (event->button.button)
        {
        case 1:
        {
            //left click
            Vec3 pos = worldManager.getBlockLookingAt(camera, 20.0f);
            if (!std::isnan(pos.x)) {
                Block* b = blockManager.getById(worldManager.getBlockIdAt(pos));
                if (!b) {
                    SDL_Log("left_click at %.0f|%.0f|%.0f (block not found)",
                        pos.x, pos.x, pos.z);
                    break;
                }

                SDL_Log("left_click at %.0f|%.0f|%.0f",
                    pos.x, pos.x, pos.z);

                worldManager.setBlockIdAt(state, pos, 0);

                Entity* player = entityManager.getEntityById(0);
                Inventory* inventory;
                Data* found = player->getData(INVENTORY);
                if (found == nullptr) {
                    SDL_Log("no Inventory found");
                    break;
                }
                else
                    inventory = static_cast<Inventory*>(found);

                Item* item = itemManager.getItemByName(b->getName());
                ItemInstance items{ item, 1 };
                SDL_Log("new Items: %s, %dx", item->getName().c_str(), items.count);
                if (!inventory->addItemToInventory(items))
                    SDL_Log("[main] Items couldn't be added");
                inventory->printContents();
            }

            break;
        }

        case 3:
        {
            //right click
            int face;
            Vec3 pos = worldManager.getBlockLookingAt(camera, 20.0f, &face);
            if (!std::isnan(pos.x)) {
                switch (face) {
                case FACE_BACK:
                    pos.x--; break;

                case FACE_FRONT:
                    pos.x++; break;

                case FACE_RIGHT:
                    pos.y--; break;

                case FACE_LEFT:
                    pos.y++; break;

                case FACE_UP:
                    pos.z++; break;

                case FACE_DOWN:
                    pos.z--; break;

                default:
                    SDL_Log("[Event] right click: no face"); break;
                }

                Entity* player = entityManager.getEntityById(0);
                if (player == nullptr) {            // getEntityById can return null
                    SDL_Log("[Event] right click: no player entity (id 0)");
                    return SDL_APP_CONTINUE;
                }

                Inventory* inventory;
                Data* found = player->getData(INVENTORY);
                if (found == nullptr) {
                    //SDL_Log("no Inventory found");
                    return SDL_APP_CONTINUE;
                }
                else
                    inventory = static_cast<Inventory*>(found);

                ItemInstance instance = inventory->getItemsFromSlot(selectedSlot);
                if(!renderDebugUI) inventory->takeItems(selectedSlot, 1);
                if (instance.isEmpty())
                    return SDL_APP_CONTINUE;

                Item_Placable* placebleItem;
                if (instance.item->getCategory() == ITEM_CATEGORY_BLOCK) {
                    placebleItem = static_cast<Item_Placable*>(instance.item);
                    SDL_Log("[Event] right click: Item: %s(%d)",
                        placebleItem->getName().c_str(), placebleItem->getID());
                }
                else {
                    SDL_Log("[Event] right click: Item not Placable");
                    return SDL_APP_CONTINUE;
                }

                worldManager.setBlockIdAt(state, pos, placebleItem->getBlockId());

            }
            break;
        }

        default:
            break;
        }
    }
    if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        if (event->wheel.y > 0)
            selectedSlot++;
        else
            selectedSlot--;

        if (selectedSlot > 9)
            selectedSlot = 9;
        if (selectedSlot < 0)
            selectedSlot = 0;

        SDL_Log("[Event] mouse: selectedSlot = %d", selectedSlot);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    AppState* state = (AppState*)appstate;

    SDL_AppResult r = App_Update(state);
    if (r != SDL_APP_CONTINUE) return r;

    r = App_Render(state);
    if (r != SDL_APP_CONTINUE) return r;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    AppState* state = (AppState*)appstate;
    if (state) {
        worldManager.destroy(state);

        if (state->gpu)
            SDL_DestroyGPUDevice(state->gpu);
        if (state->window)
            SDL_DestroyWindow(state->window);
        SDL_free(state);
    }
    SDL_Quit();
}