#include "runtime/audio_state.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nes::runtime {
namespace {
constexpr std::array<uint8_t,32> lengths = {10,254,20,2,40,4,80,6,160,8,60,10,14,12,26,14,12,16,24,18,48,20,96,22,192,24,72,26,16,28,32,30};
constexpr std::array<uint16_t,16> noise_periods = {4,8,14,30,60,88,118,148,188,236,354,472,708,944,1890,3778};
constexpr std::array<uint16_t,16> dmc_periods = {398,354,316,298,276,236,210,198,176,148,132,118,98,78,66,50};
constexpr uint8_t duty[4][8] = {{0,1,0,0,0,0,0,0},{0,1,1,0,0,0,0,0},{0,1,1,1,1,0,0,0},{1,0,0,1,1,1,1,1}};
constexpr uint8_t triangle[32] = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
}
AudioState::AudioState() {
    samples_.reserve(sample_rate);
    for(unsigned p=1;p<pulse_mix_.size();++p) pulse_mix_[p]=static_cast<float>(95.88/(8128.0/p+100.0));
    for(unsigned t=0;t<16;++t)for(unsigned n=0;n<16;++n)for(unsigned d=0;d<128;++d) {
        const double sum=t/8227.0+n/12241.0+d/22638.0;
        tnd_mix_[t*2048u+n*128u+d]=sum==0?0:static_cast<float>(159.79/(1.0/sum+100.0));
    }
}
void AudioState::Envelope::clock() {
    if(start) {start=false;decay=15;divider=static_cast<uint8_t>(control&15u);}
    else if(divider) --divider;
    else {divider=static_cast<uint8_t>(control&15u);if(decay)--decay;else if(control&0x20u)decay=15;}
}
int AudioState::Pulse::sweep_target(unsigned channel) const noexcept {
    const int change=timer>>(sweep&7u);
    const int target=(sweep&8u)?static_cast<int>(timer)-change-(channel==0?1:0):static_cast<int>(timer)+change;
    return std::max(0,target);
}
bool AudioState::Pulse::muted(unsigned channel) const noexcept {return timer<8||sweep_target(channel)>0x7FF;}
void AudioState::Pulse::clock_timer() {
    if(divider)--divider;else {divider=timer;phase=static_cast<uint8_t>((phase+1u)&7u);}
}
void AudioState::Pulse::clock_sweep(unsigned channel) {
    if(!sweep_divider&&(sweep&0x80u)&&(sweep&7u)&&!muted(channel))timer=static_cast<uint16_t>(sweep_target(channel));
    if(!sweep_divider||sweep_reload){sweep_divider=static_cast<uint8_t>((sweep>>4)&7u);sweep_reload=false;}
    else --sweep_divider;
}
uint8_t AudioState::Pulse::output(unsigned channel) const noexcept {
    return length&&!muted(channel)&&duty[envelope.control>>6][phase]?envelope.volume():0;
}
void AudioState::restart_dmc() {
    dmc_current_address_=static_cast<uint16_t>(0xC000u+static_cast<unsigned>(dmc_address_)*64u);
    dmc_remaining_=static_cast<uint16_t>(static_cast<unsigned>(dmc_length_)*16u+1u);
}
void AudioState::request_dmc_read() {
    if(!dmc_buffer_full_&&dmc_remaining_&&!dmc_start_delay_)dmc_request_=true;
}
void AudioState::complete_dmc_read(uint8_t value) {
    if(!dmc_request_||!dmc_remaining_||dmc_buffer_full_)throw std::logic_error("DMC read without an outstanding request");
    dmc_request_=false;dmc_buffer_=value;dmc_buffer_full_=true;
    dmc_current_address_=dmc_current_address_==0xFFFF?0x8000:static_cast<uint16_t>(dmc_current_address_+1u);
    if(--dmc_remaining_==0){if(dmc_control_&0x40u)restart_dmc();else if(dmc_control_&0x80u)dmc_irq_=true;}
}
void AudioState::write_register(uint16_t address,uint8_t value) {
    if(address>=0x4000&&address<=0x4007) {
        auto& p=pulses_[(address>>2)&1u];
        switch(address&3u) {
            case 0:p.envelope.control=value;break;
            case 1:p.sweep=value;p.sweep_reload=true;break;
            case 2:p.timer=static_cast<uint16_t>((p.timer&0x700u)|value);break;
            case 3:p.timer=static_cast<uint16_t>((p.timer&0xFFu)|((value&7u)<<8));
                if(p.enabled)p.length=lengths[value>>3];p.phase=0;p.envelope.start=true;break;
        }
        return;
    }
    switch(address) {
        case 0x4008:triangle_control_=value;break;
        case 0x400A:triangle_timer_=static_cast<uint16_t>((triangle_timer_&0x700u)|value);break;
        case 0x400B:triangle_timer_=static_cast<uint16_t>((triangle_timer_&0xFFu)|((value&7u)<<8));
            if(triangle_enabled_)triangle_length_=lengths[value>>3];triangle_reload_=true;break;
        case 0x400C:noise_envelope_.control=value;break;
        case 0x400E:noise_period_=value;break;
        case 0x400F:if(noise_enabled_)noise_length_=lengths[value>>3];noise_envelope_.start=true;break;
        case 0x4010:dmc_control_=value;if(!(value&0x80u))dmc_irq_=false;break;
        case 0x4011:dmc_level_=static_cast<uint8_t>(value&0x7Fu);break;
        case 0x4012:dmc_address_=value;break;
        case 0x4013:dmc_length_=value;break;
        case 0x4015:
            for(unsigned i=0;i<2;++i){pulses_[i].enabled=(value&(1u<<i))!=0;if(!pulses_[i].enabled)pulses_[i].length=0;}
            triangle_enabled_=(value&4u)!=0;noise_enabled_=(value&8u)!=0;
            if(!triangle_enabled_)triangle_length_=0;if(!noise_enabled_)noise_length_=0;
            if(!(value&0x10u)){dmc_remaining_=0;dmc_request_=false;dmc_start_delay_=0;}
            else if(!dmc_remaining_){restart_dmc();dmc_start_delay_=static_cast<uint8_t>((cycles_&1u)?3:4);}
            dmc_irq_=false;break;
        case 0x4017:
            frame_control_=value;if(value&0x40u)frame_irq_=false;
            frame_reset_delay_=static_cast<uint8_t>((cycles_&1u)?4:3);break;
        default:break;
    }
}
uint8_t AudioState::read_status(uint8_t open_bus) {
    uint8_t result=static_cast<uint8_t>(open_bus&0x20u);
    if(pulses_[0].length)result=static_cast<uint8_t>(result|1u);
    if(pulses_[1].length)result=static_cast<uint8_t>(result|2u);
    if(triangle_length_)result=static_cast<uint8_t>(result|4u);
    if(noise_length_)result=static_cast<uint8_t>(result|8u);
    if(dmc_remaining_)result=static_cast<uint8_t>(result|0x10u);
    if(frame_irq_)result=static_cast<uint8_t>(result|0x40u);
    if(dmc_irq_)result=static_cast<uint8_t>(result|0x80u);
    frame_irq_=false;return result;
}
void AudioState::quarter_frame() {
    for(auto& p:pulses_)p.envelope.clock();noise_envelope_.clock();
    if(triangle_reload_)triangle_linear_=static_cast<uint8_t>(triangle_control_&0x7Fu);
    else if(triangle_linear_)--triangle_linear_;
    if(!(triangle_control_&0x80u))triangle_reload_=false;
}
void AudioState::half_frame() {
    for(unsigned i=0;i<2;++i){auto& p=pulses_[i];if(p.length&&!(p.envelope.control&0x20u))--p.length;p.clock_sweep(i);}
    if(triangle_length_&&!(triangle_control_&0x80u))--triangle_length_;
    if(noise_length_&&!(noise_envelope_.control&0x20u))--noise_length_;
}
void AudioState::clock_frame_counter() {
    ++frame_cycle_;
    if(frame_reset_delay_&&--frame_reset_delay_==0){frame_cycle_=0;if(frame_control_&0x80u){quarter_frame();half_frame();}return;}
    if(frame_cycle_==8313||frame_cycle_==24939)quarter_frame();
    else if(frame_cycle_==16627){quarter_frame();half_frame();}
    if(frame_control_&0x80u){if(frame_cycle_==41565){quarter_frame();half_frame();}else if(frame_cycle_==41566)frame_cycle_=0;}
    else {
        if(frame_cycle_>=33252&&frame_cycle_<=33254&&!(frame_control_&0x40u))frame_irq_=true;
        if(frame_cycle_==33253){quarter_frame();half_frame();}
        if(frame_cycle_==33254)frame_cycle_=0;
    }
}
void AudioState::clock_dmc() {
    if(dmc_divider_){--dmc_divider_;return;}
    dmc_divider_=static_cast<uint16_t>(dmc_periods[dmc_control_&15u]/2u-1u);
    if(!dmc_silence_){if(dmc_shift_&1u){if(dmc_level_<=125)dmc_level_=static_cast<uint8_t>(dmc_level_+2u);}
        else if(dmc_level_>=2)dmc_level_=static_cast<uint8_t>(dmc_level_-2u);}
    dmc_shift_=static_cast<uint8_t>(dmc_shift_>>1);
    if(--dmc_bits_==0){dmc_bits_=8;dmc_silence_=!dmc_buffer_full_;if(dmc_buffer_full_){dmc_shift_=dmc_buffer_;dmc_buffer_full_=false;}request_dmc_read();}
}
AudioLevels AudioState::levels() const noexcept {
    return {pulses_[0].output(0),pulses_[1].output(1),triangle[triangle_phase_],
        static_cast<uint8_t>(noise_length_&&!(noise_shift_&1u)?noise_envelope_.volume():0),dmc_level_};
}
AudioCounters AudioState::counters() const noexcept {
    return {{{pulses_[0].length,pulses_[1].length,triangle_length_,noise_length_}},
        {{pulses_[0].envelope.decay,pulses_[1].envelope.decay,noise_envelope_.decay}},triangle_linear_,
        noise_shift_,dmc_current_address_,dmc_remaining_,frame_cycle_,{{pulses_[0].timer,pulses_[1].timer}}};
}
float AudioState::mix() const noexcept {
    const auto l=levels();return pulse_mix_[l.pulse1+l.pulse2]+tnd_mix_[static_cast<unsigned>(l.triangle)*2048u+static_cast<unsigned>(l.noise)*128u+l.dmc];
}
float AudioState::filter(double sample) {
    // The host presentation filter is separate from the channel state. First-
    // order90Hz/440Hz high-pass and14kHz low-pass remove DC and ultrasonic edges.
    if(!filter_initialized_){filter_initialized_=true;hp90_previous_input_=sample;}
    constexpr double hp90=0.9882881515,hp440=0.9440294903,lp14k=0.8400022802;
    const double first=hp90*(hp90_previous_output_+sample-hp90_previous_input_);
    hp90_previous_input_=sample;hp90_previous_output_=first;
    const double second=hp440*(hp440_previous_output_+first-hp440_previous_input_);
    hp440_previous_input_=first;hp440_previous_output_=second;
    lowpass_previous_+=lp14k*(second-lowpass_previous_);
    return static_cast<float>(std::clamp(lowpass_previous_*1.4,-1.0,1.0));
}
void AudioState::tick() {
    clock_frame_counter();
    if(triangle_divider_)--triangle_divider_;
    else {triangle_divider_=triangle_timer_;if(triangle_length_&&triangle_linear_)triangle_phase_=static_cast<uint8_t>((triangle_phase_+1u)&31u);}
    if(!(cycles_&1u)) {
        for(auto& p:pulses_)p.clock_timer();
        if(noise_divider_)--noise_divider_;
        else {noise_divider_=static_cast<uint16_t>(noise_periods[noise_period_&15u]/2u-1u);
            const unsigned tap=(noise_period_&0x80u)?6:1;
            const uint16_t feedback=static_cast<uint16_t>((noise_shift_^(noise_shift_>>tap))&1u);
            noise_shift_=static_cast<uint16_t>((noise_shift_>>1)|(feedback<<14));}
        clock_dmc();
    }
    if(dmc_start_delay_)--dmc_start_delay_;
    request_dmc_read();
    sample_sum_+=mix();++sample_cycles_;
    // PAL CPU clock is53203425/32Hz, avoiding a rounded-Hz resampler drift.
    sample_accumulator_+=static_cast<uint64_t>(sample_rate)*32u;
    if(sample_accumulator_>=53203425u) {
        sample_accumulator_-=53203425u;const float output=filter(sample_sum_/sample_cycles_);
        sample_sum_=0;sample_cycles_=0;++produced_samples_;
        if(samples_.size()<sample_rate)samples_.push_back(output);else ++dropped_samples_;
    }
    ++cycles_;
}
void AudioState::advance_cpu_cycles(uint64_t count) {for(uint64_t i=0;i<count;++i)tick();}
std::vector<float> AudioState::take_samples() {std::vector<float> output;samples_.swap(output);return output;}
}
