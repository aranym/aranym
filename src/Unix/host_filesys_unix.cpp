/*
	Host filesystem, Unix systems

	ARAnyM (C) 2005 Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include "tools.h"
#include "parameters.h"
#include "host_filesys.h"

#define DEBUG 0
#include "debug.h"

#include <cstdlib>

/*
 * Get the path to a user home folder.
 */
char *HostFilesys::getHomeFolder(char *buffer, unsigned int bufsize)
{
	buffer[0] = '\0';	// last resort - current folder

	// Unix-like systems define HOME variable as the user home folder
	char *home = getenv("HOME");
	if (home && strlen(home) < bufsize)
		strcpy(buffer, home);
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
	// data folder is defined at configure time in ARANYM_DATADIR (using --datadir)
	return safe_strncpy(buffer, ARANYM_DATADIR, bufsize);
}

int HostFilesys::makeDir(char *filename, int perm)
{
#ifdef OS_mingw
	(void) perm;
	return mkdir(filename);
#else
	return mkdir(filename, perm);
#endif
}
