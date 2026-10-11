#pragma once
#include "runtime/bus.hpp"
#include <array>
#include <filesystem>
#include <iosfwd>
#include <string>

namespace nes::game {
// Optional observer: delegates the existing ABI without altering any access.
class GameplayDiagnostic {
public:
    static std::filesystem::path local_log_path(const std::filesystem::path& directory);
    void begin(std::ostream& output, bool god, bool items, bool mods);
    RuntimeServices services(runtime::Bus& bus, RuntimeServices original, const CPUState& cpu);
    void frame(const CPUState& cpu, bool god, bool items, bool mods);
    void slice(const CPUState& cpu);
    void missing_address(const CPUState& cpu);
    void error(const char* code);
    void end(const char* reason);
    bool failed() const noexcept { return io_failed_; }
    bool assisted() const noexcept { return assisted_; }
private:
    std::ostream* output_{};
    runtime::Bus* bus_{};
    RuntimeServices original_{};
    CPUState cpu_{};
    std::array<uint8_t, 26> progress_{};
    uint64_t event_cycle_{}, event_frame_{}, sequence_{}, last_frame_{}, frame_cycle_{}, slice_cycle_{};
    unsigned idle_slices_{}, candidate_frames_{};
    int room_=-1, candidate_=-1;
    uint8_t screen_{}, death_{}, low_bank_{}, high_bank_{}, event_bank_{255};
    bool started_{}, ended_{}, io_failed_{}, assisted_{}, god_{}, items_{}, mods_{};
    bool dirty_{true}, mapper_dirty_{}, death_pending_{}, ending_seen_{}, frame_warning_{}, idle_warning_{};
    void event(const char* kind, const std::string& fields = {}) noexcept;
    void flush() noexcept;
    void observe();
    void write(uint16_t address, uint8_t value, uint8_t offset);
    void assistance(bool god, bool items, bool mods);
    bool stable_room() const;
};
}
