#ifdef OS_cygwin
#ifndef _CYGWIN_TOOLS_H
#define _CYGWIN_TOOLS_H

#include "parameters.h"
#include <sys/cygwin.h>

char *cygwin_path_to_win32(char *path, size_t size)
{
	char winpath[1024];
	cygwin_conv_to_full_win32_path(path, winpath);
	safe_strncpy(path, winpath, size);
	return path;
}
#endif /* _CYGWIN_TOOLS_H */
#endif /* OS_cygwin */
