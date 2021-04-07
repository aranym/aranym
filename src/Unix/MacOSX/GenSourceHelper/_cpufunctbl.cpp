#ifndef __CUPFUNCTBL_CPP
#define __CUPFUNCTBL_CPP

#ifdef CPU_x86_64
#include "cpufunctbl_x86_64.cpp"
#endif

#ifdef CPU_i386
#include "cpufunctbl_i386.cpp"
#endif

#ifdef CPU_arm
#include "cpufunctbl_arm64.cpp"
#endif

#endif
