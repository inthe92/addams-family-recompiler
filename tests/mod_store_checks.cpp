#include "runtime/mod_manager.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace nes::mods;
static void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv){try{
    if(argc!=7)throw std::runtime_error("root python helper good-zip bad-zip order-zip required");std::filesystem::path root=argv[1];unsigned n=0;
    {
        Manager m(root,argv[2],argv[3]);m.install(argv[4]);require(m.records().size()==1&&!m.active(),"install default disabled");++n;
        m.enable("example.hello-native-lab",true);require(m.active(),"enable failed");++n;m.update({1,99,7,std::nullopt});m.enable("example.hello-native-lab",false);require(!m.active(),"disable failed");++n;
        m.enable("example.hello-native-lab",true);
    }
    {Manager m(root,argv[2],argv[3]);require(m.active(),"enabled persistence failed");++n;m.install(argv[5]);m.enable("test.bad",true);m.update({2,123,7,std::nullopt});
        require(!m.records()[1].enabled&&!m.records()[1].error.empty(),"failing script not disabled/persisted");++n;m.uninstall("test.bad");require(m.records().size()==1,"uninstall failed");++n;
        m.uninstall("example.hello-native-lab");require(!m.active()&&m.records().empty(),"unload/delete failed");++n;
    }
    std::ifstream input(root/"mods.log");std::string log((std::istreambuf_iterator<char>(input)),{});require(log.find("Hello from Lua!")!=std::string::npos&&log.find("PAL update")!=std::string::npos&&log.find("Mod unloaded")!=std::string::npos,"lifecycle logs missing");++n;
    {Manager m(root,argv[2],argv[3]);m.install(argv[4]);m.enable("example.hello-native-lab",true);}
    std::ofstream(root/"session-active")<<"synthetic crash marker";
    {Manager m(root,argv[2],argv[3]);require(!m.active()&&!m.records()[0].enabled,"unclean session auto-loaded mods");++n;}
    require(!std::filesystem::exists(root/"session-active"),"session marker leaked");++n;
    {auto ordered=root.parent_path()/"ordered store";Manager m(ordered,argv[2],argv[3]);m.install(argv[4]);m.enable("example.hello-native-lab",true);m.install(argv[6]);m.enable("aaa.first",true);m.update({1,99,7,std::nullopt});std::ifstream in(ordered/"mods.log");std::string lines((std::istreambuf_iterator<char>(in)),{});require(lines.find("aaa.first: FIRST")<lines.find("example.hello-native-lab: PAL update"),"callback order not stable by ID");++n;}
    std::cout<<"Managed store cases="<<n<<" PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
