#include "runtime/bus.hpp"
#include <stdexcept>

namespace nes::runtime {
RuntimeServices Bus::services() {
    return RuntimeServices{this,
        [](void* context, uint16_t a) { return static_cast<Bus*>(context)->read8(a); },
        [](void* context, uint16_t a, uint8_t v, uint8_t offset) { static_cast<Bus*>(context)->write8(a, v, offset); },
        [](void* context, uint16_t a) { return static_cast<Bus*>(context)->cartridge_.bank_for_pc(a); },
        [](void* context, uint8_t cycles) { static_cast<Bus*>(context)->advance(cycles); },
        [](void* context, CPUState* cpu, uint8_t mask) { return static_cast<Bus*>(context)->boundary(*cpu, mask); }};
}
void Bus::sync_devices(uint64_t target) {
    if (target < device_cycles_) throw std::logic_error("Device time cannot move backwards");
    const uint64_t delta = target - device_cycles_;
    video_.advance_cpu_cycles(delta);
    audio_.advance_cpu_cycles(delta);
    device_cycles_ = target;
}
uint8_t Bus::read8(uint16_t address) {
    if (address < 0x2000) open_bus_ = ram_.read(address);
    else if (address < 0x4000) open_bus_ = video_.read_register(address);
    else if (address == 0x4015) open_bus_ = audio_.read_status(open_bus_);
    else if (address == 0x4016 || address == 0x4017) {
        const unsigned port = address & 1u;
        const uint8_t value = strobe_ ? buttons_[port] : controller_shift_[port];
        open_bus_ = static_cast<uint8_t>((open_bus_ & 0xE0u) | (value & 1u));
        if (!strobe_) controller_shift_[port] = static_cast<uint8_t>((controller_shift_[port] >> 1) | 0x80u);
    } else if (address >= 0x6000) open_bus_ = cartridge_.cpu_read(address, open_bus_);
    return open_bus_;
}
void Bus::write8(uint16_t address, uint8_t value, uint8_t offset) {
    const uint64_t when = cycles_ + offset;
    sync_devices(when);
    cartridge_.cpu_write(address, value, when);
    open_bus_ = value;
    if (address < 0x2000) ram_.write(address, value);
    else if (address < 0x4000) video_.write_register(address, value);
    else if (address == 0x4014) { dma_pending_ = true; dma_page_ = value; }
    else if (address == 0x4016) {
        const bool next = (value & 1u) != 0;
        if (strobe_ || next) controller_shift_ = buttons_;
        strobe_ = next;
    } else if (address <= 0x4017 && (address <= 0x4013 || address == 0x4015 || address == 0x4017)) {
        audio_.write_register(address, value);
        if (audio_writes_.size() < 1000000) audio_writes_.push_back({when, address, value});
    }
}
void Bus::advance(uint64_t cycles) { cycles_ += cycles; sync_devices(cycles_); }
void Bus::set_buttons(unsigned port, uint8_t buttons) {
    if (port > 1) throw std::out_of_range("Only two controller ports are available");
    buttons_[port] = buttons;
    if (strobe_) controller_shift_[port] = buttons;
}
void Bus::perform_dma() {
    dma_pending_ = false; ++dma_count_;
    // Start at the next instruction boundary. Preserve OAMADDR wrapping and
    // the 513/514-cycle stall. A DMC get takes precedence over an OAM get.
    advance(1u + (cycles_ & 1u));
    const uint16_t base = static_cast<uint16_t>(dma_page_ * 256u);
    for (unsigned i = 0; i < 256; ++i) {
        if (audio_.dmc_read_requested()) perform_dmc_dma(true);
        const uint8_t value = read8(static_cast<uint16_t>(base + i));
        advance(1); video_.write_oam(value); advance(1);
    }
}
void Bus::perform_dmc_dma(bool during_oam) {
    if (!audio_.dmc_read_requested()) return;
    // PAL DMC halts at instruction fetch boundaries. The ordinary halt/dummy/
    // alignment/get costs3/4cycles; during OAM, the get plus realignment costs2.
    // Rare1/3cycle end-of-OAM overlaps and cancellation races remain unverified.
    const uint16_t address = audio_.dmc_read_address();
    advance(during_oam ? 1u : 2u + (cycles_ & 1u));
    const uint8_t value = read8(address);
    audio_.complete_dmc_read(value); advance(1); ++dmc_dma_count_;
}
void Bus::deliver_interrupt(CPUState& cpu, uint16_t vector) {
    write8(static_cast<uint16_t>(0x0100u | cpu.SP), static_cast<uint8_t>(cpu.PC >> 8), 2); --cpu.SP;
    write8(static_cast<uint16_t>(0x0100u | cpu.SP), static_cast<uint8_t>(cpu.PC), 3); --cpu.SP;
    write8(static_cast<uint16_t>(0x0100u | cpu.SP), cpu_status_for_push(&cpu, 0), 4); --cpu.SP;
    cpu_set_flag(&cpu, CPU_I, 1);
    const uint8_t low = read8(vector), high = read8(static_cast<uint16_t>(vector + 1u));
    cpu.PC = static_cast<uint16_t>(low | (static_cast<uint16_t>(high) << 8));
    advance(7);
}
uint8_t Bus::boundary(CPUState& cpu, uint8_t irq_mask) {
    if (irq_mask == 0xFF) irq_mask = static_cast<uint8_t>(cpu.P & CPU_I);
    if (dma_pending_) perform_dma();
    if (audio_.dmc_read_requested()) perform_dmc_dma();
    if (video_.consume_nmi()) { deliver_interrupt(cpu, 0xFFFA); ++nmi_count_; return 1; }
    if (audio_.irq_pending() && !irq_mask) { deliver_interrupt(cpu, 0xFFFE); ++irq_count_; return 1; }
    return static_cast<uint8_t>(video_.pending_frame());
}
}
