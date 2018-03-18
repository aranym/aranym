/*
 * tools.cpp - non-32bit CPU and miscelany utilities
 *
 * Copyright (c) 2001-2003 STanda of ARAnyM developer team (see AUTHORS)
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
#include <cstring>
#include <stdlib.h>

#include "tools.h"

#if SIZEOF_VOID_P != 4 || DEBUG_NON32BIT

// the instance for the memptr <-> void* conversions
// usefull for systems where the sizeof(void*) != 4
NativeTypeMapper<void*> memptrMapper;

#endif  // SIZEOF_VOID_P != 4

char *safe_strncpy(char *dest, const char *src, size_t size)
{
	if (dest == NULL) return NULL;
	if (size > 0) {
		strncpy(dest, src != NULL ? src : "", size);
		dest[size-1] = '\0';
	}
	return dest;
}

char *safe_strncat(char *dest, const char *src, size_t size)
{
	if (dest == NULL) return NULL;
	if (size > 0) {
		if (src && (strlen(src) + strlen(dest)) < size)
			strcpy(dest + strlen(dest), src);
	}
	return dest;
}

char *my_canonicalize_file_name(const char *filename, bool append_slash)
{
	if (filename == NULL)
		return NULL;

#if (!defined HAVE_CANONICALIZE_FILE_NAME && defined HAVE_REALPATH) || defined __CYGWIN__
#ifdef PATH_MAX
	int path_max = PATH_MAX;
#else
	int path_max = pathconf(filename, _PC_PATH_MAX);
	if (path_max <= 0)
		path_max = 4096;
#endif
#endif

	char *resolved;
#if defined HAVE_CANONICALIZE_FILE_NAME
	resolved = canonicalize_file_name(filename);
#elif defined HAVE_REALPATH
	char *tmp = (char *)malloc(path_max);
	char *realp = realpath(filename, tmp);
	resolved = (realp != NULL) ? strdup(realp) : NULL;
	free(tmp);
#else
	resolved = NULL;
#endif
	if (resolved == NULL)
		resolved = strdup(filename);
	if (resolved)
	{
#ifdef __CYGWIN__
		char *tmp2 = (char *)malloc(path_max);
		strcpy(tmp2, resolved);
		cygwin_path_to_win32(tmp2, path_max);
		free(resolved);
		resolved = tmp2;
#endif
		strd2upath(resolved, resolved);
		if (append_slash)
		{
			size_t len = strlen(resolved);
			if (len > 1 && resolved[len - 1] != *DIRSEPARATOR)
			{
				resolved = (char *)realloc(resolved, len + sizeof(DIRSEPARATOR));
				if (resolved)
					strcat(resolved, DIRSEPARATOR);
			}
		}
	}
	return resolved;
}
