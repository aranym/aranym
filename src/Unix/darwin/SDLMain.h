/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>

@interface SDLMain : NSObject
{
}

- (void) applicationDidFinishLaunching: (NSNotification *) note;
- (void) applicationWillTerminate: (NSNotification *) aNotification;
- (IBAction) quit:(id)obj;
- (IBAction) showPrefs:(id)obj;
- (IBAction) makeFullscreen:(id)obj;

@end
