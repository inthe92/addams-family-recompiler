#include "runtime/bus.hpp"
#include "runtime/mod_manager.hpp"
#include "resource_fixture.hpp"
#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace nes::runtime;
unsigned checks=0;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);++checks;}
template<class F>void reject(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"invalid resource accepted");}
static PrgTextResource res(const uint8_t* prg,unsigned bank=0,unsigned offset=0x800){return {"own.mod","label",bank,offset,resource_crc32(prg+bank*16384+offset,3),{4,5,6}};}
struct Trace{std::vector<uint64_t> stable,pixels;};
Trace run(bool active,bool toggle=false){std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};for(auto b:resource_bytes)prg[b.offset]=b.value;
 // Authored contrasting patterns, no commercial font/assets.
 for(unsigned tile=1;tile<=6;++tile)for(unsigned row=0;row<8;++row)chr[tile*16+row]=uint8_t(0xffu<<(tile-1));
 Bus bus({prg.data(),prg.size(),chr.data(),chr.size()});auto cpu=cpu_initial_state(0x8000);auto s=bus.services();if(active)bus.cartridge().set_text_resources({res(prg.data())});Trace t;
 for(unsigned f=0;f<8;++f){if(toggle&&f==3)bus.cartridge().set_text_resources({});bus.set_buttons(0,uint8_t(f*17));auto before=bus.video().frames();while(bus.video().frames()==before){uint32_t n=1000;if(!game_bank0_resource(&cpu,&s,&n))throw std::runtime_error("authored native code stopped");}
 bus.video().consume_frame();auto pcm=bus.audio().take_samples();t.stable.insert(t.stable.end(),{cpu.PC,cpu.SP,cpu.X,cpu.Y,cpu.P,bus.cycles(),bus.video().dots(),bus.video().frames(),bus.audio().cycles(),bus.dma_count(),bus.nmi_count(),bus.irq_count(),bus.dmc_dma_count()});for(unsigned i=0;i<2048;++i)t.stable.push_back(bus.ram().read(i));for(float sample:pcm){uint32_t bits;std::memcpy(&bits,&sample,4);t.stable.push_back(bits);}for(auto pixel:bus.video().frame())t.pixels.push_back(pixel);}
 return t;}
int main(int argc,char**argv){try{
 std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};prg[0x800]=1;prg[0x801]=2;prg[0x802]=3;auto original=prg;Bus b({prg.data(),prg.size(),chr.data(),chr.size()});auto r=res(prg.data());
 check(b.read8(0x8800)==1,"original fallback");b.cartridge().set_text_resources({r});check(b.read8(0x8800,2)==4,"bounded replacement");check(b.read8(0x5000)==4,"replacement does not drive open bus");check(b.read8(0x87ff)==0&&b.read8(0x8803)==0,"range escaped");check(b.cycles()==0,"untimed resource advances CPU");b.advance(4);check(b.cycles()==4&&b.audio().cycles()==4,"timed resource duplicates cycles");check(prg==original,"PRG mutated");
 auto bad=r;bad.original_crc32^=1;reject([&]{b.cartridge().set_text_resources({bad});});check(b.cartridge().cpu_read(0x8800,0)==4,"atomic publication lost previous overlay");reject([&]{b.cartridge().set_text_resources({r,r});});bad=r;bad.offset=16383;reject([&]{b.cartridge().set_text_resources({bad});});bad=r;bad.bank=2;reject([&]{b.cartridge().set_text_resources({bad});});bad=r;bad.bytes[0]=255;reject([&]{b.cartridge().set_text_resources({bad});});bad=r;bad.bytes.clear();reject([&]{b.cartridge().set_text_resources({bad});});b.cartridge().set_text_resources({});check(b.read8(0x8800)==1,"disable original fallback");
 // Same physical overlay in the mapper's fixed high bank, unaffected low bank.
 auto high=res(prg.data(),1);b.cartridge().set_text_resources({high});check(b.read8(0xc800)==4&&b.read8(0x8800)==1,"physical bank isolation");
 auto ref=run(false),translated=run(true),restored=run(true,true);check(ref.stable==translated.stable,"resource changes CPU/cycles/RAM/input/PCM/NMI/DMA");check(ref.pixels!=translated.pixels,"native PPU did not render resource");check(ref.stable==restored.stable,"deactivation changes unaffected trace");check(std::equal(ref.pixels.end()-256*240,ref.pixels.end(),restored.pixels.end()-256*240),"fresh state failed to restore original raster");check(run(false).pixels==ref.pixels,"mods-disabled nondeterminism");
 if(argc==7){nes::mods::Manager m(argv[1],argv[2],argv[3],{"0.9.0","own-fixture",std::string(64,'a'),argv[4]});m.install(argv[5]);m.install(argv[6]);check(!m.active()&&m.resources().empty(),"installation enabled/executed data package");m.enable("translate.one",true);check(m.active()&&m.resources().size()==1&&m.resources()[0].bytes==std::vector<uint8_t>({67,65,66}),"native manager transport");reject([&]{m.enable("translate.two",true);});check(m.resources()[0].owner=="translate.one","conflict altered active owner");m.enable("translate.one",false);check(!m.active()&&m.resources().empty(),"manager disable lifetime");m.enable("translate.two",true);check(m.resources()[0].bytes==std::vector<uint8_t>({66,65,67}),"second package same binary");m.uninstall("translate.two");check(m.resources().empty(),"uninstall resources");}
 std::cout<<"Resource native checks="<<checks<<" PASS\n";
 // Relative microbenchmarks: native lookup only; never a game FPS benchmark.
 volatile uint64_t sink=0;Cartridge c({prg.data(),prg.size(),chr.data(),chr.size()});std::vector<PrgTextResource> many;for(unsigned i=0;i<16;++i)many.push_back(res(prg.data(),0,0x800+i*4));
 for(unsigned n:{0u,1u,2u,16u}){c.set_text_resources(std::vector<PrgTextResource>(many.begin(),many.begin()+n));auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<2000000;++i)sink+=c.cpu_read(uint16_t(0x8000+(i%4096)),0);auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count();std::cout<<"Lookup resources="<<n<<" ns/read="<<ns/2000000<<" owned_glyph_bytes="<<n*3<<'\n';}
 for(unsigned n:{0u,1u,2u,16u}){std::vector<PrgTextResource> selected(many.begin(),many.begin()+n);double publish=0,clear=0;for(unsigned i=0;i<10000;++i){auto begin=std::chrono::steady_clock::now();c.set_text_resources(selected);auto middle=std::chrono::steady_clock::now();c.set_text_resources({});auto end=std::chrono::steady_clock::now();publish+=std::chrono::duration<double,std::nano>(middle-begin).count();clear+=std::chrono::duration<double,std::nano>(end-middle).count();}std::cout<<"Publication resources="<<n<<" ns/set="<<publish/10000<<" ns/clear="<<clear/10000<<" descriptor_and_glyph_bytes="<<n*(sizeof(PrgTextResource)+3)<<'\n';}
 auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<10000;++i){c.set_text_resources(many);c.set_text_resources({});}std::cout<<"Publish+clear16 ns/pair="<<std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/10000<<" sink="<<sink<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
