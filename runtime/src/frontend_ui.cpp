#include "runtime/frontend_ui.hpp"
#include <algorithm>
namespace nes::ui {
void FrontendUI::start(double now){started_=true;started_at_=now;dismissed_=false;}
bool FrontendUI::banner_visible(double now)const{return started_&&!dismissed_&&!modal_&&now>=started_at_&&now-started_at_<10.0;}
float FrontendUI::banner_alpha(double now)const{
    if(!banner_visible(now))return 0;
    return static_cast<float>(std::min({1.0,(now-started_at_)/0.15,(10.0-(now-started_at_))/0.25}));
}
void FrontendUI::handle(Action action,bool repeat,bool simultaneous){
    if(action==Action::focus_lost){focused_=false;input_barrier_=true;armed_=false;return;}
    if(action==Action::focus_gained){focused_=true;return;}
    if(repeat||simultaneous||quit_||(!focused_&&action!=Action::request_exit))return;
    if(action==Action::dismiss_help){dismissed_=true;return;}
    if(action==Action::request_exit&&!modal_){
        if(request_barrier_)return;
        modal_=true;exit_selected_=false;armed_=false;input_barrier_=true;action_seen_=true;return;
    }
    if(!modal_)return;
    if(action==Action::cancel&&(!action_seen_||armed_)){modal_=false;armed_=false;request_barrier_=true;input_barrier_=true;action_seen_=true;return;}
    if(!armed_)return;
    if(action==Action::previous||action==Action::next){exit_selected_=action==Action::next;action_seen_=true;return;}
    if(action==Action::confirm){
        if(action_seen_)return; // Navigation and confirmation cannot coalesce in one event batch.
        action_seen_=true;
        if(exit_selected_)quit_=true;
        else{modal_=false;armed_=false;request_barrier_=true;input_barrier_=true;}
    }
}
void FrontendUI::observe_neutral(bool modal_idle,bool game_idle){
    if(modal_&&focused_&&modal_idle)armed_=true;
    if(!modal_&&modal_idle)request_barrier_=false;
    if(game_idle)input_barrier_=false;
}
void FrontendUI::click_button(bool exit){if(!modal_||!focused_)return;if(!exit){handle(Action::cancel);return;}if(!armed_)return;exit_selected_=true;action_seen_=false;handle(Action::confirm);}
bool Rect::contains(int px,int py)const{return px>=x&&py>=y&&px<x+width&&py<y+height;}
Rect overlay_rect(int width,int height,bool modal){
    if(width<=0||height<=0)return {};
    const int bw=640,bh=modal?180:112;
    const double scale=std::min({width/960.0,height/720.0,width*0.94/bw,height*0.90/bh});
    const int w=std::max(1,static_cast<int>(bw*scale)),h=std::max(1,static_cast<int>(bh*scale));
    return {(width-w)/2,modal?(height-h)/2:std::max(0,static_cast<int>(16*scale)),w,h};
}
Rect cancel_button(){return {110,112,180,44};}Rect exit_button(){return {350,112,180,44};}
}
