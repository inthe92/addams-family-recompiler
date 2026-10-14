#pragma once
#include "runtime/frontend_ui.hpp"
#include <string_view>
namespace nes::mods {
inline constexpr unsigned hud_width=256,hud_height=40,hud_lines_per_mod=8,hud_text_limit=40;
bool valid_hud_text(int x,int y,std::string_view text) noexcept;
nes::ui::Rect hud_rect(int width,int height,bool fullscreen);
}
