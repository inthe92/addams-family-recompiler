#pragma once
#include "runtime/bus.hpp"

namespace nes::game {
// Optional preview mod. The recovered ASM and generated game remain unchanged.
class GodMode {
public:
    RuntimeServices services(nes::runtime::Bus& bus, RuntimeServices original);
    void set_enabled(bool enabled) noexcept { enabled_=enabled; if(!enabled)rescue_pending_=false; }
    bool enabled() const noexcept { return enabled_; }
    void grant_all_items() noexcept { all_items_=true; }
    bool all_items() const noexcept { return all_items_; }
    void begin_frame();
    uint64_t blocked_damage() const noexcept { return blocked_damage_; }
    uint64_t rescues() const noexcept { return rescues_; }
private:
    nes::runtime::Bus* bus_{};
    RuntimeServices original_{};
    bool enabled_{},rescue_pending_{},all_items_{};
    uint64_t blocked_damage_{},rescues_{};
    void write(uint16_t address,uint8_t value,uint8_t offset);
};
}
