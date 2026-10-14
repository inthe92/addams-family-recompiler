#include "runtime/mod_visual.hpp"
#include "runtime/original_presentation.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
namespace nes::mods {
bool valid_visual_text(int x,int y,std::string_view s) noexcept{
 if(s.empty()||s.size()>48||x<0||y<0||x+int(s.size())*6>320||y+7>240)return false;
 for(unsigned char c:s)if(c<32||c>126||!std::strchr(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:./-?(),>",std::toupper(c)))return false;return true;
}
static void blend(nes::ui::Bitmap&b,int x,int y,const uint8_t*src){
 if(x<0||y<0||x>=int(b.width)||y>=int(b.height))return;auto*out=b.rgba.data()+(y*b.width+x)*4;
 const unsigned a=src[3],old=out[3],result=a+(old*(255-a)+127)/255;if(!result)return;
 for(unsigned i=0;i<3;++i)out[i]=uint8_t((unsigned(src[i])*a+unsigned(out[i])*old*(255-a)/255+result/2)/result);out[3]=uint8_t(result);
}
void paint_visual(nes::ui::Bitmap&b,const VisualCommand&c,const std::vector<ImageAsset>&assets){
 if(c.kind==VisualCommand::rectangle){for(int y=0;y<c.height;++y)for(int x=0;x<c.width;++x)blend(b,c.x+x,c.y+y,c.rgba.data());}
 else if(c.kind==VisualCommand::text){nes::ui::paint_text(b,c.x,c.y,c.value);}
 else {auto found=std::find_if(assets.begin(),assets.end(),[&](const auto&a){return a.id==c.value;});if(found==assets.end())throw std::runtime_error("Mod image resource missing");
  for(int y=0;y<c.height;++y)for(int x=0;x<c.width;++x){auto src=found->bitmap.rgba.data()+((unsigned(y)*found->bitmap.height/c.height)*found->bitmap.width+unsigned(x)*found->bitmap.width/c.width)*4;std::array<uint8_t,4>pixel{src[0],src[1],src[2],uint8_t(unsigned(src[3])*c.rgba[3]/255)};blend(b,c.x+x,c.y+y,pixel.data());}}
}
nes::ui::Rect visual_rect(int w,int h,bool fullscreen,unsigned space){auto r=nes::runtime::original_viewport(w,h,fullscreen);if(space){r.x=0;r.y=0;}return {r.x,r.y,r.width,r.height};}
}
