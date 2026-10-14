#include "runtime/mod_frontend.hpp"
#include "runtime/application_icon.hpp"
#include <filesystem>
#include <array>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#ifdef __APPLE__
std::string choose_mod_zip_macos();
std::string choose_mod_profile_macos(bool,const std::string&);
#endif
namespace nes::mods {
namespace fs=std::filesystem;
Frontend::Frontend(bool safe,const std::string& directory,Target target){
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
        if(storage.empty()){char* pref=SDL_GetPrefPath("nes-native-lab",NES_MOD_STORE_NAME);if(!pref)throw std::runtime_error(SDL_GetError());storage=fs::u8path(pref);SDL_free(pref);}
        manager_=std::make_unique<Manager>(storage,python,helper,std::move(target));refresh();
    }catch(const std::exception& e){panel_.message(e.what());}
}
void Frontend::end_events(){
    // Native modal pickers queue focus/input events while SDL is suspended.
    // SDL_PollEvent pumps these events; review only after the batch has drained.
    if(!visible())deferred_profile_review_.clear();
    if(focused_&&!deferred_profile_review_.empty()){auto file=std::move(deferred_profile_review_);deferred_profile_review_.clear();review_profile(file);}
}
void Frontend::refresh(){if(manager_){panel_.records(manager_->records());panel_.profiles(manager_->profiles());if(!manager_->error().empty())panel_.message(manager_->error());}}
void Frontend::install(const std::string& zip){if(zip.empty())return;try{if(!manager_)throw std::runtime_error("Mod Manager unavailable or safe mode active");manager_->install(fs::u8path(zip));panel_.message("Mod installed. Select Enable to run it.");refresh();}catch(const std::exception& e){panel_.message(e.what());}}
void Frontend::navigate(PanelAction action){if(navigation_used_)return;navigation_used_=true;execute(panel_.handle(action));}
std::string Frontend::profile_picker(bool save,const std::string&name){
#ifdef __APPLE__
    return choose_mod_profile_macos(save,name);
#elif defined(_WIN32)
    wchar_t file[32768]{};if(save){auto initial=fs::u8path(name+".json").wstring();std::copy(initial.begin(),initial.end(),file);}OPENFILENAMEW p{};p.lStructSize=sizeof(p);p.lpstrFile=file;p.nMaxFile=32768;p.lpstrTitle=save?L"Export Mod Profile":L"Import Mod Profile";p.lpstrFilter=L"Mod profile JSON\0*.json\0";p.lpstrDefExt=L"json";p.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?0:OFN_FILEMUSTEXIST);if(save?GetSaveFileNameW(&p):GetOpenFileNameW(&p))return fs::path(file).u8string();return {};
#else
    if(!save){panel_.message("Drop a profile JSON into Mods to review and import.");return {};}
    auto directory=manager_->root()/"exports";if(fs::is_symlink(directory))throw std::runtime_error("Linked export directory; choose a safe store");fs::create_directories(directory);return (directory/(name+".json")).u8string();
