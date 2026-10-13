#pragma once
#include "runtime/video_state.hpp"
#include "runtime/frontend_ui.hpp"
#include <memory>
#include <string>
struct SDL_Window;
namespace nes::frontend { class FrontendTiming; class FrameSnapshot; }
namespace nes::runtime {
// SDL owns the window/context; this object owns only OpenGL rendering objects.
class GLPresenter {
public:
    explicit GLPresenter(SDL_Window* window, nes::frontend::FrontendTiming* timing=nullptr);
    ~GLPresenter();
    GLPresenter(const GLPresenter&) = delete;
    GLPresenter& operator=(const GLPresenter&) = delete;
    void present(const VideoState& video);
    // Experimental: retain texture only for this completed publication.
    void present(const nes::frontend::FrameSnapshot& snapshot);
    // Reject extended/cropped buffers; production presentation is always 256 x 240.
    void present(const uint8_t* indices, unsigned width, unsigned height);
    void present_overlay(const nes::ui::FrontendUI& ui, double monotonic_seconds);
    void present_panel(const nes::ui::Bitmap& bitmap,nes::ui::Rect rect);
    void capture(const std::string& path);
    const char* version() const;
private:
    bool present_pixels(const uint8_t*,unsigned,unsigned,bool upload);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
