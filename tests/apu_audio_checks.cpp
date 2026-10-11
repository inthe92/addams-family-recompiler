#include "runtime/audio_state.hpp"
#include "runtime/bus.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Synthetic-only recovery of the useful legacy audio checks, with extra
// independent NESdev contracts. See docs/APU_AUDIO_AUDIT.md for scope/sources.
// No private headers, ROM bytes, sound driver, or reference emulator dependency.
using namespace nes::runtime;
extern "C" uint8_t game_bank0_fixture(CPUState*, const RuntimeServices*, uint32_t*);
static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
static void until(AudioState& a, uint64_t cycle) {
    require(cycle >= a.cycles(), "Monotonic test schedule");
    a.advance_cpu_cycles(cycle-a.cycles());
}
struct Fixture {
    std::array<uint8_t,32768> prg{};
    std::array<uint8_t,8192> chr{};
    Fixture() { prg[0x7ffe]=0x23; prg[0x7fff]=0xc1; prg[0x4000]=0xa7; }
    std::unique_ptr<Bus> bus() const { return std::make_unique<Bus>(CartridgeData{prg.data(),prg.size(),chr.data(),chr.size()}); }
};
static void start_pulse(AudioState& a, unsigned channel, uint8_t control, uint16_t timer) {
    const uint16_t base=uint16_t(0x4000+4*channel);
    a.write_register(0x4015,3); a.write_register(base,control);
    a.write_register(base+1,8); // Explicitly disable sweep without overflow mute.
    a.write_register(base+2,uint8_t(timer)); a.write_register(base+3,uint8_t(timer>>8));
}
static void startup() {
    AudioState a;
    require(a.cycles()==0 && a.produced_samples()==0 && a.dropped_samples()==0, "Fresh clock/sample counters");
    require(a.read_status(0xff)==0x20 && !a.irq_pending() && !a.dmc_read_requested(), "Fresh disabled status and bit 5 open bus");
    require(a.counters().length==std::array<uint8_t,4>{} && a.counters().linear==0, "Fresh length/linear counters");
    require(a.take_samples().empty(), "Fresh sample queue");
    a.write_register(0x4011,0xff); require(a.levels().dmc==127,"Direct DAC is seven bits");
    const auto before=a.counters();
    for(uint16_t reg : {0x4009,0x400d,0x4014,0x4016}) a.write_register(reg,0xff);
    require(a.counters().length==before.length && a.levels().dmc==127,"Non-APU registers ignored by AudioState");
    AudioState fresh; require(fresh.levels().dmc==0 && fresh.cycles()==0,"Reconstruction starts a new session; NOT a soft-reset test");
}
static void lengths() {
    // NESdev length table, independently declared, all entries/all four channels.
    const uint8_t table[]={10,254,20,2,40,4,80,6,160,8,60,10,14,12,26,14,12,16,24,18,48,20,96,22,192,24,72,26,16,28,32,30};
    AudioState a;
    for(unsigned i=0;i<32;++i) {
        a.write_register(0x4015,0);
        for(uint16_t reg : {0x4003,0x4007,0x400b,0x400f}) a.write_register(reg,uint8_t(i<<3));
        require(a.counters().length==std::array<uint8_t,4>{},"Disabled high writes cannot load length");
        a.write_register(0x4015,15);
        require(a.read_status()==0,"Enable alone does not reload length");
        for(uint16_t reg : {0x4003,0x4007,0x400b,0x400f}) a.write_register(reg,uint8_t(i<<3));
        require(a.counters().length==std::array<uint8_t,4>{table[i],table[i],table[i],table[i]},"Independent length table");
        for(unsigned mask=0;mask<16;++mask) {
            a.write_register(0x4015,15);
            for(uint16_t reg : {0x4003,0x4007,0x400b,0x400f}) a.write_register(reg,uint8_t(i<<3));
            a.write_register(0x4015,uint8_t(mask));
            require(a.read_status(0xe0)==uint8_t(mask|0x20),"Each channel enable/disable and status mask");
        }
    }
}
static void frame4() {
    AudioState a; a.write_register(0x4015,15);
    for(uint16_t reg : {0x4003,0x4007,0x400b,0x400f}) a.write_register(reg,0);
    until(a,8312); require(a.counters().envelope[0]==0,"Before PAL quarter event");
    until(a,8313); require(a.counters().envelope[0]==15,"PAL quarter event 8313");
    until(a,16626); require(a.counters().length[0]==10,"Before PAL half event");
    until(a,16627); require(a.counters().length==std::array<uint8_t,4>{9,9,9,9},"PAL half event 16627");
    until(a,24939); require(a.counters().envelope[0]==13,"Third quarter event 24939");
    until(a,33251); require(!a.irq_pending(),"Before PAL frame IRQ");
    for(uint64_t c : {33252,33253,33254}) {
        until(a,c); require(a.irq_pending() && (a.read_status()&0x40),"Three PAL IRQ assertion clocks");
        // Current read-clear behavior is observed here, not certified at a race.
    }
    require(a.counters().length[0]==8 && a.counters().frame_cycle==0,"Final half event and wrap");
    a.advance_cpu_cycles(33252); a.write_register(0x4017,0x40);
    require(!a.irq_pending(),"IRQ inhibit clears frame IRQ immediately");
    a.advance_cpu_cycles(70000); require(!a.irq_pending(),"Inhibited frame IRQ stays clear");
}
static void frame5() {
    for(unsigned parity=0;parity<2;++parity) {
        AudioState a; a.advance_cpu_cycles(parity); a.write_register(0x4015,1); a.write_register(0x4003,0);
        a.write_register(0x4017,0x80); const unsigned delay=parity?4:3;
        a.advance_cpu_cycles(delay-1); require(a.counters().length[0]==10,"Reset delay not elapsed");
        a.advance_cpu_cycles(1); require(a.counters().length[0]==9 && a.counters().frame_cycle==0,"Delayed five-step quarter/half event");
        a.advance_cpu_cycles(33253); require(a.counters().length[0]==8 && !a.irq_pending(),"Five-step fourth position has no clock/IRQ");
        a.advance_cpu_cycles(8312); require(a.counters().length[0]==7,"Five-step final half event 41565");
        a.advance_cpu_cycles(1); require(a.counters().frame_cycle==0 && !a.irq_pending(),"Five-step wrap 41566");
    }
}
static void envelopes() {
    AudioState a; a.write_register(0x4015,11);
    for(uint16_t reg : {0x4000,0x4004,0x400c}) a.write_register(reg,2);
    for(uint16_t reg : {0x4003,0x4007,0x400f}) a.write_register(reg,0);
    until(a,8313); require(a.counters().envelope==std::array<uint8_t,3>{15,15,15},"All envelope restarts");
    until(a,24939); require(a.counters().envelope[0]==15,"Envelope divider period V+1");
    until(a,33253); require(a.counters().envelope[0]==14,"Fourth quarter decrements period-2 envelope");
    for(uint16_t reg : {0x4000,0x4004,0x400c}) a.write_register(reg,0x30);
    for(uint16_t reg : {0x4003,0x4007,0x400f}) a.write_register(reg,0);
    a.advance_cpu_cycles(8314+4*33254);
    require(a.counters().envelope==std::array<uint8_t,3>{15,15,15},"Looping envelopes continue in constant volume mode");
    require(a.counters().length==std::array<uint8_t,4>{10,10,0,10},"Length halt for pulse/noise");
}
static void pulse_waveforms() {
    // Waveform duty positions from NESdev; divider phase is the project's
    // deterministic bootstrap. This does not certify power-on phase on hardware.
    const uint8_t waveform[4][8]={{0,1,0,0,0,0,0,0},{0,1,1,0,0,0,0,0},{0,1,1,1,1,0,0,0},{1,0,0,1,1,1,1,1}};
    for(unsigned channel=0;channel<2;++channel) for(unsigned duty=0;duty<4;++duty) {
        AudioState a; start_pulse(a,channel,uint8_t((duty<<6)|0x3f),99);
        for(unsigned c=1;c<=3200;++c) {
            a.advance_cpu_cycles(1); const auto l=a.levels();
            const unsigned phase=(1+(c-1)/200)%8;
            require((channel?l.pulse2:l.pulse1)==15*waveform[duty][phase],"Pulse duty and 16*(timer+1) period");
        }
        const uint16_t base=uint16_t(0x4000+channel*4);
        a.write_register(base+2,7); a.write_register(base+3,0);
        require((channel?a.levels().pulse2:a.levels().pulse1)==0,"Pulse period below eight muted");
        a.write_register(base+1,0); a.write_register(base+2,0); a.write_register(base+3,4);
        require((channel?a.levels().pulse2:a.levels().pulse1)==0,"Sweep target overflow mutes disabled sweep");
    }
    for(unsigned channel=0;channel<2;++channel) {
        AudioState a; start_pulse(a,channel,0x3f,99); a.advance_cpu_cycles(100);
        a.write_register(uint16_t(0x4003+channel*4),0);
        a.advance_cpu_cycles(100);
        require((channel?a.levels().pulse2:a.levels().pulse1)==0,"High timer write resets duty phase");
        a.advance_cpu_cycles(1);
        require((channel?a.levels().pulse2:a.levels().pulse1)==15,"High timer write preserves divider's original expiry");
    }
}
static void sweep() {
    AudioState a;
    for(unsigned i=0;i<2;++i) { start_pulse(a,i,0x30,1000); a.write_register(uint16_t(0x4001+i*4),0x99); }
    until(a,16627); require(a.counters().pulse_timer==std::array<uint16_t,2>{499,500},"Negative sweep channel asymmetry");
    until(a,33253); require(a.counters().pulse_timer==std::array<uint16_t,2>{499,500},"Sweep divider waits one half event");
    until(a,33254+16627); require(a.counters().pulse_timer==std::array<uint16_t,2>{249,250},"Sweep divider P+1 half events");
}
static void triangle_waveform() {
    AudioState a; a.write_register(0x4015,4); a.write_register(0x4008,3);
    a.write_register(0x400a,9); a.write_register(0x400b,0);
    until(a,8312); require(a.counters().linear==0,"Linear reload waits for quarter clock");
    until(a,8313); require(a.counters().linear==3,"Triangle linear reload");
    const uint8_t first=a.levels().triangle; a.advance_cpu_cycles(320);
    require(a.levels().triangle==first,"32*(timer+1) CPU period");
    until(a,33253); require(a.counters().linear==0,"Triangle linear decrements to zero");
    const uint8_t held=a.levels().triangle; a.advance_cpu_cycles(1000);
    require(a.levels().triangle==held,"Halted triangle holds DAC, does not force zero");
    a.write_register(0x4008,0x83); a.write_register(0x400b,0); a.write_register(0x4017,0xc0);
    a.advance_cpu_cycles((a.cycles()&1)?4:3);
    require(a.counters().linear==3,"Five-step reset reloads linear counter");
    a.advance_cpu_cycles(50000); require(a.counters().length[2]==10 && a.counters().linear==3,"Triangle control halts length and preserves reload");
    a.write_register(0x4015,0); const uint8_t disabled=a.levels().triangle;
    a.advance_cpu_cycles(320); require(a.levels().triangle==disabled,"Channel disable freezes triangle DAC");
}
static void noise() {
    const uint16_t periods[]={4,8,14,30,60,88,118,148,188,236,354,472,708,944,1890,3778};
    for(unsigned mode=0;mode<2;++mode) for(unsigned index=0;index<16;++index) {
        AudioState a; a.write_register(0x4015,8); a.write_register(0x400c,0x3f);
        a.write_register(0x400e,uint8_t((mode<<7)|index)); a.write_register(0x400f,0);
        uint16_t expected=1;
        for(unsigned step=0;step<16;++step) {
            a.advance_cpu_cycles(step?periods[index]:1);
            const unsigned tap=mode?6:1;
            expected=uint16_t((expected>>1)|(((expected^(expected>>tap))&1)<<14));
            require(a.counters().noise_shift==expected,"Independent PAL LFSR taps/period");
            require(a.levels().noise==((expected&1)?0:15),"Noise LFSR output gate");
        }
    }
}
static void dmc_bits() {
    const unsigned periods[]={398,354,316,298,276,236,210,198,176,148,132,118,98,78,66,50};
    for(unsigned rate=0;rate<16;++rate) {
        AudioState a; a.write_register(0x4010,uint8_t(0x80|rate)); a.write_register(0x4011,64);
        a.write_register(0x4012,0xff); a.write_register(0x4013,0); a.write_register(0x4015,0x10);
        a.advance_cpu_cycles(3); require(!a.dmc_read_requested(),"Initial DMC request delay");
        a.advance_cpu_cycles(1); require(a.dmc_read_requested() && a.dmc_read_address()==0xffc0,"DMC start formula");
        a.complete_dmc_read(0xa7); require(a.read_status()==0x80 && a.irq_pending(),"DMC IRQ on fetch, retained by status read");
        const uint8_t values[]={66,68,70,68,66,68,66,68};
        for(unsigned bit=0;bit<8;++bit) {
            until(a,1+uint64_t(8+bit)*periods[rate]);
            require(a.levels().dmc==values[bit],"Independent LSB-first DMC deltas and all PAL rate entries");
        }
        a.write_register(0x4015,0); require(!a.irq_pending() && a.levels().dmc==68,"DMC disable retains DAC and clears IRQ");
    }
    for(uint8_t level : {0,1,126,127}) {
        AudioState a; a.write_register(0x4010,15); a.write_register(0x4011,level); a.write_register(0x4015,16);
        until(a,4); a.complete_dmc_read(level<2?0:0xff); until(a,751);
        require(a.levels().dmc==level,"DMC saturation preserves low DAC bit");
    }
}
static void dmc_wrap_loop() {
    AudioState a; a.write_register(0x4010,0xcf); a.write_register(0x4012,0xff);
    a.write_register(0x4013,4); a.write_register(0x4015,16);
    unsigned reads=0;
    for(unsigned cycle=0;cycle<27000 && reads<66;++cycle) {
        a.advance_cpu_cycles(1);
        if(a.dmc_read_requested()) {
            const auto expected=uint16_t(reads<64?0xffc0+reads:reads==64?0x8000:0xffc0);
            require(a.dmc_read_address()==expected,"DMC FFFF-to-8000 wrap and 65-byte loop restart");
            a.complete_dmc_read(0); ++reads;
        }
    }
    require(reads==66 && !a.irq_pending(),"Loop suppresses sample-end IRQ");
}
static void dmc_cancel() {
    AudioState a; a.write_register(0x4015,16); until(a,4);
    require(a.dmc_read_requested(),"Outstanding request");
    a.write_register(0x4015,0); require(!a.dmc_read_requested() && !(a.read_status()&16),"Explicit request cancel");
    bool rejected=false; try { a.complete_dmc_read(0); } catch(const std::logic_error&) { rejected=true; }
    require(rejected,"Unexpected DMC completion rejected");
    a.write_register(0x4010,0x8f); a.write_register(0x4015,16); a.advance_cpu_cycles(4); a.complete_dmc_read(0);
    require(a.irq_pending(),"Sample-end IRQ set"); a.write_register(0x4010,15);
    require(!a.irq_pending(),"Disabling DMC IRQ clears latch");
}
static void dma_irq() {
    for(unsigned parity=0;parity<2;++parity) {
        Fixture f; auto b=f.bus(); b->advance(parity);
        for(auto item : {std::pair<uint16_t,uint8_t>{0x4010,0x8f},{0x4012,0},{0x4013,0},{0x4015,16}}) {
            b->write8(item.first,item.second); b->advance(2);
        }
        b->advance(2); auto cpu=cpu_initial_state(0xc100); cpu.P=CPU_U; const auto start=b->cycles();
        require(b->boundary(cpu)==1 && b->dmc_dma_count()==1 && b->irq_count()==1 && cpu.PC==0xc123,"Synthetic mapped DMC fetch and IRQ vector");
        require(b->cycles()==start+3+parity+7 && b->audio().cycles()==b->cycles(),"Existing DMC boundary stall and IRQ share APU clock");
        require(b->ram().read(0x1fd)==0xc1 && b->ram().read(0x1fc)==0 && b->ram().read(0x1fb)==CPU_U,"Hardware IRQ stack");
        require(b->audio().counters().dmc_remaining==0,"DMA completed the single-byte sample");
    }
}
static void native_irq_masks() {
    for(unsigned opcode : {0x58,0x78,0x28,0x40}) {
        Fixture f; auto b=f.bus(); b->advance(33252); auto s=b->services();
        auto cpu=cpu_initial_state(uint16_t(0x8000+opcode*16));
        if(opcode==0x78) cpu.P=CPU_U;
        if(opcode==0x28) b->ram().write(0x1fe,CPU_U);
        if(opcode==0x40) {
            cpu.SP=0xfa; b->ram().write(0x1fb,CPU_U); b->ram().write(0x1fc,0); b->ram().write(0x1fd,0xc1);
        }
        uint32_t budget=1; require(game_bank0_fixture(&cpu,&s,&budget) && budget==0,"Existing generated C IRQ fixture executes");
        if(opcode==0x58 || opcode==0x28) {
            require(b->irq_count()==0 && !(cpu.P&CPU_I),"CLI/PLP retain old-I mask");
            cpu.PC=0x8ea0; budget=1;
            require(game_bank0_fixture(&cpu,&s,&budget) && b->irq_count()==1,"Following native NOP delivers APU IRQ");
        } else require(b->irq_count()==1 && cpu.PC==0xc123,"SEI old-I and RTI restored-I delivery");
    }
}
static void native_register_timing() {
    Fixture f; auto b=f.bus(); b->audio().write_register(0x4015,1); b->audio().write_register(0x4003,0);
    b->ram().write(0xf8,0x17); b->ram().write(0xf9,0x40); b->advance(8308);
    auto s=b->services(); auto cpu=cpu_initial_state(0x8810); cpu.A=0xc0; uint32_t budget=1;
    require(game_bank0_fixture(&cpu,&s,&budget) && budget==0,"Generated STA (zp,X) to frame register");
    require(b->audio_writes().size()==1 && b->audio_writes()[0].cycle==8313 && b->audio_writes()[0].address==0x4017,
        "Native six-cycle STA reaches APU on offset five");
    require(b->cycles()==8314 && b->audio().cycles()==8314 && b->audio().counters().envelope[0]==15,
        "Old quarter event occurs before timed frame-counter write; no double advancement");
    b->advance(3); require(b->audio().counters().length[0]==9,"Odd-clock frame reset completes at 8317");
    b=f.bus(); b->ram().write(0xf8,0x15); b->ram().write(0xf9,0x40); b->advance(33255);
    s=b->services(); cpu=cpu_initial_state(0x8b10); budget=1;
    require(game_bank0_fixture(&cpu,&s,&budget) && budget==0,"Generated LDA (zp),Y from status");
    require(cpu.A==0x40 && cpu.P==uint8_t(CPU_I|CPU_U) && cpu.PC==0x8b12 && cpu.SP==0xfd,
        "Status read in generated C sets expected CPU state");
    require(b->cycles()==33260 && b->audio().cycles()==33260 && !b->audio().irq_pending(),
        "Noncollision native status read acknowledges IRQ with exact total cycles");
}
static void pcm_determinism() {
    AudioState whole, chunked;
    for(auto* a : {&whole,&chunked}) {a->write_register(0x4017,0x40);start_pulse(*a,0,0x7f,99);}
    constexpr uint64_t cycles=1662607; whole.advance_cpu_cycles(cycles);
    std::vector<float> parts;
    while(chunked.cycles()<cycles) {
        chunked.advance_cpu_cycles(std::min<uint64_t>(997,cycles-chunked.cycles()));
        auto pcm=chunked.take_samples(); parts.insert(parts.end(),pcm.begin(),pcm.end());
    }
    auto pcm=whole.take_samples();
    require(pcm==parts && pcm.size()==cycles*48000*32/53203425,"Identical PCM independent of tick and drain batches; rational PAL sample count");
    require(pcm.size()==47999 && whole.dropped_samples()==0 && whole.take_samples().empty(),"Sample queue drain/count");
    double energy=0; for(float v:pcm){ require(std::isfinite(v) && std::abs(v)<=1,"Finite bounded samples"); energy+=v*v; }
    require(energy>10,"Pulse PCM has nonzero energy, NOT proof of original-song fidelity");
    whole.advance_cpu_cycles(1662642);
    const auto retained=whole.take_samples().size();
    std::cout<<"Queue retained="<<retained<<" dropped="<<whole.dropped_samples()<<" produced="<<whole.produced_samples()<<'\n';
    const auto expected_total=whole.cycles()*48000*32/53203425;
    require(retained==48000 && whole.produced_samples()==expected_total && whole.dropped_samples()==expected_total-47999-48000,
        "One-second APU queue cap reports overflow from rational sample count");
}
static void mixer_step() {
    AudioState a; a.advance_cpu_cycles(70); auto lead=a.take_samples();
    require(lead.size()==2 && lead[0]==0 && lead[1]==0,"Initial constant DAC filtered DC");
    a.write_register(0x4011,64); a.advance_cpu_cycles(70); auto out=a.take_samples();
    require(out.size()==2,"Two full post-write samples");
    const auto tnd=[](double d){return 159.79/(1/(15.0/8227+d/22638)+100);};
    const double delta=tnd(64)-tnd(0), pi=std::acos(-1.0);
    const double hp90=std::exp(-2*pi*90/48000), hp440=std::exp(-2*pi*440/48000), lp=1-std::exp(-2*pi*14000/48000);
    // Second sample ends at CPU clock 70; next two end at 104 and 139.
    // Analytical response of two DC-blocking filters and one low-pass to a
    // constant input step. No production lookup tables/filter helpers are used.
    const double first=delta*hp90*hp440;
    std::cout.precision(12);
    std::cout<<"Mixer first="<<out[0]<<" reference="<<first*lp*1.4<<" delta="<<delta<<'\n';
    // The declared 440 Hz production coefficient corresponds to approximately
    // 440.015 Hz. Allow 2e-6 absolute PCM error for these two samples; do not
    // treat this approximate filter model as measured analog hardware fidelity.
    require(std::abs(out[0]-first*lp*1.4)<2e-6,"Independent nonlinear mixer and first filtered step");
    const double second=delta*hp90*hp440*(hp90+hp440-1);
    require(std::abs(out[1]-lp*(second+(1-lp)*first)*1.4)<2e-6,"Independent second filtered step");
}

