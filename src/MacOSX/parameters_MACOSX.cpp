/* MJ 2001 */

#include "sysdeps.h"
#include "tools.h"
#include "parameters.h"
#include "host_filesys.h"

# include <cstdlib>

int get_geometry(char *dev_path, geo_type geo) {
  return -1;
}

/*
 * Get the path to a user home folder.
 */
char *HostFilesys::getHomeFolder(char *buffer, unsigned int bufsize)
{
	buffer[0] = '\0';	// last resort - current folder

	// Unix-like systems define HOME variable as the user home folder
	char *home = getenv("HOME");

	if ( home )
		strncpy( buffer, home, bufsize );

	return buffer;
}

/*
 * Get the path to folder with user-specific files (configuration, NVRAM)
 */
char *HostFilesys::getConfFolder(char *buffer, unsigned int bufsize)
{
	// local cache
	static char path[512] = "";

	if (strlen(path) == 0) {
		char *home = HostFilesys::getHomeFolder(path, 512);

		int homelen = strlen(home);
		if (homelen > 0) {
			unsigned int len = strlen(ARANYMHOME);
			if ((homelen + 1 + len + 1) < bufsize) {
				strcpy(path, home);
				if (homelen)
					strcat(path, DIRSEPARATOR);
				strcat(path, ARANYMHOME);
			}
		}
	}

	return safe_strncpy(buffer, path, bufsize);
}

char *HostFilesys::getDataFolder(char *buffer, unsigned int bufsize)
{
 
 static char path[512]="";
 CFURLRef tosURL;

	if (strlen (path) ==0)
	{
		tosURL=CFBundleCopyResourcesDirectoryURL ( mainBundle);
		if (tosURL)
		{
			CFURLGetFileSystemRepresentation (tosURL, true, (uint8 *)path, 512);
			delete (tosURL);
			unsigned int len = strlen(path);
			if ((len+1) < 512) 
					strcat(path, DIRSEPARATOR);
		}
	}	
	return safe_strncpy(buffer, path, bufsize);
}

int HostFilesys::makeDir(char *filename, int perm)
{
	return mkdir(filename, perm);
}
