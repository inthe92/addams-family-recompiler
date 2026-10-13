#pragma once
#include "runtime/mod_panel.hpp"
#include "runtime/gl_presenter.hpp"
#include <SDL.h>
#include <set>
namespace nes::mods {
class Frontend {
public:
    Frontend(bool safe,const std::string& directory="");
    bool handle(const SDL_Event&,int ww,int wh,int dw,int dh);
    void begin_events(){panel_.begin_events();}
    void show(){panel_.open();}
    bool visible()const{return panel_.visible();}
    void draw(nes::runtime::GLPresenter&,int w,int h);
    void update(const Observation& o){if(manager_)manager_->update(o);}
    bool active()const{return manager_&&manager_->active();}
private:
    std::unique_ptr<Manager> manager_;
    Panel panel_;
    std::set<SDL_Keycode> held_;
    std::set<Uint8> buttons_;
    bool focused_=true;
    void execute(Command);
    void install(const std::string&);
    void refresh();
};
int manager_window(bool safe,const std::string& directory);
}
