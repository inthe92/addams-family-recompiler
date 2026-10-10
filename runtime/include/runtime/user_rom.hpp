#pragma once
namespace nes::runtime {
// Only a locally imported ROM can provide executable functions and resources.
// The distributable frontend has no statically linked recovered game library.
class UserRomModule {
public:
    bool prepare(int argc,char** argv,bool interactive=true);
    ~UserRomModule();
private:
    void* handle_{};
};
}
