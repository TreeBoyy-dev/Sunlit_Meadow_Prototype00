#pragma once
#include "Vectors.h"
#include "UIAction.h"
#include "Item.h"        // ItemInstance, Item
#include <string>
#include <functional>

class UI_Renderer;
class Inventory;
class MenuManager;
struct UITexture;        // defined in SurvivalUI.h

enum class MouseButton { Left, Right, Middle };

// --- model orientation constants, copied from drawInventoryHotbar() ----------
constexpr float kBlockPitch = 0.79f;
constexpr float kBlockYaw   = 0.91f;
constexpr float kBlockRoll  = -2.43f;
constexpr float kItemPitch  = 1.6f;
constexpr float kItemYaw    = 1.6f;
constexpr float kItemRoll   = 0.0f;

// Shared helper: draw an ItemInstance (model + count text) into a square panel.
// Used by SlotWidget, DisplayWidget and the MenuManager cursor stack.
void DrawItemInstance(UI_Renderer* ui, const ItemInstance& inst,
                      float x, float y, float size);

// =============================================================================
class Widget {
public:
    Vec2 pos{ 0, 0 };    // relative to menu origin, pixels
    Vec2 size{ 0, 0 };
    bool visible = true;
    bool hovered = false; // updated by MenuManager on mouse motion

    virtual ~Widget() = default;

    // menuOrigin = menu's top-left in screen pixels
    virtual void draw(UI_Renderer* ui, Vec2 menuOrigin) = 0;

    // localMouse = mouse relative to menu origin. Return true if handled.
    virtual bool onClick(Vec2 localMouse, MouseButton b, MenuManager& mgr) { return false; }
    virtual void onDrag (Vec2 localMouse, MenuManager& mgr) {}
    virtual void onRelease(MenuManager& mgr) {}

    bool contains(Vec2 localMouse) const {
        return localMouse.x >= pos.x && localMouse.x <= pos.x + size.x &&
               localMouse.y >= pos.y && localMouse.y <= pos.y + size.y;
    }
};

// --- 4.1 visual-only decoration ----------------------------------------------
class DisplayWidget : public Widget {
public:
    std::string  textureName;      // looked up in UITextureSet; "" = none
    ItemInstance itemDisplay{};    // alternatively render an item model
    float pitch = kItemPitch, yaw = kItemYaw, roll = kItemRoll;

    void draw(UI_Renderer* ui, Vec2 origin) override;
};

// --- 4.2 interactive inventory slot ------------------------------------------
class SlotWidget : public Widget {
public:
    Inventory* inv  = nullptr;   // live backing store, never a copy
    int        slot = 0;
    bool       outputOnly = false;

    // fired after any change to this slot (used for auto-craft on grid change)
    std::function<void()> onChanged;

    void draw(UI_Renderer* ui, Vec2 origin) override;
    bool onClick(Vec2 local, MouseButton b, MenuManager& mgr) override;
};

// --- 4.3 button ----------------------------------------------------------------
class ButtonWidget : public Widget {
public:
    std::string label;
    std::string textureName;   // optional; "" = flat rect
    UIAction    action;

    void draw(UI_Renderer* ui, Vec2 origin) override;
    bool onClick(Vec2 local, MouseButton b, MenuManager& mgr) override;
};

// --- 4.4 slider ------------------------------------------------------------------
class SliderWidget : public Widget {
public:
    float    minV = 0.0f, maxV = 1.0f, value = 0.0f;
    UIAction onChange;         // called with ctx.value = value

    void draw(UI_Renderer* ui, Vec2 origin) override;
    bool onClick(Vec2 local, MouseButton b, MenuManager& mgr) override;
    void onDrag (Vec2 local, MenuManager& mgr) override;
    void onRelease(MenuManager& mgr) override;

private:
    void setFromMouse(float localX, MenuManager& mgr);
};

// --- 4.5 label / live readout -----------------------------------------------------
class LabelWidget : public Widget {
public:
    std::string text;                          // static text, or:
    std::function<std::string()> liveText;     // evaluated every frame if set
    SDL_FColor color{ 1.0f, 1.0f, 1.0f, 1.0f };

    void draw(UI_Renderer* ui, Vec2 origin) override;
};
