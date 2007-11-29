/* MJ 2001 */

#include <CoreFoundation/CoreFoundation.h>
#include "sysdeps.h"
#include "parameters.h"
#include "tools.h"
#include "host_filesys.h"

#include <cstdlib>


extern char gAranymFilesDirectory[MAXPATHLEN];

int get_geometry(const char *dev_path, geo_type geo) {
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

	if (home)
		strncpy(buffer, home, bufsize);

	return buffer;
}

/*
 * Get the path to folder with user-specific files (configuration, NVRAM)
 */
char *HostFilesys::getConfFolder(char *buffer, unsigned int bufsize)
{
	//printf("Conf folder ---------> %s\n", gAranymFilesDirectory);
	return safe_strncpy(buffer, gAranymFilesDirectory, bufsize);
}

char *HostFilesys::getDataFolder(char *buffer, unsigned int bufsize)
{
 	//printf("Data folder ---------> %s\n", gAranymFilesDirectory);
	return safe_strncpy(buffer, gAranymFilesDirectory, bufsize);
}

int HostFilesys::makeDir(char *filename, int perm)
{
	return mkdir(filename, perm);
}

