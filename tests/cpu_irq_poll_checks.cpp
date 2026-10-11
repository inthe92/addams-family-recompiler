// Execute authored programs through generated C and the real runtime only.
// Expectations come from published PAL program contracts or CPU documentation;
// they deliberately remain failures until production is corrected.
#include "runtime/bus.hpp"
#include "irq_poll_fixtures.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nes::runtime;

struct Access {
    uint16_t pc, address;
    uint8_t value, offset;
    uint64_t cycle;
    bool source_after;
};
struct Entry {
    CPUState cpu;
    uint16_t after_instruction;
    uint64_t cycle;
    std::array<uint8_t, 3> stack; // P, PCL, PCH from SP+1 upward.
    bool source_after;
};

class Check {
    const IRQFixture& f;
    // CartridgeData holds pointers: buffers must outlive Bus and execution.
    std::array<uint8_t, 32768> prg{};
    std::array<uint8_t, 8192> chr{};
    std::unique_ptr<Bus> bus;
    CPUState cpu = cpu_initial_state(0x8000);
    RuntimeServices original{};
    uint16_t current_pc{};
    uint64_t origin{};
    unsigned instructions{}, failures{}, critical_seen{};
    bool critical_source_before{};
    uint8_t critical_mask{};
    std::vector<Access> reads, writes;
    std::vector<Entry> entries;

    void require(bool condition, const std::string& description) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL " << f.name << " seed=" << f.seed << " phase=" << f.phase
                      << ": " << description << '\n';
        }
    }
    static uint8_t read(void* context, uint16_t address, uint8_t offset) {
        auto& p = *static_cast<Check*>(context);
        const uint8_t value = p.original.read8(p.original.context, address, offset);
        if (address == 0x4015)
            p.reads.push_back({p.current_pc, address, value, offset,
                              p.bus->audio().cycles(), p.bus->audio().irq_pending()});
        return value;
    }
    static void write(void* context, uint16_t address, uint8_t value, uint8_t offset) {
        auto& p = *static_cast<Check*>(context);
        p.original.write8(p.original.context, address, value, offset);
        if (address == 0x4017 || (address == 0x21 && p.current_pc == 0x9000))
            p.writes.push_back({p.current_pc, address, value, offset,
                               p.bus->audio().cycles(), p.bus->audio().irq_pending()});
        if (address == 0x4017 && p.current_pc == p.f.origin)
            p.origin = p.bus->audio().cycles();
    }
    static uint8_t bank(void* context, uint16_t pc) {
        auto& p = *static_cast<Check*>(context);
        return p.original.bank_for_pc(p.original.context, pc);
    }
    static void advance(void* context, uint8_t cycles) {
        auto& p = *static_cast<Check*>(context);
        p.original.advance(p.original.context, cycles);
        if (p.current_pc == p.f.critical) {
            ++p.critical_seen;
            p.require(cycles == p.f.critical_cycles, "critical instruction cycle count");
        }
    }
    static uint8_t boundary(void* context, CPUState* cpu, uint8_t mask) {
        auto& p = *static_cast<Check*>(context);
        if (p.current_pc == p.f.critical) p.critical_mask = mask;
        const auto count = p.bus->irq_count();
        const auto next_pc = cpu->PC;
        const auto result = p.original.boundary(p.original.context, cpu, mask);
        if (p.bus->irq_count() != count) {
            std::array<uint8_t, 3> stack{};
            for (unsigned i = 0; i < 3; ++i)
                stack[i] = p.bus->ram().read(uint16_t(0x100 | uint8_t(cpu->SP + 1 + i)));
            p.entries.push_back({*cpu, next_pc, p.bus->cycles(), stack,
                                 p.bus->audio().irq_pending()});
        }
        return result;
    }
    static void poll(void* context, uint8_t total, uint8_t offsets) {
        auto& p = *static_cast<Check*>(context);
        p.original.poll_irq(p.original.context, total, offsets);
    }

