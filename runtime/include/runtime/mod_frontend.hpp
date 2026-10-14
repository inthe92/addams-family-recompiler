#pragma once
#include "runtime/mod_panel.hpp"
#include "runtime/cartridge.hpp"
#include "runtime/gl_presenter.hpp"
#include <SDL.h>
#include <set>
namespace nes::mods {
// SDL ticks wrap: only an older timestamp can belong to the completed native modal.
inline bool focus_precedes_review(Uint32 event,Uint32 review){return event&&review&&Sint32(event-review)<=0;}
class Frontend {
public:
    Frontend(bool safe,const std::string& directory="",Target target={});
    bool handle(const SDL_Event&,int ww,int wh,int dw,int dh);
    void begin_events(){panel_.begin_events();navigation_used_=false;}
    void end_events();
    void show(){panel_.open();}
    void main_menu(){launch_menu_=true;panel_.main();}
    bool play_requested()const{return play_requested_;}
    void launch_options(bool fullscreen,bool muted){panel_.fullscreen=fullscreen;panel_.muted=muted;}
    bool launch_fullscreen()const{return panel_.fullscreen;}
    bool launch_muted()const{return panel_.muted;}
    bool visible()const{return panel_.visible();}
    void draw(nes::runtime::GLPresenter&,int w,int h);
    void update(const Observation& o){if(manager_)manager_->update(o);}
    void complete(const Observation& o){if(manager_)manager_->complete(o);}
    void apply_resources(nes::runtime::Cartridge&);
    void draw_hud(nes::runtime::GLPresenter&,int w,int h,bool fullscreen);
    unsigned jump_extension()const{return manager_?manager_->jump_extension():0;}
    bool needs_memory()const{return manager_&&manager_->needs_memory();}
    bool active()const{return manager_&&manager_->active();}
private:
    std::unique_ptr<Manager> manager_;
    Panel panel_;bool launch_menu_{},play_requested_{};
    std::set<SDL_Keycode> held_;
    std::set<Uint8> buttons_;
    Uint32 profile_review_ticks_{};
    std::string pending_profile_file_,deferred_profile_review_;bool navigation_used_{};std::array<int,2> axes_{};
    void navigate(PanelAction);
    void review_profile(const std::string&);
    std::string profile_picker(bool save,const std::string& name);
    bool focused_=true;uint64_t applied_resources_=~uint64_t(0);
    void execute(Command);
    void install(const std::string&);
    void refresh();
};
int manager_window(bool safe,const std::string& directory,Target target={},bool launch_menu=false,bool* fullscreen=nullptr,bool* muted=nullptr);
}
