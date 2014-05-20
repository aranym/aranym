#ifndef WIN32_SUPP_H
#define WIN32_SUPP_H

#if defined OS_mingw || defined OS_cygwin
#define _WIN32_VERSION 0x501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>

int win32_errno_from_oserr(DWORD oserrno, int deferrno = EINVAL);
inline void win32_set_errno(int deferrno = EINVAL) { errno = win32_errno_from_oserr(GetLastError(), deferrno); }
const char *win32_errstring(DWORD err);

#endif

#endif
