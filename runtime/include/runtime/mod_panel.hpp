#pragma once
#include "runtime/frontend_ui.hpp"
#include "runtime/mod_manager.hpp"
namespace nes::mods {
enum class PanelAction { up,down,left,right,activate,back };
enum class Command { none,install,toggle,uninstall,close,play,configure,profile_apply,profile_save,profile_rename,profile_delete,profile_export,profile_pick,profile_import,configure_defaults };
enum class Page { installed,details,configure,profiles,name,main,settings,profile_operations,confirm };
class Panel {
public:
    void open(){visible_=true;if(page_!=Page::installed)button_=0;page_=Page::installed;deleting_=false;if(message_.empty())message_="D: Details  C: Configure  P: Profiles";}
    void main(){visible_=true;page_=Page::main;button_=0;}
    void close(){visible_=false;deleting_=false;}
    bool visible()const{return visible_;}
    Page page()const{return page_;}
    void begin_events(){acted_=false;}
    Command handle(PanelAction,bool repeat=false);
    Command click(int x,int y);
    void shortcut(char);
    void confirm(Command action,std::string title,std::string description);
    void cancel_pending();
    bool awaiting(Command c)const{return page_==Page::confirm&&pending_==c;}
    void configuration_saved(){if(const auto*r=selected())settings_=r->settings;}
    void text(const std::string&);
    void erase_text(){if(page_==Page::name&&!name_.empty())name_.pop_back();}
    void records(std::vector<Record> rows){rows_=std::move(rows);if(selected_>=rows_.size())selected_=0;}
    void profiles(std::vector<Profile> rows){profiles_=std::move(rows);if(profile_>=profiles_.size())profile_=0;}
    const Record* selected()const{return rows_.empty()?nullptr:&rows_[selected_];}
    const Profile* selected_profile()const{return profiles_.empty()?nullptr:&profiles_[profile_];}
    const std::vector<Setting>& settings()const{return settings_;}
    const std::string& name()const{return name_;}
    const std::string& old_name()const{return old_name_;}
    bool fullscreen{},muted{};
    void message(std::string text){message_=std::move(text);}
    nes::ui::Bitmap bitmap()const;
    static nes::ui::Rect rect(int width,int height);
    static nes::ui::Rect button(unsigned index){return {14+int(index)*156,370,144,42};}
private:
    void move_button(int);
    Page return_page_=Page::installed;Command pending_=Command::none;std::string confirmation_title_,confirmation_text_;
    Page page_=Page::installed;bool visible_{},acted_{},deleting_{},renaming_{};
    unsigned selected_{},button_{},setting_{},profile_{},detail_scroll_{};
    std::string message_,name_,old_name_;
    std::vector<Record> rows_;std::vector<Profile> profiles_;std::vector<Setting> settings_;
    void adjust(int);
    void configure();
};
}
