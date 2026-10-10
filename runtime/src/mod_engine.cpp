#include "runtime/mod_engine.hpp"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include <stdexcept>

namespace nes::runtime {
static Bus& context(lua_State* state) {
    return *static_cast<Bus*>(lua_touserdata(state, lua_upvalueindex(1)));
}
static unsigned bounded(lua_State* state, int index, unsigned maximum) {
    const lua_Integer value = luaL_checkinteger(state, index);
    if (value < 0 || static_cast<lua_Unsigned>(value) > maximum) luaL_argerror(state,index,"Integer outside API range");
    return static_cast<unsigned>(value);
}
int ModEngine::read_ram(lua_State* state) {
    const uint16_t address = static_cast<uint16_t>(bounded(state,1,0x1FFF));
    lua_pushinteger(state,context(state).ram().read(address)); return 1;
}
int ModEngine::write_ram(lua_State* state) {
    const uint16_t address = static_cast<uint16_t>(bounded(state,1,0x1FFF));
    const uint8_t value = static_cast<uint8_t>(bounded(state,2,255));
    context(state).ram().write(address,value); return 0;
}
int ModEngine::set_buttons(lua_State* state) {
    const unsigned port = bounded(state,1,1);
    const uint8_t value = static_cast<uint8_t>(bounded(state,2,255));
    context(state).set_buttons(port,value); return 0;
}
ModEngine::ModEngine(Bus& bus) : bus_(bus), state_(luaL_newstate()) {
    if (!state_) throw std::runtime_error("Cannot create Lua mod state");
    const struct { const char* name; lua_CFunction open; } libraries[] = {
        {LUA_GNAME,luaopen_base},{LUA_TABLIBNAME,luaopen_table},{LUA_STRLIBNAME,luaopen_string},
        {LUA_MATHLIBNAME,luaopen_math},{LUA_UTF8LIBNAME,luaopen_utf8}};
    for (const auto& library : libraries) { luaL_requiref(state_,library.name,library.open,1);lua_pop(state_,1); }
    // Loading is controlled by the host. The game mod API deals with game state.
    lua_pushnil(state_);lua_setglobal(state_,"dofile");
    lua_pushnil(state_);lua_setglobal(state_,"loadfile");
    lua_newtable(state_);
    const luaL_Reg functions[] = {{"read_ram",read_ram},{"write_ram",write_ram},{"set_buttons",set_buttons},{nullptr,nullptr}};
    lua_pushlightuserdata(state_,&bus_);luaL_setfuncs(state_,functions,1);
    lua_setglobal(state_,"nes");
}
ModEngine::~ModEngine() { lua_close(state_); }
void ModEngine::check_result(int status) {
    if (status == LUA_OK) return;
    const char* error = lua_tostring(state_,-1);
    const std::string message = error ? error : "Lua mod returned a non-string error";
    lua_pop(state_,1); throw std::runtime_error(message);
}
void ModEngine::checked_call(int arguments) { check_result(lua_pcall(state_,arguments,0,0)); }
void ModEngine::load_file(const std::string& path) { check_result(luaL_loadfile(state_,path.c_str()));checked_call(0); }
void ModEngine::load_source(std::string_view source) {
    check_result(luaL_loadbuffer(state_,source.data(),source.size(),"native-mod"));checked_call(0);
}
void ModEngine::frame(const CPUState& cpu) {
    lua_getglobal(state_,"on_frame");
    if (lua_isnil(state_,-1)) { lua_pop(state_,1);return; }
    if (!lua_isfunction(state_,-1)) { lua_pop(state_,1);throw std::runtime_error("on_frame must be a function"); }
    lua_newtable(state_);
    const struct { const char* key; lua_Integer value; } fields[] = {
        {"frame",static_cast<lua_Integer>(bus_.video().frames())},{"cycles",static_cast<lua_Integer>(bus_.cycles())},
        {"A",cpu.A},{"X",cpu.X},{"Y",cpu.Y},{"SP",cpu.SP},{"P",cpu.P},{"PC",cpu.PC},
        {"screen",bus_.ram().read(0x045B)}};
    for(const auto& field:fields) {lua_pushinteger(state_,field.value);lua_setfield(state_,-2,field.key);}
    checked_call(1);
}
}
