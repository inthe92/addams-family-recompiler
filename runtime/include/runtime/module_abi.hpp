#pragma once
#include "recomp/module_abi.h"
#include <SDL_loadso.h>
#include <stdexcept>
#include <string>

namespace nes::runtime {
inline void validate_module_abi(void* handle) {
    auto version=reinterpret_cast<NesRecompAbiVersion>(SDL_LoadFunction(handle,"nes_recomp_abi_version"));
    if(!version)throw std::runtime_error("Native module ABI version missing. Reimport ROM with current tools.");
    const auto actual=version();
    if(actual!=NES_RECOMP_ABI_VERSION)throw std::runtime_error("Native module ABI mismatch: "+std::to_string(actual)+
        "; required "+std::to_string(NES_RECOMP_ABI_VERSION)+". Reimport ROM with current tools.");
}
}
