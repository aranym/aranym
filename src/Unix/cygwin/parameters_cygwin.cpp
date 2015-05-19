/*
 * parameters_cygwin.cpp - parameters specific for Cygwin build
 *
 * Copyright (c) 2001-2010 ARAnyM dev team (see AUTHORS)
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
#include "tools.h"
#include "parameters.h"
#include "host_filesys.h"
#include "win32_supp.h"

#define DEBUG 0
#include "debug.h"

# include <cstdlib>
# include <ShlObj.h>

#define ARADATA		"aranym"

int get_geometry(const char *dev_path, geo_type geo)
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
	wchar_t szPath[MAX_PATH];
	if (SUCCEEDED(::SHGetFolderPathW(NULL, 
		CSIDL_PROFILE, 
		NULL, 
		0, 
		szPath))) 
	{
		char *path = win32_widechar_to_utf8(szPath);
		safe_strncpy(buffer, path, bufsize);
		free(path);
		strd2upath(buffer, buffer);
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
	static char *real_program_name;
	if (real_program_name == NULL)
	{
		wchar_t name[4096];
		GetModuleFileNameW(GetModuleHandle(NULL), name, sizeof(name) / sizeof(name[0]));
		real_program_name = win32_widechar_to_utf8(name);
	}
	safe_strncpy(buffer, real_program_name, bufsize);
	// strip out filename and separator from the path
	char *ptr = strrchr(buffer, '/');	// first try Unix separator
	char *ptr2 = strrchr(buffer, '\\');	// then DOS sep.
	if (ptr2 > ptr)
		ptr = ptr2;
	if (ptr != NULL)
		ptr[0] = '\0';
	else
		buffer[0] = '\0';	// last resort - current folder

	return addFilename(buffer, ARADATA, bufsize);
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
