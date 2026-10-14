#include "runtime/cartridge.hpp"
#include <stdexcept>
#include <algorithm>

namespace nes::runtime {
static bool power_of_two(std::size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}
Cartridge::Cartridge(CartridgeData data) : data_(data) {
    if (!data.prg || !power_of_two(data.prg_size) || data.prg_size < 32768
        || data.prg_size > 262144 || !data.chr || !power_of_two(data.chr_size)
        || data.chr_size < 8192 || data.chr_size > 131072) {
        throw std::invalid_argument("Standard MMC1B PRG/CHR-ROM resource layout required");
    }
    prg_mask_ = static_cast<uint8_t>(data.prg_size / 16384 - 1);
    chr_mask_ = static_cast<uint8_t>(data.chr_size / 4096 - 1);
}

uint8_t Cartridge::bank_for_pc(uint16_t address) const noexcept {
    if (address < 0x8000) return 0xFF;
    const uint8_t window = address >= 0xC000 ? 1 : 0;
    const uint8_t mode = static_cast<uint8_t>((control_ >> 2) & 3u);
    uint8_t bank;
    if (mode < 2) bank = static_cast<uint8_t>((prg_ & 0x0Eu) + window);
    else if (mode == 2) bank = window ? static_cast<uint8_t>(prg_ & 0x0Fu) : 0;
    else bank = window ? prg_mask_ : static_cast<uint8_t>(prg_ & 0x0Fu);
    return static_cast<uint8_t>(bank & prg_mask_);
}

void Cartridge::set_text_resources(std::vector<PrgTextResource> resources){
    if(resources.size()>16)throw std::invalid_argument("Too many text resources");
    std::sort(resources.begin(),resources.end(),[](const auto&a,const auto&b){return a.bank*16384+a.offset<b.bank*16384+b.offset;});
    unsigned end=0;bool first=true;
    for(auto& r:resources){
        if(r.owner.empty()||r.id.empty()||r.bank>=data_.prg_size/16384||r.offset>=16384||r.bytes.empty()||r.bytes.size()>64||r.offset+r.bytes.size()>16384)throw std::invalid_argument("Invalid bounded text resource");
        const unsigned start=r.bank*16384+r.offset;
        if(!first&&start<end)throw std::invalid_argument("Conflicting text resource owners");
        if(resource_crc32(data_.prg+start,unsigned(r.bytes.size()))!=r.original_crc32)throw std::invalid_argument("Text resource does not match loaded game; translation rejected");
        if(std::any_of(r.bytes.begin(),r.bytes.end(),[](uint8_t v){return v>127;}))throw std::invalid_argument("Text control byte in glyph replacement");
        if(r.composition){
            auto& c=*r.composition;
            if(c.chr_bank>=data_.chr_size/4096||c.nametable<0x2000||c.nametable>=0x3000||
               (c.nametable&0x3ff)>=960||((c.nametable&31)+r.bytes.size()>32)||c.mirroring>3||c.overrides.size()>8)
                throw std::invalid_argument("Invalid bounded background consumer");
            for(const auto& other:resources)if(&other!=&r&&other.composition){
                const auto& o=*other.composition;
                // Conservative row overlap across mirrored aliases; fail closed.
                auto a=c.nametable&0x3ffu,b=o.nametable&0x3ffu;
                if(c.chr_bank==o.chr_bank&&c.mirroring==o.mirroring&&a<b+other.bytes.size()&&b<a+r.bytes.size())
                    throw std::invalid_argument("Conflicting background resource owners");
            }
            c.original_cells.assign(data_.prg+start,data_.prg+start+r.bytes.size());
            c.tiles.clear();c.tiles.reserve(r.bytes.size());
            for(auto code:r.bytes){std::array<uint8_t,16> tile{};
                std::copy_n(data_.chr+c.chr_bank*4096+unsigned(code)*16,16,tile.begin());c.tiles.push_back(tile);}
            std::array<bool,64> used{};
            for(const auto& g:c.overrides){if(g.cell>=r.bytes.size()||used[g.cell])throw std::invalid_argument("Duplicate/out-of-range glyph cell");used[g.cell]=true;c.tiles[g.cell]=g.pixels;}
        }
        end=start+unsigned(r.bytes.size());first=false;
    }
    text_resources_=std::move(resources);
    has_compositions_=std::any_of(text_resources_.begin(),text_resources_.end(),[](const auto&r){return bool(r.composition);});
    ++text_resource_revision_;
}
uint8_t Cartridge::cpu_read(uint16_t address, uint8_t open_bus) const {
    if (address >= 0x8000) {
        const unsigned bank=bank_for_pc(address),offset=address&0x3FFFu;
        if(!text_resources_.empty())for(const auto& r:text_resources_)
            if(!r.composition&&r.bank==bank&&offset>=r.offset&&offset-r.offset<r.bytes.size())return r.bytes[offset-r.offset];
        return data_.prg[std::size_t(bank)*16384+offset];
    }
    if (address >= 0x6000 && prg_ram_enabled()) return prg_ram_[address & 0x1FFFu];
    return open_bus;
}

void Cartridge::cpu_write(uint16_t address, uint8_t value, uint64_t cycle) {
    if (has_previous_write_ && cycle <= previous_write_cycle_) {
        throw std::logic_error("CPU write timestamps must increase");
    }
    const bool consecutive = has_previous_write_ && cycle == previous_write_cycle_ + 1;
    previous_write_cycle_ = cycle;
    has_previous_write_ = true;
    if (address < 0x8000) {
        if (address >= 0x6000 && prg_ram_enabled()) prg_ram_[address & 0x1FFFu] = value;
        return;
    }
    // D7 reset is never ignored, including the second RMW write.
    if ((value & 0x80u) != 0) {
        shift_ = 0x10;
        control_ = static_cast<uint8_t>(control_ | 0x0Cu);
        return;
    }
    if (consecutive) return;
    const bool full = (shift_ & 1u) != 0;
    shift_ = static_cast<uint8_t>((shift_ >> 1) | ((value & 1u) << 4));
    if (!full) return;
    switch ((address >> 13) & 3u) {
        case 0: control_ = shift_; break;
        case 1: chr0_ = shift_; break;
        case 2: chr1_ = shift_; break;
        case 3: prg_ = shift_; break;
    }
    shift_ = 0x10;
}

uint32_t Cartridge::chr_offset(uint16_t address) const {
    if (address >= 0x2000) throw std::out_of_range("CHR address outside pattern tables");
    uint8_t bank;
    if ((control_ & 0x10u) != 0) bank = address < 0x1000 ? chr0_ : chr1_;
    else bank = static_cast<uint8_t>((chr0_ & 0x1Eu) + (address >= 0x1000 ? 1u : 0u));
    return static_cast<uint32_t>(bank & chr_mask_) * 4096u + (address & 0x0FFFu);
}
uint8_t Cartridge::chr_read(uint16_t address) const { return data_.chr[chr_offset(address)]; }

uint16_t Cartridge::nametable_offset(uint16_t address) const {
    if (address < 0x2000 || address >= 0x3F00) throw std::out_of_range("Address outside nametables");
    const uint16_t table = static_cast<uint16_t>((address >> 10) & 3u);
    uint16_t page;
    switch (control_ & 3u) {
        case 0: page = 0; break;
        case 1: page = 1; break;
        case 2: page = static_cast<uint16_t>(table & 1u); break;
        default: page = static_cast<uint16_t>(table >> 1); break;
    }
    return static_cast<uint16_t>(page * 1024u + (address & 0x03FFu));
}
}
