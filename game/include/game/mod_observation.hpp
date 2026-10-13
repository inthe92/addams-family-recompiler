#pragma once
#include "runtime/mod_lua.hpp"
#include "runtime/bus.hpp"
namespace nes::game {
// Revision-specific, side-effect-free observation. Room ID is already used by
// GameplayDiagnostic. Actor poses do not reliably identify Gomez in every scene;
// API v1 reports no player until a separately validated adapter is available.
inline nes::mods::Observation mod_observation(const nes::runtime::Bus& bus){
    return {bus.video().frames(),bus.cycles(),bus.ram().read(0x045B),std::nullopt};
}
}
