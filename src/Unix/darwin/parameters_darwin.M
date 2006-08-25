/* Stefan Kuhr 2004 */




#include "sysdeps.h"
#include "tools.h"
#include "parameters.h"
#include "host_filesys.h"



#ifdef HAVE_NEW_HEADERS
# include <cstdlib>
#else
# include <stdlib.h>
#endif
#import <Cocoa/Cocoa.h>




int get_geometry(char */*dev_path*/, geo_type /*geo*/) {
  return -1;
}




//  Get the path to the user's home folder.
char *HostFilesys::getHomeFolder(char *buffer, unsigned int bufsize)
{
	NSString *homePath = [@"~/" stringByExpandingTildeInPath];
	[homePath getCString: buffer maxLength: bufsize];
	return(buffer);
}




//  Get the path to folder with user-specific configuration files (configuration, NVRAM)
//  Under Mac OS X this could be for example one of these:
//		~/Library/Preferences
//		/Library/Preferences
//		/System/Library/Preferences
char *HostFilesys::getConfFolder(char *buffer, unsigned int bufsize)
{
	//  set the preferences folder name
	NSString *preferencesPath = @"Preferences";
	//  get an array of all library pathes
	NSArray* constLibrarySearchPaths = NSSearchPathForDirectoriesInDomains(NSAllLibrariesDirectory, NSAllDomainsMask, true);
	NSMutableArray* librarySearchPaths = [NSMutableArray array];

	NSString* currentPath;
	//  iterate through all librarys and append the preference folder
	for (NSEnumerator* searchPathEnum = [constLibrarySearchPaths objectEnumerator]; (currentPath = [searchPathEnum nextObject]) != nil; )
		//  construct whole path description like ~/Library/Preferences
		[librarySearchPaths addObject: [[currentPath stringByAppendingPathComponent: preferencesPath] stringByStandardizingPath]];

	//  helper to check existence
	NSFileManager* fileManager = [NSFileManager defaultManager];
	BOOL isDirectory = NO;

	//  iterate through all librarys and check if they exist
	for (NSEnumerator* searchPathEnum = [librarySearchPaths objectEnumerator]; (currentPath = [searchPathEnum nextObject]) != nil; )
	{
		[fileManager fileExistsAtPath: currentPath isDirectory: &isDirectory];
		if (isDirectory)
			//  it's a valid and existing directory
			break;
	}

	//  if Preferences folder couldn't be found, take first choise
	if (currentPath == nil)
		currentPath = [librarySearchPaths objectAtIndex: 0];
	//  return this path, convert it to a C string and copy into buffer
	[currentPath getCString: buffer maxLength: bufsize];
	//  return the string as result
	return(buffer);
}





//  Get the path to folder with user-specific data files (ROM, EmuTOS)
//  Under Mac OS X this could be for example one of these:
//		~/Library/Application Support
//		/Library/Application Support
//		/System/Library/Application Support
char *HostFilesys::getDataFolder(char *buffer, unsigned int bufsize)
{
	//  get the application's bundle, thus it's name (ARAnyM)
	NSBundle* bundle=[NSBundle mainBundle];
	//  the subpath inside the library folders
	NSString *applicationSupportSubpath;

	if (bundle != nil)
		//  we can ask for the application's name
		applicationSupportSubpath = [@"Application Support/" stringByAppendingPathComponent: [[[bundle bundlePath] lastPathComponent] stringByDeletingPathExtension]];
	else
		//  we take the configured name
		applicationSupportSubpath = @"Application Support/"ARANYMHOME;

	//  get an array of all library pathes
	NSArray* constLibrarySearchPaths = NSSearchPathForDirectoriesInDomains(NSAllLibrariesDirectory, NSAllDomainsMask, true);
	NSMutableArray* librarySearchPaths = [NSMutableArray array];

	NSString* currentPath;
	//  iterate through all librarys and append Application Support folder 
	for (NSEnumerator* searchPathEnum = [constLibrarySearchPaths objectEnumerator]; (currentPath = [searchPathEnum nextObject]) != nil; )
		//  construct whole path description like ~/Library/Application Support
		[librarySearchPaths addObject: [[currentPath stringByAppendingPathComponent: applicationSupportSubpath] stringByStandardizingPath]];

	//  add Application Support path inside the bundle
	if (bundle != nil)
	{
		//  get bundle's path and build path to Application Support folder inside the bundle
		NSString* applicationSupportPathInsideBundle = [[bundle bundlePath] stringByAppendingPathComponent: @"Contents/"ARANYMHOME];
//		if ([librarySearchPaths count] >= 1)
//			[librarySearchPaths insertObject: applicationSupportPathInsideBundle atIndex: 1];
//		else
			[librarySearchPaths addObject: applicationSupportPathInsideBundle];
	}

	//  helper to check existence
	NSFileManager* fileManager = [NSFileManager defaultManager];
	BOOL isDirectory = NO;
		
	//  iterate through the pathes to find first existing one
	for (NSEnumerator* searchPathEnum = [librarySearchPaths objectEnumerator]; (currentPath = [searchPathEnum nextObject]) != nil; )
	{
		//  check if the path exists
		[fileManager fileExistsAtPath: currentPath isDirectory: &isDirectory];
		if (isDirectory)
			//  it's a valid and existing directory
			break;
	}
	//  if Application Support folder couldn't be found, take the first choise
	if (currentPath == nil)
		//currentPath = [librarySearchPaths objectAtIndex: 0];
		currentPath = @"*";
	//  return this path, convert it to a C string and copy into buffer
	[currentPath getCString: buffer maxLength: bufsize];
	//  return the string as result
	return(buffer);
}

int HostFilesys::makeDir(char *filename, int perm)
{
	return mkdir(filename, perm);
}


