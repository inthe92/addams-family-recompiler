#include "runtime/mod_panel.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
namespace nes::mods {
static std::string permissions(const Record&r){std::string s;for(const auto&v:std::vector<std::pair<unsigned,const char*>>{{1,"observe.player"},{2,"overlay.text"},{4,"memory.read"},{8,"memory.observe"},{16,"resources.text"},{32,"gameplay.jump"},{64,"graphics.layers"},{128,"ui.draw"}})if(r.permissions&v.first){if(!s.empty())s+=", ";s+=v.second;}return s.empty()?"None (v1 read-only)":s;}
void Panel::configure(){if(const auto*r=selected()){settings_=r->settings;setting_=0;button_=0;page_=Page::configure;message_=settings_.empty()?"This mod declares no configurable options.":"Changes apply only after Apply.";}}
void Panel::confirm(Command command,std::string title,std::string description){return_page_=page_;pending_=command;confirmation_title_=std::move(title);confirmation_text_=std::move(description);page_=Page::confirm;button_=0;acted_=true;}
void Panel::cancel_pending(){deleting_=false;if(page_==Page::confirm){page_=return_page_;pending_=Command::none;button_=0;}}
void Panel::move_button(int delta){do{button_=(button_+4+delta)%4;}while(((page_==Page::name||page_==Page::confirm)&&button_!=0&&button_!=3&&!(page_==Page::confirm&&button_==1))||(page_==Page::settings&&button_==2));}
void Panel::shortcut(char c){if(!visible_||acted_)return;
    if(page_==Page::profiles&&c=='o'){page_=Page::profile_operations;button_=0;return;}
    if(page_==Page::configure&&c=='r'){if(!settings_.empty())confirm(Command::configure_defaults,"RESTORE DEFAULT SETTINGS?","Unsaved edits will be discarded. Active mod may restart.");return;}
    if(page_!=Page::installed)return;if(c=='d'&&selected()){page_=Page::details;button_=detail_scroll_=0;}if(c=='c')configure();if(c=='p'){page_=Page::profiles;button_=0;}}
void Panel::text(const std::string&s){if(page_!=Page::name)return;for(char c:s)if(name_.size()<24&&((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' '||c=='-'))name_+=c;}
void Panel::adjust(int sign){if(settings_.empty())return;auto&s=settings_[setting_];if(s.type=="boolean")s.value=s.value=="true"?"false":"true";else if(s.type=="enum"){auto i=std::find(s.choices.begin(),s.choices.end(),s.value);if(!s.choices.empty())s.value=s.choices[(i-s.choices.begin()+s.choices.size()+sign)%s.choices.size()];}else{auto value=std::clamp(std::stod(s.value)+sign*s.step,s.minimum,s.maximum);std::ostringstream out;if(s.type=="integer")out<<std::llround(value);else out.precision(12),out<<value;s.value=out.str();}}
Command Panel::handle(PanelAction a,bool repeat){
    if(!visible_||repeat||acted_)return Command::none;
    if(a==PanelAction::back){if(page_==Page::confirm){cancel_pending();acted_=true;return Command::none;}if(page_==Page::profile_operations){page_=Page::profiles;button_=0;acted_=true;return Command::none;}acted_=true;deleting_=false;if(page_==Page::details||page_==Page::configure||page_==Page::profiles){page_=Page::installed;button_=0;return Command::none;}if(page_==Page::name){page_=Page::profiles;button_=0;return Command::none;}if(page_==Page::settings){page_=Page::main;button_=0;return Command::none;}close();return Command::close;}
    if(a==PanelAction::up||a==PanelAction::down){int delta=a==PanelAction::up?-1:1;deleting_=false;
        if(page_==Page::name){const std::string alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -";if(name_.empty())name_="A";else{auto pos=alphabet.find(name_.back());if(pos==std::string::npos)pos=0;name_.back()=alphabet[(pos+alphabet.size()+delta)%alphabet.size()];}}
        else if(page_==Page::main||page_==Page::settings||page_==Page::confirm)move_button(delta);
        else if(page_==Page::details)detail_scroll_=unsigned(std::max(0,int(detail_scroll_)+delta));
        else if(page_==Page::profiles&&!profiles_.empty())profile_=(profile_+profiles_.size()+delta)%profiles_.size();
        else if(page_==Page::configure&&!settings_.empty())setting_=(setting_+settings_.size()+delta)%settings_.size();
        else if(page_==Page::installed&&!rows_.empty())selected_=(selected_+rows_.size()+delta)%rows_.size();return Command::none;}
    if(a==PanelAction::left||a==PanelAction::right){move_button(a==PanelAction::left?-1:1);deleting_=false;return Command::none;}
    acted_=true;
    if(page_==Page::confirm){auto command=pending_;auto old=return_page_;if(button_==1){page_=old;pending_=Command::none;button_=0;return command;}cancel_pending();return Command::none;}
    if(page_==Page::profile_operations){if(button_==3){page_=Page::profiles;button_=0;return Command::none;}if(button_==2)return Command::profile_pick;if(!selected_profile()){message_="Select a profile first.";return Command::none;}if(button_==1)return Command::profile_export;if(selected_profile()->name=="Original"){message_="Original cannot be deleted.";return Command::none;}confirm(Command::profile_delete,"DELETE PROFILE?",selected_profile()->name+" - Installed mods and other profiles will remain.");return Command::none;}
    if(page_==Page::main){if(button_==0){close();return Command::play;}if(button_==1){open();acted_=true;return Command::none;}if(button_==2){page_=Page::settings;button_=0;return Command::none;}close();return Command::close;}
    if(page_==Page::settings){if(button_==0)fullscreen=!fullscreen;else if(button_==1)muted=!muted;else if(button_==3){page_=Page::main;button_=0;}return Command::none;}
    if(page_==Page::name){if(button_==3){page_=Page::profiles;button_=0;return Command::none;}if(button_!=0)return Command::none;if(name_.empty()){message_="Enter a profile name.";return Command::none;}page_=Page::profiles;button_=0;if(!renaming_&&std::any_of(profiles_.begin(),profiles_.end(),[&](const auto&p){return p.name==name_;})){confirm(Command::profile_save,"REPLACE SAVED PROFILE?",name_+" - Current mod selections and settings will replace it.");return Command::none;}return renaming_?Command::profile_rename:Command::profile_save;}
    if(page_==Page::configure){if(button_<2){adjust(button_==0?-1:1);return Command::none;}if(button_==2)return settings_.empty()?Command::none:Command::configure;page_=Page::installed;button_=0;return Command::none;}
    if(page_==Page::profiles){if(button_==3){page_=Page::installed;button_=0;return Command::none;}if(button_==0){if(!selected_profile())return Command::none;if(!deleting_){deleting_=true;message_="Review permissions in Details. Apply again to confirm.";return Command::none;}deleting_=false;return Command::profile_apply;}
        if(button_==2&&(!selected_profile()||selected_profile()->name=="Original")){message_="Original is built-in and cannot be renamed.";return Command::none;}
        renaming_=button_==2;old_name_=renaming_?selected_profile()->name:"";name_=renaming_?old_name_:"Profile "+std::to_string(profiles_.size());page_=Page::name;button_=0;return Command::none;}
    if(page_==Page::details){if(button_==3){page_=Page::installed;button_=0;return Command::none;}if(button_==1){configure();return Command::none;}}
    else {if(button_==0)return Command::install;if(button_==3){close();return Command::close;}}
    if(!selected()){message_="No mod selected.";return Command::none;}
    if((page_==Page::details&&button_==0)||(page_==Page::installed&&button_==1)){deleting_=false;return Command::toggle;}
    if(!deleting_){deleting_=true;message_="Select Uninstall again to confirm.";return Command::none;}deleting_=false;return Command::uninstall;
}
Command Panel::click(int x,int y){
    if(page_==Page::installed){for(unsigned i=0;i<3;++i)if(nes::ui::Rect{14+int(i)*208,330,198,30}.contains(x,y)){shortcut("dcp"[i]);return Command::none;}}
    if(page_==Page::profiles&&nes::ui::Rect{14,330,612,30}.contains(x,y)){shortcut('o');return Command::none;}
    if(page_==Page::configure&&nes::ui::Rect{14,330,612,30}.contains(x,y)){shortcut('r');return Command::none;}
    for(unsigned i=0;i<4;++i)if(button(i).contains(x,y)){if(button_!=i)deleting_=false;button_=i;return handle(PanelAction::activate);}
    const unsigned selected=page_==Page::profiles?profile_:page_==Page::configure?setting_:selected_,first=selected>=6?selected-5:0;
    for(unsigned i=0;i<6;++i)if(nes::ui::Rect{14,52+int(i)*28,612,26}.contains(x,y)){unsigned k=first+i;if(page_==Page::installed&&k<rows_.size())selected_=k;if(page_==Page::profiles&&k<profiles_.size())profile_=k;if(page_==Page::configure&&k<settings_.size())setting_=k;deleting_=false;}return Command::none;
}
nes::ui::Rect Panel::rect(int width,int height){if(width<=0||height<=0)return {};double scale=std::min(width*0.94/640,height*0.9/430);int w=int(640*scale),h=int(430*scale);return {(width-w)/2,(height-h)/2,w,h};}
nes::ui::Bitmap Panel::bitmap()const {
    if(!visible_)return {};std::vector<std::string> lines,labels;
    if(page_==Page::confirm){lines={confirmation_title_,confirmation_text_.substr(0,48),confirmation_text_.substr(std::min<size_t>(48,confirmation_text_.size()),48),"Cancel is selected by default.","Choose Confirm explicitly to continue."};labels={"Cancel","Confirm"," ","Back"};}
    else if(page_==Page::profile_operations){lines={"PROFILE OPERATIONS",selected_profile()?selected_profile()->name:"No profile selected","Delete removes only the profile.","Export contains IDs and typed settings only.","Import validates files, without running scripts.","Imported profiles stay inactive until Apply.",message_};labels={"Delete","Export","Import","Back"};}
    else if(page_==Page::main){lines={"NES NATIVE LAB","PLAY - ORIGINAL PAL 4:3","MODS - INSTALL AND MANAGE","SETTINGS - LAUNCH OPTIONS","EXIT","Use arrows, Enter or gamepad A.","EXPERIMENTAL MOD MANAGER COMPLETION02"};labels={"Play","Mods","Settings","Exit"};}
    else if(page_==Page::settings){lines={"SETTINGS","Fullscreen at launch: "+std::string(fullscreen?"On":"Off"),"Mute at launch: "+std::string(muted?"On":"Off"),"F11 toggles fullscreen during play.","F2 opens Mods. Escape confirms exit.","Original PAL timing and ABI3 are unchanged.","Launch options apply to this session."};labels={"Fullscreen","Mute"," ","Back"};}
    else if(page_==Page::name){lines={renaming_?"RENAME PROFILE":"SAVE NEW PROFILE",name_,"Type a name. Backspace deletes.","Pad X deletes, Y adds; up/down changes last letter.","Saves selected mods and their settings.",message_};labels={"Save"," "," ","Cancel"};}
    else if(page_==Page::profiles){lines={"MOD PROFILES"};unsigned first=profile_>=6?profile_-5:0;for(unsigned i=0;i<6;++i)lines.push_back(first+i<profiles_.size()?std::string(first+i==profile_?"> ":"  ")+profiles_[first+i].name+(profiles_[first+i].active?" (Active)":""):"");if(const auto*p=selected_profile())lines.push_back(p->summary);lines.push_back("Original keeps packages; forced safe mode differs.");lines.push_back(message_);labels={deleting_?"Confirm":"Apply","Save New","Rename","Back"};}
    else if(page_==Page::configure){lines={"CONFIGURE MOD"};unsigned first=setting_>=6?setting_-5:0;for(unsigned i=0;i<6;++i)lines.push_back(first+i<settings_.size()?std::string(first+i==setting_?"> ":"  ")+settings_[first+i].label+": "+settings_[first+i].value:"");lines.push_back("Up/down selects an option. Apply saves.");lines.push_back(message_);labels={"Decrease","Increase","Apply","Back"};}
    else if(page_==Page::details&&selected()){const auto&r=*selected();std::vector<std::string> body={r.name+" "+r.version,"Author: "+r.author,"Category: "+r.category,"Status: "+std::string(r.enabled?"Enabled":"Disabled"),"API "+std::to_string(r.api_version),"Permissions: "+permissions(r),"Dependencies: "+r.dependencies,"Conflicts: "+r.conflicts,"Description: "+r.description,"Error: "+(r.error.empty()?"None":r.error),message_};std::vector<std::string> wrapped;for(auto&line:body)for(size_t i=0;i<line.size();i+=48)wrapped.push_back(line.substr(i,48));lines={"MOD DETAILS - UP/DOWN TO SCROLL"};unsigned start=std::min<unsigned>(detail_scroll_,wrapped.size()>10?wrapped.size()-10:0);for(unsigned i=start;i<wrapped.size()&&i<start+10;++i)lines.push_back(wrapped[i]);labels={r.enabled?"Disable":"Enable","Configure",deleting_?"Confirm":"Uninstall","Back"};}
    else {lines={"INSTALLED MODS - API 1/2"};unsigned first=selected_>=6?selected_-5:0;for(unsigned i=0;i<6;++i){if(first+i<rows_.size()){const auto&r=rows_[first+i];lines.push_back(std::string(first+i==selected_?"> ":"  ")+r.name.substr(0,25)+" "+r.version+" "+(r.enabled?"Enabled":"Disabled"));}else lines.emplace_back();}if(const auto*r=selected()){lines.push_back("Author: "+r->author);lines.push_back("Category: "+r->category);lines.push_back(r->error.empty()?message_:r->error);}else{lines.push_back("Install ZIP or drop a ZIP into the window.");lines.push_back(message_);}labels={"Install ZIP",selected()&&selected()->enabled?"Disable":"Enable",deleting_?"Confirm":"Uninstall","Back"};}
    auto b=nes::ui::menu_bitmap(lines,labels,button_);
    if(page_==Page::installed){nes::ui::paint_text(b,24,342,"D: DETAILS");nes::ui::paint_text(b,232,342,"C: CONFIGURE");nes::ui::paint_text(b,440,342,"P: PROFILES");}
    if(page_==Page::profiles)nes::ui::paint_text(b,24,342,"O / PAD RB: DELETE - EXPORT - IMPORT");
    if(page_==Page::configure)nes::ui::paint_text(b,24,342,"R / PAD X: RESTORE DEFAULTS");
    return b;
}
}
