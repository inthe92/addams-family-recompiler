#include "runtime/mod_hud.hpp"
#include "runtime/original_presentation.hpp"
#include <cctype>
#include <cstring>
namespace nes::mods {
bool valid_hud_text(int x,int y,std::string_view text) noexcept {
    if(text.empty()||text.size()>hud_text_limit||x<0||y<0||x>int(hud_width)||y>int(hud_height))return false;
    if(x+int(text.size())*6>int(hud_width)||y+7>int(hud_height))return false;
    for(unsigned char c:text){if(c<32||c>126||!std::strchr(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:./-?(),>",std::toupper(c)))return false;}
    return true;
}
nes::ui::Rect hud_rect(int w,int h,bool fullscreen){
    const auto v=nes::runtime::original_viewport(w,h,fullscreen);
    return {v.x,v.y,v.width,v.height*int(hud_height)/int(nes::runtime::original_height)};
}
}
