#import <Cocoa/Cocoa.h>
#include <string>
std::string choose_mod_zip_macos(){@autoreleasepool{NSOpenPanel* p=[NSOpenPanel openPanel];p.title=@"Install Lua Mod";p.allowedFileTypes=@[@"zip"];p.allowsMultipleSelection=NO;p.canChooseDirectories=NO;if([p runModal]!=NSModalResponseOK)return {};return std::string(p.URL.path.UTF8String);}}
