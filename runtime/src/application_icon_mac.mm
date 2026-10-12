#import <Cocoa/Cocoa.h>
namespace nes::runtime {
bool set_bundle_application_icon() {
    @autoreleasepool {
        NSString* path=[[NSBundle mainBundle] pathForResource:@"nes-native-lab" ofType:@"icns"];
        if(!path)return false;
        NSImage* image=[[NSImage alloc] initWithContentsOfFile:path];
        if(!image)return false;
        // Keep all ICNS resolutions in the Dock; SDL's bitmap replaces them.
        [NSApp setApplicationIconImage:image];
        [image release];return true;
    }
}
}
