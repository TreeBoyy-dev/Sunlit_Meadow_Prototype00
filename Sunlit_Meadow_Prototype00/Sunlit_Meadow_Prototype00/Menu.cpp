#include "Menu.h"
#include "UI_Renderer.h"
#include "SurvivalUI.h"   // UITextureSet

static constexpr float kCloseBox = 48.0f;
static constexpr float kCloseMargin = 12.0f;

bool Menu::closeBoxHit(Vec2 m) const {
    if (!closable) return false;
    const float bx = pos.x + size.x - kCloseBox - kCloseMargin;
    const float by = pos.y + kCloseMargin;
    return m.x >= bx && m.x <= bx + kCloseBox &&
           m.y >= by && m.y <= by + kCloseBox;
}

Widget* Menu::hitTest(Vec2 screenMouse) const {
    Vec2 local = toLocal(screenMouse);
    for (auto it = widgets.rbegin(); it != widgets.rend(); ++it) {
        Widget* w = it->get();
        if (w->visible && w->contains(local))
            return w;
    }
    return nullptr;
}

void Menu::draw(UI_Renderer* ui) {
    // panel background — texture if present, flat rect otherwise
    if (texture) {
        ui->drawTexture(texture, pos.x, pos.y, size.x, size.y);
    } else {
        ui->drawRect(pos.x, pos.y, size.x, size.y, 0.12f, 0.12f, 0.14f, 0.94f);
    }

    /* header strip
    if (const UITexture* t = ui->FindUITexture("menu_header")) {
        ui->drawTexture(t->texture, pos.x, pos.y, size.x, headerH);
    } else {
        ui->drawRect(pos.x, pos.y, size.x, headerH, 0.08f, 0.08f, 0.10f, 1.0f);
    }//*/

    if (!title.empty())
        ui->drawText(title.c_str(), pos.x + 12.0f, pos.y + 12.0f,
                     SDL_FColor{ 1.0f, 1.0f, 1.0f, 1.0f });

    if (closable) {
        const float bx = pos.x + size.x - kCloseBox - kCloseMargin;
        const float by = pos.y + kCloseMargin;
        if (const UITexture* t = ui->FindUITexture("menu_close")) {
            ui->drawTexture(t->texture, bx, by, kCloseBox, kCloseBox);
        } else {
            ui->drawRect(bx, by, kCloseBox, kCloseBox, 0.6f, 0.15f, 0.15f, 1.0f);
            ui->drawText("x", bx + 4.0f, by - 2.0f, SDL_FColor{ 1, 1, 1, 1 });
        }
    }

    for (auto& w : widgets)
        if (w->visible)
            w->draw(ui, pos);
}
