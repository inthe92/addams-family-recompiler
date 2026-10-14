#pragma once
#include <array>
#include <cstdint>
namespace nes::mods {
// Only canonical internal RAM. No bus/register/ROM/device reads or aliases.
struct MemorySnapshot {std::array<uint8_t,2048> bytes{};};
template<class RAM> MemorySnapshot capture_memory(const RAM& ram){
    MemorySnapshot s;for(unsigned a=0;a<s.bytes.size();++a)s.bytes[a]=ram.read(uint16_t(a));return s;
}
inline bool safe_memory_range(int64_t first,int64_t length) noexcept {
    return first>=0&&first<2048&&length>0&&length<=64&&first+length<=2048;
}
}
