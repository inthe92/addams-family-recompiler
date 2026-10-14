// Host operations with authored ZIPs; not game FPS, GPU time or input latency.
#include "runtime/mod_manager.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <vector>
namespace nes::mods{int run_mod_process(const std::vector<std::string>&,const std::filesystem::path&);}
using namespace nes::mods;
static std::map<std::string,std::vector<double>> samples;
template<class F>void measure(const std::string&name,F f){auto begin=std::chrono::steady_clock::now();f();samples[name].push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());}
int main(int argc,char**argv){try{
 if(argc!=5)throw std::runtime_error("Fresh benchmark root, Python, helper and ZIP directory required");
 std::filesystem::path root=argv[1],helper=argv[3],zips=argv[4];if(std::filesystem::exists(root))throw std::runtime_error("Benchmark root must be new");std::filesystem::create_directories(root);Target target{"0.9.0","addams-family-pal-eu-enfrde"};Manager::MemoryUsage peak{};
 for(unsigned batch=0;batch<7;++batch){
  auto store=root/std::to_string(batch);std::unique_ptr<Manager>m;measure("manager_open_empty",[&]{m=std::make_unique<Manager>(store,argv[2],helper,target);});
  measure("python_startup_only",[&]{if(run_mod_process({argv[2],"-I","-B","-c","pass"},store/"startup.log"))throw std::runtime_error("startup failed");});
  for(auto file:{"Super-Gomez-Gameplay-v1.0.zip","Speedrun-HUD-v1.0.zip","Mansion-Parallax-Demo-v1.0.zip","Configurable-Jump-Example-v1.0.zip"})measure(std::string("install_")+file,[&]{m->install(zips/file);});
  measure("enumerate_four_helper",[&]{if(run_mod_process({argv[2],"-I","-B",helper.u8string(),"--root",store.u8string(),"list"},store/"list.log"))throw std::runtime_error("list failed");});
  auto rows=m->records();for(auto&r:rows)if(r.id!="nes-native-lab.configurable-jump")measure("enable_"+r.id,[&]{m->enable(r.id,true);});Observation o;o.frame=1;o.simulation_seconds=1;m->complete(o);auto mem=m->memory_usage();peak.lua=std::max(peak.lua,mem.lua);peak.images=std::max(peak.images,mem.images);peak.planes=std::max(peak.planes,mem.planes);
  measure("save_profile",[&]{m->profile("save","Combined");});measure("profile_to_original",[&]{m->profile("apply","Original");});if(m->memory_usage().lua||m->memory_usage().images||m->memory_usage().planes)throw std::runtime_error("cleanup retained resources");
  measure("profile_restore_three",[&]{m->profile("apply","Combined");});rows=m->records();for(auto&r:rows)if(r.enabled)measure("disable_"+r.id,[&]{m->enable(r.id,false);});
  m->enable("nes-native-lab.configurable-jump",true);auto config=*std::find_if(m->records().begin(),m->records().end(),[](auto&r){return r.id=="nes-native-lab.configurable-jump";});measure("configuration_noop",[&]{if(m->configure(config.id,config.settings))throw std::runtime_error("unchanged config restarted");});m->profile("apply","Original");
  auto file=store/"portable.json";measure("export_profile",[&]{m->export_profile("Combined",file);});m->profile("delete","Combined");measure("inspect_profile",[&]{m->inspect_profile(file);});measure("import_profile_inactive",[&]{m->import_profile(file);});if(m->active())throw std::runtime_error("import activated scripts");
 }
 for(auto&sample:samples){std::sort(sample.second.begin(),sample.second.end());std::cout<<sample.first<<" median_ms="<<sample.second[sample.second.size()/2]<<" samples="<<sample.second.size()<<'\n';}
 std::cout<<"peak_tracked_lua_bytes="<<peak.lua<<" owned_image_bytes="<<peak.images<<" shared_plane_bytes="<<peak.planes<<" cleanup_tracked_bytes=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