public:
    explicit Check(const IRQFixture& fixture) : f(fixture) {
        for (unsigned i = 0; i < f.byte_count; ++i)
            prg[f.bytes[i].address - 0x8000] = f.bytes[i].value;
        bus = std::make_unique<Bus>(CartridgeData{prg.data(), prg.size(), chr.data(), chr.size()});
        bus->ram().write(0x10, uint8_t(f.seed));
        bus->ram().write(0x20, uint8_t(f.phase));
        original = bus->services();
    }

    bool run() {
        RuntimeServices services{this, read, write, bank, advance, boundary, poll};
        while (cpu.PC != f.stop) {
            if (++instructions > 100000 || bus->cycles() > 200000)
                throw std::runtime_error("finite synthetic execution budget exceeded");
            current_pc = cpu.PC;
            if (current_pc == f.critical) critical_source_before = bus->audio().irq_pending();
            uint32_t left = 1;
            if (!f.execute(&cpu, &services, &left) || left)
                throw std::runtime_error("instruction missing from generated C dispatch");
        }
        require(origin != 0 && (origin & 1u) == 0, "synchronized origin W exists and has normalized parity");
        require(cpu.PC == f.stop && cpu.SP == 0xfd, "final PC and restored SP");
        require(cpu.A == f.final_a && cpu.X == f.final_x && cpu.Y == 0 && cpu.P == f.final_p,
                "final A/X/Y/P");
        require(bus->cycles() == origin + f.total && bus->audio().cycles() == bus->cycles(),
                "total CPU/APU cycles expected=" + std::to_string(origin + f.total) +
                " observed=" + std::to_string(bus->cycles()));
        require(bus->nmi_count() == 0 && bus->dma_count() == 0 && bus->dmc_dma_count() == 0,
                "isolated fixture has no NMI/OAM/DMC DMA");
        require(bus->irq_count() == f.irq_count && entries.size() == f.irq_count,
                "IRQ entries expected=" + std::to_string(f.irq_count) +
                " observed=" + std::to_string(entries.size()));

        bool sync_start = false, sync_read = false, sync_end = false;
        uint64_t sync_cycle = 0;
        for (const auto& w : writes) {
            if (w.pc == f.sync_start) { sync_start = w.value == 0 && w.offset == 3; sync_cycle = w.cycle; }
            if (w.pc == f.sync_end) sync_end = w.value == 0x40 && w.offset == 3 && w.cycle < origin;
            if (w.pc == f.origin) require(w.value == 0 && w.offset == 3, "origin $4017 write value/offset");
        }
        std::vector<Access> data;
        for (const auto& r : reads) {
            if (r.pc == f.sync_read)
                sync_read = r.offset == 3 && r.cycle == sync_cycle + 33255;
            if (r.pc == f.read1 || r.pc == f.read2) data.push_back(r);
        }
        require(sync_start && sync_read && sync_end, "sync write/read/clear ordering and delay");
        require(data.size() == f.bit_count, "number of program status reads");
        for (unsigned i = 0; i < data.size(); ++i) {
            const auto& r = data[i];
            const unsigned bit = i == 0 ? f.bit1 : f.bit2;
            require(r.offset == 3 && r.value == bit * 0x40 && r.cycle == origin + f.read_relative + i * 4,
                    "status read value/offset/cycle");
            // No requirement that source_after == returned bit: these are different observables.
        }
        if (f.critical) {
            require(critical_seen == 1, "critical instruction executed exactly once");
            require(critical_mask == (std::string(f.group) == "held_masked" ? CPU_I : 0),
                    "I mask at critical instruction poll");
            if (std::string(f.group).find("held_") == 0 || std::string(f.group) == "branches")
                require(critical_source_before, "IRQ already held before critical instruction");
        }
        for (const auto& e : entries) {
            require(e.cpu.PC == 0x9000 && e.cpu.SP == 0xfa, "IRQ vector PC and SP");
            require(e.after_instruction == f.successor && e.stack[1] == (f.successor & 0xff) &&
                    e.stack[2] == (f.successor >> 8), "return PC pushed after the expected instruction");
            require(e.stack[0] == f.entry_p && e.cpu.P == (f.entry_p | CPU_I),
                    "saved P (B=0/U=1) and entry I flag");
            require(e.cpu.A == f.entry_a && e.cpu.X == f.final_x && e.cpu.Y == 0,
                    "A/X/Y at IRQ entry");
            require(e.cycle == origin + f.entry, "IRQ entry cycle expected=" +
                    std::to_string(origin + f.entry) + " observed=" + std::to_string(e.cycle));
        }
        unsigned stores = 0;
        for (const auto& w : writes) if (w.pc == 0x9000) {
            ++stores;
            require(w.value == f.entry_a && w.offset == 2 && w.cycle == origin + f.entry + 2,
                    "handler marker and store cycle");
        }
        require(stores == f.irq_count, "handler executes once per expected IRQ");
        if (f.irq_count) require(bus->ram().read(0x21) == f.entry_a, "handler result marker");

        std::cout << f.name << " seed=" << f.seed << " phase=" << f.phase
                  << " W=" << origin << " PC=" << cpu.PC << " SP=" << unsigned(cpu.SP)
                  << " A=" << unsigned(cpu.A) << " X=" << unsigned(cpu.X) << " Y=" << unsigned(cpu.Y)
                  << " P=" << unsigned(cpu.P) << " cycles=" << bus->cycles()
                  << " entries=" << entries.size() << " result=" << (failures ? "FAIL" : "PASS") << '\n';
        for (const auto& r : data)
            std::cout << "  $4015 cycle=" << r.cycle << " W+" << r.cycle-origin
                      << " status=" << unsigned(r.value) << " source_after=" << r.source_after << '\n';
        for (const auto& e : entries)
            std::cout << "  IRQ cycle=" << e.cycle << " PC=" << e.cpu.PC
                      << " returnPC=" << e.after_instruction << " stack=" << unsigned(e.stack[0])
                      << ',' << unsigned(e.stack[1]) << ',' << unsigned(e.stack[2])
                      << " source_after=" << e.source_after << '\n';
        return failures == 0;
    }
};

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "One contract selector required\n"; return 2; }
    const std::string selector = argv[1];
    unsigned passed = 0, failed = 0, harness_errors = 0;
    for (const auto& f : irq_fixtures) {
        const bool selected = selector == f.group ||
            (selector == "pal08_alignment0" && std::string(f.group) == "pal08" && f.phase == 0) ||
            (selector == "pal08_alignment1" && std::string(f.group) == "pal08" && f.phase == 1);
        if (!selected) continue;
        try { Check p(f); if (p.run()) ++passed; else ++failed; }
        catch (const std::exception& e) {
            ++failed; ++harness_errors;
            std::cerr << "HARNESS ERROR " << f.name << ": " << e.what() << '\n';
        }
    }
    if (!passed && !failed) { std::cerr << "Unknown/empty selector: " << selector << '\n'; return 2; }
    std::cout << "IRQ contracts executed=" << passed+failed << " passed=" << passed
              << " failed=" << failed << " harness_errors=" << harness_errors << '\n';
    return failed ? 1 : 0;
}
