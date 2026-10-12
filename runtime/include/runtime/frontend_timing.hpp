#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>

namespace nes::frontend {
// Observation only. No SDL, NES services, deadlines or execution callbacks.
enum class TimingStage { simulation, audio, render, swap, wait, capture, conversion, upload, count };
enum class TimingCount { nes_frames, ppu_frames, native_slices, mod_hooks, god_hooks,
    diagnostic_frames, diagnostic_slices, audio_batches, pcm_samples, wav_batches,
    render_passes, paused_render_passes, nes_draws, ui_draws, swaps, captures,
    paused_iterations, nes_conversions, nes_uploads, cached_draws, count };
struct TimingStats { uint64_t intervals{}, ticks{}, minimum{}, maximum{}; };
class FrontendTiming {
public:
    using Clock = uint64_t (*)(void*) noexcept;
    class Scope {
    public:
        Scope(FrontendTiming&, TimingStage) noexcept;
        ~Scope() { stop(); }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        void stop() noexcept;
    private:
        FrontendTiming* owner_{};
        TimingStage stage_{};
        uint64_t start_{};
    };
    void start(uint64_t frequency, Clock, void* context, bool unthrottled) noexcept;
    bool enabled() const noexcept { return enabled_; }
    void record(TimingCount kind, uint64_t amount=1) noexcept {
        if(enabled_) counts_[static_cast<unsigned>(kind)] += amount;
    }
    uint64_t count(TimingCount kind) const noexcept { return counts_[static_cast<unsigned>(kind)]; }
    const TimingStats& stats(TimingStage kind) const noexcept { return stages_[static_cast<unsigned>(kind)]; }
    uint64_t clock_reads() const noexcept { return clock_reads_; }
    uint64_t invalid_clock_samples() const noexcept { return invalid_clock_samples_; }
    // One bounded summary at exit; formatting/IO failure cannot change gameplay.
    bool report(std::ostream&, const char* reason, uint64_t legacy_frames) noexcept;
private:
    bool enabled_{}, unthrottled_{};
    uint64_t frequency_{}, session_start_{}, clock_reads_{}, invalid_clock_samples_{};
    Clock clock_{};
    void* context_{};
    std::array<uint64_t,static_cast<unsigned>(TimingCount::count)> counts_{};
    std::array<TimingStats,static_cast<unsigned>(TimingStage::count)> stages_{};
    uint64_t tick() noexcept;
    void interval(TimingStage, uint64_t begin, uint64_t end) noexcept;
};
}
