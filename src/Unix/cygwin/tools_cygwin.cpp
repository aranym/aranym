#include "sysdeps.h"

#ifdef OS_cygwin

#include "tools.h"
#include <sys/cygwin.h>
#include <cygwin/version.h>

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

char *cygwin_path_to_posix(char *path, size_t size)
{
	char posixpath[1024];
#if defined(CYGWIN_VERSION_CYGWIN_CONV) && CYGWIN_VERSION_DLL_COMBINED >= CYGWIN_VERSION_CYGWIN_CONV
	cygwin_conv_path(CCP_WIN_A_TO_POSIX | CCP_ABSOLUTE, path, posixpath, sizeof(posixpath));
#else
	cygwin_conv_to_full_path_path(path, posixpath);
#endif
	safe_strncpy(path, posixpath, size);
	return path;
}

#endif /* OS_cygwin */
