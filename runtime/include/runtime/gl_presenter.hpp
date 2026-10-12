#pragma once
#include "runtime/video_state.hpp"
#include "runtime/frontend_ui.hpp"
#include <memory>
#include <string>
struct SDL_Window;
namespace nes::runtime {
// SDL owns the window/context; this object owns only OpenGL rendering objects.
class GLPresenter {
public:
    explicit GLPresenter(SDL_Window* window);
    ~GLPresenter();
    GLPresenter(const GLPresenter&) = delete;
    GLPresenter& operator=(const GLPresenter&) = delete;
    void present(const VideoState& video);
    // Reject extended/cropped buffers; production presentation is always 256 x 240.
    void present(const uint8_t* indices, unsigned width, unsigned height);
    void present_overlay(const nes::ui::FrontendUI& ui, double monotonic_seconds);
    void capture(const std::string& path);
    const char* version() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
