#import <Cocoa/Cocoa.h>
#include <string>
std::string choose_mod_zip_macos(){@autoreleasepool{NSOpenPanel* p=[NSOpenPanel openPanel];p.title=@"Install Mod";p.allowedFileTypes=@[@"zip"];p.allowsMultipleSelection=NO;p.canChooseDirectories=NO;if([p runModal]!=NSModalResponseOK)return {};return std::string(p.URL.path.UTF8String);}}

std::string choose_mod_profile_macos(bool save,const std::string&name){@autoreleasepool{
 NSWindow* owner=[NSApp keyWindow];
 NSSavePanel* panel;
 if(save){panel=[NSSavePanel savePanel];panel.title=@"Export Mod Profile";panel.nameFieldStringValue=[NSString stringWithUTF8String:(name+".json").c_str()];}
 else{NSOpenPanel* open=[NSOpenPanel openPanel];open.title=@"Import Mod Profile";open.allowsMultipleSelection=NO;open.canChooseDirectories=NO;panel=open;}
 panel.allowedFileTypes=@[@"json"];
 auto result=[panel runModal];
 // Restore only this application's existing window, never steal another app's focus.
 if(owner&&[NSApp isActive])[owner makeKeyAndOrderFront:nil];
 if(result!=NSModalResponseOK)return {};
 return std::string(panel.URL.path.UTF8String);
}}
