#include "runtime/user_rom.hpp"
#include "game/compiled_program.h"
#include "game/recovered_resources.h"
#include "game/world_maps.h"
#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <process.h>
#include <codecvt>
#include <locale>
#else
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif
#ifdef __APPLE__
std::string choose_rom_macos();
#endif

extern "C" {
uint8_t (*imported_execute_slice)(CPUState*,const RuntimeServices*,uint32_t*)=nullptr;
uint8_t (*imported_has_address)(uint8_t,uint16_t)=nullptr;
const uint8_t (*imported_prg_data)[16384]=nullptr;
const uint8_t* imported_chr_data=nullptr;
uint16_t imported_reset_vector=0;
const GameWorldMap* imported_world_maps=nullptr;
const GameWorldEdit* imported_level13_edits=nullptr;
}
namespace nes::runtime {
namespace {
namespace fs=std::filesystem;
int run_python(const std::vector<std::string>& arguments) {
    std::vector<const char*> args;
    for(const auto& item:arguments)args.push_back(item.c_str());args.push_back(nullptr);
#ifdef _WIN32
    // argv spawning avoids shell interpretation of spaces and metacharacters.
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::vector<std::wstring> wide;
    for(const auto& item:arguments)wide.push_back(converter.from_bytes(item));
    std::vector<const wchar_t*> pointers;
    for(const auto& item:wide)pointers.push_back(item.c_str());pointers.push_back(nullptr);
    return static_cast<int>(_wspawnvp(_P_WAIT,pointers[0],pointers.data()));
#else
    pid_t child=0;
    const int error=posix_spawnp(&child,args[0],nullptr,nullptr,const_cast<char**>(args.data()),environ);
    if(error)return -1;
    int status=0;if(waitpid(child,&status,0)<0)return -1;
    return WIFEXITED(status)?WEXITSTATUS(status):-1;
#endif
}
std::string choose_rom() {
#ifdef __APPLE__
    return choose_rom_macos();
#elif defined(_WIN32)
    wchar_t file[32768]{};
    OPENFILENAMEW panel{};panel.lStructSize=sizeof(panel);panel.lpstrFile=file;panel.nMaxFile=32768;
    panel.lpstrTitle=L"Importa tu ROM compatible";
    panel.lpstrFilter=L"NES ROM\0*.nes\0All files\0*.*\0";
    panel.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    return GetOpenFileNameW(&panel)?fs::path(file).u8string():std::string{};
#else
    // No desktop-specific file-picker dependency is needed in the AppImage.
    if(SDL_Init(SDL_INIT_VIDEO)!=0)throw std::runtime_error(SDL_GetError());
    auto* window=SDL_CreateWindow("Arrastra tu archivo .nes aqui para importarlo",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,720,240,0);
    if(!window){SDL_Quit();throw std::runtime_error(SDL_GetError());}
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"Importar ROM",
        "Esta aplicacion no incluye el juego.\n\nArrastra tu ROM .nes a esta ventana.\nTambien puedes ejecutarla con --rom /ruta/juego.nes.\n\nPython y el compilador estan incluidos en este paquete.",window);
    auto* surface=SDL_GetWindowSurface(window);
    if(surface){SDL_FillRect(surface,nullptr,SDL_MapRGB(surface->format,24,32,50));SDL_UpdateWindowSurface(window);}
    std::string selected;SDL_Event event{};bool running=true;
    SDL_EventState(SDL_DROPFILE,SDL_ENABLE);
    while(running&&SDL_WaitEvent(&event)) {
        if(event.type==SDL_DROPFILE){selected=event.drop.file;SDL_free(event.drop.file);running=false;}
        if(event.type==SDL_QUIT)running=false;
    }
    SDL_DestroyWindow(window);SDL_Quit();return selected;
#endif
}
void* symbol(void* module,const char* name) {
    auto* result=SDL_LoadFunction(module,name);
    if(!result)throw std::runtime_error(std::string("Modulo local incompleto: ")+name);
    return result;
}
}
bool UserRomModule::prepare(int argc,char** argv,bool interactive) {
    fs::path rom,cache,tools;bool import_only=false;
    for(int i=1;i<argc;++i) {
        const std::string arg=argv[i];
        if(arg=="--import-only"){import_only=true;continue;}
        if(arg=="--rom"||arg=="--import-cache"||arg=="--import-tools") {
            if(++i>=argc)throw std::invalid_argument("Falta el valor de "+arg);
            if(arg=="--rom")rom=fs::u8path(argv[i]);
            else if(arg=="--import-cache")cache=fs::u8path(argv[i]);
            else tools=fs::u8path(argv[i]);
        }
    }
    if(cache.empty()) {
        char* value=SDL_GetPrefPath("nes-native-lab","user-import");
        if(!value)throw std::runtime_error(SDL_GetError());cache=fs::u8path(value);SDL_free(value);
    }
    if(tools.empty()) {
        char* value=SDL_GetBasePath();if(!value)throw std::runtime_error(SDL_GetError());
        fs::path base=fs::u8path(value);SDL_free(value);
        tools=base/"tools/recompiler";
    }
    const fs::path remembered=cache/"rom_path.txt";
    if(rom.empty()&&fs::exists(remembered)) {
        std::ifstream input(remembered);std::string value;std::getline(input,value);
        if(fs::is_regular_file(fs::u8path(value)))rom=fs::u8path(value);
    }
    if(rom.empty()&&interactive)rom=fs::u8path(choose_rom());
    if(rom.empty())return false;
    fs::create_directories(cache);
    const fs::path log=cache/"import.log";
#ifdef __APPLE__
    std::string python="/usr/bin/python3";
#elif defined(_WIN32)
    std::string python="python";
#else
    std::string python="python3";
#endif
    const fs::path resource_root=tools.parent_path().parent_path();
#ifdef _WIN32
    const fs::path bundled_python=resource_root/"toolchain/python/python.exe";
#else
    const fs::path bundled_python=resource_root/"toolchain/python/bin/python3";
#endif
    if(fs::is_regular_file(bundled_python))python=bundled_python.u8string();
    auto task=std::async(std::launch::async,[&]{return run_python({python,"-B",(tools/"import_user_rom.py").u8string(),
        "--rom",fs::absolute(rom).u8string(),"--cache",fs::absolute(cache).u8string(),"--log",log.u8string()});});
    SDL_Window* progress=nullptr;
    if(interactive&&!import_only&&SDL_Init(SDL_INIT_VIDEO)==0) {
        progress=SDL_CreateWindow("Importando ROM... primera compilacion: varios minutos",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,720,180,0);
        if(progress){auto* surface=SDL_GetWindowSurface(progress);if(surface){SDL_FillRect(surface,nullptr,SDL_MapRGB(surface->format,24,32,50));SDL_UpdateWindowSurface(progress);}}
    }
    while(task.wait_for(std::chrono::milliseconds(25))!=std::future_status::ready) {
        if(progress){SDL_Event event{};while(SDL_PollEvent(&event)){};SDL_UpdateWindowSurface(progress);}
    }
    const int result=task.get();if(progress){SDL_DestroyWindow(progress);SDL_Quit();}
    if(result!=0) {
        std::ifstream input(log);std::string details((std::istreambuf_iterator<char>(input)),{});
        if(details.size()>2500)details=details.substr(details.size()-2500);
        throw std::runtime_error("No se ha podido importar tu ROM.\nConsulta el registro de importacion.\n"+details+"\nRegistro: "+log.u8string());
    }
    std::ofstream(remembered)<<fs::absolute(rom).u8string()<<'\n';
    if(import_only)return false;
#ifdef __APPLE__
    const fs::path module=cache/"user_game.dylib";
#elif defined(_WIN32)
    const fs::path module=cache/"user_game.dll";
#else
    const fs::path module=cache/"user_game.so";
#endif
    handle_=SDL_LoadObject(module.u8string().c_str());
    if(!handle_)throw std::runtime_error(SDL_GetError());
    imported_execute_slice=reinterpret_cast<decltype(imported_execute_slice)>(symbol(handle_,"game_execute_slice"));
    imported_has_address=reinterpret_cast<decltype(imported_has_address)>(symbol(handle_,"game_has_compiled_address"));
    imported_prg_data=static_cast<const uint8_t(*)[16384]>(symbol(handle_,"game_prg_data"));
    imported_chr_data=static_cast<const uint8_t*>(symbol(handle_,"game_chr_data"));
    imported_reset_vector=*static_cast<const uint16_t*>(symbol(handle_,"game_reset_vector"));
    imported_world_maps=static_cast<const GameWorldMap*>(symbol(handle_,"game_world_maps"));
    imported_level13_edits=static_cast<const GameWorldEdit*>(symbol(handle_,"game_level13_edits"));
    return true;
}
UserRomModule::~UserRomModule(){if(handle_)SDL_UnloadObject(handle_);}
}