// Deliberately ordinary FAILing contracts, never WILL_FAIL/skip/xfail. The
// expected values below come from independent references, not current output.
static void status_open_bus() {
    Fixture f; auto b=f.bus(); b->write8(0x0010,0xa5); b->advance(1);
    const auto status=b->read8(0x4015), external=b->read8(0x5000);
    std::cout<<"$4015 status="<<unsigned(status)<<"; later open bus="<<unsigned(external)<<"; expected=165\n";
    require(status==0x20,"APU disabled status returns only retained bit 5");
    require(external==0xa5,"$4015 is internal: reading it must not overwrite external open bus (A01)");
}
static void status_irq_collision() {
    Fixture f; auto b=f.bus(); b->advance(33249);
    const auto status=b->read8(0x4015,3);
    std::cout<<"$4015 cycle="<<b->audio().cycles()<<" status="<<unsigned(status)<<" IRQ after read="<<b->audio().irq_pending()<<"; expected=1\n";
    require((status&0x40)!=0 && b->audio().cycles()==33252,"Exact first PAL IRQ assertion/read cycle");
    require(b->audio().irq_pending(),"Same-cycle frame IRQ assertion must survive status clearing (A02)");
}
static void frame_mode_delay() {
    AudioState a; until(a,33251); a.write_register(0x4017,0x80); a.advance_cpu_cycles(1);
    std::cout<<"$4017 write=33251; observe="<<a.cycles()<<" IRQ="<<a.irq_pending()<<"; old four-step IRQ expected=1 before delayed mode reset\n";
    require(a.irq_pending(),"Frame mode must remain four-step until delayed $4017 reset (A03)");
}
static void frame_mode_edges() {
    // Independent PAL landmarks from Mesen ApuFrameCounter: IRQ 33252,
    // quarter+half 33253 (four-step), final half 41565 (five-step).
    // Writes occur after the named CPU cycle; even writes reset after 3,
    // odd writes after 4. Do not derive expected events from runtime helpers.
    for(unsigned parity=0;parity<2;++parity) {
        const uint64_t write=33250+parity, reset=parity?33255:33253;
        AudioState forward; until(forward,write);
        forward.write_register(0x4015,1); forward.write_register(0x4003,0);
        forward.write_register(0x4017,0x80);
        require(forward.counters().frame_cycle==write && forward.counters().length[0]==10,
            "Mode write does not immediately reset or clock units");
        until(forward,33252); require(forward.irq_pending(),"Old four-step IRQ survives pending five-step mode");
        until(forward,reset-1);
        require(forward.counters().frame_cycle==(parity?0:33252) && forward.counters().length[0]==(parity?9:10),
            "Old PAL wrap/half events continue until delayed reset");
        until(forward,reset);
        require(forward.counters().frame_cycle==0 && forward.counters().length[0]==(parity?8:9),
            "Exact parity delay applies five-step mode and clocks one half frame");
        require(forward.irq_pending(),"Mode reset alone does not acknowledge existing IRQ");
        until(forward,reset+1); forward.read_status(); // Away from A02 collision.
        until(forward,reset+33254);
        require(!forward.irq_pending() && forward.counters().frame_cycle==33254,
            "Delayed new five-step mode has no four-step IRQ or wrap");

        AudioState reverse; reverse.write_register(0x4017,0x80); until(reverse,3);
        const uint64_t reverse_write=41566+parity, reverse_reset=parity?41571:41569;
        until(reverse,reverse_write); reverse.write_register(0x4015,1); reverse.write_register(0x4003,0);
        reverse.write_register(0x4017,0);
        until(reverse,41568); // Five-step reset at 3 + final half at 41565.
        require(reverse.counters().length[0]==9 && !reverse.irq_pending(),
            "Old five-step final half survives pending four-step mode");
        until(reverse,reverse_reset-1); require(reverse.counters().length[0]==9,"No early four-step reset clock");
        until(reverse,reverse_reset);
        require(reverse.counters().frame_cycle==0 && reverse.counters().length[0]==9,
            "Four-step reset occurs on exact parity delay without forced clocks");
        until(reverse,reverse_reset+33252); require(reverse.irq_pending(),"New four-step IRQ uses restarted PAL schedule");

        AudioState inhibit; until(inhibit,33252+parity);
        require(inhibit.irq_pending(),"Existing frame IRQ before inhibit write");
        inhibit.write_register(0x4017,0xc0);
        require(!inhibit.irq_pending(),"Bit 6 clears IRQ immediately, independently of pending mode");
        inhibit.advance_cpu_cycles(parity?4:3);
        require(!inhibit.irq_pending() && inhibit.counters().frame_cycle==0,
            "Immediate inhibit prevents old assertions throughout reset delay");

        AudioState release; release.write_register(0x4017,0x40); until(release,write+3);
        release.write_register(0x4017,0x80);
        until(release,33255); // Old inhibited four-step origin was reset at 3.
        require(release.irq_pending(),"Clearing inhibit takes effect immediately while mode remains pending");
        std::cout<<"frame_mode_edges: parity="<<parity<<" forward reset="<<reset
                 <<" reverse reset="<<reverse_reset<<"; four boundary schedules checked\n";
    }
}
int main(int argc,char** argv) {
    const std::vector<std::pair<const char*,std::function<void()>>> cases={
        {"startup",startup},{"lengths",lengths},{"frame4",frame4},{"frame5",frame5},
        {"envelopes",envelopes},{"pulse_waveforms",pulse_waveforms},{"sweep",sweep},
        {"triangle",triangle_waveform},{"noise",noise},{"dmc_bits",dmc_bits},
        {"dmc_wrap_loop",dmc_wrap_loop},{"dmc_cancel",dmc_cancel},{"dma_irq",dma_irq},
        {"native_irq_masks",native_irq_masks},{"native_register_timing",native_register_timing},
        {"pcm_determinism",pcm_determinism},{"mixer_step",mixer_step},
        {"status_open_bus",status_open_bus},{"status_irq_collision",status_irq_collision},{"frame_mode_delay",frame_mode_delay},
        {"frame_mode_edges",frame_mode_edges}};
    const std::string selection=argc==2?argv[1]:"core";
    unsigned passed=0,failed=0;
    for(unsigned i=0;i<cases.size();++i) {
        if(selection=="core" ? i>=17 : selection!=cases[i].first) continue;
        try {cases[i].second(); ++passed; std::cout<<cases[i].first<<": PASS\n";}
        catch(const std::exception& e) {++failed; std::cerr<<cases[i].first<<": FAIL: "<<e.what()<<'\n';}
    }
    if(!passed && !failed){std::cerr<<"Unknown APU test selection\n";return 2;}
    std::cout<<"APU cases executed="<<passed+failed<<" passed="<<passed<<" failed="<<failed<<'\n';
    return failed?1:0;
}
