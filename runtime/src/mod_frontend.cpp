#include "runtime/mod_frontend.hpp"
#include "runtime/application_icon.hpp"
#include <filesystem>
#include <array>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#ifdef __APPLE__
std::string choose_mod_zip_macos();
#endif
namespace nes::mods {
namespace fs=std::filesystem;
Frontend::Frontend(bool safe,const std::string& directory){
    if(safe){panel_.message("Safe mode. Restart without --no-mods to manage mods.");return;}
    try {
        char* base=SDL_GetBasePath();if(!base)throw std::runtime_error(SDL_GetError());fs::path root=fs::u8path(base);SDL_free(base);
        auto helper=root/"tools/mods/packages.py";if(!fs::is_regular_file(helper))throw std::runtime_error("Mod package helper missing. Rebuild the complete application.");
#ifdef _WIN32
        auto bundled=root/"toolchain/python/python.exe";std::string python="python";
#else
        auto bundled=root/"toolchain/python/bin/python3";std::string python="python3";
#endif
        if(fs::is_regular_file(bundled))python=bundled.u8string();
        fs::path storage=fs::u8path(directory);
        if(storage.empty()){char* pref=SDL_GetPrefPath("nes-native-lab","mods");if(!pref)throw std::runtime_error(SDL_GetError());storage=fs::u8path(pref);SDL_free(pref);}
        manager_=std::make_unique<Manager>(storage,python,helper);refresh();
    }catch(const std::exception& e){panel_.message(e.what());}
}
void Frontend::refresh(){if(manager_){panel_.records(manager_->records());if(!manager_->error().empty())panel_.message(manager_->error());}}
void Frontend::install(const std::string& zip){if(zip.empty())return;try{if(!manager_)throw std::runtime_error("Mod Manager unavailable or safe mode active");manager_->install(fs::u8path(zip));panel_.message("Mod installed. Select Enable to run it.");refresh();}catch(const std::exception& e){panel_.message(e.what());}}
void Frontend::execute(Command c){
    if(c==Command::none||c==Command::close)return;
    try {
        if(!manager_)throw std::runtime_error("Mod Manager unavailable or safe mode active");
        if(c==Command::install){
#ifdef __APPLE__
            install(choose_mod_zip_macos());
#elif defined(_WIN32)
            wchar_t file[32768]{};OPENFILENAMEW p{};p.lStructSize=sizeof(p);p.lpstrFile=file;p.nMaxFile=32768;p.lpstrTitle=L"Install Lua Mod";p.lpstrFilter=L"Mod ZIP\0*.zip\0";p.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
            if(GetOpenFileNameW(&p))install(fs::path(file).u8string());
#else
            panel_.message("Drop a mod ZIP onto this window to install.");
#endif
        }else if(const auto* r=panel_.selected()){auto id=r->id;if(c==Command::toggle){const bool enabled=!r->enabled;manager_->enable(id,enabled);panel_.message(enabled?"Mod enabled. Updates follow PAL simulation.":"Mod disabled.");}else{manager_->uninstall(id);panel_.message("Mod uninstalled.");}refresh();}
    }catch(const std::exception& e){panel_.message(e.what());}
}
bool Frontend::handle(const SDL_Event& e,int ww,int wh,int dw,int dh){
    if(e.type==SDL_KEYUP)held_.erase(e.key.keysym.sym);
    if(e.type==SDL_CONTROLLERBUTTONUP)buttons_.erase(e.cbutton.button);
    if(e.type==SDL_WINDOWEVENT){
        if(e.window.event==SDL_WINDOWEVENT_FOCUS_LOST){focused_=false;held_.clear();buttons_.clear();}
        if(e.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)focused_=true;
        return false;
    }
    if(!focused_){
        if(e.type==SDL_DROPFILE){SDL_free(e.drop.file);return true;}
        if(e.type==SDL_QUIT){panel_.close();return false;}
        return true;
    }
    if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_F2&&!e.key.repeat&&held_.insert(SDLK_F2).second){if(visible())panel_.close();else show();return true;}
    if(e.type==SDL_CONTROLLERBUTTONDOWN&&e.cbutton.button==SDL_CONTROLLER_BUTTON_RIGHTSTICK&&buttons_.insert(e.cbutton.button).second){if(visible())panel_.close();else show();return true;}
    if(e.type==SDL_DROPFILE){std::string path=e.drop.file?e.drop.file:"";SDL_free(e.drop.file);if(visible())install(path);return true;}
    if(!visible())return false;
    if(e.type==SDL_QUIT){panel_.close();return false;}
    if(e.type==SDL_KEYDOWN){auto k=e.key.keysym.sym;if(e.key.repeat||!held_.insert(k).second)return true;
        if(k==SDLK_ESCAPE)execute(panel_.handle(PanelAction::back));
        if(k==SDLK_UP)execute(panel_.handle(PanelAction::up));if(k==SDLK_DOWN)execute(panel_.handle(PanelAction::down));
        if(k==SDLK_LEFT)execute(panel_.handle(PanelAction::left));if(k==SDLK_RIGHT||k==SDLK_TAB)execute(panel_.handle(PanelAction::right));
        if(k==SDLK_RETURN||k==SDLK_SPACE)execute(panel_.handle(PanelAction::activate));
        if(k==SDLK_F11)return false;
        if(k==SDLK_q&&(e.key.keysym.mod&KMOD_GUI)){panel_.close();return false;}return true;
    }
    if(e.type==SDL_CONTROLLERBUTTONDOWN){auto b=e.cbutton.button;if(!buttons_.insert(b).second)return true;
        if(b==SDL_CONTROLLER_BUTTON_B)execute(panel_.handle(PanelAction::back));if(b==SDL_CONTROLLER_BUTTON_A)execute(panel_.handle(PanelAction::activate));
        if(b==SDL_CONTROLLER_BUTTON_DPAD_UP)execute(panel_.handle(PanelAction::up));if(b==SDL_CONTROLLER_BUTTON_DPAD_DOWN)execute(panel_.handle(PanelAction::down));
        if(b==SDL_CONTROLLER_BUTTON_DPAD_LEFT)execute(panel_.handle(PanelAction::left));if(b==SDL_CONTROLLER_BUTTON_DPAD_RIGHT)execute(panel_.handle(PanelAction::right));
        if(b==SDL_CONTROLLER_BUTTON_GUIDE){panel_.close();return false;}return true;
    }
    if(e.type==SDL_MOUSEBUTTONUP&&e.button.button==SDL_BUTTON_LEFT&&ww>0&&wh>0){auto r=Panel::rect(dw,dh);if(r.width&&r.height)execute(panel_.click(int((double(e.button.x)*dw/ww-r.x)*640/r.width),int((double(e.button.y)*dh/wh-r.y)*430/r.height)));return true;}
    // Focus events still reach the original pause/input system.
    return e.type!=SDL_WINDOWEVENT;
}
void Frontend::draw(nes::runtime::GLPresenter& presenter,int w,int h){if(visible()){refresh();presenter.present_panel(panel_.bitmap(),Panel::rect(w,h));}}
int manager_window(bool safe,const std::string& directory){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0)throw std::runtime_error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    auto* w=SDL_CreateWindow("NES Native Lab - Mod Manager (no game loaded)",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,720,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
    if(!w){SDL_Quit();throw std::runtime_error(SDL_GetError());}auto gl=SDL_GL_CreateContext(w);if(!gl){SDL_DestroyWindow(w);SDL_Quit();throw std::runtime_error(SDL_GetError());}
    int result=0;
    try {nes::runtime::set_application_icon(w);SDL_GL_SetSwapInterval(0);nes::runtime::GLPresenter presenter(w);Frontend manager(safe,directory);manager.show();
        SDL_GameController* pad=nullptr;for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_IsGameController(i)){pad=SDL_GameControllerOpen(i);break;}
        std::array<uint8_t,256*240> empty{};
        while(manager.visible()){manager.begin_events();int ww,wh,dw,dh;SDL_GetWindowSize(w,&ww,&wh);SDL_GL_GetDrawableSize(w,&dw,&dh);SDL_Event e{};while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT){manager.handle(e,ww,wh,dw,dh);break;}manager.handle(e,ww,wh,dw,dh);}presenter.present(empty.data(),256,240);manager.draw(presenter,dw,dh);SDL_GL_SwapWindow(w);SDL_Delay(16);}
        if(pad)SDL_GameControllerClose(pad);
    }catch(const std::exception& e){std::cerr<<"Mod Manager: "<<e.what()<<"\n";result=1;}
    SDL_GL_DeleteContext(gl);SDL_DestroyWindow(w);SDL_Quit();return result;
}
}
