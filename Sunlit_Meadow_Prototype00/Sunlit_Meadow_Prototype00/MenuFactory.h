#pragma once
#include "MenuManager.h"
#include <string>
#include <functional>

class Entity;
class Inventory;

namespace MenuFactory {

    // layout constants shared by all inventory-style menus
    constexpr float kSlotSize = 18.0f;
    constexpr float kEdgePad  = 10.0f;
    constexpr float uiScale   = 3.0f;

    // 'E' toggle: opens/closes the menu with id "player_inventory".
    void togglePlayerInventory(MenuManager& mgr);

    // Builds the player inventory window: 3 rows (slots 10..39) + hotbar row 0..9.
    std::unique_ptr<Menu> makePlayerInventoryMenu();

    // Opens the player inventory AND a container menu side by side.
    // 'container' must be a live Inventory (e.g. the chest entity's component).
    void openContainer(MenuManager& mgr, Entity* containerOwner,
                       Inventory* container, const std::string& title,
                       int rows = 3, int cols = 10);

    // Workbench: 'grid' and 'output' are live inventories owned by the caller
    // (usually components on the workbench entity). 'recipe' inspects the grid
    // and returns the result stack (empty = no match); it runs automatically on
    // every grid change. Taking from the output consumes one of each ingredient.
    void openWorkbench(MenuManager& mgr, Entity* owner,
                       Inventory* grid, Inventory* output,
                       std::function<ItemInstance(Inventory&)> recipe,
                       int gridRows = 3, int gridCols = 3);

} // namespace MenuFactory
