#import <Cocoa/Cocoa.h>
#include <string>
#include "runtime/ui_text.hpp"
std::string choose_rom_macos() {
    @autoreleasepool {
        [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
        NSOpenPanel* panel=[NSOpenPanel openPanel];
        panel.title=[NSString stringWithUTF8String:nes::ui::text::select_rom];
        panel.message=[NSString stringWithUTF8String:nes::ui::text::rom_instruction];
        panel.allowedFileTypes=@[@"nes"];panel.allowsMultipleSelection=NO;panel.canChooseDirectories=NO;
        if([panel runModal]!=NSModalResponseOK)return {};
        return std::string(panel.URL.path.UTF8String);
    }
}
