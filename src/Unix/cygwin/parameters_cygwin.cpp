/* MJ 2001 */

#include "sysdeps.h"
#include "config.h"
#include "tools.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

#ifdef HAVE_NEW_HEADERS
# include <cstdlib>
#else
# include <stdlib.h>
#endif

#define ARADATA		"aranym"

int get_geometry(char *dev_path, geo_type geo) {
  return -1;
}

// If Unix-like filesystem hierarchy (/home/$USER, /usr/share/aranym) is not
// available then Cygwin sets HOME to "/cygdrive/c" automagically.
// In such case we define the user folder by Windows environment variables
// data folder is in "aranym" subfolder relative to ARAnyM executable.
// Note that the FHS detection (Unix vs Windows) is an unreliable hack.
// You should think about a configure option which would clearly define
// whether this binary is Windows or Cygwin ready.

#ifndef IS_CYGWIN_FHS
#  define CYGWIN_FAKE_HOME	"/cygdrive/c"
#  define IS_CYGWIN_FHS	(getenv("HOME") && strcmp(getenv("HOME"), CYGWIN_FAKE_HOME))
#endif /* IS_CYGWIN_FHS */

/*
 * Get the path to folder with user-specific files (configuration, NVRAM)
 */
char *getConfFolder(char *buffer, unsigned int bufsize)
{
	// Unix-like systems define HOME variable as the user home folder
	if (IS_CYGWIN_FHS) {
		safe_strncpy(buffer, getenv("HOME"), bufsize);
	}
	// If not then use registry to find out the "My Documents" folder
	else if (get_home_dir(buffer, bufsize)) {
		buffer[0] = '\0';	// last resort - current folder
	}
	return addFilename(buffer, ARANYMHOME, bufsize);
}

char *getDataFolder(char *buffer, unsigned int bufsize)
{
	// test if Unix-like filesystem is in place
	// if it's not, data folder path is extracted from argv[0]
	// (path to aranym executable) + ARADATA subfolder
	if (IS_CYGWIN_FHS) {
		return safe_strncpy(buffer, ARANYM_DATADIR, bufsize);
	}

	// remember path to program
	safe_strncpy(buffer, program_name, bufsize);
	// strip out filename and separator from the path
	char *ptr = strrchr(buffer, '/');	// first try Unix separator
	if (ptr != NULL)
		ptr[0] = '\0';
	else if ((ptr = strrchr(buffer, '\\')) != NULL)	// then DOS sep.
		ptr[0] = '\0';
	else
		buffer[0] = '\0';	// last resort - complete filename out

	return addFilename(buffer, ARADATA, bufsize);
}
