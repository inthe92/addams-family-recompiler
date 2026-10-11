#include "recomp/runtime_services.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

extern "C" uint8_t game_bank0_program(CPUState*, const RuntimeServices*, uint32_t*);
// Reads and writes carry absolute cycle offsets within each synthetic program.
struct Access { char kind; uint16_t address; uint8_t value; int cycle; };
struct Memory {
    std::array<uint8_t,65536> bytes{};
    std::vector<Access> trace;
    unsigned cycles{};
    RuntimeServices services() {
        return {this,
            [](void* p,uint16_t a,uint8_t offset) {auto& m=*static_cast<Memory*>(p);auto v=m.bytes[a];m.trace.push_back({'R',a,v,int(m.cycles+offset)});return v;},
            [](void* p,uint16_t a,uint8_t v,uint8_t offset) {auto& m=*static_cast<Memory*>(p);m.bytes[a]=v;m.trace.push_back({'W',a,v,int(m.cycles+offset)});},
            [](void*,uint16_t)->uint8_t {return 0;},
            [](void* p,uint8_t n) {static_cast<Memory*>(p)->cycles+=n;}, nullptr};
    }
};
static void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
static void execute(CPUState& cpu,Memory& memory,unsigned instructions) {
    auto services=memory.services();uint32_t left=instructions;
    for(unsigned attempts=0;left&&attempts<instructions;++attempts) {
        auto before=left;
        require(game_bank0_program(&cpu,&services,&left)&&left<before,"Program dispatch/progress");
    }
    require(left==0,"Instruction budget consumed");
}
static void trace(const Memory& m,const std::vector<Access>& expected) {
    require(m.trace.size()==expected.size(),"Program observable bus footprint");
    for(std::size_t i=0;i<expected.size();++i){auto a=m.trace[i],e=expected[i];
        require(a.kind==e.kind&&a.address==e.address&&a.value==e.value&&a.cycle==e.cycle,"Program ordered bus trace");}
}
int main() {
    Memory sub;auto cpu=cpu_initial_state(0x8000);
    cpu.A=0x80;cpu.X=0x17;cpu.Y=0x29;cpu.SP=0;cpu.P=0x6d;
    execute(cpu,sub,4);
    require(cpu.A==0x80&&cpu.X==0x17&&cpu.Y==0x29&&cpu.SP==0&&cpu.P==0xed&&cpu.PC==0x8003,"JSR/PHA/PLA/RTS state and stack wrap");
    require(sub.cycles==19,"JSR/PHA/PLA/RTS cycles");
    trace(sub,{{'W',0x100,0x80,3},{'W',0x1ff,2,4},{'W',0x1fe,0x80,8},
               {'R',0x1fe,0x80,12},{'R',0x1ff,2,16},{'R',0x100,0x80,17}});
    Memory interrupt;interrupt.bytes[0xfffe]=0;interrupt.bytes[0xffff]=0x83;
    cpu=cpu_initial_state(0x8100);cpu.A=0x33;cpu.X=0x17;cpu.Y=0x29;cpu.SP=0;cpu.P=0xe9;
    execute(cpu,interrupt,2);
    require(cpu.A==0x33&&cpu.X==0x17&&cpu.Y==0x29&&cpu.SP==0&&cpu.P==0xe9&&cpu.PC==0x8102,"BRK/RTI restore and B/U normalization");
    require(interrupt.cycles==13,"BRK/RTI cycles");
    trace(interrupt,{{'W',0x100,0x81,2},{'W',0x1ff,2,3},{'W',0x1fe,0xf9,4},
        {'R',0xfffe,0,5},{'R',0xffff,0x83,6},{'R',0x1fe,0xf9,10},{'R',0x1ff,2,11},{'R',0x100,0x81,12}});
    std::cout<<"2 synthetic native programs: stack wrap, JSR/RTS, BRK/RTI, status, ordered bus accesses and cycles: OK\n";
}
