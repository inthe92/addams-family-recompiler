#include "runtime/presentation_clock.hpp"
#include <cmath>
#include <atomic>
#include <stdexcept>
#include <string>

namespace nes::frontend {
uint64_t FrameSnapshot::next_publication() noexcept {
    // Unique even when a stream/sequence is restarted or an object address is
    // reused. Copying an immutable publication legitimately retains its ID.
    static std::atomic<uint64_t> serial{0};
    return serial.fetch_add(1,std::memory_order_relaxed)+1;
}
unsigned parse_experimental_refresh(const char* value) {
    const std::string text=value?value:"";
    if(text=="60")return 60;if(text=="90")return 90;if(text=="120")return 120;
    throw std::invalid_argument("--experimental-present-hz requires 60, 90 or 120");
}
PresentationClock::PresentationClock(uint64_t frequency,unsigned refresh,uint64_t now)
    :frequency_(static_cast<double>(frequency)),draw_period_(refresh?frequency_/refresh:0),
     nes_deadline_(static_cast<double>(now)),draw_deadline_(static_cast<double>(now)),last_now_(now) {
    if(!frequency || (refresh!=60 && refresh!=90 && refresh!=120))
        throw std::invalid_argument("Invalid experimental presentation clock configuration");
}
double PresentationClock::monotonic(uint64_t now) noexcept {
    last_now_=std::max(last_now_,now);return static_cast<double>(last_now_);
}
PresentationClock::Due PresentationClock::poll(uint64_t now) noexcept {
    const double tick=monotonic(now);
    // Same 100 ms host-lateness threshold as original pacing. No NES cycles
    // are skipped/added: resume with one normal unit, not a suspension backlog.
    if(!paused_ && tick>nes_deadline_+frequency_/10.0){nes_deadline_=tick;++reanchors_;}
    return {!paused_ && tick>=nes_deadline_,tick>=draw_deadline_};
}
void PresentationClock::simulation_complete(uint64_t now) noexcept {
    nes_deadline_=nes::runtime::next_pal_deadline(nes_deadline_,frequency_,monotonic(now));
}
void PresentationClock::presentation_complete(uint64_t now) noexcept {
    const double tick=monotonic(now);
    // Missed display opportunities are discarded, never burst-replayed. Even
    // forced final capture draws leave the next opportunity in the future.
    const double missed=std::max(0.0,std::floor((tick-draw_deadline_)/draw_period_));
    draw_deadline_+=(missed+1.0)*draw_period_;
}
void PresentationClock::set_paused(bool paused,uint64_t now) noexcept {
    const double tick=monotonic(now);
    if(paused_==paused)return;
    paused_=paused;
    if(!paused_){nes_deadline_=tick;draw_deadline_=tick;}
}
double PresentationClock::next_deadline() const noexcept {
    return paused_?draw_deadline_:std::min(nes_deadline_,draw_deadline_);
}
}
