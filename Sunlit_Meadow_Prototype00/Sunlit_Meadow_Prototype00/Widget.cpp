#include "Widget.h"
#include "Menu.h"
#include "MenuManager.h"
#include "Inventory.h"
#include "SurvivalUI.h"   // UITextureSet
#include "Globals.h"      // ui (only for screen size fallbacks)
#include "UI_Renderer.h"

#include <algorithm>
#include <cstdio>

// ---------------------------------------------------------------------------

void DrawItemInstance(UI_Renderer* ui, const ItemInstance& inst,
                      float x, float y, float size) {
    if (inst.isEmpty()) return;

    ItemModel* model = inst.item->getModel();
    if (model != nullptr && !model->isEmpty()) {
        const bool isBlock = (inst.item->getCategory() == ITEM_CATEGORY_BLOCK);
        const float pitch = isBlock ? kBlockPitch : kItemPitch;
        const float yaw   = isBlock ? kBlockYaw   : kItemYaw;
        const float roll  = isBlock ? kBlockRoll  : kItemRoll;
        ui->drawItemModel(model, x, y, size, size, pitch, yaw, roll);
    }

    if (inst.count > 1) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d", (int)inst.count);
        // bottom-right corner of the slot
        ui->drawText(buf, x + size - 14.0f, y + size - 20.0f,
                     SDL_FColor{ 1.0f, 1.0f, 1.0f, 1.0f });
    }
}

// =========================== DisplayWidget ==================================
void DisplayWidget::draw(UI_Renderer* ui, Vec2 origin) {
    if (!visible) return;
    const float x = origin.x + pos.x, y = origin.y + pos.y;

    if (const UITexture* t = ui->FindUITexture(textureName))
        ui->drawTexture(t->texture, x, y, size.x, size.y);

    if (!itemDisplay.isEmpty()) {
        ItemModel* model = itemDisplay.item->getModel();
        if (model && !model->isEmpty())
            ui->drawItemModel(model, x, y, size.x, size.y, pitch, yaw, roll);
    }
}

// =========================== SlotWidget =====================================
void SlotWidget::draw(UI_Renderer* ui, Vec2 origin) {
    if (!visible || inv == nullptr) return;
    const float x = origin.x + pos.x, y = origin.y + pos.y;

    const UITexture* tex = ui->FindUITexture("slot");
    if (tex) {
        ui->drawTexture(tex->texture, x, y, size.x, size.y);
    } else {
        ui->drawRect(x, y, size.x, size.y, 0.25f, 0.25f, 0.28f, 0.9f);
    }

    if (hovered) // subtle highlight on top
        if (tex) {
            ui->drawTexture(tex->texture, x-1, y-1, size.x+2, size.y+2);
        }
        else {
            ui->drawRect(x, y, size.x, size.y, 1.0f, 1.0f, 1.0f, 0.15f);
        }

    DrawItemInstance(ui, inv->getItemsFromSlot(slot), x, y, size.x);
}

bool SlotWidget::onClick(Vec2 local, MouseButton b, MenuManager& mgr) {
    if (inv == nullptr) return true; // still consume the click

    ItemInstance& cursor  = mgr.cursorStack;
    ItemInstance  slotItem = inv->getItemsFromSlot(slot);

    const bool sameItem = !cursor.isEmpty() && !slotItem.isEmpty() &&
                          cursor.item->getID() == slotItem.item->getID();

    if (b == MouseButton::Left) {
        if (cursor.isEmpty() && !slotItem.isEmpty()) {
            // pick up whole stack
            cursor = inv->takeItems(slot);
        }
        else if (!cursor.isEmpty() && slotItem.isEmpty()) {
            if (!outputOnly) {                       // place whole stack
                inv->setItemsToSlot(cursor, slot);
                cursor = ItemInstance{};
            }
        }
        else if (sameItem) {
            const int maxS = cursor.item->getMaxStackSize();
            if (outputOnly) {
                // merge slot -> cursor (taking from an output)
                int space = maxS - cursor.count;
                int moved = std::min<int>(space, slotItem.count);
                if (moved > 0) {
                    inv->takeItems(slot, moved);
                    cursor.count += (short)moved;
                }
            } else {
                // merge cursor -> slot up to max stack size
                int space = maxS - slotItem.count;
                int moved = std::min<int>(space, cursor.count);
                if (moved > 0) {
                    slotItem.count += (short)moved;
                    inv->setItemsToSlot(slotItem, slot);
                    cursor.count -= (short)moved;
                    if (cursor.count <= 0) cursor = ItemInstance{};
                }
            }
        }
        else if (!cursor.isEmpty() && !slotItem.isEmpty()) {
            if (!outputOnly)                         // swap
                cursor = inv->setItemsToSlot(cursor, slot);
        }
    }
    else if (b == MouseButton::Right) {
        if (cursor.isEmpty() && !slotItem.isEmpty()) {
            // pick up half (rounded up)
            int half = (slotItem.count + 1) / 2;
            cursor = inv->takeItems(slot, half);
        }
        else if (!cursor.isEmpty() && !outputOnly) {
            // place exactly one
            if (slotItem.isEmpty()) {
                ItemInstance one{ cursor.item, 1 };
                inv->setItemsToSlot(one, slot);
                cursor.count--;
                if (cursor.count <= 0) cursor = ItemInstance{};
            }
            else if (sameItem &&
                     slotItem.count < cursor.item->getMaxStackSize()) {
                slotItem.count++;
                inv->setItemsToSlot(slotItem, slot);
                cursor.count--;
                if (cursor.count <= 0) cursor = ItemInstance{};
            }
        }
    }

    if (onChanged) onChanged();   // auto-craft hooks etc.
    return true;
}