#endif
}
void Frontend::review_profile(const std::string&file){try{if(!manager_)throw std::runtime_error("Safe mode: profile import unavailable");manager_->inspect_profile(fs::u8path(file));pending_profile_file_=file;profile_review_ticks_=SDL_GetTicks();panel_.confirm(Command::profile_import,"IMPORT REVIEWED PROFILE?",manager_->preview_name()+": "+manager_->preview_summary()+". It will not be activated.");}catch(const std::exception&e){pending_profile_file_.clear();panel_.message(e.what());}}
void Frontend::execute(Command c){
    if(c==Command::play){play_requested_=true;return;}
    if(c==Command::close){if(launch_menu_&&panel_.page()!=Page::main)panel_.main();return;}
    if(c==Command::none)return;
    try {
        if(!manager_)throw std::runtime_error("Mod Manager unavailable or safe mode active");
        if(c==Command::profile_delete){if(const auto*p=panel_.selected_profile())manager_->profile("delete",p->name);panel_.message("Profile deleted. Installed mods and other profiles preserved.");refresh();return;}
        if(c==Command::profile_export){if(const auto*p=panel_.selected_profile()){auto name=p->name;auto file=profile_picker(true,name);if(!file.empty()){manager_->export_profile(name,fs::u8path(file));panel_.message("Profile exported: "+file);}}return;}
        if(c==Command::profile_pick){auto file=profile_picker(false,"");if(!file.empty())deferred_profile_review_=file;return;}
        if(c==Command::profile_import){manager_->import_profile(fs::u8path(pending_profile_file_));pending_profile_file_.clear();panel_.message("Profile imported but not activated. Select it, then Apply.");refresh();return;}
        if(c==Command::configure_defaults){if(const auto*r=panel_.selected()){bool changed=manager_->configure(r->id,{});panel_.message(changed?"Default settings restored.":"Already using default settings. No restart.");}refresh();panel_.configuration_saved();return;}
        if(c==Command::profile_apply){if(const auto*p=panel_.selected_profile())manager_->profile("apply",p->name);panel_.message("Profile applied. Changes take effect on resume.");refresh();return;}
        if(c==Command::profile_save){manager_->profile("save",panel_.name());panel_.message("Profile saved.");refresh();return;}
        if(c==Command::profile_rename){manager_->rename_profile(panel_.old_name(),panel_.name());panel_.message("Profile renamed.");refresh();return;}
        if(c==Command::configure){if(const auto*r=panel_.selected()) {bool changed=manager_->configure(r->id,panel_.settings());panel_.message(changed?"Settings saved. Changed active mod restarted.":"No settings changed. No restart.");}refresh();panel_.configuration_saved();return;}
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
        if(e.window.event==SDL_WINDOWEVENT_FOCUS_LOST){focused_=false;held_.clear();buttons_.clear();axes_={};if(!panel_.awaiting(Command::profile_import)||!focus_precedes_review(e.window.timestamp,profile_review_ticks_))panel_.cancel_pending();}
        if(e.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)focused_=true;
        return false;
    }
    if(!focused_){
        if(e.type==SDL_DROPFILE){SDL_free(e.drop.file);return true;}
        if(e.type==SDL_QUIT){panel_.close();return false;}
        return true;
    }
    if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_F2&&!e.key.repeat&&held_.insert(SDLK_F2).second){if(visible()){if(!launch_menu_)panel_.close();}else show();return true;}
    if(e.type==SDL_CONTROLLERBUTTONDOWN&&e.cbutton.button==SDL_CONTROLLER_BUTTON_RIGHTSTICK&&buttons_.insert(e.cbutton.button).second){if(visible()){if(!launch_menu_)panel_.close();}else show();return true;}
    if(e.type==SDL_DROPFILE){std::string path=e.drop.file?e.drop.file:"";SDL_free(e.drop.file);if(visible()){auto extension=fs::u8path(path).extension().u8string();std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return char(std::tolower(c));});if(extension==".json")deferred_profile_review_=path;else install(path);}return true;}
    if(!visible())return false;
    if(e.type==SDL_QUIT){panel_.close();return false;}
    if(e.type==SDL_TEXTINPUT){panel_.text(e.text.text);return true;}
    if(e.type==SDL_KEYDOWN){auto k=e.key.keysym.sym;if(e.key.repeat||!held_.insert(k).second)return true;
        if(k==SDLK_BACKSPACE)panel_.erase_text();
        if(k==SDLK_d)panel_.shortcut('d');if(k==SDLK_c)panel_.shortcut('c');if(k==SDLK_p)panel_.shortcut('p');if(k==SDLK_o)panel_.shortcut('o');if(k==SDLK_r)panel_.shortcut('r');
        if(k==SDLK_ESCAPE)execute(panel_.handle(PanelAction::back));
        if(k==SDLK_UP)navigate(PanelAction::up);if(k==SDLK_DOWN)navigate(PanelAction::down);
        if(k==SDLK_LEFT)navigate(PanelAction::left);if(k==SDLK_RIGHT||k==SDLK_TAB)navigate(PanelAction::right);
        if(k==SDLK_RETURN||k==SDLK_SPACE)execute(panel_.handle(PanelAction::activate));
        if(k==SDLK_F11)return false;
        if(k==SDLK_q&&(e.key.keysym.mod&KMOD_GUI)){panel_.close();return false;}return true;
    }
    if(e.type==SDL_CONTROLLERBUTTONDOWN){auto b=e.cbutton.button;if(!buttons_.insert(b).second)return true;
        if(b==SDL_CONTROLLER_BUTTON_X){if(panel_.page()==Page::name)panel_.erase_text();else if(panel_.page()==Page::configure)panel_.shortcut('r');else panel_.shortcut('d');}if(b==SDL_CONTROLLER_BUTTON_Y){if(panel_.page()==Page::name)panel_.text("A");else panel_.shortcut('c');}if(b==SDL_CONTROLLER_BUTTON_LEFTSHOULDER)panel_.shortcut('p');if(b==SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)panel_.shortcut('o');
        if(b==SDL_CONTROLLER_BUTTON_B)execute(panel_.handle(PanelAction::back));if(b==SDL_CONTROLLER_BUTTON_A)execute(panel_.handle(PanelAction::activate));
        if(b==SDL_CONTROLLER_BUTTON_DPAD_UP)navigate(PanelAction::up);if(b==SDL_CONTROLLER_BUTTON_DPAD_DOWN)navigate(PanelAction::down);
        if(b==SDL_CONTROLLER_BUTTON_DPAD_LEFT)navigate(PanelAction::left);if(b==SDL_CONTROLLER_BUTTON_DPAD_RIGHT)navigate(PanelAction::right);
        if(b==SDL_CONTROLLER_BUTTON_GUIDE){panel_.close();return false;}return true;
    }
    if(e.type==SDL_CONTROLLERAXISMOTION){if(e.caxis.axis<=SDL_CONTROLLER_AXIS_LEFTY){unsigned axis=e.caxis.axis;int value=e.caxis.value;int direction=value>16000?1:value<-16000?-1:0;if(std::abs(value)<8000)axes_[axis]=0;else if(direction&&direction!=axes_[axis]){axes_[axis]=direction;navigate(axis==0?(direction<0?PanelAction::left:PanelAction::right):(direction<0?PanelAction::up:PanelAction::down));}}return true;}
    if(e.type==SDL_MOUSEBUTTONUP&&e.button.button==SDL_BUTTON_LEFT&&ww>0&&wh>0){auto r=Panel::rect(dw,dh);if(r.width&&r.height)execute(panel_.click(int((double(e.button.x)*dw/ww-r.x)*640/r.width),int((double(e.button.y)*dh/wh-r.y)*430/r.height)));return true;}
    // Focus events still reach the original pause/input system.
    return e.type!=SDL_WINDOWEVENT;
}
void Frontend::apply_resources(nes::runtime::Cartridge& cartridge){
    if(!manager_||applied_resources_==manager_->resource_revision())return;
    try{cartridge.set_text_resources(manager_->resources());}
    catch(const std::exception&e){manager_->reject_resources(e.what());cartridge.set_text_resources({});refresh();}
    applied_resources_=manager_->resource_revision();
}
void Frontend::draw_hud(nes::runtime::GLPresenter& presenter,int w,int h,bool fullscreen){
    if(manager_&&visible())for(unsigned i=0;i<2;++i)if(!manager_->visual(i).width)presenter.present_mod_plane({}, {}, manager_->visual_revision(),i);
    if(manager_&&!visible()){for(unsigned i=0;i<2;++i)presenter.present_mod_plane(manager_->visual(i),visual_rect(w,h,fullscreen,i),manager_->visual_revision(),i);presenter.present_panel(manager_->overlay(),hud_rect(w,h,fullscreen));}
}
void Frontend::draw(nes::runtime::GLPresenter& presenter,int w,int h){if(visible()){refresh();presenter.present_panel(panel_.bitmap(),Panel::rect(w,h));}}
int manager_window(bool safe,const std::string& directory,Target target,bool launch_menu,bool* fullscreen,bool* muted){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0)throw std::runtime_error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    auto* w=SDL_CreateWindow(launch_menu?"NES Native Lab":"NES Native Lab - Mod Manager (no game loaded)",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,720,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
    if(!w){SDL_Quit();throw std::runtime_error(SDL_GetError());}auto gl=SDL_GL_CreateContext(w);if(!gl){SDL_DestroyWindow(w);SDL_Quit();throw std::runtime_error(SDL_GetError());}
    int result=0;
    try {nes::runtime::set_application_icon(w);SDL_GL_SetSwapInterval(0);nes::runtime::GLPresenter presenter(w);Frontend manager(safe,directory,std::move(target));if(launch_menu)manager.main_menu();else manager.show();manager.launch_options(fullscreen&&*fullscreen,muted&&*muted);SDL_StartTextInput();
        SDL_GameController* pad=nullptr;for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_IsGameController(i)){pad=SDL_GameControllerOpen(i);break;}
        std::array<uint8_t,256*240> empty{};
        while(manager.visible()){manager.begin_events();int ww,wh,dw,dh;SDL_GetWindowSize(w,&ww,&wh);SDL_GL_GetDrawableSize(w,&dw,&dh);SDL_Event e{};while(SDL_PollEvent(&e)){if(e.type==SDL_CONTROLLERDEVICEADDED&&!pad&&SDL_IsGameController(e.cdevice.which))pad=SDL_GameControllerOpen(e.cdevice.which);if(e.type==SDL_CONTROLLERDEVICEREMOVED&&pad&&SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==e.cdevice.which){SDL_GameControllerClose(pad);pad=nullptr;}if(e.type==SDL_QUIT){manager.handle(e,ww,wh,dw,dh);break;}if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_F11&&!e.key.repeat)SDL_SetWindowFullscreen(w,(SDL_GetWindowFlags(w)&SDL_WINDOW_FULLSCREEN)?0:SDL_WINDOW_FULLSCREEN_DESKTOP);manager.handle(e,ww,wh,dw,dh);}manager.end_events();presenter.present(empty.data(),256,240);manager.draw(presenter,dw,dh);SDL_GL_SwapWindow(w);SDL_Delay(16);}
        if(fullscreen)*fullscreen=manager.launch_fullscreen();if(muted)*muted=manager.launch_muted();if(launch_menu&&manager.play_requested())result=1;SDL_StopTextInput();
        if(pad)SDL_GameControllerClose(pad);
    }catch(const std::exception& e){std::cerr<<"Mod Manager: "<<e.what()<<"\n";result=launch_menu?-1:1;}
    SDL_GL_DeleteContext(gl);SDL_DestroyWindow(w);SDL_Quit();return result;
}
}
