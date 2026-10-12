#include "runtime/frontend_timing.hpp"
#include <algorithm>
#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>

namespace nes::frontend {
void FrontendTiming::start(uint64_t frequency,Clock clock,void* context,bool fast) noexcept {
    if(enabled_ || !frequency || !clock) return;
    frequency_=frequency;clock_=clock;context_=context;unthrottled_=fast;enabled_=true;
    session_start_=tick();
}
uint64_t FrontendTiming::tick() noexcept { ++clock_reads_;return clock_(context_); }
FrontendTiming::Scope::Scope(FrontendTiming& owner,TimingStage stage) noexcept : stage_(stage) {
    if(owner.enabled_){owner_=&owner;start_=owner.tick();}
}
void FrontendTiming::Scope::stop() noexcept {
    if(!owner_)return;
    owner_->interval(stage_,start_,owner_->tick());owner_=nullptr;
}
void FrontendTiming::interval(TimingStage stage,uint64_t begin,uint64_t end) noexcept {
    if(end<begin){++invalid_clock_samples_;return;}
    auto& s=stages_[static_cast<unsigned>(stage)];const auto duration=end-begin;
    s.minimum=s.intervals?std::min(s.minimum,duration):duration;
    s.maximum=std::max(s.maximum,duration);s.ticks+=duration;++s.intervals;
}
bool FrontendTiming::report(std::ostream& out,const char* reason,uint64_t legacy_frames) noexcept {
    if(!enabled_)return false;
    try {
        const auto end=tick();if(end<session_start_)++invalid_clock_samples_;
        const double wall=end>=session_start_?double(end-session_start_)/frequency_:0;
        static constexpr const char* names[]={"nes_frames_executed","ppu_frames_produced","native_slice_calls",
            "mod_hook_calls","god_hook_calls","diagnostic_frame_calls","diagnostic_slice_calls",
            "audio_batches","pcm_samples_collected","wav_batches","render_passes",
            "paused_render_passes","nes_draw_commands","ui_draw_commands","buffer_swaps_completed",
            "captures_completed","paused_iterations","nes_pixel_conversions","nes_texture_uploads","cached_nes_draws"};
        static constexpr const char* stages[]={"simulation","audio","render","swap","wait","capture","pixel_conversion","texture_upload"};
        static_assert(sizeof(names)/sizeof(*names)==static_cast<unsigned>(TimingCount::count));
        static_assert(sizeof(stages)/sizeof(*stages)==static_cast<unsigned>(TimingStage::count));
        std::ostringstream json;json.imbue(std::locale::classic());json<<std::setprecision(12);
        json<<"{\"schema\":1,\"unthrottled\":"<<(unthrottled_?"true":"false")
            <<",\"clock_frequency\":"<<frequency_<<",\"wall_seconds\":"<<wall
            <<",\"legacy_frame_units\":"<<legacy_frames<<",\"end_reason\":\"";
        // Only a host reason identifier, never a ROM path/error payload.
        for(const char* p=reason;*p;++p)if((*p>='a'&&*p<='z')||*p=='_')json<<*p;
        json<<"\",\"clock_reads\":"<<clock_reads_<<",\"invalid_clock_samples\":"<<invalid_clock_samples_<<",\"counts\":{";
        for(unsigned i=0;i<counts_.size();++i){if(i)json<<',';json<<'"'<<names[i]<<"\":"<<counts_[i];}
        json<<"},\"stages\":{";
        for(unsigned i=0;i<stages_.size();++i){if(i)json<<',';const auto&s=stages_[i];
            json<<'"'<<stages[i]<<"\":{\"intervals\":"<<s.intervals<<",\"ticks\":"<<s.ticks
                <<",\"seconds\":"<<double(s.ticks)/frequency_<<",\"min_ticks\":"<<s.minimum<<",\"max_ticks\":"<<s.maximum<<'}';}
        json<<"}}";out<<"Timing diagnostic: "<<json.str()<<'\n';return bool(out);
    } catch(...) {return false;}
}
}
