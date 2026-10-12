#pragma once
#include "runtime/frontend_ui.hpp"
#include <SDL.h>
#include <set>
#include <utility>
namespace nes::ui {
// SDL translation only; no CPU, PPU, mapper, module or ROM access.
class FrontendEvents {
public:
    explicit FrontendEvents(FrontendUI& ui):ui_(ui){}
    void handle(const SDL_Event& event,int window_width,int window_height,int drawable_width,int drawable_height,bool simultaneous=false);
private:
    FrontendUI& ui_;int axis_direction_{};
    std::set<SDL_Keycode> held_keys_;
    std::set<std::pair<SDL_JoystickID,Uint8>> held_buttons_;
};
uint8_t game_buttons(const Uint8* keys,SDL_GameController* pad);
bool modal_controls_idle(const Uint8* keys,SDL_GameController* pad);
bool all_controls_idle(const Uint8* keys,SDL_GameController* pad);
unsigned modal_pressed_count(const Uint8* keys,SDL_GameController* pad);
}
