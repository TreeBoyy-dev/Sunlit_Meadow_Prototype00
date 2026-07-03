#pragma once
#include "Widget.h"
#include <vector>
#include <memory>
#include <string>

class Entity;
class UI_Renderer;

class Menu {
public:
    Vec2 pos{ 0, 0 };        // top-left, screen pixels
    Vec2 size{ 0, 0 };
    std::string title;
    std::string id;          // stable identifier ("player_inventory", "chest", ...)
    SDL_GPUTexture* texture;
    float headerH   = 24.0f; // draggable strip at the top
    bool  draggable = true;
    bool  closable  = true;
    Entity* owner   = nullptr; // workbench/chest entity, for ActionContext

    std::vector<std::unique_ptr<Widget>> widgets;

    void draw(UI_Renderer* ui);

    Vec2 toLocal(Vec2 screenMouse) const {
        return Vec2{ screenMouse.x - pos.x, screenMouse.y - pos.y };
    }
    bool contains(Vec2 screenMouse) const {
        return screenMouse.x >= pos.x && screenMouse.x <= pos.x + size.x &&
               screenMouse.y >= pos.y && screenMouse.y <= pos.y + size.y;
    }
    bool headerHit(Vec2 screenMouse) const {
        return contains(screenMouse) && (screenMouse.y - pos.y) <= headerH;
    }
    bool closeBoxHit(Vec2 screenMouse) const; // small box at header's right edge

    // topmost widget under the mouse (widgets added later are on top)
    Widget* hitTest(Vec2 screenMouse) const;

    // convenience: add a widget and return a typed pointer to it
    template <typename T>
    T* add(std::unique_ptr<T> w) {
        T* raw = w.get();
        widgets.push_back(std::move(w));
        return raw;
    }
};
