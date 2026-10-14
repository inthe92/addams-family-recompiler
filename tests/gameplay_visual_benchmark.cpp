// Release microcosts only: no ROM, monitor FPS or input-latency claims.
#include "runtime/mod_lua.hpp"
#include "game/super_jump.hpp"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <vector>
extern "C" uint8_t game_bank7_super_jump(CPUState*,const RuntimeServices*,uint32_t*);
template<class F>void measure(const char*name,unsigned count,F f){std::vector<double> values;for(unsigned b=0;b<7;++b){auto t=std::chrono::steady_clock::now();for(unsigned i=0;i<count;++i)f();values.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-t).count()/count);}std::sort(values.begin(),values.end());std::cout<<name<<" us/unit min="<<values.front()<<" median="<<values[3]<<" max="<<values.back()<<" samples=7 iterations="<<count<<'\n';}
int main(){using namespace nes::mods;
 std::vector<uint8_t> prg(131072),chr(131072);nes::runtime::Bus bus({prg.data(),prg.size(),chr.data(),chr.size()});CPUState cpu=cpu_initial_state(0xecfe);nes::game::SuperJump jump;auto base=bus.services(),wrapped=jump.services(bus,base,cpu);bus.ram().write(0x471,255);bus.ram().write(0x45d,128);bus.ram().write(0x13,160);
 auto execute=[&](RuntimeServices&s){bus.ram().write(0x15,0);cpu.PC=0xecfe;cpu.A=1;uint32_t n=1;game_bank7_super_jump(&cpu,&s,&n);bus.ram().write(0x15,6);cpu.PC=0xee2a;n=1;game_bank7_super_jump(&cpu,&s,&n);bus.audio().take_samples();};
 measure("native two instructions / original",10000,[&]{execute(base);});measure("native two instructions / adapter disabled",10000,[&]{execute(wrapped);});jump.request(12);measure("native two instructions / jump enabled",10000,[&]{execute(wrapped);});
 ImageAsset image{"own",{64,32,std::vector<uint8_t>(64*32*4,200)}};LuaMod mod("function on_update() ui.rect('viewport',4,4,198,39,0,0,0,190);ui.text('viewport',8,8,'OWN TIMER');graphics.layer('viewport','own',210,180,100,50,-4,180,20,10,0.4) end",2097152,2,ui_draw|graphics_layers,{image});Observation o;mod.load(o);
 measure("Lua capture text/rectangle/own layer",10000,[&]{mod.update(o);});nes::ui::Bitmap b{320,240,std::vector<uint8_t>(320*240*4)};
 measure("CPU alpha composition three primitives",1000,[&]{std::fill(b.rgba.begin(),b.rgba.end(),0);for(auto&c:mod.visual())paint_visual(b,c,mod.assets());});
 measure("activate/unload own layer VM",100,[&]{LuaMod m("function on_update() end",2097152,2,graphics_layers,{image});m.load(o);m.unload();});
 std::cout<<"Lua allocated bytes="<<mod.memory_used()<<" owned image bytes="<<image.bitmap.rgba.size()<<" shared plane bytes="<<b.rgba.size()<<". Not a game benchmark.\n";
}
