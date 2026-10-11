#include "runtime/bus.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nes::runtime;
static unsigned assertions=0;
static void require(bool condition,const char* message) {
    ++assertions;if(!condition)throw std::runtime_error(message);
}
template<class E,class F> static void rejects(F action,const char* message) {
    bool caught=false;try{action();}catch(const E&){caught=true;}require(caught,message);
}
// Eight unmistakable artificial PRG banks and eight CHR pages, not a ROM.
struct Fixture {
    std::vector<uint8_t> prg=std::vector<uint8_t>(8*16384);
    std::vector<uint8_t> chr=std::vector<uint8_t>(8*4096);
    Fixture() {
        for(unsigned bank=0;bank<8;++bank) {
            std::fill_n(prg.begin()+bank*16384,16384,uint8_t(0x40+bank));
            std::fill_n(chr.begin()+bank*4096,4096,uint8_t(0x20+bank));
        }
    }
    CartridgeData data() const{return {prg.data(),prg.size(),chr.data(),chr.size()};}
    std::unique_ptr<Bus> bus() const{return std::make_unique<Bus>(data());}
};
static void serial(Cartridge& cart,uint16_t address,uint8_t value,uint64_t& time) {
    for(unsigned bit=0;bit<5;++bit){time+=2;cart.cpu_write(address,uint8_t((value>>bit)&1),time);}
}
static void warm(Bus& b) {b.advance(33300);(void)b.video().read_register(0x2002);}
static void address(VideoState& video,uint16_t location) {
    (void)video.read_register(0x2002);
    video.write_register(0x2006,uint8_t(location>>8));video.write_register(0x2006,uint8_t(location));
}
static void ram_mirrors() {
    MainRAM ram;
    for(unsigned a=0;a<0x800;++a){require(ram.read(uint16_t(a))==0,"Deterministic RAM init");ram.write(uint16_t(a),uint8_t(a*29+7));}
    for(unsigned mirror=0;mirror<4;++mirror)for(unsigned a=0;a<0x800;++a)
        require(ram.read(uint16_t(a+mirror*0x800))==uint8_t(a*29+7),"RAM mirror value");
    ram.write(0x1fff,0x42);require(ram.read(0x7ff)==0x42,"Write through highest mirror");
    ram.clear(0x91);require(ram.read(0x1800)==0x91,"RAM clear aliases");
}
static void ram_bounds() {
    MainRAM ram;rejects<std::out_of_range>([&]{ram.read(0x2000);},"RAM read rejects device range");
    rejects<std::out_of_range>([&]{ram.write(0xffff,1);},"RAM write rejects cartridge range");
}
static void ram_stack() {
    MainRAM ram;auto cpu=cpu_initial_state(0x8000);cpu.SP=0;
    ram.push(cpu,0x12);require(cpu.SP==0xff&&ram.read(0x100)==0x12,"Stack push wraps below zero");
    ram.push(cpu,0x34);require(cpu.SP==0xfe&&ram.read(0x1ff)==0x34,"Stack remains in page one");
    require(ram.pull(cpu)==0x34&&cpu.SP==0xff,"Stack LIFO");
    require(ram.pull(cpu)==0x12&&cpu.SP==0,"Stack pull wraps above FF");
}
static void mmc1_prg_modes() {
    Fixture f;Cartridge cart(f.data());uint64_t time=0;
    require(cart.bank_for_pc(0x7fff)==0xff&&cart.cpu_read(0xc000)==0x47,"MMC1 reset fixed last bank");
    serial(cart,0xe000,5,time);
    const unsigned expected[4][2]={{4,5},{4,5},{0,5},{5,7}};
    for(unsigned mode=0;mode<4;++mode){serial(cart,0x8000,uint8_t(mode*4),time);
        for(unsigned window=0;window<2;++window){auto pc=uint16_t(0x8000+window*0x4000);
            require(cart.bank_for_pc(pc)==expected[mode][window],"MMC1 PRG bank mapping");
            require(cart.cpu_read(pc)==0x40+expected[mode][window],"MMC1 PRG read mapping");}}
}
static void mmc1_chr_and_mirroring() {
    Fixture f;Cartridge cart(f.data());uint64_t time=0;
    serial(cart,0xa000,3,time);serial(cart,0xc000,6,time);serial(cart,0x8000,0x1c,time);
    require(cart.chr_read(0)==0x23&&cart.chr_read(0x1000)==0x26,"MMC1 separate CHR pages");
    serial(cart,0x8000,0x0c,time);
    require(cart.chr_read(0)==0x22&&cart.chr_read(0x1000)==0x23,"MMC1 8K even CHR pair");
    const unsigned pages[4][4]={{0,0,0,0},{1,1,1,1},{0,1,0,1},{0,0,1,1}};
    for(unsigned mode=0;mode<4;++mode){serial(cart,0x8000,uint8_t(0x0c+mode),time);
        for(unsigned table=0;table<4;++table)
            require(cart.nametable_offset(uint16_t(0x2023+table*0x400))==pages[mode][table]*0x400+0x23,"MMC1 nametable mirroring");}
}
static void mmc1_serial_and_adjacent() {
    Fixture f;Cartridge cart(f.data());uint64_t time=0;
    // The fifth address selects the destination; earlier writes may differ.
    for(unsigned bit=0;bit<5;++bit){time+=2;cart.cpu_write(bit==4?0xe000:0x8000,uint8_t((5u>>bit)&1),time);}
    require(cart.prg_register()==5&&cart.control()==0x0c&&cart.shift_register()==0x10,"Serial commit destination and reset");
    cart.cpu_write(0x8000,1,time+=2);require(cart.shift_register()==0x18,"Serial first bit");
    cart.cpu_write(0xa000,0,++time);require(cart.shift_register()==0x18,"Adjacent D0 write suppressed");
    cart.cpu_write(0xc000,0x80,++time);require(cart.shift_register()==0x10&&(cart.control()&0x0c)==0x0c,"Adjacent D7 reset accepted");
    cart.cpu_write(0x5000,0,time+=2);cart.cpu_write(0x8000,1,++time);
    require(cart.shift_register()==0x10,"Adjacent CPU write outside serial range still gates next write");
    rejects<std::logic_error>([&]{cart.cpu_write(0x8000,0,time);},"Nonincreasing mapper timestamps rejected");
}
static void mmc1_prg_ram() {
    Fixture f;Cartridge cart(f.data());uint64_t time=0;
    cart.cpu_write(0x6123,0x42,time+=2);require(cart.cpu_read(0x6123)==0x42,"PRG RAM stores bytes");
    serial(cart,0xe000,0x10,time);require(!cart.prg_ram_enabled()&&cart.cpu_read(0x6123,0xea)==0xea,"Disabled PRG RAM returns open bus");
    cart.cpu_write(0x6123,0x99,time+=2);serial(cart,0xe000,0,time);
    require(cart.cpu_read(0x6123)==0x42,"Disabled PRG RAM write ignored");
    require(cart.cpu_read(0x5000,0x67)==0x67,"Unmapped cartridge read returns open bus");
}
static void bus_mirror_and_open() {
    Fixture f;auto ptr=f.bus();auto& b=*ptr;
    b.write8(0x080a,0xe5,2);require(b.cycles()==0&&b.video().dots()==6,"Timed write advances devices at offset, not CPU boundary");
    b.advance(3);require(b.read8(0x180a)==0xe5&&b.read8(0x5000)==0xe5,"Bus RAM mirroring/open bus");
    require(b.read8(0x8000)==0x40&&b.read8(0xc000)==0x47,"Bus PRG routing");
    b.write8(0x4000,0x31,2);b.advance(3);
    require(b.audio_writes().size()==1&&b.audio_writes()[0].cycle==5&&b.audio_writes()[0].address==0x4000&&b.audio_writes()[0].value==0x31,"APU observable write timestamp");
}
static void bus_read_clock() {
    Fixture f;auto ptr=f.bus();auto& b=*ptr;
    (void)b.read8(0x8000,2);
    require(b.cycles()==0&&b.audio().cycles()==2&&b.video().dots()==6,"Read synchronizes devices, not CPU anchor");
    (void)b.read8(0x8000,2);
    require(b.audio().cycles()==2&&b.video().dots()==6,"Repeated timestamp does not double advance");
    b.write8(0x0a,0x52,3);
    require(b.read8(0x80a,4)==0x52&&b.audio().cycles()==4&&b.video().dots()==12,"Ordered read/write with RAM side effects");
    rejects<std::logic_error>([&]{(void)b.read8(0x8000,3);},"Backward timed read rejected");
    b.advance(5);
    require(b.cycles()==5&&b.audio().cycles()==5&&b.video().dots()==16,"Finish advances residual interval only");
    (void)b.read8(0x8000,1);b.advance(2);
    require(b.cycles()==7&&b.audio().cycles()==7&&b.video().dots()==22,"Next instruction gets a new time anchor");
}
static void bus_controller() {
    Fixture f;auto ptr=f.bus();auto& b=*ptr;
    b.set_buttons(0,0xa5);b.set_buttons(1,0x3c);b.write8(0x4016,1);b.advance(1);
    require((b.read8(0x4016)&1)==1&&(b.read8(0x4016)&1)==1,"High strobe rereads A");
    b.set_buttons(0,0xa4);require((b.read8(0x4016)&1)==0,"High strobe follows live A");
    b.write8(0x4016,0);b.advance(1);
    for(unsigned bit=0;bit<8;++bit){require((b.read8(0x4016)&1)==((0xa4u>>bit)&1),"Controller 1 serial order");require((b.read8(0x4017)&1)==((0x3cu>>bit)&1),"Controller 2 serial order");}
    require((b.read8(0x4016)&1)==1,"Controller beyond eighth bit returns one");
    rejects<std::out_of_range>([&]{b.set_buttons(2,0);},"Invalid controller rejected");
}
static void bus_ppu_mirror() {
    Fixture f;auto ptr=f.bus();auto& b=*ptr;warm(b);
    b.write8(0x2008,0x04);b.advance(1);require(b.video().control()==4,"Bus PPU register write alias");
    b.write8(0x3ffb,0x10);b.advance(1);b.write8(0x2004,0x52);b.advance(1);
    b.write8(0x2003,0x10);b.advance(1);require(b.read8(0x3ffc)==0x52,"Bus OAM read alias");
}
static void bus_nmi_entry() {
    Fixture f;f.prg[7*16384+0x3ffa]=0;f.prg[7*16384+0x3ffb]=0x90;
    auto ptr=f.bus();auto& b=*ptr;warm(b);b.video().write_register(0x2000,0x80);
    b.advance(58931-b.cycles());auto cpu=cpu_initial_state(0x8123);cpu.SP=0;cpu.P=0x69;
    require(b.boundary(cpu)==1&&b.nmi_count()==1,"NMI delivery at boundary");
    require(cpu.PC==0x9000&&cpu.SP==0xfd&&cpu.P==0x6d&&b.cycles()==58938,"NMI vector/status/cycles");
    require(b.ram().read(0x100)==0x81&&b.ram().read(0x1ff)==0x23&&b.ram().read(0x1fe)==0x69,"NMI stack order, wrap and hardware B=0/U=1");
}
static void ppu_startup_and_control() {
    Fixture f;auto ptr=f.bus();auto& b=*ptr;auto& v=b.video();
    v.write_register(0x2000,0x83);v.write_register(0x2001,0x18);
    require(v.control()==0&&v.mask()==0,"Deterministic startup write gate");
    warm(b);v.write_register(0x2000,3);v.write_register(0x2001,1);
    require(v.control()==3&&v.mask()==1&&(v.temporary_address()&0xc00)==0xc00,"PPUCTRL nametable and PPUMASK");
    require(v.dots()==33300u*16/5,"PAL dot ratio accumulator");
}
static void ppu_address_latch() {
    Fixture f;auto ptr=f.bus();auto& b=*ptr;warm(b);auto& v=b.video();
    v.write_register(0x2006,0x21);(void)v.read_register(0x2002);
    v.write_register(0x2006,0x23);v.write_register(0x2006,0x45);
    require(v.address()==0x2345,"Status read resets incomplete address pair");
    v.write_register(0x2007,0x5a);require(v.read_memory(0x2345)==0x5a&&v.address()==0x2346,"PPUDATA write/increment one");
    v.write_register(0x2000,4);address(v,0x2100);v.write_register(0x2007,0x41);
    require(v.address()==0x2120&&v.data_writes()==2,"PPUDATA increment 32 and write count");
    v.write_register(0x2005,0x2b);(void)v.read_register(0x2002);v.write_register(0x2005,0x18);
    require((v.temporary_address()&31)==3,"Status resets shared scroll/address toggle");
}
static void ppu_buffered_data() {
    Fixture f;auto ptr=f.bus();auto& b=*ptr;warm(b);auto& v=b.video();
    v.write_memory(0x2100,0x5a);v.write_memory(0x2101,0xc3);address(v,0x2100);
    require(v.read_register(0x2007)==0,"PPUDATA initial buffer");
    require(v.read_register(0x2007)==0x5a&&v.read_register(0x2007)==0xc3&&v.address()==0x2103,"PPUDATA delayed buffer and increments");
    v.write_memory(0x3f10,0xff);require(v.read_memory(0x3f00)==0x3f&&v.read_memory(0x3f20)==0x3f,"Palette alias and six-bit storage");
    v.write_memory(0x2f10,0xa7);address(v,0x3f10);
    require(v.read_register(0x2007)==0x3f,"Palette read bypasses buffer");
    address(v,0x2100);require(v.read_register(0x2007)==0xa7,"Palette read fills buffer from underlying nametable");
    v.write_register(0x2001,1);address(v,0x3f00);require(v.read_register(0x2007)==0x30,"Palette grayscale masking");
    v.write_memory(0,0xaa);require(v.read_memory(0)==0x20,"CHR ROM writes ignored");
}
static void ppu_oam_wrap() {
    Fixture f;auto ptr=f.bus();auto& v=ptr->video();
    v.write_register(0x2003,0xff);v.write_register(0x2004,0xbb);v.write_register(0x2004,0xcc);
    require(v.oam()[255]==0xbb&&v.oam()[0]==0xcc,"OAM write wraps at FF");
    v.write_register(0x2003,0xff);require(v.read_register(0x2004)==0xbb&&v.read_register(0x2004)==0xbb,"OAM read does not increment address");
}
static void ppu_status_and_nmi() {
    Fixture f;auto ptr=f.bus();auto& b=*ptr;warm(b);auto& v=b.video();
    v.write_register(0x2000,0x80);b.advance(58931-b.cycles());
    require(v.status()==0x80&&v.consume_nmi()&&!v.consume_nmi(),"Vblank NMI edge consumed once");
    v.write_register(0x2003,0x1b);require(v.read_register(0x3ffa)==0x9b&&v.status()==0,"PPUSTATUS low latch bits and vblank clear");
    require(v.read_register(0x2002)==0x1b&&!v.consume_nmi(),"Repeated status read and no new NMI edge");
    require(v.consume_frame()&&!v.consume_frame(),"Frame notification consumed once");
}
struct Case {const char* group;const char* name;void(*run)();};
int main(int argc,char** argv) {
    const Case cases[]={
        {"ram","mirrors",ram_mirrors},{"ram","bounds",ram_bounds},{"ram","stack",ram_stack},
        {"mmc1","prg_modes",mmc1_prg_modes},{"mmc1","chr_and_mirroring",mmc1_chr_and_mirroring},
        {"mmc1","serial_and_adjacent",mmc1_serial_and_adjacent},{"mmc1","prg_ram",mmc1_prg_ram},
        {"bus","mirrors_and_open",bus_mirror_and_open},{"bus","read_clock",bus_read_clock},{"bus","controllers",bus_controller},
        {"bus","ppu_aliases",bus_ppu_mirror},{"bus","nmi_entry",bus_nmi_entry},
        {"ppu","startup_and_control",ppu_startup_and_control},{"ppu","address_latch",ppu_address_latch},
        {"ppu","buffered_data",ppu_buffered_data},{"ppu","oam_wrap",ppu_oam_wrap},{"ppu","status_and_nmi",ppu_status_and_nmi}};
    unsigned executed=0;
    for(const auto& c:cases)if(argc==1||std::string(argv[1])==c.group) {
        try{c.run();std::cout<<c.group<<'/'<<c.name<<": OK\n";++executed;}
        catch(const std::exception& e){std::cerr<<c.group<<'/'<<c.name<<": FAIL: "<<e.what()<<'\n';return 1;}
    }
    if(!executed){std::cerr<<"Unknown test group\n";return 2;}
    std::cout<<executed<<" synthetic device cases; "<<assertions<<" assertions: OK\n";
}
