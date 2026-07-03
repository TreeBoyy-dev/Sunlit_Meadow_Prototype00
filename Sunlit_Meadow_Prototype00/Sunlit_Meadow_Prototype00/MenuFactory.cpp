#include "MenuFactory.h"
#include "Inventory.h"
#include "Globals.h"

namespace MenuFactory {

// ---------------------------------------------------------------------------
static Inventory* playerInventory() {
    Entity* player = entityManager.getEntityById(0);
    if (!player) return nullptr;
    return static_cast<Inventory*>(player->getData(INVENTORY));
}

// Adds a rows x cols grid of SlotWidgets bound to 'inv', starting at slot
// index 'firstSlot', with its top-left at (x, y) menu-relative. Returns the
// y coordinate just below the grid.
static float addSlotGrid(Menu* menu, Inventory* inv, int firstSlot,
                         int rows, int cols, float x, float y, float edgePad,
                         std::function<void()> onChanged = nullptr) {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            auto s = std::make_unique<SlotWidget>();
            s->inv  = inv;
            s->slot = firstSlot + r * cols + c;
            s->pos = Vec2{ x + c * ((kSlotSize + edgePad)*uiScale),
                            y + r * ((kSlotSize + edgePad) * uiScale) };
            s->size = Vec2{ (kSlotSize + edgePad) * uiScale, (kSlotSize + edgePad) * uiScale };
            s->onChanged = onChanged;
            menu->add(std::move(s));
        }
    }
    return y + rows * ((kSlotSize + edgePad) * uiScale);
}

static float gridWidth(int cols, float edgePad) { return (cols + edgePad)*kSlotSize; }

// ---------------------------------------------------------------------------
std::unique_ptr<Menu> makePlayerInventoryMenu() {
    auto menu = std::make_unique<Menu>();
    menu->id    = "player_inventory";
    menu->title = "Inventory";

    UITexture* tex = ui.FindUITexture("inventory_generic30");
    menu->texture = tex->texture;
    menu->size = { (float)tex->w * uiScale, (float)tex->h * uiScale };

    Inventory* inv = playerInventory();

    const int cols = 10;
    const float contentX = kEdgePad;

    // main storage: slots 10..39 (3 rows of 10)
    addSlotGrid(menu.get(), inv, 10, 3, cols, 7 * uiScale, 14 * uiScale, 0.0f);

    // gap, then the hotbar row: slots 0..9
    addSlotGrid(menu.get(), inv, 0, 1, cols, 7 * uiScale, 73 * uiScale, 0.0f);

    //menu->size = Vec2{ gridWidth(cols) + kEdgePad * 2.0f, y + kEdgePad };
    menu->pos  = Vec2{ (ui.getScreenW() - menu->size.x) * 0.5f,
                       (ui.getScreenH() - menu->size.y) * 0.5f };
    return menu;
}

void togglePlayerInventory(MenuManager& mgr) {
    if (Menu* existing = mgr.findById("player_inventory")) {
        mgr.close(existing);
        return;
    }
    mgr.open(makePlayerInventoryMenu());
}

// ---------------------------------------------------------------------------
void openContainer(MenuManager& mgr, Entity* containerOwner,
                   Inventory* container, const std::string& title,
                   int rows, int cols) {
    if (container == nullptr) {
        SDL_Log("[MenuFactory] openContainer: container inventory is null");
        return;
    }

    // player inventory on the right (open it first so the container ends topmost)
    Menu* playerMenu = mgr.findById("player_inventory");
    if (!playerMenu)
        playerMenu = mgr.open(makePlayerInventoryMenu());

    auto menu = std::make_unique<Menu>();
    menu->id    = "container";
    menu->title = title;
    menu->owner = containerOwner;

    float y = menu->headerH + kEdgePad;
    y = addSlotGrid(menu.get(), container, 0, rows, cols, kEdgePad, y, kEdgePad);
    menu->size = Vec2{ gridWidth(cols, kEdgePad) + kEdgePad * 2.0f, y + kEdgePad };

    // side by side: container left of the player inventory
    const float gap = 16.0f;
    float totalW = menu->size.x + gap + playerMenu->size.x;
    float startX = (ui.getScreenW() - totalW) * 0.5f;
    float midY   = ui.getScreenH() * 0.5f;

    menu->pos       = Vec2{ startX, midY - menu->size.y * 0.5f };
    playerMenu->pos = Vec2{ startX + menu->size.x + gap,
                            midY - playerMenu->size.y * 0.5f };

    mgr.open(std::move(menu));
}

// ---------------------------------------------------------------------------
void openWorkbench(MenuManager& mgr, Entity* owner,
                   Inventory* grid, Inventory* output,
                   std::function<ItemInstance(Inventory&)> recipe,
                   int gridRows, int gridCols) {
    if (!grid || !output) {
        SDL_Log("[MenuFactory] openWorkbench: grid/output inventory is null");
        return;
    }

    Menu* playerMenu = mgr.findById("player_inventory");
    if (!playerMenu)
        playerMenu = mgr.open(makePlayerInventoryMenu());

    auto menu = std::make_unique<Menu>();
    menu->id    = "workbench";
    menu->title = "Workbench";
    menu->owner = owner;

    // recompute the output whenever the grid changes
    auto recompute = [grid, output, recipe]() {
        ItemInstance result = recipe ? recipe(*grid) : ItemInstance{};
        output->setItemsToSlot(result, 0); // v1: always overwritten by the recipe
    };

    // when the output is taken, consume one of each grid ingredient, then recompute
    auto onOutputChanged = [grid, output, gridRows, gridCols, recompute]() {
        if (output->getItemsFromSlot(0).isEmpty()) {
            for (int i = 0; i < gridRows * gridCols; ++i)
                grid->takeItems(i, 1);
        }
        recompute();
    };

    float y = menu->headerH + kEdgePad;
    float gridBottom = addSlotGrid(menu.get(), grid, 0, gridRows, gridCols,
                                   kEdgePad, y, kEdgePad, recompute);

    // output slot to the right of the grid, vertically centered on it
    auto out = std::make_unique<SlotWidget>();
    out->inv  = output;
    out->slot = 0;
    out->outputOnly = true;
    out->onChanged  = onOutputChanged;
    float gw = gridWidth(gridCols, kEdgePad);
    out->pos  = Vec2{ kEdgePad + gw + 32.0f,
                      y + (gridBottom - y - kSlotSize) * 0.5f };
    out->size = Vec2{ kSlotSize, kSlotSize };
    menu->add(std::move(out));

    menu->size = Vec2{ kEdgePad + gw + 32.0f + kSlotSize + kEdgePad,
                       gridBottom + kEdgePad };

    const float gap = 16.0f;
    float totalW = menu->size.x + gap + playerMenu->size.x;
    float startX = (ui.getScreenW() - totalW) * 0.5f;
    float midY   = ui.getScreenH() * 0.5f;
    menu->pos       = Vec2{ startX, midY - menu->size.y * 0.5f };
    playerMenu->pos = Vec2{ startX + menu->size.x + gap,
                            midY - playerMenu->size.y * 0.5f };

    recompute();
    mgr.open(std::move(menu));
}

} // namespace MenuFactory
