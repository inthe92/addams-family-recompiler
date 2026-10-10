#pragma once
#include "runtime/bus.hpp"
#include <string>
#include <string_view>
struct lua_State;
namespace nes::runtime {
class ModEngine {
public:
    explicit ModEngine(Bus& bus);
    ~ModEngine();
    ModEngine(const ModEngine&) = delete;
    ModEngine& operator=(const ModEngine&) = delete;
    void load_file(const std::string& path);
    void load_source(std::string_view source);
    // Called once at frame boundaries by the host, outside any native C block.
    // Registers are exposed as a snapshot; RAM mutations use byte-valued APIs.
    void frame(const CPUState& cpu);
private:
    Bus& bus_;
    lua_State* state_{};
    void checked_call(int arguments);
    void check_result(int status);
    static int read_ram(lua_State* state);
    static int write_ram(lua_State* state);
    static int set_buttons(lua_State* state);
};
}
