#include "runtime/user_rom.hpp"
#include "runtime/bus.hpp"
#include "game/compiled_program.h"
#include "game/recovered_resources.h"
#include "game/world_view.hpp"
#include "game/actor_view.hpp"
#include "game/god_mode.hpp"
#include <iostream>
int main(int argc,char** argv) {
  try {
    nes::runtime::UserRomModule module;
    if(!module.prepare(argc,argv,false))throw std::runtime_error("Pass --rom and --import-cache for the private import probe");
    nes::runtime::Bus bus({&game_prg_data[0][0],131072u,game_chr_data,131072u});
    nes::game::ActorView actors;nes::game::GodMode god;
    auto services=god.services(bus,actors.services(bus));god.set_enabled(true);
    CPUState cpu=cpu_initial_state(game_reset_vector);uint64_t previous=UINT64_MAX;
    while(bus.video().frames()<1800) {
      const auto frame=bus.video().frames();
      if(previous!=frame){previous=frame;bus.set_buttons(0,static_cast<uint8_t>(frame>=190&&(frame-190)%240<30?8:0));god.begin_frame();}
      uint32_t budget=1000;
      if(!game_execute_slice(&cpu,&services,&budget))throw std::runtime_error("Missing native routine");
      if(bus.video().consume_frame())(void)bus.audio().take_samples();
    }
    nes::game::WorldView view;view.compose(bus,&actors);
    if(bus.ram().read(0x045B)!=0||bus.ram().read(0x71)!=0||!view.extended_background())throw std::runtime_error("Native garden boot failed");
    std::cout<<"Imported native module: 1800 frames, garden, GOD, horizontal map: OK\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
