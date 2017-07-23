/*	 SDLMain.m - main entry point for our Cocoa-ized SDL app
	   Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
	   Non-NIB-Code & other changes: Max Horn <max@quendi.de>

	Feel free to customize this file to suit your needs
	Adapted for aranym
*/

#import "SDL_compat.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>
#import "parameters.h"

static int gArgc;
static char **gArgv;
static BOOL gFinderLaunch;
static BOOL gCalledAppMainline = FALSE;
char gAranymFilesDirectory[MAXPATHLEN];	 // Path to the "AranymFiles" folder


@interface NSApplication (SDLApplication)
@end

@implementation NSApplication (SDLApplication)
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
	event.key.keysym.sym = bx_options.hotkeys.warmreboot.sym;
	event.key.keysym.mod = bx_options.hotkeys.warmreboot.mod;
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
		
		if (isDirectory)
			//  it's a valid and existing directory
			break;
	}
	
	//	 if Preferences folder couldn't be found, take last choice
	if (currentPath == nil)
		currentPath = [[[searchPaths lastObject] stringByExpandingTildeInPath] stringByStandardizingPath];

	//	 store this path, convert it to a C string and copy into buffer
	[currentPath getFileSystemRepresentation: gAranymFilesDirectory maxLength: MAXPATHLEN];
	char realPath[MAXPATHLEN];	 // Temporary storage for the resolved path
	realpath(gAranymFilesDirectory, realPath);
	strncpy(gAranymFilesDirectory, realPath, MAXPATHLEN);
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


void guialert(const char *fmt, ...)
{
	va_list args;
	char *buf = NULL;
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
	va_start(args, fmt);
	(void) vasprintf(&buf, fmt, args);
	va_end(args);
	NSRunAlertPanel (@"Error:", @"%s", @"Ok", nil, nil, buf);
	free(buf);
}


#if !SDL_VERSION_ATLEAST(2, 0, 0)
SDL_GLContext SDL_GL_GetCurrentContext(void)
{
	return [NSOpenGLContext currentContext];
}
#endif


/*
 * Only called with ctx != 0,
 * to restore the current SDL context
 */
void SDL_GL_SetCurrentContext(SDL_GLContext ctx)
{
    NSAutoreleasePool *pool;
    pool = [[NSAutoreleasePool alloc] init];
	NSOpenGLContext *nscontext = (NSOpenGLContext *)ctx;
	[nscontext makeCurrentContext];
    [pool release];
}


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

	NSApplicationMain (argc, (const char **)argv);

	return 0;
}
