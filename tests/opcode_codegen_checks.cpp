#include "recomp/runtime_services.h"
#include "opcode_fixtures.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

extern "C" uint8_t game_bank0_fixture(CPUState*,const RuntimeServices*,uint32_t*);
// Every represented native read and write has an instruction-relative offset.
struct Access { char kind; uint16_t address; uint8_t value; int offset; };
struct Write { uint16_t address; uint8_t value; uint8_t cycle; };
struct FixtureBus {
    std::array<uint8_t,65536> memory;
    std::vector<Write> writes;
    std::vector<Access> accesses;
    unsigned cycles{},advance_calls{},boundary_calls{},poll_calls{};
    uint8_t irq_mask{},poll_total{},poll_offsets{};
};
static void require(bool condition,const char* what,const OpcodeFixture& f) {
    if(!condition)throw std::runtime_error(std::string(what)+" opcode="+std::to_string(f.opcode)+" PC="+std::to_string(f.pc));
}
static void flag(CPUState& c,uint8_t mask,bool value) { c.P=uint8_t((c.P&uint8_t(~mask))|(value?mask:0)); }
static void nz(CPUState& c,uint8_t value) {flag(c,CPU_Z,value==0);flag(c,CPU_N,value>=128);}
static int signed_byte(uint8_t value) {return value<128?int(value):int(value)-256;}

// Base timings from the documented NMOS instruction table. Zero entries are
// undocumented instructions and are never included in the fixture registry.
static constexpr uint8_t base_cycles[256]={
 7,6,0,0,0,3,5,0,3,2,2,0,0,4,6,0,
 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
 6,6,0,0,3,3,5,0,4,2,2,0,4,4,6,0,
 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
 6,6,0,0,0,3,5,0,3,2,2,0,3,4,6,0,
 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
 6,6,0,0,0,3,5,0,4,2,2,0,5,4,6,0,
 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
 0,6,0,0,3,3,3,0,2,0,2,0,4,4,4,0,
 2,6,0,0,4,4,4,0,2,5,2,0,0,5,0,0,
 2,6,2,0,3,3,3,0,2,2,2,0,4,4,4,0,
 2,5,0,0,4,4,4,0,2,4,2,0,4,4,4,0,
 2,6,0,0,3,3,5,0,2,2,2,0,4,4,6,0,
 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0,
 2,6,0,0,3,3,5,0,2,2,2,0,4,4,6,0,
 2,5,0,0,0,4,6,0,2,4,0,0,0,4,7,0
};

