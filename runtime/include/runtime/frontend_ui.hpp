#pragma once
#include <cstdint>
#include <vector>
#include <string>
namespace nes::ui {
enum class Action { request_exit, cancel, confirm, previous, next, dismiss_help, focus_lost, focus_gained };
class FrontendUI {
public:
    void start(double monotonic_seconds);
    void begin_events() { action_seen_=false; }
    void handle(Action action,bool repeat=false,bool simultaneous=false);
    void observe_neutral(bool modal_controls_released,bool game_controls_released);
    bool banner_visible(double now) const;
    float banner_alpha(double now) const;
    bool modal() const { return modal_; }
    bool exit_selected() const { return exit_selected_; }
    bool quit_requested() const { return quit_; }
    bool paused() const { return modal_||external_paused_||!focused_; }
    void external_pause(bool value){if(external_paused_!=value){external_paused_=value;input_barrier_=true;}}
    bool game_input_enabled() const { return !paused()&&!input_barrier_; }
    bool armed() const { return armed_; }
    void click_button(bool exit);
private:
    bool started_{},dismissed_{},modal_{},exit_selected_{},quit_{},armed_{},focused_{true};
    bool input_barrier_{},request_barrier_{},action_seen_{};
    bool external_paused_{};
    double started_at_{};
};
struct Rect { int x{},y{},width{},height{}; bool contains(int px,int py) const; };
struct Bitmap { unsigned width{},height{}; std::vector<uint8_t> rgba; };
Rect overlay_rect(int width,int height,bool modal);
Rect cancel_button(); Rect exit_button();
Bitmap overlay_bitmap(const FrontendUI& ui,double now);
Bitmap prompt_bitmap(const char* title,const char* message);
Bitmap menu_bitmap(const std::vector<std::string>& lines,const std::vector<std::string>& buttons,unsigned selected);
}
