#include "runtime/mod_panel.hpp"
#include <algorithm>
namespace nes::mods {
Command Panel::handle(PanelAction a,bool repeat){
    if(!visible_||repeat||acted_)return Command::none;
    if(a==PanelAction::back){close();acted_=true;return Command::close;}
    if(a==PanelAction::up||a==PanelAction::down){deleting_=false;if(!rows_.empty())selected_=a==PanelAction::up?(selected_+rows_.size()-1)%rows_.size():(selected_+1)%rows_.size();return Command::none;}
    if(a==PanelAction::left||a==PanelAction::right){button_=(button_+(a==PanelAction::left?3:1))%4;deleting_=false;return Command::none;}
    acted_=true;
    if(button_==0)return Command::install;
    if(button_==3){close();return Command::close;}
    if(!selected()){message_="No mod selected.";return Command::none;}
    if(button_==1){deleting_=false;return Command::toggle;}
    if(!deleting_){deleting_=true;message_="Select Uninstall again to confirm.";return Command::none;}
    deleting_=false;return Command::uninstall;
}
Command Panel::click(int x,int y){
    for(unsigned i=0;i<4;++i)if(button(i).contains(x,y)){if(button_!=i)deleting_=false;button_=i;return handle(PanelAction::activate);}
    const unsigned first=selected_>=6?selected_-5:0;
    for(unsigned i=0;i<6&&first+i<rows_.size();++i)if(nes::ui::Rect{14,52+int(i)*28,612,26}.contains(x,y)){selected_=first+i;deleting_=false;}
    return Command::none;
}
nes::ui::Rect Panel::rect(int width,int height){if(width<=0||height<=0)return {};double scale=std::min(width*0.94/640,height*0.9/430);int w=int(640*scale),h=int(430*scale);return {(width-w)/2,(height-h)/2,w,h};}
nes::ui::Bitmap Panel::bitmap()const {
    if(!visible_)return {};
    std::vector<std::string> lines{"MODS - API 1"};const unsigned first=selected_>=6?selected_-5:0;
    for(unsigned i=0;i<6;++i){if(first+i<rows_.size()){const auto&r=rows_[first+i];lines.push_back(std::string(first+i==selected_?"> ":"  ")+r.name.substr(0,25)+" "+r.version+" "+(r.enabled?"Enabled":"Disabled"));}else lines.emplace_back();}
    if(const auto*r=selected()){lines.push_back("Author: "+r->author);lines.push_back(r->description.substr(0,50));lines.push_back(r->description.substr(std::min<size_t>(50,r->description.size()),50));lines.push_back(r->error.empty()?message_:r->error);}
    else {lines.push_back("Install a ZIP to add a mod.");lines.push_back(message_);lines.emplace_back();lines.emplace_back();}
    std::vector<std::string> labels{"Install Mod",selected()&&selected()->enabled?"Disable":"Enable",deleting_?"Confirm":"Uninstall","Back"};
    return nes::ui::menu_bitmap(lines,labels,button_);
}
}
