#include "runtime/bus.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

extern "C" uint8_t game_bank0_timing(CPUState*, const RuntimeServices*, uint32_t*);
using nes::runtime::Bus;
struct ReadTrace {
    Bus* bus;
    unsigned count{};
    uint64_t cycle{};
    uint8_t before{},after{},value{};
};
static void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
static bool check(uint64_t start,bool warm) {
    std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};
    // Video buffers live on the heap, including on Windows' smaller stack.
    auto native_storage=std::make_unique<Bus>(nes::runtime::CartridgeData{prg.data(),prg.size(),chr.data(),chr.size()});
    auto reference_storage=std::make_unique<Bus>(nes::runtime::CartridgeData{prg.data(),prg.size(),chr.data(),chr.size()});
    auto& native=*native_storage;auto& reference=*reference_storage;
    for(auto* bus:{&native,&reference}) {
        if(warm) {
            bus->advance(33300);
            (void)bus->video().read_register(0x2002);
            // Leave the address latch halfway through a write pair.
            bus->video().write_register(0x2006,0x21);
            // Set open-bus low bits to zero without changing the write toggle.
            bus->video().write_register(0x2003,0);
        }
        bus->advance(start-bus->cycles());
        require(bus->video().status()==0,"Synthetic pre-vblank setup");
    }
    ReadTrace observed{&native};
    RuntimeServices services{&observed,
        [](void* p,uint16_t a,uint8_t offset)->uint8_t {
            auto& t=*static_cast<ReadTrace*>(p);require(a==0x2002,"Unexpected generated read");
            ++t.count;t.cycle=t.bus->cycles()+offset;
            t.value=t.bus->read8(a,offset);
            // Returned high bits expose the pre-clear status without adding
            // a second bus access or synchronizing outside the real read.
            t.before=uint8_t(t.value&0xe0);t.after=t.bus->video().status();
            require(t.bus->video().dots()==(t.cycle*16u)/5u,"PAL device time at read");
            require(t.bus->audio().cycles()==t.cycle,"APU device time at read");
            return t.value;
        },
        [](void*,uint16_t,uint8_t,uint8_t){throw std::runtime_error("LDA wrote the bus");},
        [](void*,uint16_t)->uint8_t{return 0;},
        [](void* p,uint8_t n){static_cast<ReadTrace*>(p)->bus->advance(n);},nullptr};
    auto cpu=cpu_initial_state(0x8000);cpu.A=0x5a;cpu.X=0x17;cpu.Y=0x81;cpu.SP=0xfe;cpu.P=0x6d;
    uint32_t left=1;
    require(game_bank0_timing(&cpu,&services,&left)&&left==0,"Generated LDA dispatch");
    // Independent scheduling expectation: absolute LDA reads at zero-based
    // offset 3, takes four CPU cycles. Same devices isolate the ABI discrepancy.
    reference.advance(3);const auto reference_cycle=reference.cycles();
    const auto before=reference.video().status(),value=reference.read8(0x2002),after=reference.video().status();
    reference.advance(1);
    require(reference_cycle==start+3&&before==0x80&&value==0x80&&after==0,"Timed reference value/side effects");
    require(native.cycles()==start+4&&reference.cycles()==start+4,"LDA total cycles");
    require(observed.count==1&&cpu.X==0x17&&cpu.Y==0x81&&cpu.SP==0xfe&&cpu.PC==0x8003,"LDA final unchanged registers/PC");
    const auto expected_p=uint8_t((0x6d&0x7d)|(value==0?2:0)|(value>=128?128:0));
    const bool matches=observed.cycle==reference_cycle&&observed.value==value&&observed.after==after&&
        native.video().status()==reference.video().status()&&cpu.A==value&&cpu.P==expected_p;
    if(warm)for(auto* bus:{&native,&reference}) {
        bus->video().write_register(0x2006,0x23);bus->video().write_register(0x2006,0x45);
        require(bus->video().address()==0x2345,"PPUSTATUS must reset address write latch");
    }
    std::cout<<"start="<<start<<" read_cycle="<<observed.cycle<<" expected="<<reference_cycle
        <<" A="<<unsigned(cpu.A)<<" expected_A="<<unsigned(value)
        <<" P="<<unsigned(cpu.P)<<" expected_P="<<unsigned(expected_p)
        <<" read_status="<<unsigned(observed.before)<<"->"<<unsigned(observed.after)
        <<" expected_status="<<unsigned(before)<<"->"<<unsigned(after)
        <<" final_status="<<unsigned(native.video().status())<<" expected="<<unsigned(reference.video().status())
        <<" consumed=4/4"<<(warm?"; latch reset checked":"")<<'\n';
    return matches;
}
int main() {
    try {
        const bool first=check(25680,false),second=check(58927,true);
        if(first&&second){std::cout<<"2 PPUSTATUS timing regressions: PASS\n";return 0;}
        std::cerr<<"PPUSTATUS timing regression FAILED: first="<<first<<" warm="<<second<<'\n';
        return 1;
    } catch(const std::exception& e){std::cerr<<"Diagnostic error: "<<e.what()<<'\n';return 2;}
}
