#import <Cocoa/Cocoa.h>
#include <string>
std::string choose_rom_macos() {
    @autoreleasepool {
        [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
        NSOpenPanel* panel=[NSOpenPanel openPanel];
        panel.title=@"Importa tu ROM compatible";
        panel.message=@"La aplicacion no incluye el juego. Selecciona tu ROM .nes europea compatible.";
        panel.allowedFileTypes=@[@"nes"];panel.allowsMultipleSelection=NO;panel.canChooseDirectories=NO;
        if([panel runModal]!=NSModalResponseOK)return {};
        return std::string(panel.URL.path.UTF8String);
    }
}
