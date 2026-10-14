#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>
namespace nes::runtime {
// Internal typed backend; never exposed as an address/patch API to Lua/ZIPs.
struct VisualGlyph { unsigned cell{}; std::array<uint8_t,16> pixels{}; };
// Trusted host consumer metadata, never supplied as addresses by mod packages.
struct TextComposition {
    unsigned chr_bank{},nametable{},mirroring{};
    std::vector<VisualGlyph> overrides;
    std::vector<uint8_t> original_cells;
    std::vector<std::array<uint8_t,16>> tiles;
};
struct PrgTextResource {
    std::string owner,id;
    unsigned bank{},offset{};
    uint32_t original_crc32{};
    std::vector<uint8_t> bytes;
    std::optional<TextComposition> composition;
};
inline uint32_t resource_crc32(const uint8_t* data,unsigned count) noexcept {
    uint32_t crc=0xffffffffu;for(unsigned i=0;i<count;++i){crc^=data[i];for(unsigned b=0;b<8;++b)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}return ~crc;
}
}
