//	SDLMain.m - main entry point for our Cocoa-ized SDL app




#import <Cocoa/Cocoa.h>




@interface SDLMain : NSObject
{
}

- (BOOL) application: (NSApplication *) theApplication openFile: (NSString *) filename;
- (void) applicationDidFinishLaunching: (NSNotification *) aNotificaton;
- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) sender;
- (IBAction) showPrefs: (id) sender;
- (IBAction) makeFullscreen: (id) sender;
- (IBAction) makeScreenshot: (id) sender;
- (IBAction) reboot: (id) sender;
- (IBAction) debug: (id) sender;

@end
