#include "MenuManager.h"
#include "UI_Renderer.h"
#include "Inventory.h"
#include "Globals.h"

// ---------------------------------------------------------------------------
Menu* MenuManager::findById(const std::string& id) {
    for (auto& m : menus)
        if (m->id == id) return m.get();
    return nullptr;
}

Menu* MenuManager::open(std::unique_ptr<Menu> m) {
    const bool wasEmpty = menus.empty();
    menus.push_back(std::move(m));
    if (wasEmpty) onOpenTransition();
    return menus.back().get();
}

void MenuManager::close(Menu* m) {
    if (draggedMenu == m)      draggedMenu = nullptr;
    if (activeSliderMenu == m) { activeSlider = nullptr; activeSliderMenu = nullptr; }

    for (auto it = menus.begin(); it != menus.end(); ++it) {
        if (it->get() == m) { menus.erase(it); break; }
    }
    if (menus.empty()) onCloseTransition();
}

void MenuManager::closeTop() {
    if (menus.empty()) return;
    close(menus.back().get());
}

void MenuManager::closeAll() {
    draggedMenu = nullptr;
    activeSlider = nullptr;
    activeSliderMenu = nullptr;
    menus.clear();
    onCloseTransition();
}

void MenuManager::bringToFront(Menu* m) {
    for (auto it = menus.begin(); it != menus.end(); ++it) {
        if (it->get() == m) {
            std::unique_ptr<Menu> keep = std::move(*it);
            menus.erase(it);
            menus.push_back(std::move(keep));
            return;
        }
    }
}

// --- mouse-mode transitions --------------------------------------------------
void MenuManager::onOpenTransition() {
    if (window) SDL_SetWindowRelativeMouseMode(window, false);
    // start the cursor mid-screen so the carried item isn't drawn at 0|0
    mousePos = Vec2{ ui.getScreenW() * 0.5f, ui.getScreenH() * 0.5f };
}

void MenuManager::onCloseTransition() {
    returnCursorStackToPlayer();
    if (window) SDL_SetWindowRelativeMouseMode(window, true);
}

void MenuManager::returnCursorStackToPlayer() {
    if (cursorStack.isEmpty()) return;

    Entity* player = entityManager.getEntityById(0);
    Inventory* inv = nullptr;
    if (player)
        inv = static_cast<Inventory*>(player->getData(INVENTORY));

    if (inv == nullptr || !inv->addItemToInventory(cursorStack)) {
        // item entities don't exist yet -> the stack is deleted for now
        SDL_Log("[MenuManager] cursor stack lost (inventory full / no player): %s x%d",
                cursorStack.item ? cursorStack.item->getName().c_str() : "?",
                (int)cursorStack.count);
    }
    cursorStack = ItemInstance{};
}

// --- context ------------------------------------------------------------------
ActionContext MenuManager::makeContext(Menu* menu) {
    ActionContext ctx;
    ctx.manager = this;
    ctx.menu = menu;

    Entity* actor = (menu && menu->owner) ? menu->owner
                                          : entityManager.getEntityById(0);
    ctx.actor = actor;
    if (actor)
        ctx.actorInv = static_cast<Inventory*>(actor->getData(INVENTORY));
    return ctx;
}

// --- input routing --------------------------------------------------------------
bool MenuManager::handleMouseMotion(float x, float y) {
    mousePos = Vec2{ x, y };

    if (draggedMenu) {
        draggedMenu->pos = Vec2{ x - dragOffset.x, y - dragOffset.y };
        return true;
    }
    if (activeSlider && activeSliderMenu) {
        activeSlider->onDrag(activeSliderMenu->toLocal(mousePos), *this);
        return true;
    }

    // hover flags: only the topmost menu under the mouse gets hover
    bool foundMenu = false;
    for (auto it = menus.rbegin(); it != menus.rend(); ++it) {
        Menu* menu = it->get();
        Widget* hit = (!foundMenu && menu->contains(mousePos))
                      ? menu->hitTest(mousePos) : nullptr;
        if (!foundMenu && menu->contains(mousePos)) foundMenu = true;

        for (auto& w : menu->widgets)
            w->hovered = (w.get() == hit);
    }
    return anyOpen();
}

bool MenuManager::handleMouseDown(MouseButton b, float x, float y) {
    mousePos = Vec2{ x, y };

    // top to bottom
    for (auto it = menus.rbegin(); it != menus.rend(); ++it) {
        Menu* menu = it->get();
        if (!menu->contains(mousePos)) continue;

        bringToFront(menu);

        if (menu->closeBoxHit(mousePos)) {
            close(menu);
            return true;
        }
        if (menu->headerHit(mousePos)) {
            if (menu->draggable && b == MouseButton::Left) {
                draggedMenu = menu;
                dragOffset = Vec2{ mousePos.x - menu->pos.x,
                                   mousePos.y - menu->pos.y };
            }
            return true;
        }

        if (Widget* w = menu->hitTest(mousePos)) {
            // fill context menu for buttons: route through a small shim so the
            // widget knows which menu it lives in during this click
            if (auto* btn = dynamic_cast<ButtonWidget*>(w)) {
                if (b == MouseButton::Left && btn->action) {
                    ActionContext ctx = makeContext(menu);
                    btn->action(ctx);
                }
                return true;
            }
            if (auto* sld = dynamic_cast<SliderWidget*>(w)) {
                activeSliderMenu = menu;
                sld->onClick(menu->toLocal(mousePos), b, *this);
                return true;
            }
            w->onClick(menu->toLocal(mousePos), b, *this);
            return true;
        }

        return true; // click on empty panel space: still consumed
    }
    return false;    // nothing hit -> world interaction may proceed
}

bool MenuManager::handleMouseUp(MouseButton b, float x, float y) {
    mousePos = Vec2{ x, y };
    bool consumed = (draggedMenu != nullptr) || (activeSlider != nullptr);

    draggedMenu = nullptr;
    if (activeSlider) {
        activeSlider->onRelease(*this);
        activeSlider = nullptr;
        activeSliderMenu = nullptr;
    }
    return consumed || anyOpen();
}

void MenuManager::update(float dt) {
    // drags are event-driven; kept for future needs (tooltips, animations)
    (void)dt;
}

// --- draw ----------------------------------------------------------------------
void MenuManager::draw(UI_Renderer* uiR) {
    // back to front so the topmost menu is painted last
    for (auto& m : menus)
        m->draw(uiR);

    if (!cursorStack.isEmpty()) {
        const float s = 40.0f;
        DrawItemInstance(uiR, cursorStack,
                         mousePos.x - s * 0.5f, mousePos.y - s * 0.5f, s);
    }
}
