#include "runtime/application_icon.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("Output RGBA path required");
        SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
        auto surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>(nes::runtime::make_application_icon(),SDL_FreeSurface);
        if(!surface || surface->w!=64 || surface->h!=64 || surface->format->format!=SDL_PIXELFORMAT_RGBA32)
            throw std::runtime_error("Wrong native icon surface");
        std::ofstream out(argv[1],std::ios::binary);
        for(int y=0;y<surface->h;++y)out.write(static_cast<const char*>(surface->pixels)+y*surface->pitch,64*4);
        out.close();if(!out)throw std::runtime_error("Cannot write icon pixels");
        auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(
            SDL_CreateWindow("Official icon contract",0,0,64,64,SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
        if(!window || !nes::runtime::set_application_icon(window.get()) || nes::runtime::set_application_icon(nullptr))
            throw std::runtime_error("Window icon application contract failed");
        window.reset();surface.reset();SDL_Quit();
        std::cout<<"SDL official icon surface and window assignment: PASS\n";
    } catch(const std::exception& error) {SDL_Quit();std::cerr<<error.what()<<'\n';return 1;}
}
