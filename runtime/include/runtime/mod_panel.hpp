#pragma once
#include "runtime/frontend_ui.hpp"
#include "runtime/mod_manager.hpp"
namespace nes::mods {
enum class PanelAction { up,down,left,right,activate,back };
enum class Command { none,install,toggle,uninstall,close };
class Panel {
public:
    void open(){visible_=true;if(message_.empty())message_="Use arrows and Enter, or drop a mod ZIP.";deleting_=false;}
    void close(){visible_=false;deleting_=false;}
    bool visible()const{return visible_;}
    void begin_events(){acted_=false;}
    Command handle(PanelAction,bool repeat=false);
    Command click(int x,int y);
    void records(std::vector<Record> rows){rows_=std::move(rows);if(selected_>=rows_.size())selected_=0;}
    const Record* selected()const{return rows_.empty()?nullptr:&rows_[selected_];}
    void message(std::string text){message_=std::move(text);}
    nes::ui::Bitmap bitmap()const;
    static nes::ui::Rect rect(int width,int height);
    static nes::ui::Rect button(unsigned index){return {14+int(index)*156,370,144,42};}
private:
    bool visible_{},acted_{},deleting_{};
    unsigned selected_{},button_{};
    std::string message_;
    std::vector<Record> rows_;
};
}