int main(){
 FixtureBus bus;RuntimeServices s{
  &bus,
  [](void*p,uint16_t a,uint8_t t)->uint8_t{auto& b=*static_cast<FixtureBus*>(p);const auto v=b.memory[a];b.accesses.push_back({'R',a,v,t});return v;},
  [](void*p,uint16_t a,uint8_t v,uint8_t t){auto&b=*static_cast<FixtureBus*>(p);b.memory[a]=v;b.writes.push_back({a,v,t});b.accesses.push_back({'W',a,v,t});},
  [](void*,uint16_t)->uint8_t{return 0;},
  [](void*p,uint8_t t){auto& b=*static_cast<FixtureBus*>(p);b.cycles+=t;++b.advance_calls;},
  [](void*p,CPUState*,uint8_t mask)->uint8_t{auto& b=*static_cast<FixtureBus*>(p);b.irq_mask=mask;++b.boundary_calls;return 0;},
  [](void*p,uint8_t total,uint8_t offsets){auto& b=*static_cast<FixtureBus*>(p);
    if(b.advance_calls!=1 || b.boundary_calls || b.cycles!=total)throw std::runtime_error("Poll must follow one advance and precede boundary");
    ++b.poll_calls;b.poll_total=total;b.poll_offsets=offsets;}};
 uint32_t cases=0, legacy_cases=0;
 for(const auto&f:opcode_fixtures)for(unsigned scenario=0;scenario<6;++scenario)for(uint8_t initial_flags:{uint8_t(0x24),uint8_t(0x65),uint8_t(0xe7),uint8_t(0x2f)}){
  bus.memory.fill(0xa5);bus.memory[0x30ff]=0x56;bus.memory[0x3000]=0x12;bus.memory[0x3100]=0x34;bus.writes.clear();bus.accesses.clear();bus.cycles=0;
  bus.advance_calls=bus.boundary_calls=bus.poll_calls=0;bus.irq_mask=0xff;
  if(scenario==1)bus.memory.fill(0);
  if(scenario>=2){
   for(unsigned a=0;a<65536;++a)bus.memory[a]=uint8_t(a*73u+(a/256u)*19u+scenario*11u);
   // Distinct bytes make the pointer wrap observable, not hidden by fill bytes.
   bus.memory[0xff]=0xff;bus.memory[0x00]=0x2f;bus.memory[0x100]=0x77;
   bus.memory[0xf8]=0xff;bus.memory[0xf9]=0x2f;
  }
  auto reference_memory=bus.memory;
  std::vector<Access> expected_accesses;
  auto read=[&](uint16_t a,unsigned offset){const auto v=reference_memory[a];expected_accesses.push_back({'R',a,v,int(offset)});return v;};
  CPUState cpu=cpu_initial_state(f.pc);cpu.A=0x7b;cpu.X=0x0f;cpu.Y=0xf0;cpu.P=initial_flags;
  if(scenario){
   constexpr uint8_t registers[5][4]={{0,0,0,0},{0xff,7,7,0xff},{0x80,8,8,1},{0,0xff,0xff,0xfe},{0x7f,1,1,0xfc}};
   const auto& r=registers[scenario-1];cpu.A=r[0];cpu.X=r[1];cpu.Y=r[2];cpu.SP=r[3];
   if(scenario%2==0)cpu.P=uint8_t(cpu.P&0xfb); // Exercise old-I=0 as well as old-I=1.
  }
  const uint8_t initial_status=cpu.P;
  auto expected=cpu;
  uint16_t next=uint16_t(f.pc+f.size),address=f.operand;const std::string_view name=f.name,mode=f.mode;
  uint8_t cycles=base_cycles[f.opcode],operand=0xa5;std::vector<Write> writes;
  const bool store=name=="STA"||name=="STX"||name=="STY";
  const bool rmw=name=="ASL"||name=="ROL"||name=="LSR"||name=="ROR"||name=="INC"||name=="DEC";
  if(mode=="zpx"||mode=="zpy"){
   (void)read(f.operand,2);address=uint8_t(f.operand+(mode=="zpx"?cpu.X:cpu.Y));
  }
  else if(mode=="izx"){
   (void)read(f.operand,2);const uint8_t pointer=uint8_t(f.operand+cpu.X);
   const uint8_t low=read(pointer,3),high=read(uint8_t(pointer+1),4);address=uint16_t(low+256u*high);
  }else if(mode=="abx"||mode=="aby"||mode=="izy"){
   uint16_t base=f.operand;
   if(mode=="izy"){const uint8_t low=read(uint8_t(f.operand),2),high=read(uint8_t(f.operand+1),3);base=uint16_t(low+256u*high);}
   address=uint16_t(base+(mode=="abx"?cpu.X:cpu.Y));
   const bool cross=(base/256)!=(address/256);
   if(!store&&!rmw&&cross)++cycles;
   if(store||rmw||cross)(void)read(uint16_t((base/256)*256+address%256),mode=="izy"?4:3);
  }
  if(mode=="imm")operand=uint8_t(f.operand);else if(mode=="acc")operand=cpu.A;else if(!store&&mode!="imp"&&mode!="rel"&&mode!="ind"&&name!="JMP"&&name!="JSR")operand=read(address,cycles-(rmw?3:1));
  auto write=[&](uint16_t a,uint8_t v,uint8_t t){writes.push_back({a,v,t});expected_accesses.push_back({'W',a,v,t});reference_memory[a]=v;};
  auto push=[&](uint8_t v,uint8_t t){write(uint16_t(0x100|expected.SP),v,t);--expected.SP;};
  if(name=="ADC"||name=="SBC"){
   const int carry=(expected.P&CPU_C)?1:0;const int sum=name=="ADC"?int(expected.A)+operand+carry:int(expected.A)-operand-(1-carry);
   const int signed_sum=name=="ADC"?signed_byte(expected.A)+signed_byte(operand)+carry:signed_byte(expected.A)-signed_byte(operand)-(1-carry);
   flag(expected,CPU_C,name=="ADC"?sum>255:sum>=0);flag(expected,CPU_V,signed_sum<-128||signed_sum>127);expected.A=uint8_t(sum);nz(expected,expected.A);
  }else if(name=="AND"||name=="ORA"||name=="EOR"){expected.A=name=="AND"?uint8_t(expected.A&operand):name=="ORA"?uint8_t(expected.A|operand):uint8_t(expected.A^operand);nz(expected,expected.A);}
  else if(name=="LDA"){expected.A=operand;nz(expected,operand);}else if(name=="LDX"){expected.X=operand;nz(expected,operand);}else if(name=="LDY"){expected.Y=operand;nz(expected,operand);}
  else if(name=="CMP"||name=="CPX"||name=="CPY"){const uint8_t reg=name=="CMP"?expected.A:name=="CPX"?expected.X:expected.Y;flag(expected,CPU_C,reg>=operand);nz(expected,uint8_t(int(reg)-operand));}
  else if(name=="BIT"){flag(expected,CPU_Z,(expected.A&operand)==0);flag(expected,CPU_N,operand>=128);flag(expected,CPU_V,(operand&64)!=0);}
  else if(store)write(address,name=="STA"?cpu.A:name=="STX"?cpu.X:cpu.Y,uint8_t(cycles-1));
  else if(rmw){
   const bool carry=(expected.P&CPU_C)!=0;uint8_t result;
   if(name=="ASL"||name=="ROL"){const unsigned shifted=unsigned(operand)*2u;result=uint8_t(shifted+(name=="ROL"&&carry?1u:0u));flag(expected,CPU_C,shifted>=256);}
   else if(name=="LSR"||name=="ROR"){result=uint8_t(operand/2u+(name=="ROR"&&carry?128u:0u));flag(expected,CPU_C,(operand%2u)!=0);}
   else result=uint8_t(int(operand)+(name=="INC"?1:-1));
   nz(expected,result);if(mode=="acc")expected.A=result;else{write(address,operand,uint8_t(cycles-2));write(address,result,uint8_t(cycles-1));}
  }else if(mode=="rel"){
   const uint8_t mask=name=="BCC"||name=="BCS"?CPU_C:name=="BEQ"||name=="BNE"?CPU_Z:name=="BMI"||name=="BPL"?CPU_N:CPU_V;
   const bool want=name=="BCS"||name=="BEQ"||name=="BMI"||name=="BVS",taken=((expected.P&mask)!=0)==want;
   if(taken){const auto target=uint16_t(int(next)+(f.operand<128?int(f.operand):int(f.operand)-256));cycles=uint8_t(cycles+1+((next>>8)!=(target>>8)));next=target;}
  }else if(name=="JMP"){
   if(mode=="ind"){const auto low=read(f.operand,3),high=read(uint16_t((f.operand/256)*256+uint8_t(f.operand+1)),4);next=uint16_t(low+256u*high);}
   else next=f.operand;
  }
  else if(name=="JSR"){const uint16_t ret=uint16_t(next-1);push(uint8_t(ret>>8),3);push(uint8_t(ret),4);next=f.operand;}
  else if(name=="RTS"||name=="RTI"){
   unsigned offset=3;
   auto pull=[&](){expected.SP=uint8_t(expected.SP+1);return read(uint16_t(256+expected.SP),offset++);};
   if(name=="RTI")expected.P=uint8_t((pull()&0xcf)|0x20);
   const auto low=pull(),high=pull();next=uint16_t(low+256u*high+(name=="RTS"?1u:0u));
  }
  else if(name=="BRK"){const uint16_t ret=uint16_t(f.pc+2);push(uint8_t(ret>>8),2);push(uint8_t(ret),3);push(uint8_t(expected.P|0x30),4);flag(expected,CPU_I,true);const auto low=read(0xfffe,5),high=read(0xffff,6);next=uint16_t(low+256u*high);}
  else if(name=="PHA")push(expected.A,2);else if(name=="PHP")push(uint8_t(expected.P|0x30),2);
  else if(name=="PLA"){++expected.SP;expected.A=read(uint16_t(256+expected.SP),3);nz(expected,expected.A);}else if(name=="PLP"){++expected.SP;expected.P=uint8_t((read(uint16_t(256+expected.SP),3)&0xcf)|0x20);}
  else if(name=="CLC")flag(expected,CPU_C,false);else if(name=="SEC")flag(expected,CPU_C,true);
  else if(name=="CLI")flag(expected,CPU_I,false);else if(name=="SEI")flag(expected,CPU_I,true);
  else if(name=="CLD")flag(expected,CPU_D,false);else if(name=="SED")flag(expected,CPU_D,true);else if(name=="CLV")flag(expected,CPU_V,false);
  else if(name=="INX"||name=="DEX"){expected.X=uint8_t(int(expected.X)+(name=="INX"?1:-1));nz(expected,expected.X);}
  else if(name=="INY"||name=="DEY"){expected.Y=uint8_t(int(expected.Y)+(name=="INY"?1:-1));nz(expected,expected.Y);}
  else if(name=="TAX"){expected.X=expected.A;nz(expected,expected.X);}else if(name=="TAY"){expected.Y=expected.A;nz(expected,expected.Y);}
  else if(name=="TXA"){expected.A=expected.X;nz(expected,expected.A);}else if(name=="TYA"){expected.A=expected.Y;nz(expected,expected.A);}
  else if(name=="TSX"){expected.X=expected.SP;nz(expected,expected.X);}else if(name=="TXS")expected.SP=expected.X;
  else require(name=="NOP","Missing specification",f);
  expected.PC=next;uint32_t left=1;require(game_bank0_fixture(&cpu,&s,&left)&&left==0,"Native fixture dispatch",f);
  require(cpu.A==expected.A&&cpu.X==expected.X&&cpu.Y==expected.Y&&cpu.SP==expected.SP&&cpu.P==expected.P&&cpu.PC==expected.PC,"Register/flag/PC specification",f);
  require(bus.irq_mask == ((name=="CLI"||name=="SEI"||name=="PLP"?initial_status:expected.P)&CPU_I),"IRQ poll uses oldI forCLI/SEI/PLP and restoredI forRTI",f);
  require(bus.advance_calls==1&&bus.boundary_calls==1,"Exactly one completion and boundary poll",f);
  const auto plan=uint8_t(name=="BRK"?0:mode=="rel"?(cycles==4?0x0a:2):(1u<<(cycles-1)));
  require(bus.poll_calls==1&&bus.poll_total==cycles&&bus.poll_offsets==plan,
          "Documented IRQ detector poll offsets (branch first/fixup, ordinary last, no BRK poll)",f);
  require(bus.cycles==cycles,"Documented timing",f);require(bus.writes.size()==writes.size(),"Write footprint",f);
  for(std::size_t i=0;i<writes.size();++i)require(bus.writes[i].address==writes[i].address&&bus.writes[i].value==writes[i].value&&bus.writes[i].cycle==writes[i].cycle,"Ordered write/offset",f);
  require(bus.accesses.size()==expected_accesses.size(),"Observable read/write footprint (fetches not modeled)",f);
  for(std::size_t i=0;i<expected_accesses.size();++i){const auto& a=bus.accesses[i];const auto& e=expected_accesses[i];
   require(a.kind==e.kind&&a.address==e.address&&a.value==e.value&&a.offset==e.offset,"Ordered observable bus accesses",f);
  }
  require(bus.memory==reference_memory,"Final memory effects",f);
  int prior=-1;for(const auto& access:bus.accesses){require(access.offset>prior&&unsigned(access.offset)<cycles,"Ordered offsets within instruction",f);prior=access.offset;}
  ++cases;if(scenario==0&&f.legacy)++legacy_cases;
 }
 const auto matrix_cases=cases;
 // PLP/RTI must ignore saved B/U while restoring every other bit. Testing all
 // saved bytes is small and avoids depending on incidental memory patterns.
 for(const auto& f:opcode_fixtures)if(f.legacy&&(f.opcode==0x28||f.opcode==0x40))for(unsigned saved=0;saved<256;++saved){
  bus.memory.fill(0);bus.writes.clear();bus.accesses.clear();bus.cycles=0;
  bus.advance_calls=bus.boundary_calls=bus.poll_calls=0;bus.irq_mask=0xff;
  const bool rti=f.opcode==0x40;
  auto cpu=cpu_initial_state(f.pc);cpu.A=0x18;cpu.X=0x29;cpu.Y=0x3a;cpu.P=0x65;cpu.SP=rti?0xfe:0xff;
  bus.memory[rti?0x1ff:0x100]=uint8_t(saved);
  if(rti){bus.memory[0x100]=0x34;bus.memory[0x101]=0x92;}
  const auto expected_memory=bus.memory;
  const uint8_t status=uint8_t((saved&0xcf)|0x20);
  const std::vector<Access> accesses=rti?std::vector<Access>{{'R',0x1ff,uint8_t(saved),3},{'R',0x100,0x34,4},{'R',0x101,0x92,5}}:
      std::vector<Access>{{'R',0x100,uint8_t(saved),3}};
  uint32_t left=1;require(game_bank0_fixture(&cpu,&s,&left)&&left==0,"Status fixture dispatch",f);
  require(cpu.A==0x18&&cpu.X==0x29&&cpu.Y==0x3a&&cpu.P==status&&cpu.SP==(rti?1:0)&&cpu.PC==(rti?0x9234:uint16_t(f.pc+1)),"All saved status bytes; B/U normalized; stack wrap",f);
  require(bus.cycles==(rti?6u:4u)&&bus.advance_calls==1&&bus.boundary_calls==1&&bus.irq_mask==(rti?(status&4):4),"Status instruction cycles/poll selection",f);
  require(bus.poll_calls==1&&bus.poll_total==(rti?6:4)&&bus.poll_offsets==(rti?0x20:8),"Status instruction detector descriptor",f);
  require(bus.writes.empty()&&bus.memory==expected_memory&&bus.accesses.size()==accesses.size(),"Status read-only memory effects",f);
  for(std::size_t i=0;i<accesses.size();++i)require(bus.accesses[i].kind=='R'&&bus.accesses[i].address==accesses[i].address&&bus.accesses[i].value==accesses[i].value&&bus.accesses[i].offset==accesses[i].offset,"Status ordered stack reads",f);
  ++cases;
 }
 std::cout<<cases<<" native instruction cases: "<<legacy_cases<<" reused + "<<matrix_cases-legacy_cases<<" extended + "<<cases-matrix_cases<<" saved-status cases; all 151 official encodings; registers/flags/PC/SP, ordered bus accesses and total cycles: OK\n";
}
