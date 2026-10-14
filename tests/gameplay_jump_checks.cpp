#include "game/super_jump.hpp"
#include "game/god_mode.hpp"
#include "game/gameplay_diagnostic.hpp"
#include "runtime/mod_lua.hpp"
#include "runtime/mod_manager.hpp"
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>
extern "C" uint8_t game_bank7_super_jump(CPUState*,const RuntimeServices*,uint32_t*);
using namespace nes::game;using namespace nes::mods;using nes::runtime::Bus;
static unsigned cases=0;
static void check(bool ok,const char* m){if(!ok)throw std::runtime_error(m);++cases;}
struct F {
 std::vector<uint8_t> prg=std::vector<uint8_t>(131072),chr=std::vector<uint8_t>(131072);
 std::unique_ptr<Bus> bus=std::make_unique<Bus>(nes::runtime::CartridgeData{prg.data(),prg.size(),chr.data(),chr.size()});
 CPUState cpu=cpu_initial_state(0xecfe);SuperJump jump;GodMode god;GameplayDiagnostic diagnostic;std::ostringstream log;
 RuntimeServices s;
 F(){bus->ram().write(0x471,255);bus->ram().write(0x45d,0x80);bus->ram().write(0x13,160);log.str("");diagnostic.begin(log,false,false,true);auto inner=god.services(*bus,bus->services());inner=diagnostic.services(*bus,inner,cpu);s=jump.services(*bus,inner,cpu);}
 void run(uint16_t pc){cpu.PC=pc;uint32_t n=1;if(!game_bank7_super_jump(&cpu,&s,&n)||n)throw std::runtime_error("authored C did not execute");}
 void start(){cpu.A=1;run(0xecfe);}
 void step(){bus->ram().write(0x15,6);run(0xee2a);}
};
struct Tap {
 RuntimeServices base;std::vector<std::array<unsigned,4>> accesses;unsigned polls{},advances{},boundaries{};
 RuntimeServices services(){return {this,
 [](void*p,uint16_t a,uint8_t t){auto&q=*static_cast<Tap*>(p);auto v=q.base.read8(q.base.context,a,t);q.accesses.push_back({'R',a,v,t});return v;},
 [](void*p,uint16_t a,uint8_t v,uint8_t t){auto&q=*static_cast<Tap*>(p);q.accesses.push_back({'W',a,v,t});q.base.write8(q.base.context,a,v,t);},
 [](void*p,uint16_t a){auto&q=*static_cast<Tap*>(p);return q.base.bank_for_pc(q.base.context,a);},
 [](void*p,uint8_t t){auto&q=*static_cast<Tap*>(p);++q.advances;q.base.advance(q.base.context,t);},
 [](void*p,CPUState*c,uint8_t m){auto&q=*static_cast<Tap*>(p);++q.boundaries;return q.base.boundary(q.base.context,c,m);},
 [](void*p,uint8_t n,uint8_t o){auto&q=*static_cast<Tap*>(p);++q.polls;q.base.poll_irq(q.base.context,n,o);}};}
};
int main(int argc,char**argv){try{
 {F f;auto before=f.bus->cycles();f.start();f.step();check(f.bus->ram().read(0x15)==7&&f.jump.extended_steps()==0,"no mods original phase");check(f.bus->cycles()-before==8,"original instruction cycles");}
 {F f;f.jump.request(12);f.start();for(unsigned n=0;n<13;++n){auto before=f.bus->cycles();f.step();check(f.bus->cycles()-before==5,"INC cycles preserved");check(f.bus->ram().read(0x15)==(n<12?6:7),"bounded phase extension");check(!(f.cpu.P&(CPU_N|CPU_Z)),"RMW flags preserved");}check(f.jump.extended_steps()==12,"12 extended original steps");f.jump.request(0);f.step();check(f.bus->ram().read(0x15)==7,"deactivation restores original INC");}
 for(auto a:{0x45b,0xb1,0x80,0x70,0x600,0x72,0x7c,0x60a,0x60d,0x61c,0x73,0x1c}){F f;f.jump.request(12);f.start();f.bus->ram().write(uint16_t(a),1);f.step();check(f.bus->ram().read(0x15)==7,"special state must cancel power");f.bus->ram().write(uint16_t(a),0);f.step();check(f.bus->ram().read(0x15)==7,"canceled state reactivated existing jump");}
 {F f;f.jump.request(12);f.start();f.bus->ram().write(0x71,2);f.step();check(f.bus->ram().read(0x15)==7,"room change cancels lease");}
 {F f;f.jump.request(12);f.start();f.bus->ram().write(0x471,0);f.step();check(f.bus->ram().read(0x15)==7,"loading cancels lease");}
 {F f;f.bus->ram().write(0x45d,0);f.jump.request(12);f.start();f.step();check(f.bus->ram().read(0x15)==7,"automatic jump cannot acquire power");}
 {F f;f.bus->ram().write(0x71,2);f.jump.request(12);f.start();f.step();check(f.bus->ram().read(0x15)==7,"unvalidated room cannot activate power");}
 {F f;f.jump.request(12);f.step();check(f.bus->ram().read(0x15)==7,"enable midair does not create power lease");}
 {F f;f.jump.request(12);f.start();f.bus->ram().write(0x13,40);f.step();check(f.bus->ram().read(0x15)==7,"top edge guard");}
 {F f;Tap tap{f.bus->services(),{}};f.s=f.jump.services(*f.bus,tap.services(),f.cpu);f.jump.request(12);f.start();tap.accesses.clear();tap.polls=tap.advances=tap.boundaries=0;f.step();
  const std::vector<std::array<unsigned,4>> expected={{'R',0x15,6,2},{'W',0x15,6,3},{'W',0x15,6,4}};
  check(tap.accesses==expected,"original RMW read/old/new order and offsets, no extra accesses");check(tap.polls==1&&tap.advances==1&&tap.boundaries==1,"exactly one poll/advance/boundary forwarded");
 }
 // Disabled wrapper + GOD + diagnostic must preserve CPU, RAM and devices.
 {F a,b;for(auto* f:{&a,&b}){f->bus->write8(0x4017,0x40);f->bus->advance(100);f->cpu.A=1;}auto plain=b.bus->services();b.s=plain;
  a.start();b.start();a.step();b.step();check(a.cpu.PC==b.cpu.PC&&a.cpu.SP==b.cpu.SP&&a.cpu.A==b.cpu.A&&a.cpu.X==b.cpu.X&&a.cpu.Y==b.cpu.Y&&a.cpu.P==b.cpu.P,"disabled full chain registers");
  bool same=true;for(unsigned i=0;i<2048;++i)same&=a.bus->ram().read(i)==b.bus->ram().read(i);check(same,"disabled full chain RAM");check(a.bus->cycles()==b.bus->cycles()&&a.bus->video().dots()==b.bus->video().dots()&&a.bus->irq_count()==b.bus->irq_count()&&a.bus->nmi_count()==b.bus->nmi_count()&&a.bus->dma_count()==b.bus->dma_count(),"disabled full chain timing and interrupts");}
 std::cout<<"Super jump synthetic cases="<<cases<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
