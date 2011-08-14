/*	 SDLMain.m - main entry point for our Cocoa-ized SDL app
	   Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
	   Non-NIB-Code & other changes: Max Horn <max@quendi.de>

	Feel free to customize this file to suit your needs
	Adapted for aranym
*/

#import "SDL.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>
#import "parameters.h"


static int gArgc;
static char **gArgv;
static BOOL gFinderLaunch;
static BOOL gCalledAppMainline = FALSE;
char gAranymFilesDirectory[MAXPATHLEN];	 // Path to the "AranymFiles" folder


@interface SDLApplication : NSApplication
@end

@implementation SDLApplication
/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
	/* Post a SDL_QUIT event */
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}
@end

/* The main class of the application, the application's delegate */
@implementation SDLMain

/* The user selected the "Preferencs..." menu entry */
- (IBAction)doSetup:(id)sender
{
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.setup.sym;
	event.key.keysym.mod = bx_options.hotkeys.setup.mod;
	SDL_PushEvent(&event);
}


/* The user selected the "Fullscreen" menu entry */
- (IBAction)doFullscreen:(id)sender
{
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.fullscreen.sym;
	event.key.keysym.mod = bx_options.hotkeys.fullscreen.mod;
	SDL_PushEvent(&event);
}

/* The user selected the "Screenshot" menu entry */
- (IBAction)doScreenshot:(id)sender
{
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.screenshot.sym;
	event.key.keysym.mod = bx_options.hotkeys.screenshot.mod;
	SDL_PushEvent(&event);
}

/* The user selected the "Reboot" menu entry */
- (IBAction)doReboot:(id)sender
{
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.reboot.sym;
	event.key.keysym.mod = bx_options.hotkeys.reboot.mod;
	SDL_PushEvent(&event);
}

/* The user selected the "Reboot" menu entry */
- (IBAction)doDebug:(id)sender
{
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.debug.sym;
	event.key.keysym.mod = bx_options.hotkeys.debug.mod;
	SDL_PushEvent(&event);
}

/* A nice joke :-) */
- (IBAction)help:(id)sender
{
	NSRunAlertPanel (@"Oh help, where have ye gone?", 
		@"Sorry, there is no help available.\n\nThis message brought to you by We Don't Document, Inc.\n\n", @"Rats", @"Good, I never read it anyway", nil);
}


/* Set the working directory to the .app's parent directory */
- (void) setupWorkingDirectory:(BOOL)shouldChdir
{
	if (shouldChdir)
	{
		char parentdir[MAXPATHLEN];
		CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
		if (CFURLGetFileSystemRepresentation(url2, true, (UInt8 *)parentdir, MAXPATHLEN)) {
			assert ( chdir (parentdir) == 0 );	/* chdir to the binary app's parent */
		}
		CFRelease(url);
		CFRelease(url2);
	}
}

- (void) findAranymFilesDirectory
{
	//	 get the application's bundle, thus it's name (ARAnyM)
	NSBundle* bundle=[NSBundle mainBundle];
	NSDictionary *infoDict = [bundle infoDictionary];
	
	NSArray* searchPaths = [infoDict objectForKey: @"AranymHomeDirectories"];
	
	//	 helper to check existence
	NSFileManager* fileManager = [NSFileManager defaultManager];
	BOOL isDirectory = NO;

	//	 iterate through all folders and check if they exist
	NSEnumerator* searchPathEnum = [searchPaths objectEnumerator];
	NSString* currentPath;
	while ((currentPath = [searchPathEnum nextObject]) != nil)
	{
		currentPath = [[currentPath stringByExpandingTildeInPath] stringByStandardizingPath];
		[fileManager fileExistsAtPath: currentPath isDirectory: &isDirectory];
		[currentPath getCString: gAranymFilesDirectory maxLength: MAXPATHLEN encoding: NSUTF8StringEncoding];
		printf("--> %s %d\n", gAranymFilesDirectory, isDirectory);
		
		if (isDirectory)
			//  it's a valid and existing directory
			break;
	}
	
	//	 if Preferences folder couldn't be found, take first choise
	if (currentPath == nil)
		currentPath = [[[searchPaths objectAtIndex: 0] stringByExpandingTildeInPath] stringByStandardizingPath];

	//	 store this path, convert it to a C string and copy into buffer
	[currentPath getCString: gAranymFilesDirectory maxLength: MAXPATHLEN encoding: NSUTF8StringEncoding];
}


/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	int status;

	/* Set the working directory to the .app's parent directory */
	[self setupWorkingDirectory:gFinderLaunch];
	[self findAranymFilesDirectory];

#ifdef ENABLE_LILO
	/* Enable linux booting if available! */
	boot_lilo = 1;
#endif

	/* Hand off to main application code */
	gCalledAppMainline = TRUE;
	status = SDL_main (gArgc, gArgv);

	/* We're done, thank you for playing */
	exit(status);
}
@end




#ifdef main
#  undef main
#endif


/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, char **argv)
{
	/* Copy the arguments into a global variable */
	/* This is passed if we are launched by double-clicking */
	if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) 
	{
		gArgv = (char **) SDL_malloc(sizeof (char *) * 2);
		gArgv[0] = argv[0];
		gArgv[1] = NULL;
		gArgc = 1;
		gFinderLaunch = YES;
	} 
	else 
	{
		int i;
		gArgc = argc;
		gArgv = (char **) SDL_malloc(sizeof (char *) * (argc+1));
		for (i = 0; i <= argc; i++)
			gArgv[i] = argv[i];
		gFinderLaunch = NO;
	}

	[SDLApplication poseAsClass:[NSApplication class]];
	NSApplicationMain (argc, (const char **)argv);

	return 0;
}
