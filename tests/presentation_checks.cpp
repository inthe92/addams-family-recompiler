#include "runtime/original_presentation.hpp"
#include "runtime/palette.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace nes::runtime;
static void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
static unsigned geometry() {
    struct Case {int w,h;bool full;PictureViewport expected;};
    const Case cases[]={
        {960,720,false,{0,0,960,720}}, {1920,1080,false,{320,60,1280,960}},
        {1920,1080,true,{240,0,1440,1080}}, {1366,768,true,{171,0,1024,768}},
        {1280,720,false,{160,0,960,720}}, {800,600,false,{80,60,640,480}},
        {1000,1000,true,{0,125,1000,750}}, {600,900,false,{140,330,320,240}},
        {600,900,true,{0,225,600,450}}, {319,239,false,{1,1,316,237}},
        {256,240,false,{0,24,256,192}}, {1073,723,false,{56,1,960,720}},
        {0,720,false,{}}, {960,0,true,{}}, {-1,720,false,{}}, {3,2,true,{}},
        {1920,1440,false,{0,0,1920,1440}} // Retina drawable, not logical size
    };
    for(const auto& c:cases) {
        const auto v=original_viewport(c.w,c.h,c.full);
        require(v.x==c.expected.x&&v.y==c.expected.y&&v.width==c.expected.width&&v.height==c.expected.height,"viewport differs from authored geometry");
    }
    // Resize sequence including odd/small/portrait values, with independent bounds.
    for(int w=4;w<=2100;w+=37)for(int h=3;h<=1400;h+=41)for(bool full:{false,true}) {
        const auto v=original_viewport(w,h,full);
        require(v.width*3==v.height*4,"aspect distorted");
        require(v.x>=0&&v.y>=0&&v.x+v.width<=w&&v.y+v.height<=h,"image clipped");
        require(std::abs(w-v.width-2*v.x)<=1&&std::abs(h-v.height-2*v.y)<=1,"image not centered");
        if(full||w<320||h<240)require(v.width+4>w||v.height+3>h,"fullscreen/fallback not largest exact fit");
        else require(v.height%240==0&&v.width%320==0,"window integer vertical scaling lost");
    }
    return 18; // 17 explicit cases plus the deterministic resize sweep
}
static unsigned pixels() {
    std::array<uint8_t,256*240> image{};
    for(std::size_t i=0;i<image.size();++i)image[i]=static_cast<uint8_t>((i*17+i/256*13)%256);
    const auto saved=image;OriginalRGBA converted{};
    original_rgba(image.data(),256,240,converted);
    for(std::size_t i=0;i<image.size();++i) {
        const auto color=rgb_palette[saved[i]%64];
        require(converted[i*4]==((color>>16)&255)&&converted[i*4+1]==((color>>8)&255)&&converted[i*4+2]==(color&255)&&converted[i*4+3]==255,"source pixel not preserved");
    }
    require(image==saved,"presentation modified original frame");
    unsigned rejected=0;
    for(const auto size:{std::pair<unsigned,unsigned>{427,240},{256,192},{0,240},{256,241}}) {
        const auto before=converted;
        try {original_rgba(image.data(),size.first,size.second,converted);}
        catch(const std::invalid_argument&) {++rejected;}
        require(converted==before,"invalid frame changed output");
    }
    try {original_rgba(nullptr,256,240,converted);}catch(const std::invalid_argument&){++rejected;}
    require(rejected==5,"extended/nonoriginal frames accepted");
    return 7; // full frame, immutability, five rejected inputs
}
static unsigned pacing() {
    constexpr double frequency=1000000,period=frequency/50.007;
    double baseline=0;
    for(unsigned frame=0;frame<1000;++frame)baseline=next_pal_deadline(baseline,frequency,baseline);
    require(std::abs(baseline-1000*period)<0.001,"PAL deadline drift");
    for(int refresh:{50,60,120,144}) {
        double deadline=0;
        for(unsigned frame=0;frame<1000;++frame) {
            // Independent display presentations/geometry cannot advance the clock.
            for(int present=0;present<refresh;++present)(void)original_viewport(1920,1080,present%2==0);
            deadline=next_pal_deadline(deadline,frequency,deadline);
        }
        require(deadline==baseline,"display refresh changed PAL host rate");
    }
    require(next_pal_deadline(0,frequency,period+frequency/10)==period,"pause threshold changed");
    require(next_pal_deadline(0,frequency,frequency)==frequency,"long pause must resume without catch-up burst");
    return 7;
}
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("geometry|pixels|pacing required");
        const std::string mode=argv[1];unsigned count=0;
        if(mode=="geometry")count=geometry();else if(mode=="pixels")count=pixels();else if(mode=="pacing")count=pacing();else throw std::runtime_error("unknown contract");
        std::cout<<count<<" original presentation "<<mode<<" cases: PASS\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
