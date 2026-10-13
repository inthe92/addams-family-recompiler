#pragma once
#include <cstdint>
#include <chrono>
#include <deque>
#include <optional>
#include <string>
struct lua_State;
struct lua_Debug;
namespace nes::mods {
struct Player { int x{},y{}; };
struct Observation { uint64_t frame{},cpu_cycles{}; std::optional<unsigned> room; std::optional<Player> player; };
// Host-owned value snapshots only. No Bus, CPUState, filesystem or ROM services.
class LuaMod {
public:
    explicit LuaMod(const std::string& source, std::size_t memory_limit=2*1024*1024);
    ~LuaMod();
    LuaMod(const LuaMod&)=delete;
    LuaMod& operator=(const LuaMod&)=delete;
    void load(const Observation&);
    void update(const Observation&);
    void unload();
    const std::deque<std::string>& logs() const {return logs_;}
    std::deque<std::string> take_logs(){auto result=std::move(logs_);logs_.clear();return result;}
    uint64_t updates() const {return updates_;}
    std::size_t memory_used() const {return used_;}
private:
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
    static void hook(lua_State*,lua_Debug*);
    void checked(int);
    void invoke(const char*);
};
}
