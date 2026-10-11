#include "runtime/module_abi.hpp"
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=5)return 2;
    try {
        for(int i=1;i<=4;++i) {
            auto* handle=SDL_LoadObject(argv[i]);
            if(!handle)throw std::runtime_error(SDL_GetError());
            std::string error;
            try{nes::runtime::validate_module_abi(handle);}catch(const std::runtime_error& e){error=e.what();}
            SDL_UnloadObject(handle);
            const bool correct=i==4?error.empty():error.find(i==1?"version missing":"ABI mismatch")!=std::string::npos;
            if(!correct)throw std::runtime_error("Module ABI guard unexpected result: "+error);
            std::cout<<(i==1?"Missing ABI rejected":i==2?"Old ABI rejected":i==3?"ABI v2 rejected":"Current ABI accepted")<<": OK\n";
        }
        std::cout<<"4 real dynamic module ABI cases: PASS\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
