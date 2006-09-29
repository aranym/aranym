/*
 * parameters_cygwin.cpp - parameters specific for Cygwin build
 *
 * Copyright (c) 2001-2006 ARAnyM dev team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include "config.h"
#include "tools.h"
#include "parameters.h"
#include "host_filesys.h"

#define DEBUG 0
#include "debug.h"

# include <cstdlib>
# include <ShlObj.h>

#define ARADATA		"aranym"

int get_geometry(char *dev_path, geo_type geo)
{
	(void)dev_path;
	(void)geo;
	return -1;
}

/*
 * Get the path to a user home folder.
 */
char *HostFilesys::getHomeFolder(char *buffer, unsigned int bufsize)
{
	char szPath[MAX_PATH];
	if (SUCCEEDED(::SHGetFolderPath(NULL, 
		CSIDL_PROFILE, 
		NULL, 
		0, 
		szPath))) 
	{
		safe_strncpy(buffer, szPath, bufsize);
	}
	else {
		buffer[0] = '\0';	// last resort - current folder
	}

	return buffer;
}

/*
 * Get the path to folder with user-specific files (configuration, NVRAM)
 */
char *HostFilesys::getConfFolder(char *buffer, unsigned int bufsize)
{
	HostFilesys::getHomeFolder(buffer, bufsize);
	return addFilename(buffer, ARANYMHOME, bufsize);
}

char *HostFilesys::getDataFolder(char *buffer, unsigned int bufsize)
{
	// data folder is where the program resides
	safe_strncpy(buffer, program_name, bufsize);
	// strip out filename and separator from the path
	char *ptr = strrchr(buffer, '/');	// first try Unix separator
	if (ptr != NULL)
		ptr[0] = '\0';
	else if ((ptr = strrchr(buffer, '\\')) != NULL)	// then DOS sep.
		ptr[0] = '\0';
	else
		buffer[0] = '\0';	// last resort - current folder

	return addFilename(buffer, ARADATA, bufsize);
}

int HostFilesys::makeDir(char *filename, int perm)
{
	return mkdir(filename, perm);
}
