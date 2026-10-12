#include "runtime/presentation_clock.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace nes::frontend;
static void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static uint64_t tick(double x){return static_cast<uint64_t>(std::ceil(x));}
int main(int argc,char**argv){try{
    const std::string mode=argc>1?argv[1]:"schedule";unsigned cases=0;
    if(mode=="snapshot") {
        FrameSnapshot s;require(!s.valid(),"no completed image initially");++cases;
        FrameSnapshot::Pixels live{};live[17]=42;s.complete(live,1,7,1234,5000);
        require(s.valid()&&s.sequence()==1&&s.ppu_frame()==7&&s.cpu_cycle()==1234&&s.host_tick()==5000,"snapshot metadata");++cases;
        live.fill(9);require(s.pixels()[17]==42&&s.pixels()[18]==0,"snapshot aliases live raster");++cases;
        auto saved=s.pixels();nes::runtime::OriginalRGBA rgba;
        for(unsigned n=0;n<120;++n)nes::runtime::original_rgba(s.pixels().data(),256,240,rgba);
        require(s.pixels()==saved&&s.sequence()==1,"repeat presentation mutated pixels/sequence");++cases;
        const auto first=s.publication();require(first!=0,"completed publication has zero ID");
        s.complete(live,1,7,1234,5000);require(s.publication()!=first,"republished same metadata must invalidate");++cases;
        auto copy=s;require(copy.publication()==s.publication()&&copy.pixels()==s.pixels(),"immutable copy identity");
        copy.complete(saved,1,7,1234,5000);require(copy.publication()!=s.publication(),"changed copy reuses publication");++cases;
        FrameSnapshot restarted;restarted.complete(live,1,7,1234,5000);
        require(restarted.publication()!=s.publication()&&restarted.publication()!=copy.publication(),"new stream reuses publication ID");++cases;
    } else {
        for(unsigned hz:{60u,90u,120u}) {
            // Execute the actual scheduler with a virtual microsecond clock.
            PresentationClock c(1000000,hz,0);uint64_t now=0,nes=0,draw=0;
            while(now<600000000) {
                auto due=c.poll(now);
                if(due.simulate){++nes;c.simulation_complete(now);}
                if(due.present){++draw;c.presentation_complete(now);}
                now=tick(c.next_deadline());
            }
            require(nes==30005&&draw==600*hz,"10 minute PAL/presentation counts");
            std::cout<<"Virtual "<<hz<<" Hz: NES="<<nes<<" draws="<<draw<<'\n';++cases;
            // Short stalls: keep PAL ticks, drop missed graphics, no draw bursts.
            PresentationClock j(1000000,hz,0);now=0;nes=draw=0;
            while(now<10000000) {
                auto due=j.poll(now);
                if(due.simulate){++nes;j.simulation_complete(now);}
                if(due.present){++draw;j.presentation_complete(now);require(!j.poll(now).present,"same-tick presentation burst");}
                now+=7000; // independent polling jitter, less than reanchor limit
            }
            require(nes==500&&draw<=10*hz&&draw>400,"jitter preserves PAL without inventing draws");++cases;
        }
        PresentationClock p(1000000,120,0);p.simulation_complete(0);p.presentation_complete(0);
        p.set_paused(true,10000);require(!p.poll(900000000).simulate&&p.poll(900000000).present,"pause continues presentation only");
        p.presentation_complete(900000000);p.set_paused(false,900000000);
        require(p.poll(900000000).simulate,"resume starts one immediate normal unit");p.simulation_complete(900000000);
        require(!p.poll(900000000).simulate,"resume must not catch up pause backlog");++cases;
        PresentationClock sleep(1000000,90,0);sleep.simulation_complete(0);sleep.presentation_complete(0);
        require(sleep.poll(5000000).simulate&&sleep.suspension_reanchors()==1,"suspension reanchor");sleep.simulation_complete(5000000);
        require(!sleep.poll(5000000).simulate,"suspension backlog burst");++cases;
        PresentationClock independent(1000000,60,0);const double d=independent.simulation_deadline();
        independent.presentation_complete(90000);require(independent.simulation_deadline()==d,"graphics modifies PAL deadline");++cases;
        PresentationClock back(1000000,120,1000);back.simulation_complete(1000);back.presentation_complete(1000);
        require(!back.poll(0).simulate&&!back.poll(0).present,"backward host clock guard");++cases;
        bool rejected=false;try{PresentationClock bad(0,60,0);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected,"invalid frequency accepted");rejected=false;try{PresentationClock bad(1000,50,0);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected,"unapproved presentation rate accepted");++cases;
        for(auto text:{"60","90","120"})require(parse_experimental_refresh(text)>0,"CLI approved rate");
        for(auto text:{"0","50","120Hz","60.0","-60",""}){rejected=false;try{parse_experimental_refresh(text);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"invalid CLI rate accepted");}++cases;
    }
    std::cout<<"Presentation clock "<<mode<<" cases="<<cases<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
