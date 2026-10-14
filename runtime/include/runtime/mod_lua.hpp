#pragma once
#include "runtime/mod_api.hpp"
#include "runtime/mod_memory.hpp"
#include "runtime/mod_hud.hpp"
#include "runtime/mod_visual.hpp"
#include <cstdint>
#include <chrono>
#include <deque>
#include <optional>
#include <string>
struct lua_State;
struct lua_Debug;
namespace nes::mods {
struct Player { int x{},y{}; };
struct Observation { uint64_t frame{},cpu_cycles{}; std::optional<unsigned> room; std::optional<Player> player; std::optional<GameSnapshot> game; double step_seconds=1.0/50.007; std::optional<MemorySnapshot> memory; double real_seconds{},simulation_seconds{},paused_seconds{};uint8_t buttons{};bool reset_timer{},toggle_ui{},toggle_layers{}; };
// Host-owned value snapshots only. No Bus, CPUState, filesystem or ROM services.
class LuaMod {
public:
    explicit LuaMod(const std::string& source, std::size_t memory_limit=2*1024*1024,unsigned api_version=1,uint8_t permissions=0,std::vector<ImageAsset> assets={},std::vector<Setting> settings={});
    ~LuaMod();
    LuaMod(const LuaMod&)=delete;
    LuaMod& operator=(const LuaMod&)=delete;
    void load(const Observation&);
    void update(const Observation&);
    void unload();
    const std::deque<std::string>& logs() const {return logs_;}
    std::deque<std::string> take_logs(){auto result=std::move(logs_);logs_.clear();return result;}
    unsigned api_version()const {return api_version_;}
    const std::vector<VisualCommand>& visual()const{return visual_;}
    const std::vector<ImageAsset>& assets()const{return assets_;}
    unsigned jump_extension()const{return jump_extension_;}
    bool needs_memory()const{return (permissions_&memory_read)!=0;}
    const std::vector<nes::ui::TextLine>& overlay()const {return overlay_;}
    uint64_t updates() const {return updates_;}
    std::size_t memory_used() const {return used_;}
private:
    std::vector<Setting> settings_;
    static int config_get(lua_State*);
    unsigned api_version_;uint8_t permissions_;unsigned jump_extension_{},pending_pixels_{};
    std::vector<ImageAsset> assets_;std::vector<VisualCommand> pending_visual_,visual_;
    std::vector<nes::ui::TextLine> pending_overlay_,overlay_;
    bool drawing_{},registering_{};
    struct Watch {int callback;uint16_t first,length;bool valid{};std::array<uint8_t,64> before{};};
    std::vector<Watch> watches_;
    unsigned watch_bytes_{};
    void clear_watches() noexcept;
    static int memory_u8(lua_State*);
    static int memory_range(lua_State*);
    static int memory_watch(lua_State*);
    static void notify_watches(lua_State*);
    lua_State* state_{};
    std::size_t used_{},limit_;
    int budget_{};
    std::chrono::steady_clock::time_point deadline_;
    Observation observation_;
    const char* callback_{};
    uint64_t updates_{};
    std::deque<std::string> logs_;
    static void* allocate(void*,void*,std::size_t,std::size_t);
    static LuaMod& self(lua_State*);
    static int initialize(lua_State*);
    static int call(lua_State*);
    static int info(lua_State*);
    static int status(lua_State*);
    static int player(lua_State*);
    static int hud_text(lua_State*);
    static int set_modifier(lua_State*);
    static int clock_state(lua_State*);
    static int input_state(lua_State*);
    static int ui_text(lua_State*);
    static int ui_rect(lua_State*);
    static int graphic_layer(lua_State*);
    static void hook(lua_State*,lua_Debug*);
    void checked(int);
    void invoke(const char*);
};
}
