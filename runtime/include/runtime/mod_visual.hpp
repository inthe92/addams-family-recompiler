#pragma once
#include "runtime/frontend_ui.hpp"
#include <array>
#include <string>
#include <string_view>
#include <vector>
namespace nes::mods {
inline constexpr unsigned visual_width=320,visual_height=240,visual_quota=16;
struct ImageAsset {std::string id;nes::ui::Bitmap bitmap;};
struct VisualCommand {
    enum Kind {rectangle,text,image} kind{};
    unsigned space{};int x{},y{},width{},height{},z{};
    std::array<uint8_t,4> rgba{};std::string value;
};
bool valid_visual_text(int,int,std::string_view) noexcept;
void paint_visual(nes::ui::Bitmap&,const VisualCommand&,const std::vector<ImageAsset>&);
nes::ui::Rect visual_rect(int,int,bool,unsigned);
}
