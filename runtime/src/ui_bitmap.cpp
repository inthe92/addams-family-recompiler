// Small authored 5x7 UI glyphs. No game graphics, font files or UI dependency.
#include "runtime/frontend_ui.hpp"
#include "runtime/ui_text.hpp"
#include <array>
#include <cctype>
#include <cstring>
#include <string_view>
namespace nes::ui {
namespace {
const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:./-?(),";
constexpr uint8_t glyphs[][5]={
 {126,9,9,9,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},{65,65,127,65,65},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},{3,4,120,4,3},{97,81,73,69,67},
 {62,81,73,69,62},{0,66,127,64,0},{98,81,73,73,70},{34,65,73,73,54},{24,20,18,127,16},{39,69,69,69,57},{62,73,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,41,30},
 {0,54,54,0,0},{0,96,96,0,0},{32,16,8,4,2},{8,8,8,8,8},{2,1,81,9,6},{0,28,34,65,0},{0,65,34,28,0},{0,80,48,0,0}};
void pixel(Bitmap& b,int x,int y,uint8_t r,uint8_t g,uint8_t blue,uint8_t alpha){if(x<0||y<0||x>=int(b.width)||y>=int(b.height))return;auto p=(std::size_t(y)*b.width+x)*4;b.rgba[p]=r;b.rgba[p+1]=g;b.rgba[p+2]=blue;b.rgba[p+3]=alpha;}
void box(Bitmap& b,Rect r,uint8_t shade,uint8_t alpha){for(int y=r.y;y<r.y+r.height;++y)for(int x=r.x;x<r.x+r.width;++x)pixel(b,x,y,shade,shade,shade,alpha);}
void words(Bitmap& b,int x,int y,std::string_view text,int scale=2){
 int origin=x;
 for(unsigned char c:text){if(c=='\n'){x=origin;y+=18;continue;}c=static_cast<unsigned char>(std::toupper(c));const char* found=std::strchr(alphabet,c);
 if(found){const auto& glyph=glyphs[found-alphabet];for(int col=0;col<5;++col)for(int row=0;row<7;++row)if(glyph[col]&(1u<<row))for(int yy=0;yy<scale;++yy)for(int xx=0;xx<scale;++xx)pixel(b,x+col*scale+xx,y+row*scale+yy,255,255,255,255);}
 x+=6*scale;
 }
}
Bitmap panel(unsigned w,unsigned h){Bitmap b{w,h,std::vector<uint8_t>(w*h*4)};for(int y=0;y<int(h);++y)for(int x=0;x<int(w);++x){int cx=x<8?8:x>=int(w)-8?int(w)-9:x,cy=y<8?8:y>=int(h)-8?int(h)-9:y;if((x-cx)*(x-cx)+(y-cy)*(y-cy)<=64)pixel(b,x,y,12,16,24,224);}return b;}
void button(Bitmap& b,Rect r,const char* label,bool selected){box(b,r,selected?120:48,255);box(b,{r.x+2,r.y+2,r.width-4,r.height-4},selected?46:24,255);words(b,r.x+(r.width-int(std::strlen(label))*12)/2,r.y+15,label);}
}
Bitmap overlay_bitmap(const FrontendUI& ui,double now){
 if(ui.modal()){auto b=panel(640,180);words(b,206,18,text::quit_title);words(b,86,54,text::quit_question);button(b,cancel_button(),text::cancel,!ui.exit_selected());button(b,exit_button(),text::exit_game,ui.exit_selected());return b;}
 if(!ui.banner_visible(now))return {};
 auto b=panel(640,112);words(b,16,12,text::controls_title);for(int i=0;i<4;++i)words(b,16,34+i*18,text::controls[i]);
 const float alpha=ui.banner_alpha(now);for(std::size_t i=3;i<b.rgba.size();i+=4)b.rgba[i]=static_cast<uint8_t>(b.rgba[i]*alpha);return b;
}
Bitmap prompt_bitmap(const char* title,const char* message){auto b=panel(720,180);words(b,20,20,title);words(b,20,58,message,1);return b;}
}
