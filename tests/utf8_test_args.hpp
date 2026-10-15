#pragma once
// Match native Unicode file pickers when a Python fixture launches a Win32 main.
#include <string>
#include <vector>
#include <utility>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
namespace nes::test {
class Utf8Args {
    std::vector<std::string> strings_;
    std::vector<char*> pointers_;
public:
    Utf8Args(int argc,char**argv){
#ifdef _WIN32
        int count{};auto wide=CommandLineToArgvW(GetCommandLineW(),&count);
        if(!wide)throw std::runtime_error("Unable to read Unicode fixture arguments");
        try{for(int i=0;i<count;++i){auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide[i],-1,nullptr,0,nullptr,nullptr);
            if(!size)throw std::runtime_error("Invalid Unicode fixture argument");
            std::string text(size,'\0');if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide[i],-1,text.data(),size,nullptr,nullptr))throw std::runtime_error("UTF-8 fixture conversion failed");
            text.pop_back();strings_.push_back(std::move(text));}}
        catch(...){LocalFree(wide);throw;}LocalFree(wide);
#else
        for(int i=0;i<argc;++i)strings_.emplace_back(argv[i]);
#endif
        for(auto&text:strings_)pointers_.push_back(text.data());pointers_.push_back(nullptr);
    }
    int size()const{return int(strings_.size());}
    char**data(){return pointers_.data();}
};
}
