#pragma once
#include "Menu.h"
#include "UIAction.h"
#include <vector>
#include <memory>

class UI_Renderer;

class MenuManager {
public:
    std::vector<std::unique_ptr<Menu>> menus; // order = z-order; back() is topmost
    ItemInstance cursorStack{};               // item "on the mouse"
    Vec2 mousePos{ 0, 0 };                    // absolute, updated from events

    // drag state
    Menu*        draggedMenu  = nullptr;
    Vec2         dragOffset{ 0, 0 };
    SliderWidget* activeSlider = nullptr;
    Menu*        activeSliderMenu = nullptr;

    // set once in SDL_AppInit; used to toggle relative mouse mode
    void setWindow(SDL_Window* w) { window = w; }

    bool anyOpen() const { return !menus.empty(); }
    Menu* findById(const std::string& id);

    Menu* open(std::unique_ptr<Menu> m);      // pushes on top, returns raw ptr
    void  close(Menu* m);
    void  closeTop();                         // ESC
    void  closeAll();
    void  bringToFront(Menu* m);

    // Return true if the manager consumed the input (skip world interaction).
    bool handleMouseMotion(float x, float y);
    bool handleMouseDown(MouseButton b, float x, float y);
    bool handleMouseUp  (MouseButton b, float x, float y);

    void update(float dt);                    // reserved (drags run off events)
    void draw(UI_Renderer* ui);               // menus back->front, cursor item last

    // Build an ActionContext with actor = player entity 0 (or menu->owner if set).
    ActionContext makeContext(Menu* menu);

private:
    SDL_Window* window = nullptr;
    void onOpenTransition();   // first menu opened  -> absolute mouse
    void onCloseTransition();  // last menu closed   -> relative mouse, drop cursor stack
    void returnCursorStackToPlayer();
};
