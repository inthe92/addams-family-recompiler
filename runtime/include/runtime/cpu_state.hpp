#pragma once
#include "recomp/cpu_state.h"
#include <type_traits>

namespace nes::runtime {
using CPUState = ::CPUState;
static_assert(sizeof(uint8_t) == 1 && sizeof(uint16_t) == 2);
static_assert(std::is_standard_layout_v<CPUState>);
static_assert(std::is_trivially_copyable_v<CPUState>);
// Serialize explicit fields for save files; struct padding is not a file format.
}
