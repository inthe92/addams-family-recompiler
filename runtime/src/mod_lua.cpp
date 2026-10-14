#include "runtime/mod_lua.hpp"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <string_view>
#include <stdexcept>
namespace nes::mods {
void* LuaMod::allocate(void* p,void* old,std::size_t before,std::size_t after) {
    auto& s=*static_cast<LuaMod*>(p);if(!old)before=0;
    if(!after){std::free(old);s.used_-=before;return nullptr;}
    if(after>before && after-before>s.limit_-s.used_)return nullptr;
    void* result=std::realloc(old,after);if(result)s.used_=s.used_-before+after;return result;
}
LuaMod& LuaMod::self(lua_State* L){return **static_cast<LuaMod**>(lua_getextraspace(L));}
void LuaMod::hook(lua_State* L,lua_Debug*) {
    auto& s=self(L);s.budget_-=1000;
    if(s.budget_<=0 || std::chrono::steady_clock::now()>s.deadline_)luaL_error(L,"Mod execution budget exceeded");
}
int LuaMod::initialize(lua_State* L) {
    luaL_requiref(L,"_G",luaopen_base,1);lua_pop(L,1);
    for(const char* name:{"dofile","loadfile","load","collectgarbage","pcall","xpcall","getmetatable","setmetatable","rawset","rawget","rawequal","print","warn"}){
        lua_pushnil(L);lua_setglobal(L,name);
    }
    // No io/os/package/debug/coroutine library is opened. Pattern matching and
    // table.sort are omitted: their C work is not interrupted by VM count hooks.
    luaL_requiref(L,"math",luaopen_math,1);lua_pop(L,1);
    lua_getglobal(L,"math");for(const char* n:{"random","randomseed"}){lua_pushnil(L);lua_setfield(L,-2,n);}lua_pop(L,1);
    luaL_requiref(L,"string",luaopen_string,1);lua_pop(L,1);
    lua_getglobal(L,"string");for(const char* n:{"find","match","gmatch","gsub","dump","rep","format","pack","unpack","packsize"}){lua_pushnil(L);lua_setfield(L,-2,n);}lua_pop(L,1);
    luaL_requiref(L,"table",luaopen_table,1);lua_pop(L,1);lua_getglobal(L,"table");lua_pushnil(L);lua_setfield(L,-2,"sort");lua_pop(L,1);
    lua_newtable(L);lua_pushcfunction(L,info);lua_setfield(L,-2,"info");lua_setglobal(L,"log");
    lua_newtable(L);lua_pushcfunction(L,status);lua_setfield(L,-2,"get_status");if(self(L).api_version_==1||(self(L).permissions_&observe_player)){lua_pushcfunction(L,player);lua_setfield(L,-2,"get_player");}if(self(L).permissions_&gameplay_jump){lua_newtable(L);lua_pushcfunction(L,set_modifier);lua_setfield(L,-2,"set_modifier");lua_setfield(L,-2,"player");}if(self(L).api_version_==2){lua_pushcfunction(L,clock_state);lua_setfield(L,-2,"get_clocks");lua_pushcfunction(L,input_state);lua_setfield(L,-2,"get_input");}lua_setglobal(L,"game");
    if(self(L).api_version_==2&&(self(L).permissions_&overlay_text)){lua_newtable(L);lua_pushcfunction(L,hud_text);lua_setfield(L,-2,"text");lua_setglobal(L,"hud");}
    if(self(L).permissions_&ui_draw){lua_newtable(L);lua_pushcfunction(L,ui_text);lua_setfield(L,-2,"text");lua_pushcfunction(L,ui_rect);lua_setfield(L,-2,"rect");lua_setglobal(L,"ui");}
    if(self(L).permissions_&graphics_layers){lua_newtable(L);lua_pushcfunction(L,graphic_layer);lua_setfield(L,-2,"layer");lua_setglobal(L,"graphics");}
    if(self(L).api_version_==2&&(self(L).permissions_&memory_read)){
        lua_newtable(L);lua_newtable(L);lua_pushcfunction(L,memory_u8);lua_setfield(L,-2,"read_u8");lua_pushcfunction(L,memory_range);lua_setfield(L,-2,"read_range");
        if(self(L).permissions_&memory_observe){lua_pushcfunction(L,memory_watch);lua_setfield(L,-2,"observe");}lua_setfield(L,-2,"memory");lua_setglobal(L,"nes");
    }
    if(self(L).api_version_==2){lua_newtable(L);lua_pushcfunction(L,config_get);lua_setfield(L,-2,"get");lua_setglobal(L,"config");}
    lua_pushinteger(L,self(L).api_version_);lua_setglobal(L,"MOD_API_VERSION");return 0;
}
LuaMod::LuaMod(const std::string& source,std::size_t limit,unsigned api,uint8_t permissions,std::vector<ImageAsset> assets,std::vector<Setting> settings):settings_(std::move(settings)),api_version_(api),permissions_(permissions),assets_(std::move(assets)),limit_(limit) {
    if((api!=1&&api!=2)||(permissions&~239u)||(api==1&&permissions)||((permissions&memory_observe)&&!(permissions&memory_read)))throw std::runtime_error("Unsupported mod API/permissions");
    if(source.size()>128*1024 || limit<65536)throw std::runtime_error("Mod source or memory limit invalid");
    if(settings_.size()>8||(api==1&&!settings_.empty()))throw std::runtime_error("Unsupported settings contract");
    for(unsigned i=0;i<settings_.size();++i){const auto&s=settings_[i];
        if(s.key.empty()||s.key.size()>32||s.label.size()>32||s.value.size()>32)throw std::runtime_error("Invalid setting bounds");
        for(unsigned j=0;j<i;++j)if(settings_[j].key==s.key)throw std::runtime_error("Duplicate setting");
        if(s.type=="boolean"){if(s.value!="true"&&s.value!="false")throw std::runtime_error("Invalid boolean setting");}
        else if(s.type=="enum"){if(s.choices.empty()||s.choices.size()>8||std::find(s.choices.begin(),s.choices.end(),s.value)==s.choices.end())throw std::runtime_error("Invalid enum setting");}
        else if(s.type=="integer"||s.type=="float"){char*end=nullptr;double v=std::strtod(s.value.c_str(),&end);if(!end||end==s.value.c_str()||*end||!std::isfinite(v)||!std::isfinite(s.minimum)||!std::isfinite(s.maximum)||!std::isfinite(s.step)||s.minimum>s.maximum||s.step<=0||v<s.minimum||v>s.maximum||(s.type=="integer"&&v!=std::floor(v)))throw std::runtime_error("Invalid numeric setting");}
        else throw std::runtime_error("Unknown setting type");
    }
    if(assets_.size()>2||(!assets_.empty()&&!(permissions&graphics_layers)))throw std::runtime_error("Image quota/permission invalid");
    for(unsigned i=0;i<assets_.size();++i){auto&a=assets_[i];if(a.id.empty()||a.id.size()>64||!a.bitmap.width||!a.bitmap.height||a.bitmap.width>128||a.bitmap.height>128||a.bitmap.rgba.size()!=a.bitmap.width*a.bitmap.height*4)throw std::runtime_error("Invalid owned image");for(unsigned j=0;j<i;++j)if(assets_[j].id==a.id)throw std::runtime_error("Duplicate image ID");}
    pending_visual_.reserve(visual_quota);visual_.reserve(visual_quota);
    state_=lua_newstate(allocate,this,0);if(!state_)throw std::runtime_error("Cannot allocate mod state");
    *static_cast<LuaMod**>(lua_getextraspace(state_))=this;
    try {
        lua_pushcfunction(state_,initialize);checked(lua_pcall(state_,0,0,0));
        checked(luaL_loadbufferx(state_,source.data(),source.size(),"mod-main","t"));
        budget_=100000;deadline_=std::chrono::steady_clock::now()+std::chrono::milliseconds(20);
        lua_sethook(state_,hook,LUA_MASKCOUNT,1000);checked(lua_pcall(state_,0,0,0));
    }catch(...){lua_close(state_);state_=nullptr;throw;}
}
LuaMod::~LuaMod(){if(state_)lua_close(state_);}
void LuaMod::checked(int code) {
    if(code==LUA_OK)return;
    const char* p=lua_type(state_,-1)==LUA_TSTRING?lua_tostring(state_,-1):nullptr;std::string message=p?std::string(p).substr(0,300):"Lua allocation or non-string error";
    lua_settop(state_,0);throw std::runtime_error(message);
}
int LuaMod::config_get(lua_State*L){
    if(lua_gettop(L)!=1||lua_type(L,1)!=LUA_TSTRING)return luaL_error(L,"config.get requires a declared setting ID");
    size_t length=0;const char*name=lua_tolstring(L,1,&length);const auto key=std::string_view(name,length);
    for(const auto&s:self(L).settings_)if(s.key==key){
        if(s.type=="boolean")lua_pushboolean(L,s.value=="true");
        else if(s.type=="enum")lua_pushlstring(L,s.value.data(),s.value.size());
        else if(s.type=="integer")lua_pushinteger(L,static_cast<lua_Integer>(std::strtod(s.value.c_str(),nullptr)));
        else lua_pushnumber(L,std::strtod(s.value.c_str(),nullptr));return 1;
    }
    return luaL_error(L,"Unknown mod setting");
}
int LuaMod::call(lua_State* L) {
    auto& s=self(L);if(s.api_version_==2&&std::string_view(s.callback_)=="on_update")notify_watches(L);lua_getglobal(L,s.callback_);
    if(lua_isnil(L,-1)){lua_pop(L,1);return 0;}
    if(!lua_isfunction(L,-1))return luaL_error(L,"Mod callback must be a function");
    const bool update=std::string_view(s.callback_)=="on_update";
    if(update)lua_pushnumber(L,s.api_version_==1?1.0/50.007:s.observation_.step_seconds);
    lua_call(L,update?1:0,0);return 0;
}
void LuaMod::invoke(const char* name) {
    registering_=api_version_==2&&std::string_view(name)=="on_load";callback_=name;pending_overlay_.clear();pending_visual_.clear();pending_pixels_=0;drawing_=api_version_==2&&std::string_view(name)!="on_unload";budget_=100000;deadline_=std::chrono::steady_clock::now()+std::chrono::milliseconds(20);
    try{lua_pushcfunction(state_,call);checked(lua_pcall(state_,0,0,0));}
    catch(...){registering_=false;clear_watches();drawing_=false;pending_overlay_.clear();overlay_.clear();visual_.clear();pending_visual_.clear();jump_extension_=0;throw;}
    registering_=false;drawing_=false;if(api_version_==2){overlay_=std::move(pending_overlay_);visual_.swap(pending_visual_);}
}
void LuaMod::load(const Observation& o){observation_=o;invoke("on_load");}
void LuaMod::update(const Observation& o){if(api_version_==2&&(!std::isfinite(o.step_seconds)||o.step_seconds<=0||o.step_seconds>1))throw std::runtime_error("Invalid host simulation interval");observation_=o;invoke("on_update");++updates_;}
void LuaMod::unload(){jump_extension_=0;clear_watches();overlay_.clear();try{invoke("on_unload");}catch(...){jump_extension_=0;throw;}jump_extension_=0;overlay_.clear();visual_.clear();pending_visual_.clear();}
int LuaMod::info(lua_State* L) {
    auto& s=self(L);size_t length=0;
    if(lua_type(L,1)!=LUA_TSTRING)return luaL_error(L,"log.info requires a string");
    const char* text=lua_tolstring(L,1,&length);
    if(length>300)return luaL_error(L,"Mod log message too long");
    try {
        std::string line(text,length);for(auto& c:line)if(static_cast<unsigned char>(c)<32||static_cast<unsigned char>(c)>126)c='?';
        if(s.logs_.size()==128)s.logs_.pop_front();s.logs_.push_back(std::move(line));
    }catch(...){return luaL_error(L,"Cannot store mod log");}
    return 0;
}
int LuaMod::status(lua_State* L) {
    const auto& o=self(L).observation_;lua_newtable(L);
    lua_pushinteger(L,static_cast<lua_Integer>(o.frame));lua_setfield(L,-2,"frame");
    lua_pushinteger(L,static_cast<lua_Integer>(o.cpu_cycles));lua_setfield(L,-2,"cpu_cycles");
    lua_pushnumber(L,self(L).api_version_==1?50.007:1.0/o.step_seconds);lua_setfield(L,-2,"simulation_hz");
    if(self(L).api_version_==1){if(o.room){lua_pushinteger(L,*o.room);lua_setfield(L,-2,"room_id");}}
    else {const auto* g=o.game?&*o.game:nullptr;lua_pushstring(L,g?g->state.c_str():"unavailable");lua_setfield(L,-2,"state");
        if(g){lua_pushstring(L,g->game_id.c_str());lua_setfield(L,-2,"game_id");if(g->room){lua_pushinteger(L,*g->room);lua_setfield(L,-2,"room_id");}}}
    return 1;
}
int LuaMod::player(lua_State* L) {
    auto& s=self(L);
    if(s.api_version_==2){const auto& g=s.observation_.game;if(!g||!g->player){lua_pushnil(L);return 1;}
        lua_newtable(L);lua_pushinteger(L,g->player->x);lua_setfield(L,-2,"x");lua_pushinteger(L,g->player->y);lua_setfield(L,-2,"y");
        lua_pushliteral(L,"nes_screen");lua_setfield(L,-2,"space");lua_pushliteral(L,"player_reference");lua_setfield(L,-2,"coordinate_kind");return 1;}
    const auto& p=s.observation_.player;if(!p){lua_pushnil(L);return 1;}
    lua_newtable(L);lua_pushinteger(L,p->x);lua_setfield(L,-2,"x");lua_pushinteger(L,p->y);lua_setfield(L,-2,"y");return 1;
}
int LuaMod::set_modifier(lua_State* L){auto& s=self(L);
    if(!(s.permissions_&gameplay_jump)||!s.drawing_||lua_gettop(L)!=2||lua_type(L,1)!=LUA_TSTRING||!lua_isinteger(L,2))return luaL_error(L,"set_modifier requires an update/load callback, name and integer");
    size_t n{};const char* name=lua_tolstring(L,1,&n);auto value=lua_tointeger(L,2);
    if(std::string_view(name,n)!="jump_extension"||value<0||value>12)return luaL_error(L,"Supported modifier: jump_extension, integer 0..12");s.jump_extension_=unsigned(value);return 0;
}
int LuaMod::clock_state(lua_State* L){const auto&o=self(L).observation_;lua_newtable(L);lua_pushnumber(L,o.real_seconds);lua_setfield(L,-2,"real_seconds");lua_pushnumber(L,o.simulation_seconds);lua_setfield(L,-2,"simulation_seconds");lua_pushnumber(L,o.paused_seconds);lua_setfield(L,-2,"paused_seconds");return 1;}

int LuaMod::input_state(lua_State* L){const auto&o=self(L).observation_;lua_newtable(L);lua_pushinteger(L,o.buttons);lua_setfield(L,-2,"buttons");lua_pushboolean(L,o.reset_timer);lua_setfield(L,-2,"reset_timer");lua_pushboolean(L,o.toggle_ui);lua_setfield(L,-2,"toggle_ui");lua_pushboolean(L,o.toggle_layers);lua_setfield(L,-2,"toggle_layers");return 1;}
static unsigned visual_space(lua_State*L,int i){if(lua_type(L,i)!=LUA_TSTRING)luaL_error(L,"Expected viewport or window space");size_t n;const char*s=lua_tolstring(L,i,&n);if(std::string_view(s,n)=="viewport")return 0;if(std::string_view(s,n)=="window")return 1;luaL_error(L,"Expected viewport or window space");return 0;}
static int bounded(lua_State*L,int i,int lo,int hi){if(!lua_isinteger(L,i))luaL_error(L,"Expected bounded integer");auto v=lua_tointeger(L,i);if(v<lo||v>hi)luaL_error(L,"Visual value outside bounds");return int(v);}
int LuaMod::ui_text(lua_State*L){
 auto&s=self(L);if(!s.drawing_||lua_gettop(L)!=4||lua_type(L,4)!=LUA_TSTRING||s.pending_visual_.size()>=visual_quota)return luaL_error(L,"ui.text requires callback, space, x, y, text and quota");
 const auto space=visual_space(L,1);const int x=bounded(L,2,0,320),y=bounded(L,3,0,240);size_t n{};const char*t=lua_tolstring(L,4,&n);
 if(!valid_visual_text(x,y,std::string_view(t,n)))return luaL_error(L,"UI text does not fit or contains unsupported characters");
 if(s.pending_pixels_+n*42>100000)return luaL_error(L,"Visual pixel quota exceeded");s.pending_pixels_+=unsigned(n*42);
 bool failed=false;try{VisualCommand c;c.kind=VisualCommand::text;c.space=space;c.x=x;c.y=y;c.value.assign(t,n);s.pending_visual_.push_back(std::move(c));}catch(...){failed=true;}if(failed)return luaL_error(L,"Cannot store UI text");return 0;
}
int LuaMod::ui_rect(lua_State*L){
 auto&s=self(L);if(!s.drawing_||lua_gettop(L)!=9||s.pending_visual_.size()>=visual_quota)return luaL_error(L,"ui.rect requires callback, space, x,y,w,h,r,g,b,a and quota");
 const auto space=visual_space(L,1);const int x=bounded(L,2,0,319),y=bounded(L,3,0,239),w=bounded(L,4,1,320-x),h=bounded(L,5,1,240-y);std::array<uint8_t,4> rgba{};for(unsigned i=0;i<4;++i)rgba[i]=uint8_t(bounded(L,6+i,0,255));
 if(s.pending_pixels_+w*h>100000)return luaL_error(L,"Visual pixel quota exceeded");s.pending_pixels_+=unsigned(w*h);
 bool failed=false;try{VisualCommand c;c.kind=VisualCommand::rectangle;c.space=space;c.x=x;c.y=y;c.width=w;c.height=h;c.rgba=rgba;s.pending_visual_.push_back(std::move(c));}catch(...){failed=true;}if(failed)return luaL_error(L,"Cannot store rectangle");return 0;
}
int LuaMod::graphic_layer(lua_State*L){
 auto&s=self(L);if(!s.drawing_||(lua_gettop(L)!=11&&lua_gettop(L)!=12)||lua_type(L,2)!=LUA_TSTRING||s.pending_visual_.size()>=visual_quota)return luaL_error(L,"graphics.layer requires callback and eleven arguments");
 const auto space=visual_space(L,1);size_t n{};const char*id=lua_tolstring(L,2,&n);const std::string_view name(id,n);
 if(n>64||std::none_of(s.assets_.begin(),s.assets_.end(),[&](const auto&a){return a.id==name;}))return luaL_error(L,"Unknown declared image");
 for(const auto&c:s.pending_visual_)if(c.kind==VisualCommand::image&&c.value==name)return luaL_error(L,"Duplicate layer/image ID in callback");
 const int x=bounded(L,3,-320,320),y=bounded(L,4,-240,240),w=bounded(L,5,1,320),h=bounded(L,6,1,240),z=bounded(L,7,-16,16),alpha=bounded(L,8,0,255);
 for(int i=9;i<=lua_gettop(L);++i)if(lua_type(L,i)!=LUA_TNUMBER||!std::isfinite(lua_tonumber(L,i)))return luaL_error(L,"Camera/parallax must be finite numbers");
 const double cx=lua_tonumber(L,9),cy=lua_tonumber(L,10),factor=lua_tonumber(L,11),fy=lua_gettop(L)==12?lua_tonumber(L,12):factor;if(std::abs(cx)>4096||std::abs(cy)>4096||factor<0||factor>1||fy<0||fy>1)return luaL_error(L,"Camera/parallax exceeds bounds");
 if(s.pending_pixels_+w*h>100000)return luaL_error(L,"Visual pixel quota exceeded");s.pending_pixels_+=unsigned(w*h);
 bool failed=false;try{VisualCommand c;c.kind=VisualCommand::image;c.space=space;c.x=x-int(cx*factor);c.y=y-int(cy*fy);c.width=w;c.height=h;c.z=z;c.rgba[3]=uint8_t(alpha);c.value.assign(id,n);s.pending_visual_.push_back(std::move(c));}catch(...){failed=true;}if(failed)return luaL_error(L,"Cannot store layer");return 0;
}

int LuaMod::hud_text(lua_State* L){
    auto& s=self(L);
    if(!s.drawing_||lua_gettop(L)!=3||!lua_isinteger(L,1)||!lua_isinteger(L,2)||lua_type(L,3)!=LUA_TSTRING)
        return luaL_error(L,"hud.text requires callback context, two integers and text");
    const auto x=lua_tointeger(L,1),y=lua_tointeger(L,2);std::size_t length{};const char* text=lua_tolstring(L,3,&length);
    if(x<0||y<0||x>hud_width||y>hud_height||!valid_hud_text(int(x),int(y),std::string_view(text,length)))
        return luaL_error(L,"HUD text exceeds region/font limits");
    if(s.pending_overlay_.size()>=hud_lines_per_mod)return luaL_error(L,"HUD primitive quota exceeded");
    try{s.pending_overlay_.push_back({int(x),int(y),std::string(text,length)});}catch(...){return luaL_error(L,"Cannot store HUD text");}return 0;
}

void LuaMod::clear_watches() noexcept {for(const auto& w:watches_)luaL_unref(state_,LUA_REGISTRYINDEX,w.callback);watches_.clear();watch_bytes_=0;}
int LuaMod::memory_u8(lua_State* L){
    if(lua_gettop(L)!=1||!lua_isinteger(L,1)||!safe_memory_range(lua_tointeger(L,1),1))return luaL_error(L,"read_u8 requires a canonical RAM address (0..2047)");
    const auto& m=self(L).observation_.memory;if(!m){lua_pushnil(L);return 1;}lua_pushinteger(L,m->bytes[std::size_t(lua_tointeger(L,1))]);return 1;
}
int LuaMod::memory_range(lua_State* L){
    if(lua_gettop(L)!=2||!lua_isinteger(L,1)||!lua_isinteger(L,2)||!safe_memory_range(lua_tointeger(L,1),lua_tointeger(L,2)))return luaL_error(L,"read_range requires canonical RAM and 1..64 bytes");
    const auto& m=self(L).observation_.memory;if(!m){lua_pushnil(L);return 1;}const auto a=lua_tointeger(L,1),n=lua_tointeger(L,2);lua_createtable(L,int(n),0);for(int i=0;i<n;++i){lua_pushinteger(L,m->bytes[std::size_t(a+i)]);lua_rawseti(L,-2,i+1);}return 1;
}
int LuaMod::memory_watch(lua_State* L){
    auto& s=self(L);
    if(!s.registering_||lua_gettop(L)!=3||!lua_isinteger(L,1)||!lua_isinteger(L,2)||!lua_isfunction(L,3)||!safe_memory_range(lua_tointeger(L,1),lua_tointeger(L,2)))return luaL_error(L,"observe requires on_load, canonical RAM range and callback");
    const auto a=lua_tointeger(L,1),n=lua_tointeger(L,2);
    if(s.watches_.size()>=4||s.watch_bytes_+n>128)return luaL_error(L,"RAM observer quota exceeded");
    for(const auto& w:s.watches_)if(a<w.first+w.length&&w.first<a+n)return luaL_error(L,"Overlapping RAM observers forbidden");
    lua_pushvalue(L,3);const int ref=luaL_ref(L,LUA_REGISTRYINDEX);
    try{s.watches_.push_back({ref,uint16_t(a),uint16_t(n)});}catch(...){luaL_unref(L,LUA_REGISTRYINDEX,ref);return luaL_error(L,"Cannot allocate RAM observer");}s.watch_bytes_+=unsigned(n);lua_pushinteger(L,s.watches_.size());return 1;
}
void LuaMod::notify_watches(lua_State* L){
    auto& s=self(L);const auto& m=s.observation_.memory;
    for(auto& w:s.watches_){if(!m){w.valid=false;continue;}unsigned changes=0;
        if(w.valid)for(unsigned i=0;i<w.length;++i)if(w.before[i]!=m->bytes[w.first+i])++changes;
        if(changes){lua_rawgeti(L,LUA_REGISTRYINDEX,w.callback);lua_createtable(L,int(changes),0);unsigned row=0;
            for(unsigned i=0;i<w.length;++i)if(w.before[i]!=m->bytes[w.first+i]){lua_createtable(L,0,3);lua_pushinteger(L,w.first+i);lua_setfield(L,-2,"address");lua_pushinteger(L,w.before[i]);lua_setfield(L,-2,"old_value");lua_pushinteger(L,m->bytes[w.first+i]);lua_setfield(L,-2,"new_value");lua_rawseti(L,-2,++row);}lua_call(L,1,0);
        }
        for(unsigned i=0;i<w.length;++i)w.before[i]=m->bytes[w.first+i];w.valid=true;
    }
}

}
