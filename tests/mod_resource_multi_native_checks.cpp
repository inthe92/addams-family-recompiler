#include "utf8_test_args.hpp"
#include "runtime/bus.hpp"
#include "runtime/mod_manager.hpp"
#include "multi_resource_fixture.hpp"
#include <array>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace nes::runtime;
static unsigned checks=0;
static void check(bool ok,const char*message){if(!ok)throw std::runtime_error(message);++checks;}
struct Trace{std::vector<uint64_t> stable;std::vector<uint8_t> pixels;};
static Trace run(unsigned fields,bool disable=false){
 std::array<uint8_t,32768> prg{};std::array<uint8_t,8192> chr{};
 for(auto b:multi_resource_bytes)prg[b.offset]=b.value;
 for(unsigned tile=1;tile<=6;++tile)for(unsigned row=0;row<8;++row)chr[tile*16+row]=uint8_t(0xffu<<(tile-1));
 auto original=prg;Bus bus({prg.data(),prg.size(),chr.data(),chr.size()});
 std::vector<PrgTextResource> resources;for(unsigned i=0;i<fields;++i)resources.push_back({"own.multi",i?"second":"label",0,0x800+i*16,resource_crc32(prg.data()+0x800+i*16,3),{4,5,6}});
 bus.cartridge().set_text_resources(resources);auto cpu=cpu_initial_state(0x8000);auto s=bus.services();Trace out;
 for(unsigned f=0;f<10;++f){if(disable&&f==3)bus.cartridge().set_text_resources({});const auto buttons=uint8_t(f*17);bus.set_buttons(0,buttons);auto frame=bus.video().frames();
 while(bus.video().frames()==frame){uint32_t budget=1000;if(!game_bank0_multi_resource(&cpu,&s,&budget))throw std::runtime_error("synthetic native execution stopped");}
 bus.video().consume_frame();out.stable.insert(out.stable.end(),{f,buttons,cpu.PC,cpu.SP,cpu.X,cpu.Y,cpu.P,bus.cycles(),bus.video().dots(),bus.video().frames(),bus.audio().cycles(),bus.dma_count(),bus.nmi_count(),bus.irq_count(),bus.dmc_dma_count()});
 // A contains the last glyph temporarily, so compare unaffected registers/RAM,
 // including the actual controller latch, separately from intentional data reads.
 for(unsigned i=0;i<2048;++i)out.stable.push_back(bus.ram().read(i));
 for(float sample:bus.audio().take_samples()){uint32_t bits;std::memcpy(&bits,&sample,4);out.stable.push_back(bits);}
 auto&pixels=bus.video().frame();out.pixels.insert(out.pixels.end(),pixels.begin(),pixels.end());}
 if(prg!=original)throw std::runtime_error("source PRG modified");return out;
}
int main(int argc,char**argv){try{nes::test::Utf8Args args(argc,argv);argc=args.size();argv=args.data();
 auto original=run(0),one=run(1),two=run(2),disabled=run(2,true);
 check(original.stable==one.stable,"one field alters unaffected native state");check(original.stable==two.stable,"two fields alter cycles/RAM/PCM/input/NMI/IRQ/OAM/DMC DMA");check(original.stable==disabled.stable,"disable alters unaffected native state");
 check(original.pixels!=one.pixels&&one.pixels!=two.pixels,"second field not independently rendered");
 bool first=false,second=false,contained=true;for(std::size_t i=0;i<two.pixels.size();++i)if(original.pixels[i]!=two.pixels[i]){unsigned xy=unsigned(i%(256*240)),x=xy%256,y=xy/256;first|=y<8;second|=y>=8&&y<16;contained&=x<24&&y<16;}
 check(first&&second&&contained,"unexpected text raster regions");check(std::equal(original.pixels.end()-256*240,original.pixels.end(),disabled.pixels.end()-256*240),"disable failed to restore both fields after reload");check(run(0).stable==original.stable&&run(0).pixels==original.pixels,"no-mod native trace nondeterministic");
 std::array<uint8_t,32768>prg{};std::array<uint8_t,8192>chr{};prg[0x800]=65;prg[0x801]=66;prg[0x802]=67;prg[0x810]=65;prg[0x811]=66;prg[0x812]=67;
 Cartridge c({prg.data(),prg.size(),chr.data(),chr.size()});PrgTextResource a{"own","label",0,0x800,resource_crc32(prg.data()+0x800,3),{67,65,66}},b{"own","second",0,0x810,resource_crc32(prg.data()+0x810,3),{66,65,67}};
 c.set_text_resources({a,b});check(c.cpu_read(0x8800,0)==67&&c.cpu_read(0x8810,0)==66,"two fields publication");
 for(unsigned mode=0;mode<3;++mode){auto bad=b;if(mode==0)bad.original_crc32^=1;else if(mode==1)bad.bank=2;else bad.offset=a.offset;
 bool rejected=false;try{c.set_text_resources({a,bad});}catch(const std::exception&){rejected=true;}check(rejected&&c.cpu_read(0x8800,0)==67&&c.cpu_read(0x8810,0)==66,"bad second field changed active backend");}
 c.set_text_resources({});check(c.cpu_read(0x8800,0)==65&&c.cpu_read(0x8810,0)==65,"both original fallbacks");
 if(argc==7){
 const auto root=std::filesystem::u8path(argv[1]);const auto helper=std::filesystem::u8path(argv[3]);nes::mods::Target target{"0.9.0","own-fixture",std::string(64,'a'),std::filesystem::u8path(argv[4]).u8string()};
 {nes::mods::Manager m(root,argv[2],helper,target);m.install(std::filesystem::u8path(argv[5]));check(m.resources().empty(),"install enabled translation");m.enable("translate.multi",true);check(m.resources().size()==2,"native transport lost second field");c.set_text_resources(m.resources());check(c.cpu_read(0x8800,0)==67&&c.cpu_read(0x8810,0)==66,"manager to mapper chain");m.install(std::filesystem::u8path(argv[6]));bool rejected=false;try{m.enable("translate.single",true);}catch(const std::exception&){rejected=true;}check(rejected&&m.resources().size()==2,"partial owner conflict not atomic");}
 {nes::mods::Manager m(root,argv[2],helper,target);check(m.resources().size()==2,"restart lost active resources");m.enable("translate.multi",false);c.set_text_resources(m.resources());check(m.resources().empty()&&c.cpu_read(0x8800,0)==65&&c.cpu_read(0x8810,0)==65,"disable incomplete");m.enable("translate.multi",true);m.uninstall("translate.multi");check(m.resources().empty(),"uninstall left active resource");}
 {nes::mods::Manager m(root,argv[2],helper,target);check(m.resources().empty(),"restart after uninstall reactivated");}
 }
 std::cout<<"Resource multi native checks="<<checks<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
