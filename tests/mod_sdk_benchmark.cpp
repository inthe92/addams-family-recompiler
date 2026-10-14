// Host microbenchmark only. No NES execution, display or private ROM.
#include "runtime/mod_lua.hpp"
#include "runtime/ram.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
int main(){using namespace nes::mods;volatile uint64_t checksum=0;for(unsigned mode=0;mode<4;++mode){std::vector<double> times;for(unsigned batch=0;batch<7;++batch){nes::runtime::MainRAM ram;std::unique_ptr<LuaMod> lua;if(mode>=2){lua=std::make_unique<LuaMod>("local n=0;function on_load()nes.memory.observe(32,64,function(c)n=n+#c end)end",2*1024*1024,2,12);lua->load({});}Observation o;const auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<10000;++i){if(mode==3)ram.write(32,uint8_t(i));if(mode){o.memory=capture_memory(ram);checksum+=o.memory->bytes[32];}else checksum+=ram.read(32);if(lua)lua->update(o);}auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/10000;times.push_back(ns);}std::sort(times.begin(),times.end());std::cout<<"SDK host mode="<<mode<<" ns_per_unit_min="<<times.front()<<" median="<<times[3]<<" max="<<times.back()<<" batches=7 units_per_batch=10000\n";}std::cout<<"checksum="<<checksum<<"; not a game/monitor benchmark\n";}
