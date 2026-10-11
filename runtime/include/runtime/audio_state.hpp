#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace nes::runtime {
struct AudioLevels { uint8_t pulse1, pulse2, triangle, noise, dmc; };
struct AudioCounters {
    std::array<uint8_t,4> length;
    std::array<uint8_t,3> envelope;
    uint8_t linear;
    uint16_t noise_shift;
    uint16_t dmc_address, dmc_remaining;
    uint32_t frame_cycle;
    std::array<uint16_t,2> pulse_timer;
};
// Native synthesis driven by translated game writes and the PAL runtime clock.
// There is no CPU, instruction decoder, ROM file loader or third-party NES core.
class AudioState {
public:
    static constexpr unsigned sample_rate = 48000;
    AudioState();
    void write_register(uint16_t address, uint8_t value);
    uint8_t read_status(uint8_t open_bus = 0);
    void advance_cpu_cycles(uint64_t count);
    bool irq_pending() const noexcept { return frame_irq_ || dmc_irq_; }
    bool dmc_read_requested() const noexcept { return dmc_request_; }
    uint16_t dmc_read_address() const noexcept { return dmc_current_address_; }
    void complete_dmc_read(uint8_t value);
    std::vector<float> take_samples();
    AudioLevels levels() const noexcept;
    AudioCounters counters() const noexcept;
    uint64_t cycles() const noexcept { return cycles_; }
    uint64_t produced_samples() const noexcept { return produced_samples_; }
    uint64_t dropped_samples() const noexcept { return dropped_samples_; }

private:
    struct Envelope {
        uint8_t control{}, divider{}, decay{}; bool start{};
        void clock();
        uint8_t volume() const noexcept { return (control & 0x10u) ? static_cast<uint8_t>(control & 15u) : decay; }
    };
    struct Pulse {
        Envelope envelope;
        uint16_t timer{}, divider{};
        uint8_t length{}, phase{}, sweep{}, sweep_divider{};
        bool enabled{}, sweep_reload{};
        int sweep_target(unsigned channel) const noexcept;
        bool muted(unsigned channel) const noexcept;
        void clock_timer();
        void clock_sweep(unsigned channel);
        uint8_t output(unsigned channel) const noexcept;
    };
    std::array<Pulse,2> pulses_{};
    Envelope noise_envelope_;
    uint16_t triangle_timer_{}, triangle_divider_{}, noise_divider_{}, noise_shift_{1};
    uint8_t triangle_control_{}, triangle_length_{}, triangle_linear_{}, triangle_phase_{};
    uint8_t noise_period_{}, noise_length_{};
    bool triangle_enabled_{}, triangle_reload_{}, noise_enabled_{};
    uint8_t dmc_control_{}, dmc_level_{}, dmc_address_{}, dmc_length_{};
    uint16_t dmc_divider_{}, dmc_current_address_{0xC000}, dmc_remaining_{};
    uint8_t dmc_buffer_{}, dmc_shift_{}, dmc_bits_{8}, dmc_start_delay_{};
    bool dmc_buffer_full_{}, dmc_silence_{true}, dmc_request_{}, dmc_irq_{};
    uint8_t frame_control_{}, frame_pending_mode_{}, frame_reset_delay_{};
    uint32_t frame_cycle_{};
    bool frame_irq_{};
    uint64_t cycles_{}, produced_samples_{}, dropped_samples_{}, sample_accumulator_{};
    uint32_t sample_cycles_{};
    double sample_sum_{}, hp90_previous_input_{}, hp90_previous_output_{};
    double hp440_previous_input_{}, hp440_previous_output_{}, lowpass_previous_{};
    bool filter_initialized_{};
    std::array<float,31> pulse_mix_{};
    std::array<float,16*16*128> tnd_mix_{};
    std::vector<float> samples_;
    void tick();
    void quarter_frame();
    void half_frame();
    void clock_frame_counter();
    void clock_dmc();
    void request_dmc_read();
    void restart_dmc();
    float mix() const noexcept;
    float filter(double sample);
};
}
