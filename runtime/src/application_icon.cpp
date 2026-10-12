#include "runtime/application_icon.hpp"
#include "application_icon_data.hpp"
#include <cstring>
namespace nes::runtime {
#ifdef __APPLE__
bool set_bundle_application_icon();
#endif
SDL_Surface* make_application_icon() {
    auto* surface=SDL_CreateRGBSurfaceWithFormat(0,icon_data::width,icon_data::height,32,SDL_PIXELFORMAT_RGBA32);
    if(!surface)return nullptr;
    for(int y=0;y<icon_data::height;++y)
        std::memcpy(static_cast<unsigned char*>(surface->pixels)+y*surface->pitch,
            icon_data::rgba+y*icon_data::width*4,icon_data::width*4);
    return surface;
}
bool set_application_icon(SDL_Window* window) {
    if(!window)return false;
#ifdef __APPLE__
    if(set_bundle_application_icon())return true;
#endif
    auto* surface=make_application_icon();if(!surface)return false;
    SDL_SetWindowIcon(window,surface);SDL_FreeSurface(surface);return true;
}
}
