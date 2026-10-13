#include "runtime/mod_lua.hpp"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include <cstdlib>
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
    lua_newtable(L);lua_pushcfunction(L,status);lua_setfield(L,-2,"get_status");lua_pushcfunction(L,player);lua_setfield(L,-2,"get_player");lua_setglobal(L,"game");
    lua_pushinteger(L,1);lua_setglobal(L,"MOD_API_VERSION");return 0;
}
LuaMod::LuaMod(const std::string& source,std::size_t limit):limit_(limit) {
    if(source.size()>128*1024 || limit<65536)throw std::runtime_error("Mod source or memory limit invalid");
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
int LuaMod::call(lua_State* L) {
    auto& s=self(L);lua_getglobal(L,s.callback_);
    if(lua_isnil(L,-1)){lua_pop(L,1);return 0;}
    if(!lua_isfunction(L,-1))return luaL_error(L,"Mod callback must be a function");
    const bool update=std::string_view(s.callback_)=="on_update";
    if(update)lua_pushnumber(L,1.0/50.007);
    lua_call(L,update?1:0,0);return 0;
}
void LuaMod::invoke(const char* name) {
    callback_=name;budget_=100000;deadline_=std::chrono::steady_clock::now()+std::chrono::milliseconds(20);
    lua_pushcfunction(state_,call);checked(lua_pcall(state_,0,0,0));
}
void LuaMod::load(const Observation& o){observation_=o;invoke("on_load");}
void LuaMod::update(const Observation& o){observation_=o;invoke("on_update");++updates_;}
void LuaMod::unload(){invoke("on_unload");}
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
    lua_pushnumber(L,50.007);lua_setfield(L,-2,"simulation_hz");
    if(o.room){lua_pushinteger(L,*o.room);lua_setfield(L,-2,"room_id");}return 1;
}
int LuaMod::player(lua_State* L) {
    const auto& p=self(L).observation_.player;if(!p){lua_pushnil(L);return 1;}
    lua_newtable(L);lua_pushinteger(L,p->x);lua_setfield(L,-2,"x");lua_pushinteger(L,p->y);lua_setfield(L,-2,"y");return 1;
}
}
