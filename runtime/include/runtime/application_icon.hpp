#pragma once
#include <SDL.h>
namespace nes::runtime {
// Branding only; callers own the surface. No file lookup or game state access.
SDL_Surface* make_application_icon();
bool set_application_icon(SDL_Window* window);
}
