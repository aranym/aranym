//	SDLMain.M - main entry point for a Cocoa-ized SDL app




#import "SDL.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

#import "host.h"




static int	gArgc;
static char	**gArgv;
static BOOL	gFinderLaunch;

extern bool	start_GUI_thread();
extern void	grabTheMouse();
extern void	releaseTheMouse();




//  The main class of the application, the application's delegate
@implementation SDLMain




// Called when the internal event loop has just started running
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    int status;
	
    //  Hand off to main application code
    status = SDL_main(gArgc, gArgv);

    //  We're done, thank you for playing
    exit(status);
}




//  The user selected the "quit" menu entry
- (IBAction) quit:(id)obj
{
	//  Post a SDL_QUIT event
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);

	//  this will end the SDL event loop and hence end SDL_main()
	//  so terminating the NSApp in not necessary 
	//[NSApp terminate:obj];
}




//  The user selected the "Preferencs..." menu entry
- (IBAction) showPrefs:(id)obj
{
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_PRINT;
	event.key.keysym.mod = KMOD_NONE;
	SDL_PushEvent(&event);
}




//  The user selected the "Fullscreen" menu entry
- (IBAction) makeFullscreen:(id)obj
{
	releaseTheMouse();
	hostScreen.toggleFullScreen();
	grabTheMouse();

//	SDL_Event event;
//	event.type = SDL_KEYDOWN;
//	event.key.keysym.sym = SDLK_NUMLOCK;
//	event.key.keysym.mod = KMOD_NONE;
//	SDL_PushEvent(&event);
}




@end




#ifdef main
#  undef main
#endif




// Main entry point to executable - should *not* be SDL_main!
int main (int argc, char **argv)
{
    // Copy the arguments into a global variable

    // This is passed if we are launched by double-clicking
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgc = argc - 1;
        gArgv = &argv[1];
		gFinderLaunch = YES;
    } else {
        gArgc = argc;
		gArgv = argv;
		gFinderLaunch = NO;
    }

    NSApplicationMain (argc, const_cast<const char**>(argv));

    return 0;
}





