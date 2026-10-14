// Author-owned fixtures test the current mapper/PPU and an unsafe shared-slot
// counterexample. This is NOT a CHR mod implementation or new native API.
#include "runtime/bus.hpp"
#include <array>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
using namespace nes::runtime;
static unsigned checks=0;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);++checks;}
static uint8_t pixel(const std::vector<uint8_t>& tile,unsigned x,unsigned y){
 if(tile.size()!=16||x>=8||y>=8)throw std::invalid_argument("8x8 NES 2bpp tile required");
 return uint8_t(((tile[y]>>(7-x))&1)|(((tile[y+8]>>(7-x))&1)<<1));
}
static void serial(Cartridge& c,unsigned address,unsigned value,uint64_t& cycle){
 for(unsigned bit=0;bit<5;++bit){cycle+=2;c.cpu_write(uint16_t(address),uint8_t((value>>bit)&1),cycle);}
}
struct RasterResult{std::array<uint8_t,256*240> pixels;uint64_t cycles,dots;uint8_t status,readback;};
static RasterResult raster(const std::vector<uint8_t>& glyph){
 std::array<uint8_t,32768>prg{};std::array<uint8_t,8192>chr{};
 // The fixture contains only original authored data: blank tile7 and solid sprite8.
 if(!glyph.empty())std::copy(glyph.begin(),glyph.end(),chr.begin()+7*16);
 std::fill(chr.begin()+8*16,chr.begin()+9*16,0xff);
 Bus b({prg.data(),prg.size(),chr.data(),chr.size()});b.advance(34000);
 auto&v=b.video();v.write_register(0x2000,0);v.write_register(0x2001,0x1e);
 for(unsigned i=0;i<960;++i)v.write_memory(uint16_t(0x2000+i),7);
 v.write_memory(0x3f00,0);v.write_memory(0x3f03,0x30);v.write_memory(0x3f13,0x16);
 v.write_register(0x2003,0);for(unsigned i=0;i<256;++i)v.write_oam(0xff);
 v.write_register(0x2003,0);for(uint8_t value:{uint8_t(39),uint8_t(8),uint8_t(0),uint8_t(40)})v.write_oam(value);
 v.write_register(0x2005,0);v.write_register(0x2005,0);
 auto frame=v.frames();while(v.frames()==frame)b.advance(100);
 RasterResult result{v.frame(),b.cycles(),v.dots(),v.status(),0};
 // Legitimate CPU-side test read: buffered $2007 sees the same CHR source.
 v.write_register(0x2001,0);b.write8(0x2006,0);b.advance(1);b.write8(0x2006,7*16);b.advance(1);
 b.read8(0x2007);result.readback=b.read8(0x2007);return result;
}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::invalid_argument("Own fixture path required");std::ifstream file(argv[1],std::ios::binary);if(!file)throw std::runtime_error("Own fixture missing");std::vector<uint8_t>glyph((std::istreambuf_iterator<char>(file)),{});
 check(glyph.size()==16,"tile length");
 const std::array<uint8_t,8>rows={0x1a,0x24,0x42,0x62,0x52,0x4a,0x46,0x42};bool exact=true;
 for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)exact&=pixel(glyph,x,y)==(((rows[y]>>(7-x))&1)?3:0);
 check(exact,"independent original glyph pixel reference");
 for(unsigned mode=0;mode<3;++mode){bool rejected=false;try{auto bad=glyph;if(mode==0)bad.pop_back();else if(mode==1)bad.push_back(0);pixel(bad,mode==2?8:0,0);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"bad tile format/dimensions accepted");}
 std::array<uint8_t,32768>prg{};std::array<uint8_t,32768>chr{};for(unsigned bank=0;bank<8;++bank)std::fill(chr.begin()+bank*4096,chr.begin()+(bank+1)*4096,uint8_t(bank));Cartridge c({prg.data(),prg.size(),chr.data(),chr.size()});uint64_t cycle=0;
 serial(c,0x8000,0x1c,cycle);serial(c,0xa000,3,cycle);serial(c,0xc000,6,cycle);
 check(c.chr_offset(0x70)==3*4096+0x70&&c.chr_read(0x70)==3,"4KiB physical CHR mapping");
 check(c.chr_offset(0x1070)==6*4096+0x70&&c.chr_read(0x1070)==6,"separate high CHR window");
 serial(c,0x8000,0x0c,cycle);check(c.chr_read(0x70)==2&&c.chr_read(0x1070)==3,"8KiB even-pair mapping and ignored bank1");
 bool rejected=false;try{c.chr_read(0x2000);}catch(const std::out_of_range&){rejected=true;}check(rejected,"CHR bounds");
 auto original=raster({}),hypothesis=raster(glyph),restored=raster({});
 check(original.pixels!=hypothesis.pixels,"new own pixels not rendered by original NES PPU");
 check(original.cycles==hypothesis.cycles&&original.dots==hypothesis.dots,"counterexample adds cycles");
 check((original.status&0x40)==0&&(hypothesis.status&0x40)!=0,"shared blank replacement must demonstrate sprite-zero status divergence");
 check(original.readback==0&&hypothesis.readback==rows[0],"shared CHR replacement must demonstrate CPU PPUDATA divergence");
 check(original.pixels==restored.pixels&&original.status==restored.status&&original.readback==restored.readback,"fresh original source fallback not restored");
 volatile uint64_t sink=0;auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<2000000;++i)sink+=c.chr_read(uint16_t(i&0x1fff));std::cout<<"Current CHR resources=0 ns/read="<<std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/2000000<<" own_fixture_bytes="<<glyph.size()<<" sink="<<sink<<'\n';
 std::cout<<"CHR research checks="<<checks<<" PASS (unsafe-slot counterexamples; no CHR mod API)\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
