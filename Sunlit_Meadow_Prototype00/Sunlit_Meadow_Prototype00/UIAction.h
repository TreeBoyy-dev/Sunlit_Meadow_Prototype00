#pragma once
#include <functional>

class Entity;
class Inventory;
class Menu;
class MenuManager;

// Passed to every button/slider action. Whoever invokes the action fills this
// in — player entity 0 by default, or the menu's owner entity — so the same
// action (e.g. a craft) can be performed by the player, an NPC or a workstation.
struct ActionContext {
    Entity*      actor    = nullptr; // who triggered it
    Inventory*   actorInv = nullptr; // convenience: actor's INVENTORY component
    Menu*        menu     = nullptr; // the menu the widget lives in
    MenuManager* manager  = nullptr; // open/close menus, read the cursor stack
    float        value    = 0.0f;    // slider value / generic payload
};

using UIAction = std::function<void(ActionContext&)>;
