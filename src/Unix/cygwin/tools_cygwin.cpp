#ifdef OS_cygwin
#ifndef _CYGWIN_TOOLS_H
#define _CYGWIN_TOOLS_H

#include "tools.h"
#undef DATADIR	// unfortunately win32 defines a struct of the same name
#include <sys/cygwin.h>
#include <windows.h>

char *cygwin_path_to_win32(char *path, size_t size)
{
	char winpath[1024];
	cygwin_conv_to_full_win32_path(path, winpath);
	safe_strncpy(path, winpath, size);
	return path;
}

int get_home_dir(char *path, int len)
{
	LONG r;
	HKEY hkResult;
	DWORD cbData;

	r = RegOpenKeyEx
		(
			HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\Personal",
			(DWORD)0,
			KEY_READ,
			&hkResult
		);
	if (r != ERROR_SUCCESS)
		return 1;

	cbData = (DWORD)(len/(sizeof(BYTE)));

	r = RegQueryValueEx
		(
			hkResult,
			"AppData", 
			NULL,
			NULL,
			(LPBYTE)path,
			&cbData
		);
	if (r != ERROR_SUCCESS)
		return 1;

	return 0;
}

#endif /* _CYGWIN_TOOLS_H */
#endif /* OS_cygwin */
