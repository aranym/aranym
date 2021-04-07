#ifndef __CPUEMU_CPP
#define __CPUEMU_CPP

#ifdef CPU_x86_64
#include "cpuemu_x86_64.cpp"
#endif

#ifdef CPU_i386
#include "cpuemu_i386.cpp"
#endif

#ifdef CPU_arm
#include "cpuemu_arm64.cpp"
#endif

#endif
