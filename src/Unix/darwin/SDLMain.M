//	SDLMain.M - main entry point for a Cocoa-ized SDL app




#import "SDL_compat.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

#import "host.h"




#define UNUSED(param) ((void)param)




static int		gArgc;
static char		**gArgv;
static bool		gFinderLaunch;
static bool		gCalledAppMainline(false);
static char*	gConfigFile(NULL);








//	The main class of the application, the application's delegate.
@implementation SDLMain




//	Catch document open requests. 
//	Files are added to gArgv, so to the app, they'll look like command line arguments. 
//	This message may be received multiple times to open several docs on launch.
//	This message is ignored once the app's mainline has been called.
- (BOOL) application: (NSApplication *) theApplication openFile: (NSString *) filename {
	UNUSED(theApplication);

	size_t		filenamelen;
	const char*	configfile;
	char**		argv;
	
	if (gCalledAppMainline)
		//	The pplication has already been started, ignore this document.
		return(false);
	
	if (!gFinderLaunch)	
		//	Command line arguments have been passed.
		return(false);
	
	//	The gFinderLaunch is set, thus argv was "<app> -psn <number>" and gArgv now is "<app>".
	//	As there is space left in original agrv to now hold "<app> -c <config>".
	
	BOOL			isDir;
	NSFileManager*	fileManager = [NSFileManager defaultManager];
	if ([fileManager fileExistsAtPath: filename isDirectory: &isDir]) {
		if (isDir) {
			NSString*	configfilename;
			
			configfilename = [filename stringByAppendingPathComponent: @ARANYMCONFIG];
			if ([fileManager fileExistsAtPath: configfilename]) {
				filename = configfilename;
			} else {
				[configfilename release];
				configfilename = [filename stringByAppendingPathComponent: @"config"];
				if ([fileManager fileExistsAtPath: configfilename]) {
					filename = configfilename;
				} else {
					[configfilename release];
					return(false);
				}
			}
		}
		
		//	Allocate memory to keep the config file name.
		configfile = [filename UTF8String];
		filenamelen = SDL_strlen(configfile) + 1;
		gConfigFile = (char *)SDL_malloc(filenamelen);
		if (gConfigFile == NULL)
			return(false);
		SDL_strlcpy(gConfigFile, configfile, filenamelen);
		
		//	Allocate memory to hold the new argument pointers.
		argv = (char**)SDL_malloc(4 * sizeof(char*));
		if (argv == NULL) {
			SDL_free(gConfigFile);
			gConfigFile = NULL;
			return(false);
		}
			
		//	Build the "<app> -c <cofing>" arguments.
		argv[0] = gArgv[0];
		argv[1] = "-c";
		argv[2] = gConfigFile;
		argv[3] = NULL;
		
		//	Set the new into the global arguments.
		gArgv = argv;
		gArgc = 3;
		
		//	ARAnyM can only handle a single config file, thus this flag is set to ignore further files.
		gCalledAppMainline = true;
	}
	return(true);
}




//	Called when the internal event loop has just started running.
- (void) applicationDidFinishLaunching: (NSNotification *) aNotification {
	UNUSED(aNotification);
	int status;
	
	//	Remember that application has started.
	gCalledAppMainline = true;
	SDL_SetMainReady();
	
	//	Hand over to main application code.
	status = SDL_main(gArgc, gArgv);
	
	//	Terminate the application.
	//[NSApp terminate: NSApp];
	//[NSApp stop: self];
	
	//	Release fake arguments again.
	if (gConfigFile != NULL) {
		SDL_free(gArgv);
		SDL_free(gConfigFile);
	}
	
	//	We're done, thank you for playing.
	exit(status);
}




//	Called when the application should terminate.
- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) theApplication {
	UNUSED(theApplication);
	//	Post a SDL_QUIT event
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
	//	Cancel the termination, as the SDL_QUIT signal will finally lead to an exit() call.
	return(NSTerminateCancel);
}




//	The user selected the "Preferencs..." menu entry.
- (IBAction) showPrefs: (id) sender {
	UNUSED(sender);
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.setup.sym;
	event.key.keysym.mod = bx_options.hotkeys.setup.mod;
	SDL_PushEvent(&event);
}




//	The user selected the "Fullscreen" menu entry.
- (IBAction) makeFullscreen: (id) sender {
	UNUSED(sender);
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.fullscreen.sym;
	event.key.keysym.mod = bx_options.hotkeys.fullscreen.mod;
	SDL_PushEvent(&event);
}




//	The user selected the "Screenshot" menu entry.
- (IBAction) makeScreenshot: (id) sender {
	UNUSED(sender);
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.screenshot.sym;
	event.key.keysym.mod = bx_options.hotkeys.screenshot.mod;
	SDL_PushEvent(&event);
}




//	The user selected the "Reboot" menu entry */
- (IBAction) reboot: (id) sender {
	UNUSED(sender);
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.reboot.sym;
	event.key.keysym.mod = bx_options.hotkeys.reboot.mod;
	SDL_PushEvent(&event);
}




//	The user selected the "Debug" menu entry.
- (IBAction) debug: (id) sender {
	UNUSED(sender);
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = bx_options.hotkeys.debug.sym;
	event.key.keysym.mod = bx_options.hotkeys.debug.mod;
	SDL_PushEvent(&event);
}




@end




#ifdef main
	#undef main
#endif




static int IsRootCwd(void)
{
    char buf[MAXPATHLEN];
    char *cwd = getcwd(buf, sizeof (buf));
    return cwd != NULL && strcmp(cwd, "/") == 0;
}

static int IsFinderLaunch(int argc, char **argv)
{
    /* -psn_XXX is passed if we are launched from Finder, SOMETIMES */
    if ( (argc >= 2) && (strncmp(argv[1], "-psn", 4) == 0) ) {
        return 1;
    } else if ((argc == 1) && IsRootCwd()) {
        /* we might still be launched from the Finder; on 10.9+, you might not
        get the -psn command line anymore. If there's no
        command line, and if our current working directory is "/", it
        might as well be a Finder launch. */
        return 1;
    }
    return 0;  /* not a Finder launch. */
}


//	Main entry point to executable.
int main (int argc, char **argv) {
	//	Copy the arguments into a global variable.

	//	This is passed if we are launched by double-clicking.
	if (IsFinderLaunch(argc, argv)) {
		gArgc = 1;
		gArgv = argv;
		gArgv[1] = NULL;
		gFinderLaunch = true;
	} else {
		gArgc = argc;
		gArgv = argv;
		gFinderLaunch = false;
	}

	NSApplicationMain(argc, const_cast<const char**>(argv));

	return(0);
}
