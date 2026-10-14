// Public owned fixture. Measures software backend/device work, not monitor FPS.
#include "runtime/bus.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
using namespace nes::runtime;
using Clock=std::chrono::steady_clock;
static double ns(Clock::time_point t){return std::chrono::duration<double,std::nano>(Clock::now()-t).count();}
int main(){std::array<uint8_t,32768>p{};std::array<uint8_t,8192>c{};
 for(unsigned i=0;i<16;++i)c[4096+65*16+i]=0xff;p[2048]=65;p[2064]=65;
 for(unsigned count=0;count<=2;++count){Bus b({p.data(),p.size(),c.data(),c.size()});b.advance(34000);uint64_t tick=0;
 auto serial=[&](unsigned address,unsigned value){for(unsigned bit=0;bit<5;++bit){tick+=2;b.cartridge().cpu_write(uint16_t(address),uint8_t((value>>bit)&1),tick);}};
 serial(0x8000,0x1c);serial(0xc000,1);auto&v=b.video();v.write_register(0x2000,0x10);v.write_register(0x2001,0x0a);v.write_memory(0x20a0,65);v.write_memory(0x20c0,65);v.write_memory(0x3f03,0x30);
 std::vector<PrgTextResource>resources;
 for(unsigned i=0;i<count;++i){PrgTextResource r{"benchmark","field"+std::to_string(i),0,2048+i*16,resource_crc32(p.data()+2048+i*16,1),{65}};r.composition=TextComposition{1,0x20a0+i*32,0};VisualGlyph g;g.cell=0;g.pixels.fill(0x55);r.composition->overrides.push_back(g);resources.push_back(r);}
 auto t=Clock::now();for(unsigned i=0;i<2000;++i)b.cartridge().set_text_resources(resources);const auto activate=ns(t)/2000;
 volatile uint64_t checksum=0;t=Clock::now();for(unsigned i=0;i<2000000;++i)checksum+=b.cartridge().chr_read(uint16_t(i&8191));const auto read=ns(t)/2000000;
 auto frames=v.frames();t=Clock::now();b.advance(33248*200ull);const auto device=ns(t);auto completed=v.frames()-frames;for(auto pixel:v.frame())checksum+=pixel;
 t=Clock::now();for(unsigned i=0;i<2000;++i){b.cartridge().set_text_resources({});b.cartridge().set_text_resources(resources);}const auto toggle=ns(t)/2000;
 t=Clock::now();b.cartridge().set_text_resources({});const auto release=ns(t);
 std::cout<<"resources="<<count<<" chr_ns/read="<<read<<" publication_ns="<<activate<<" disable_plus_enable_ns="<<toggle<<" final_release_ns="<<release<<" device_ns/frame="<<device/completed<<" frames="<<completed<<" prepared_tile_bytes="<<count*16<<" own_glyph_bytes="<<count*16<<" checksum="<<checksum<<'\n';
 }
}
