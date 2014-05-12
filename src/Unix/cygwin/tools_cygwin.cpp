#ifdef OS_cygwin
#ifndef _CYGWIN_TOOLS_H
#define _CYGWIN_TOOLS_H

#include "tools.h"
#include <sys/cygwin.h>
#include <cygwin/version.h>
#include <windows.h>

char *cygwin_path_to_win32(char *path, size_t size)
{
	char winpath[1024];
	/* FIXME: this can fail if result exceeds MAX_PATH */
#if defined(CYGWIN_VERSION_CYGWIN_CONV) && CYGWIN_VERSION_DLL_COMBINED >= CYGWIN_VERSION_CYGWIN_CONV
	cygwin_conv_path(CCP_POSIX_TO_WIN_A | CCP_ABSOLUTE, path, winpath, sizeof(winpath));
#else
	cygwin_conv_to_full_win32_path(path, winpath);
#endif
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
			"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
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
			"Personal", 
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
