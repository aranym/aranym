#if defined(_WIN32) || defined(__CYGWIN__)
#define _WIN32_VERSION 0x501
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#if !defined(__CYGWIN__)
#include <winsock2.h>
#endif
#include <windows.h>
#endif
#undef WIN32_LEAN_AND_MEAN /* to avoid redefinition in SDL headers */
