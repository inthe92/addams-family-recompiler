#include "runtime/frontend_timing.hpp"
#include "runtime/original_presentation.hpp"
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace nes::frontend;
struct Clock { uint64_t now{}, reads{};static uint64_t get(void* p) noexcept {auto&c=*static_cast<Clock*>(p);++c.reads;return c.now;} };
static void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
static unsigned metrics(){
    unsigned cases=0;Clock c;FrontendTiming off;
    {FrontendTiming::Scope s(off,TimingStage::simulation);off.record(TimingCount::nes_frames);}
    std::ostringstream output;require(!off.report(output,"end",0)&&output.str().empty()&&off.clock_reads()==0&&off.count(TimingCount::nes_frames)==0,"disabled diagnostics must not sample clock, count or write");++cases;
    FrontendTiming on;on.start(1000,Clock::get,&c,false);
    {FrontendTiming::Scope s(on,TimingStage::simulation);c.now=10;s.stop();c.now=20;s.stop();}
    require(on.stats(TimingStage::simulation).intervals==1&&on.stats(TimingStage::simulation).ticks==10&&c.reads==3,"exact duration, one-shot stop and clock reads");++cases;
    for(uint64_t dt:{0u,5u,15u}){FrontendTiming::Scope s(on,TimingStage::audio);c.now+=dt;on.record(TimingCount::audio_batches);on.record(TimingCount::pcm_samples,dt*48);}
    const auto&a=on.stats(TimingStage::audio);require(a.intervals==3&&a.ticks==20&&a.minimum==0&&a.maximum==15&&on.count(TimingCount::pcm_samples)==960,"aggregate without fixed frame-size assumption");++cases;
    on.record(TimingCount::nes_frames);on.record(TimingCount::nes_draws,5);on.record(TimingCount::ui_draws,2);on.record(TimingCount::swaps,3);on.record(TimingCount::paused_iterations,2);
    require(on.count(TimingCount::nes_frames)==1&&on.count(TimingCount::nes_draws)==5&&on.count(TimingCount::swaps)==3&&on.count(TimingCount::audio_batches)==3,"independent counters/pause must not imply a logical step");++cases;
    FrontendTiming invalid;invalid.start(0,Clock::get,&c,false);invalid.start(1000,nullptr,&c,false);require(!invalid.enabled(),"invalid clock is observational, never a new execution path");++cases;
    c.now=100;{FrontendTiming::Scope s(on,TimingStage::wait);c.now=99;}
    require(on.invalid_clock_samples()==1&&on.stats(TimingStage::wait).intervals==0,"backwards clock cannot underflow duration or reschedule");++cases;
    try{FrontendTiming::Scope s(on,TimingStage::capture);c.now+=7;throw std::runtime_error("test-only failure");}catch(const std::runtime_error&){}
    require(on.stats(TimingStage::capture).ticks==7&&on.count(TimingCount::captures)==0,"partial failed operation interval vs completion");++cases;
    std::ostringstream bad;bad.setstate(std::ios::badbit);require(!on.report(bad,"end",1),"IO failure must not escape into gameplay");++cases;
    {FrontendTiming::Scope s(on,TimingStage::conversion);c.now+=3;on.record(TimingCount::nes_conversions);}
    {FrontendTiming::Scope s(on,TimingStage::upload);c.now+=5;on.record(TimingCount::nes_uploads);}
    on.record(TimingCount::cached_draws,2);
    require(on.stats(TimingStage::conversion).ticks==3&&on.stats(TimingStage::upload).ticks==5&&on.count(TimingCount::nes_conversions)==1&&on.count(TimingCount::nes_uploads)==1&&on.count(TimingCount::cached_draws)==2,"conversion/upload/cache independent observational metrics");++cases;
    return cases;
}
static unsigned clocks(){
    unsigned cases=0;constexpr double frequency=1000000000,seconds=600;
    // TEST-ONLY two-stream merge: prospective acceptance oracle, not a shipped
    // scheduler. Uses the unchanged PAL deadline function and synthetic time.
    for(unsigned refresh:{60u,90u,120u}){
        Clock c;FrontendTiming timing;timing.start(uint64_t(frequency),Clock::get,&c,false);
        double nes_due=0;uint64_t steps=0,draws=0;
        for(uint64_t n=0;n<uint64_t(seconds*refresh);++n){
            const double now=n*frequency/refresh;c.now=uint64_t(now);
            while(nes_due<=now){timing.record(TimingCount::nes_frames);timing.record(TimingCount::god_hooks);timing.record(TimingCount::audio_batches);++steps;nes_due=nes::runtime::next_pal_deadline(nes_due,frequency,nes_due);}
            timing.record(TimingCount::nes_draws);timing.record(TimingCount::swaps);++draws;
        }
        // Observe the same end-of-session time for every display rate. The last
        // swap precedes 600s by different amounts and is not the NES cutoff.
        c.now=uint64_t(seconds*frequency);
        while(nes_due<seconds*frequency){timing.record(TimingCount::nes_frames);timing.record(TimingCount::god_hooks);timing.record(TimingCount::audio_batches);++steps;nes_due=nes::runtime::next_pal_deadline(nes_due,frequency,nes_due);}
        require(steps==30005&&draws==600*refresh&&timing.count(TimingCount::god_hooks)==steps&&timing.count(TimingCount::audio_batches)==steps,"fixed PAL step stream under a different presentation stream");++cases;
    }
    require(nes::runtime::next_pal_deadline(0,frequency,frequency)==frequency,"long-gap reanchor must not change");++cases;
    for(bool fast:{false,true}){
        Clock c;FrontendTiming t;t.start(1000,Clock::get,&c,fast);uint64_t legacy=0;
        while(legacy<5){t.record(TimingCount::nes_frames);++legacy;if(legacy==5)t.record(TimingCount::captures);if(!fast){FrontendTiming::Scope s(t,TimingStage::wait);c.now+=20;}
            for(int i=0;i<7;++i)t.record(TimingCount::nes_draws);}
        // Ten render-only pause turns must not consume --frames or hooks/audio.
        for(int i=0;i<10;++i){t.record(TimingCount::paused_iterations);t.record(TimingCount::nes_draws);t.record(TimingCount::swaps);}
        require(legacy==5&&t.count(TimingCount::nes_frames)==5&&t.count(TimingCount::captures)==1&&t.stats(TimingStage::wait).intervals==(fast?0u:5u),"legacy frame limit, capture anchor, unthrottled and pause contract");++cases;
    }
    return cases;
}
int main(int argc,char**argv){try{
    if(argc!=2)throw std::runtime_error("metrics|clock_contract|json required");const std::string mode=argv[1];
    if(mode=="json"){Clock c;FrontendTiming t;t.start(1000,Clock::get,&c,true);{FrontendTiming::Scope s(t,TimingStage::render);c.now=25;}t.record(TimingCount::nes_frames);t.record(TimingCount::nes_draws,3);return t.report(std::cout,"frame_limit",1)?0:1;}
    unsigned n=mode=="metrics"?metrics():mode=="clock_contract"?clocks():0;require(n!=0,"unknown mode");
    std::cout<<"Frontend timing "<<mode<<" cases="<<n<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
