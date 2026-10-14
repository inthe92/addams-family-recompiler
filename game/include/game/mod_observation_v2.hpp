#pragma once
#include "runtime/mod_lua.hpp"
#include "runtime/bus.hpp"
namespace nes::game {
inline constexpr const char* mod_rom_sha256="6143af950d2c4d353c859c56962406feed9930584882fba1056bba5f2614e880";
inline constexpr const char* mod_game_id="addams-family-pal-eu-enfrde";
// Read-only profile adapter: never calls bus/PPU/APU register services.
// x/y are the game's screen-space player reference, not a bounding box/world map.
inline nes::mods::Observation mod_completed_observation(const nes::runtime::Bus& bus){
    nes::mods::Observation o;o.frame=bus.video().frames();o.cpu_cycles=bus.cycles();
    nes::mods::GameSnapshot g;g.game_id=mod_game_id;const auto& r=bus.ram();
    if(r.read(0x045B)!=0)g.state="menu";
    else if(r.read(0x0471)!=0xFF||r.read(0xB1)||r.read(0x80))g.state="transition";
    else {g.room=r.read(0x71);g.state=r.read(0x70)?"dead":"playing";
        // Conservative identity coverage verified in private native execution.
        if(g.state=="playing"&&*g.room==0&&!r.read(0x0600)&&!r.read(0x72)&&
           !r.read(0x7C)&&!r.read(0x060D)&&!r.read(0x061C)&&r.read(0x13)<240)
            g.player=nes::mods::PlayerSnapshot{r.read(0x12),r.read(0x13)};
    }
    o.game=std::move(g);return o;
}
}
