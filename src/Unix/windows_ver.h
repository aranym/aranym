#if defined(_WIN32) || defined(__CYGWIN__)
#define _WIN32_VERSION 0x501
#define WIN32_LEAN_AND_MEAN
#if !defined(__CYGWIN__)
#include <winsock2.h>
#endif
#include <windows.h>
#endif
