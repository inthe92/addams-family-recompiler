// Public authored fixture: verifies display composition, never private game data.
#include "runtime/bus.hpp"
#include "runtime/mod_manager.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace nes::runtime;
static unsigned checks{};
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);++checks;}
static void serial(Cartridge&c,unsigned a,unsigned v,uint64_t&cycle){for(unsigned i=0;i<5;++i){cycle+=2;c.cpu_write(uint16_t(a),uint8_t((v>>i)&1),cycle);}}
static PrgTextResource resource(){PrgTextResource r{"test","label",0,2048,0,{67,66,65}};
 const uint8_t original[]={65,66,67};r.original_crc32=resource_crc32(original,3);
 r.composition=TextComposition{1,0x20a0,0};VisualGlyph g;g.cell=1;const uint8_t rows[]={0x1a,0x24,0x42,0x62,0x52,0x4a,0x46,0x42};for(unsigned i=0;i<8;++i)g.pixels[i]=g.pixels[i+8]=rows[i];r.composition->overrides.push_back(g);return r;}
struct Result {std::array<uint8_t,256*240> frame;uint64_t cycles,dots,writes;uint8_t status,readback,openbus,control,bank0,bank1;uint16_t v,t;std::vector<float> pcm;};
static Result raster(bool active,unsigned mirror=0,unsigned scroll=0,unsigned priority=0,unsigned change=0){
 std::array<uint8_t,32768>p{};std::array<uint8_t,8192>c{};p[2048]=65;p[2049]=66;p[2050]=67;
 for(unsigned i=0;i<16;++i){c[4096+65*16+i]=0x81;c[4096+66*16+i]=0xff;c[4096+67*16+i]=0x44;c[8*16+i]=0xff;}
 Bus b({p.data(),p.size(),c.data(),c.size()});b.advance(34000);uint64_t w=0;serial(b.cartridge(),0x8000,0x1c|mirror,w);serial(b.cartridge(),0xa000,0,w);serial(b.cartridge(),0xc000,1,w);
 auto r=resource();r.composition->mirroring=mirror;if(active)b.cartridge().set_text_resources({r});
 auto&v=b.video();v.write_register(0x2000,0x90);v.write_register(0x2001,0x1e);
 for(unsigned i=0;i<3;++i)v.write_memory(uint16_t(0x20a0+i),uint8_t(65+i));
 v.write_memory(0x3f00,0);v.write_memory(0x3f01,0x11);v.write_memory(0x3f02,0x21);v.write_memory(0x3f03,0x30);v.write_memory(0x3f13,0x16);
 v.write_register(0x2003,0);for(unsigned i=0;i<256;++i)v.write_oam(0xff);v.write_register(0x2003,0);
 for(auto x:{uint8_t(39),uint8_t(8),uint8_t(priority),uint8_t(8)})v.write_oam(x);
 v.write_register(0x2005,uint8_t(scroll));v.write_register(0x2005,0);
 if(change==1)v.write_memory(0x20a1,65); // whole-field identity must fail closed
 if(change==2)serial(b.cartridge(),0xc000,0,w); // wrong physical font bank
 if(change==3)b.cartridge().set_text_resources({}); // no stale pointer/cache
 if(change==4){v.write_memory(0x20a1,65);v.write_memory(0x20a1,66);} // invalidate and restore identity
 auto frame=v.frames();while(v.frames()==frame)b.advance(100);
 Result out{v.frame(),b.cycles(),v.dots(),v.data_writes(),v.status(),0,0,b.cartridge().control(),b.cartridge().chr0(),b.cartridge().chr1(),v.address(),v.temporary_address(),b.audio().take_samples()};
 v.write_register(0x2001,0);b.write8(0x2006,0x14);b.advance(1);b.write8(0x2006,0x20);b.advance(1);b.read8(0x2007);out.readback=b.read8(0x2007);out.openbus=b.read8(0x4020);
 check(b.cartridge().cpu_read(0x8800)==65,"composition changed PRG data");return out;
}
static void hardware(const Result&a,const Result&b){check(a.cycles==b.cycles&&a.dots==b.dots&&a.writes==b.writes&&a.status==b.status&&a.readback==b.readback&&a.openbus==b.openbus&&a.control==b.control&&a.bank0==b.bank0&&a.bank1==b.bank1&&a.v==b.v&&a.t==b.t&&a.pcm==b.pcm,"composition changed hardware/PCM");}
int main(int argc,char**argv){try{
 for(unsigned m=0;m<4;++m)for(unsigned s:{0u,3u,7u})for(unsigned priority:{0u,0x20u}){auto a=raster(false,m,s,priority),b=raster(true,m,s,priority);hardware(a,b);check(a.frame!=b.frame,"own display pixels missing");}
 for(unsigned c:{1u,2u,3u}){auto a=raster(false,0,0,0,c),b=raster(true,0,0,0,c);hardware(a,b);check(a.frame==b.frame,"invalid context or disabled resource did not restore original");}
 auto restored=raster(true,0,0,0,4),enabled=raster(true);hardware(restored,enabled);check(restored.frame==enabled.frame,"nametable match cache failed to invalidate");
 std::array<uint8_t,32768>p{};std::array<uint8_t,8192>c{};p[2048]=65;p[2049]=66;p[2050]=67;Cartridge cart({p.data(),p.size(),c.data(),c.size()});auto valid=resource();cart.set_text_resources({valid});
 for(unsigned mode=0;mode<5;++mode){auto bad=valid;if(mode==0)bad.composition->chr_bank=2;if(mode==1)bad.composition->nametable=0x23ff;if(mode==2)bad.composition->overrides.push_back(bad.composition->overrides.front());if(mode==3)bad.composition->overrides[0].cell=3;if(mode==4)bad.composition->mirroring=4;auto rev=cart.text_resource_revision();bool caught=false;try{cart.set_text_resources({bad});}catch(const std::invalid_argument&){caught=true;}check(caught&&cart.text_resource_revision()==rev,"invalid resource partially published");}
 if(argc==7){nes::mods::Manager manager(argv[1],argv[2],argv[3],{"0.9.0","own-fixture",std::string(64,'a'),argv[4]});manager.install(argv[5]);check(!manager.active(),"install not disabled");manager.enable("compose.one",true);cart.set_text_resources(manager.resources());check(cart.has_compositions()&&cart.cpu_read(0x8800)==65,"manager composition chain incomplete");manager.install(argv[6]);bool conflict=false;try{manager.enable("compose.two",true);}catch(const std::exception&){conflict=true;}check(conflict,"owner conflict hidden");manager.enable("compose.one",false);cart.set_text_resources(manager.resources());check(!cart.has_compositions(),"disable retains glyph buffers");manager.enable("compose.two",true);cart.set_text_resources(manager.resources());check(cart.has_compositions(),"second package needs different C++");}
 auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<1000;++i)cart.set_text_resources({valid});auto ns=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/1000;std::cout<<"Composition publication ns="<<ns<<" prepared_tile_bytes=48 authored_bytes=16\n";
 std::cout<<"Composition native checks="<<checks<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