// =========================== ButtonWidget ===================================
void ButtonWidget::draw(UI_Renderer* ui, Vec2 origin) {
    if (!visible) return;
    const float x = origin.x + pos.x, y = origin.y + pos.y;

    if (const UITexture* t = ui->FindUITexture(textureName)) {
        SDL_FColor tint = hovered ? SDL_FColor{ 1.2f, 1.2f, 1.2f, 1.0f }
                                  : SDL_FColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        ui->drawTexture(t->texture, x, y, size.x, size.y, tint);
    } else {
        const float base = hovered ? 0.45f : 0.35f;
        ui->drawRect(x, y, size.x, size.y, base, base, base + 0.05f, 0.95f);
    }

    if (!label.empty())
        ui->drawText(label.c_str(), x + 8.0f, y + size.y * 0.5f - 9.0f,
                     SDL_FColor{ 1.0f, 1.0f, 1.0f, 1.0f });
}

bool ButtonWidget::onClick(Vec2 local, MouseButton b, MenuManager& mgr) {
    if (b != MouseButton::Left) return true;
    if (action) {
        ActionContext ctx = mgr.makeContext(nullptr /* menu filled by caller */);
        // MenuManager::routeClick fills ctx.menu before dispatch; this path is
        // the fallback when a button is clicked programmatically.
        action(ctx);
    }
    return true;
}

// =========================== SliderWidget ===================================
void SliderWidget::setFromMouse(float localX, MenuManager& mgr) {
    float t = (localX - pos.x) / size.x;
    t = std::clamp(t, 0.0f, 1.0f);
    value = minV + t * (maxV - minV);
    if (onChange) {
        ActionContext ctx = mgr.makeContext(nullptr);
        ctx.value = value;
        onChange(ctx);
    }
}

void SliderWidget::draw(UI_Renderer* ui, Vec2 origin) {
    if (!visible) return;
    const float x = origin.x + pos.x, y = origin.y + pos.y;
    const float midY = y + size.y * 0.5f;

    // track
    ui->drawLine(x, midY, x + size.x, midY, 4.0f, 0.2f, 0.2f, 0.22f, 1.0f);

    // knob
    float t = (maxV > minV) ? (value - minV) / (maxV - minV) : 0.0f;
    float knobX = x + t * size.x;
    ui->drawCircle(knobX, midY, size.y * 0.4f,
                   hovered ? 0.9f : 0.75f, 0.75f, 0.8f, 1.0f);
}

bool SliderWidget::onClick(Vec2 local, MouseButton b, MenuManager& mgr) {
    if (b != MouseButton::Left) return true;
    mgr.activeSlider = this;
    setFromMouse(local.x, mgr);
    return true;
}

void SliderWidget::onDrag(Vec2 local, MenuManager& mgr)  { setFromMouse(local.x, mgr); }
void SliderWidget::onRelease(MenuManager& mgr)           { if (mgr.activeSlider == this) mgr.activeSlider = nullptr; }

// =========================== LabelWidget ====================================
void LabelWidget::draw(UI_Renderer* ui, Vec2 origin) {
    if (!visible) return;
    const std::string s = liveText ? liveText() : text;
    if (s.empty()) return;
    ui->drawText(s.c_str(), origin.x + pos.x, origin.y + pos.y, color);
}
