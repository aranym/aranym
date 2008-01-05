dnl Process this file with autoconf to produce a configure script.
dnl Written in 1999 by Christian Bauer et al.
dnl Adapted for ARAnyM by Milan Jurik

AC_INIT(aranym,0.9,aranym@bobek.sh.cvut.cz,aranym)
AC_PREREQ(2.52)
AC_CONFIG_HEADER(config.h)

dnl Canonical system information.
AC_CANONICAL_HOST

dnl Options.
AC_ARG_ENABLE(nat-debug,     [  --enable-nat-debug      enable native debugger support (default=yes)], [WANT_NATDEBUG=$enableval], [WANT_NATDEBUG=yes])

AC_ARG_ENABLE(full-debug,    [  --enable-full-debug     enable full debug outputs (default=no)], [WANT_FULLDEBUG=$enableval], [WANT_FULLDEBUG=no])

AC_ARG_ENABLE(debug,
[  --enable-debug=which    specify which debugger to use (default=ndebug)],
[ case "$enableval" in
    default)  WHICH_DEBUG="ndebug";;
        uae)  WHICH_DEBUG="uae";;
     ndebug)  WHICH_DEBUG="ndebug";;
         no)  WHICH_DEBUG="no";;
	  *)  AC_MSG_ERROR([--enable-debug takes only one of the following values: default, uae, ndebug, no]);;
  esac
],
[WHICH_DEBUG="ndebug"
])

AC_ARG_ENABLE(fullhistory,     [  --enable-fullhistory    enable full history for debugger (default=no)], [WANT_HISTORY=$enableval], [WANT_HISTORY=no])

AC_ARG_ENABLE(flightrecorder,  [  --enable-flightrecorder enable CPU flight recorder (default=no)], [WANT_FLIGHTRECORDER=$enableval], [WANT_FLIGHTRECORDER=no])

AC_ARG_ENABLE(fullmmu,         [  --enable-fullmmu        enable full 68040 MMU (default=no)], [WANT_MMU=$enableval], [WANT_MMU=no])

AC_ARG_ENABLE(atc,
[  --enable-atc=ATC        select type of ATC for MMU (default=full)],
[ case "$enableval" in
        full)  ATC_TYPE="full";;
       small)  ATC_type="small";;
          no)  ATC_TYPE="no";;
           *)  AC_MSG_ERROR([--enable-atc takes only one of the following values: full, small, no]);;
  esac
], [ATC_TYPE=full])

AC_ARG_ENABLE(realstop,        [  --enable-realstop       enable real STOP instruction (default=yes)], [WANT_REALSTOP=$enableval], [WANT_REALSTOP=yes])

AC_ARG_ENABLE(dsp,             [  --enable-dsp            enable DSP 56001 (default=yes)], [WANT_DSP=$enableval], [WANT_DSP=yes])

AC_ARG_ENABLE(dsp-disasm,      [  --enable-dsp-disasm     enable DSP 56001 disassembler (default=no)], [WANT_DSP_DISASM=$enableval], [WANT_DSP_DISASM=no])

AC_ARG_ENABLE(protect2k,       [  --enable-protect2k      enable first 2kB of RAM Super protection (default=yes)], [WANT_PROTECT2K=$enableval], [WANT_PROTECT2K=yes])

AC_ARG_ENABLE(fixedfastram,    [  --enable-fixedfastram=X use fixed size FastRAM of X MB (default=no)], [WANT_FIXEDFASTRAM=$enableval], [WANT_FIXEDFASTRAM=no])

AC_ARG_ENABLE(fixedvideoram,   [  --enable-fixedvideoram  use fixed start of VideoRAM (default=no)], [WANT_FIXEDVIDEORAM=$enableval], [WANT_FIXEDVIDEORAM=no])

AC_ARG_ENABLE(blitmemmove,     [  --enable-blitmemmove    enable Blitter memmove rout (default=no)], [WANT_BLITMEMMOVE=$enableval], [WANT_BLITMEMMOVE=no])

AC_ARG_ENABLE(blitsdlblit,     [  --enable-blitsdlblit    enable SDL blitting routine EXPERIMENTAL UNSUPPORTED BUGGY (default=no)], [WANT_BLITSDLBLIT=$enableval], [WANT_BLITSDLBLIT=no])

AC_ARG_ENABLE(hostfs,          [  --enable-hostfs         enable hostfs support (default=yes)], [WANT_HOSTFS=$enableval], [WANT_HOSTFS=yes])

AC_ARG_ENABLE(ctrlkey,
[  --enable-ctrlkey=WICH        specify which key to use for control key emulation (default=both)],
[ case "$enableval" in
    dnl default is both
    ctrl)	    CONTROL_KEY="ctrl";;
    meta)		CONTROL_KEY="meta";;
    both)		CONTROL_KEY="both";;
    *)		AC_MSG_ERROR([--enable-ctrlkey takes only one of the following values: ctrl, meta, both]);;
  esac
],
[ CONTROL_KEY="both"
])

AC_ARG_ENABLE(gui,             [  --enable-gui            enable GUI (default=yes)], [WANT_SDLGUI=$enableval], [WANT_SDLGUI=yes])

AC_ARG_ENABLE(opengl,          [  --enable-opengl         enable OpenGL (default=yes)], [WANT_OPENGL=$enableval], [WANT_OPENGL=yes])

AC_ARG_WITH(mesa-headers,      [  --with-mesa-headers     Path to Mesa header files], [WITH_MESAINC=$withval], [WITH_MESAINC=no])

AC_ARG_ENABLE(nfcdrom,         [  --enable-nfcdrom        enable NatFeat CD-ROM (default=yes)], [WANT_NFCDROM=$enableval], [WANT_NFCDROM=yes])

AC_ARG_ENABLE(nfpci,           [  --enable-nfpci          enable NatFeat PCI (default=no)], [WANT_NFPCI=$enableval], [WANT_NFPCI=no])

AC_ARG_ENABLE(nfosmesa,        [  --enable-nfosmesa       enable NatFeat OSMesa (default=no)], [WANT_NFOSMESA=$enableval], [WANT_NFOSMESA=no])

AC_ARG_ENABLE(nfjpeg,          [  --enable-nfjpeg         enable NatFeat JPEG (default=yes)], [WANT_NFJPEG=$enableval], [WANT_NFJPEG=yes])

AC_ARG_ENABLE(nfclipbrd,       [  --enable-nfclipbrd      enable NatFeat Clipboard (default=no)], [WANT_NFCLIPBRD=$enableval], [WANT_NFCLIPBRD=no])

AC_ARG_ENABLE(nfvdi,           [  --enable-nfvdi          enable NatFeat VDI (default=yes)], [WANT_NFVDI=$enableval], [WANT_NFVDI=yes])

AC_ARG_ENABLE(ata-cdrom,       [  --enable-ata-cdrom      enable ATA CD-ROM emulation (default=yes)], [WANT_CDROM=$enableval], [WANT_CDROM=yes])

AC_ARG_ENABLE(epslimiter,      [  --enable-epslimiter     enable Exception per second limiter (default=no)], [WANT_EPSLIMITER=$enableval], [WANT_EPSLIMITER=no])

AC_ARG_ENABLE(lilo,            [  --enable-lilo           enable Linux-m68k loader (default=no)], WANT_LILO="$enableval", WANT_LILO="no")

AC_ARG_ENABLE(rtctimer,        [  --enable-rtctimer       enable Linux /dev/rtc as source timer (default=yes)], WANT_RTCTIMER="$enableval", WANT_RTCTIMER="yes")

AC_ARG_ENABLE(parallelx86,     [  --enable-parallelx86    enable Linux X86 hw access for parallel port (default=no)], [WANT_PARALLELX86=$enableval], [WANT_PARALLELX86=no])

AC_ARG_ENABLE(parport,         [  --enable-parport        enable Linux /dev/parport access for parallel port (default=yes)], [WANT_PARALLELPARPORT=$enableval], [WANT_PARALLELPARPORT=yes])

AC_ARG_WITH(fink,              [  --with-fink             Use Fink suite to build Aranym on MacOSX (default=no)], [WITH_FINK=$withval], [WITH_FINK=no])

AC_ARG_WITH(classicconfig,     [  --with-classicconfig	  Use Classic config names on MacOSX (default=no)], [WITH_CLASSIC_CONFIG=$withval], [WITH_CLASSIC_CONFIG=no])

dnl FPU emulation core.
AC_ARG_ENABLE(fpe,
[  --enable-fpe=FPE        specify which fpu emulator to use (default=auto)],
[ case "$enableval" in
    dnl default is always ieee, if architecture has this fp format
    auto)	FPE_CORE_TEST_ORDER="ieee uae";;
    ieee)	FPE_CORE_TEST_ORDER="ieee";;
    uae)	FPE_CORE_TEST_ORDER="uae";;
    x86)	FPE_CORE_TEST_ORDER="x86";;
    *)		AC_MSG_ERROR([--enable-fpe takes only one of the following values: auto, uae, x86, ieee]);;
  esac
],
[ FPE_CORE_TEST_ORDER="ieee uae"
])

dnl Addressing modes.
AC_ARG_ENABLE(addressing,
[  --enable-addressing=AM  specify the addressing mode to use (default=fastest)],
[ case "$enableval" in
    direct)	ADDRESSING_TEST_ORDER="direct";;
    normal)	ADDRESSING_TEST_ORDER="normal";;
    fixed)	ADDRESSING_TEST_ORDER="fixed";;
    *)		AC_MSG_ERROR([--enable-addressing takes only one of the following values: direct, fixed, normal]);;
  esac
],
[ ADDRESSING_TEST_ORDER="direct normal"
])

AC_ARG_ENABLE(addr-check,
[  --enable-addr-check=AM  specify the memory check to use (default=page)],
[ case "$enableval" in
    no)		MEMORY_CHECK="no";;
    full)	MEMORY_CHECK="full";;
    page)	MEMORY_CHECK="page";;
    hwsp)	MEMORY_CHECK="hwsp";;
    pagehwsp)	MEMORY_CHECK="pagehwsp";;
    sseg)	MEMORY_CHECK="sseg";;
    *)		AC_MSG_ERROR([--enable-addr-check takes only one of the following values: no, full, page, hwsp, pagehwsp, sseg]);;
  esac
],
[ MEMORY_CHECK="page"
])

dnl Experimental hacks around spcflags handling.
AC_ARG_ENABLE(spcflags-excl, [  --enable-spcflags-excl  enable exclusive spcflags handling (default=no)], [WANT_SPCFLAGS_EXCL=$enableval], [WANT_SPCFLAGS_EXCL=no])

dnl JIT compiler options.
AC_ARG_ENABLE(jit-compiler,    [  --enable-jit-compiler   enable JIT compiler (default=no)], [WANT_JIT=$enableval], [WANT_JIT=no])

AC_ARG_ENABLE(jit-fpu,
[  --enable-jit-fpu        enable JIT compiler for FPU instructions (default=yes)],
[WANT_JIT_FPU=$enableval],
[ if [[ "x$WANT_JIT" = "xyes" ]]; then
    WANT_JIT_FPU=yes
      else
    WANT_JIT_FPU=no
  fi
])

AC_ARG_ENABLE(jit-debug,       [  --enable-jit-debug      activate native code disassemblers (default=no)], [WANT_JIT_DEBUG=$enableval], [WANT_JIT_DEBUG=no])

AC_ARG_ENABLE(disdip,          [  --enable-disdip         enable distributed dispatcher (default=no)], [WANT_DIS_DIP=$enableval], [WANT_DIS_DIP=no])

AC_ARG_ENABLE(ethernet,        [  --enable-ethernet       enable ethernet card emulation (default=yes)], [WANT_ETHERNET=$enableval], [WANT_ETHERNET=yes])

AC_ARG_ENABLE(cxx-exceptions,  [  --enable-cxx-exceptions enable the use of C++ try/catch instead of setjmp/longjmp (default=yes)], [WANT_CXX_EXCEPTIONS=$enableval], [WANT_CXX_EXCEPTIONS=yes])

dnl Host OS type (host is build if not cross-compiling).
case "$host_os" in
  linux*)	OS_TYPE=linux;;
  openbsd*)	OS_TYPE=openbsd;;
  netbsd*)	OS_TYPE=netbsd;;
  freebsd*|kfreebsd*-gnu)	OS_TYPE=freebsd;;
  solaris*)	OS_TYPE=solaris;;
  darwin*)	OS_TYPE=darwin;;
  cygwin*)	OS_TYPE=cygwin;;
  irix*)	OS_TYPE=irix;;
  mint*)	OS_TYPE=mint;;
  beos*)	OS_TYPE=beos;;
  mingw*)	OS_TYPE=mingw;;
  *)		OS_TYPE=`echo $host_os | sed -e 's/-/_/g' | sed -e 's/\./_/g'`;;
esac
DEFINES="$DEFINES -DOS_$OS_TYPE"
AC_DEFINE_UNQUOTED([OS_TYPE], "$OS_TYPE", [Define OS type])

dnl Target CPU type.
ARCHFLAGS=
HAVE_I386=no
HAVE_SPARC=no
HAVE_POWERPC=no
HAVE_X86_64=no
case "$host_cpu" in
  i386* | i486* | i586* | i686* | i786* )
	CPU_TYPE=i386
	HAVE_I386=yes
	if [[ "x$OS_TYPE" = "xdarwin" ]]; then
		ARCHFLAGS="-arch i386"
		ISYSROOT="-isysroot /Developer/SDKs/MacOSX10.4u.sdk"
		dnl Add the ARCHFLAGS here to let configure check for the target and not host capabilities.
		CPPFLAGS="$CPPFLAGS $ISYSROOT $ARCHFLAGS"
		LDFLAGS="$LDFLAGS $ISYSROOT $ARCHFLAGS"
	fi
	;;
  sparc* ) CPU_TYPE=sparc HAVE_SPARC=yes;;
  powerpc* )
	CPU_TYPE=powerpc
	HAVE_POWERPC=yes
	if [[ "x$OS_TYPE" = "xdarwin" ]]; then
		ARCHFLAGS="-arch ppc"
		ISYSROOT=
		dnl Add the ARCHFLAGS here to let configure check for the target and not host capabilities.
		CPPFLAGS="$CPPFLAGS $ISYSROOT $ARCHFLAGS"
		LDFLAGS="$LDFLAGS $ISYSROOT $ARCHFLAGS"
	fi
	;;
  x86_64* ) CPU_TYPE=x86_64 HAVE_X86_64=yes;;
  *) CPU_TYPE=`echo $host_cpu | sed -e 's/-/_/g'`;;
esac
DEFINES="$DEFINES -DCPU_$CPU_TYPE"
AC_DEFINE_UNQUOTED([CPU_TYPE], "$CPU_TYPE", [Define CPU type])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS(unistd.h fcntl.h sys/mman.h mach/mach.h sys/ioctl.h)
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_HEADERS(termios.h termio.h alloca.h sys/statfs.h sys/statvfs.h)
AC_CHECK_HEADERS(sys/types.h sys/stat.h sys/vfs.h utime.h sys/param.h)
AC_CHECK_HEADERS(sys/mount.h types.h stat.h)
AC_CHECK_HEADERS(sys/socket.h)
AC_CHECK_HEADERS(linux/if.h linux/if_tun.h net/if.h net/if_tun.h, [], [], [
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(float, 4)
AC_CHECK_SIZEOF(double, 8)
AC_CHECK_SIZEOF(long double, 12)
AC_CHECK_SIZEOF(void *, 4)
AC_TYPE_OFF_T
AC_CHECK_TYPE(loff_t, off_t)
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, size_t)
AC_TYPE_SIGNAL
AC_HEADER_TIME
AC_STRUCT_TM
AC_SYS_LARGEFILE

AC_CXX_BOOL

dnl Checks for libraries
AC_CHECK_LIB(m, sin)

dnl Checks for library functions.
AC_CHECK_FUNCS(strdup)
AC_CHECK_FUNCS(sigaction signal)
AC_CHECK_FUNCS(mmap mprotect munmap)
AC_CHECK_FUNCS(vm_allocate vm_deallocate vm_protect)
AC_CHECK_FUNCS(strchr memcpy bcopy)
AC_CHECK_FUNCS(usleep gettimeofday)

dnl Defines caddr_t as void * if not already defined
AC_CHECK_TYPES(caddr_t)
AH_BOTTOM(
[#ifndef HAVE_CADDR_T
typedef void * caddr_t;
#endif])

dnl Darwin seems to define mach_task_self() instead of task_self().
AC_CHECK_FUNCS(mach_task_self task_self)

AC_DEFINE([DIRSEPARATOR], "/", [Default directory separator])

case "$host_os" in
  linux*)
    EXTRASYSSRCS="Unix/linux/parameters_linux.cpp Unix/linux/ethernet_linux.cpp Unix/host_filesys_unix.cpp"
    ;;
  cygwin*)
    AC_DEFINE([ARANYMHOME], "aranym", [Default ARAnyM directory])
    LIBS="$LIBS -liberty"
    EXTRASYSSRCS="Unix/cygwin/parameters_cygwin.cpp Unix/cygwin/tools_cygwin.cpp Unix/cygwin/ethernet_cygwin.cpp"
    ;;
  solaris*)
    AC_DEFINE([_XOPEN_SOURCE], 500, [Define if needed])
    AC_DEFINE([_POSIX_C_SOURCE], 199506L, [Define if needed])
    DEFINES="$DEFINES -DBSD_COMP -D__EXTENSIONS__"
    EXTRASYSSRCS="Unix/parameters_unix.cpp Unix/host_filesys_unix.cpp"
    ;;
  irix*)
    EXTRASYSSRCS="Unix/parameters_unix.cpp Unix/irix/unaligned.c Unix/host_filesys_unix.cpp"
    ;;
  freebsd*|kfreebsd*-gnu)
    AC_DEFINE([HAVE_O_FSYNC], 1, [Define if you want to use FreeBSD's imitation of O_SYNC])
    EXTRASYSSRCS="Unix/parameters_unix.cpp Unix/host_filesys_unix.cpp"
    ;;
  darwin*)
	AC_DEFINE([HAVE_O_FSYNC], 1, [Define if you want to use FreeBSD's imitation of O_SYNC])
	if test "x$WITH_FINK" = "xyes"; then
		AC_DEFINE([ARANYMHOME], "aranym", [Default ARAnyM directory])
		EXTRASYSSRCS="Unix/parameters_unix.cpp Unix/host_filesys_unix.cpp Unix/darwin/ethernet_darwin.cpp Unix/darwin/SDLHotkeysRefresh.M"
	else
		AC_DEFINE([ARANYMHOME], "ARAnyM", [Default ARAnyM directory])
		
		if test "x$WITH_CLASSIC_CONFIG" = "xyes"; then
			echo "Using classic config"
			AC_DEFINE([ARANYMCONFIG], "config", [Default ARAnyM config file])
			AC_DEFINE([ARANYMNVRAM], "nvram", [Default ARAnyM NVRAM file])
			AC_DEFINE([ARANYMKEYMAP], "keymap", [Default ARAnyM keymap])
		else	
			echo "Using preference files"
			AC_DEFINE([ARANYMCONFIG], "org.aranym.amcfg", [Default ARAnyM config file])
			AC_DEFINE([ARANYMNVRAM], "org.aranym.nvram", [Default ARAnyM NVRAM file])
			AC_DEFINE([ARANYMKEYMAP], "org.aranym.keymap", [Default ARAnyM keymap])
		fi
		
		EXTRASYSSRCS="Unix/darwin/SDLMain.M Unix/darwin/host_filesys_darwin.M Unix/darwin/parameters_darwin.cpp Unix/darwin/ethernet_darwin.cpp Unix/darwin/SDLHotkeysRefresh.M"

		BUNDLE_FRAMEWORKS= dnl Empty list of frameworks to bundle

		dnl Check for some important frameworks
		ARANYM_CHECK_FRAMEWORK(AppKit, [])
		ARANYM_CHECK_FRAMEWORK(CoreFoundation, [#include <CoreFoundation/CoreFoundation.h>])
		ARANYM_CHECK_FRAMEWORK(IOKit, [#include <IOKit/IOKitLib.h>])
		ARANYM_CHECK_FRAMEWORK(Security, [#include <Security/Security.h>])
	fi
	;;
  beos*)
    EXTRASYSSRCS="Unix/parameters_unix.cpp Unix/beos/cdrom_beos.cpp Unix/host_filesys_unix.cpp"
    CPPFLAGS="$CPPFLAGS -I\$(srcdir)/Unix/beos"
    ;;
  *)
    EXTRASYSSRCS="Unix/parameters_unix.cpp Unix/host_filesys_unix.cpp"
    ;;
esac

SYSSRCS="$SYSSRCS $EXTRASYSSRCS"

dnl Stupid hack around problem with extfs straply vs. Mac OS X
AC_CACHE_CHECK([for __wchar_t],
  ac_cv_wchar_t, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_RUN([
    #include <errno.h>
    #include <stdlib.h>
    #include <stdio.h>
  ], ac_cv_wchar_t=yes, ac_cv_wchar_t=no,
  dnl When cross-compiling, do not assume anything.
  ac_cv_wchar_t="guessing no"
  )
  AC_LANG_RESTORE
])
AC_TRANSLATE_DEFINE(HAVE_WCHAR_T, "$ac_cv_wchar_t", [Define if wchar_t is present])

if [[ "x$OS_TYPE" = "xdarwin" ]]; then
  if [[ "x$WANT_ETHERNET" = "xyes" ]]; then
	ac_cv_tun_tap_support=yes
  else
	ac_cv_tun_tap_support=no
  fi
else
  dnl Check that the host supports TUN/TAP devices
  AC_CACHE_CHECK([whether TUN/TAP is supported],
    ac_cv_tun_tap_support, [
    AC_TRY_COMPILE([
      #if defined(HAVE_LINUX_IF_H) && defined(HAVE_LINUX_IF_TUN_H)
      #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
      #endif
      #include <linux/if.h>
      #include <linux/if_tun.h>
      #endif
      #if defined(HAVE_NET_IF_H) && defined(HAVE_NET_IF_TUN_H)
      #include <net/if.h>
      #include <net/if_tun.h>
      #endif
    ], [
      struct ifreq ifr;
      memset(&ifr, 0, sizeof(ifr));
      ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    ],
    ac_cv_tun_tap_support=yes, ac_cv_tun_tap_support=no
    )
  ])
fi
AC_TRANSLATE_DEFINE(ENABLE_TUNTAP, "$ac_cv_tun_tap_support",
  [Define if your system supports TUN/TAP devices.])
  
dnl Various checks if the system supports vm_allocate() and the like functions.
have_mach_vm=no
if [[ "x$ac_cv_func_vm_allocate" = "xyes" -a "x$ac_cv_func_vm_deallocate" = "xyes" -a \
      "x$ac_cv_func_vm_protect" = "xyes" ]]; then
  have_mach_vm=yes
fi
AC_TRANSLATE_DEFINE(HAVE_MACH_VM, "$have_mach_vm",
  [Define if your system has a working vm_allocate()-based memory allocator.])

case "$host_os" in
  darwin*)	OSdarwinReq="#define OS_darwin";;
  *)		OSdarwinReq= ;;
esac

dnl Check that vm_allocate(), vm_protect() work
if [[ "x$have_mach_vm" = "xyes" ]]; then

AC_CACHE_CHECK([whether vm_protect works],
  ac_cv_vm_protect_works, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_cv_vm_protect_works=yes
  dnl First the tests that should segfault
  for test_def in NONE_READ NONE_WRITE READ_WRITE; do
    AC_TRY_RUN([
      #define CONFIGURE_TEST_VM_MAP
      #define TEST_VM_PROT_$test_def
	  $OSdarwinReq
      #include "$srcdir/src/Unix/vm_alloc.cpp"
    ], ac_cv_vm_protect_works=no, rm -f core,
    dnl When cross-compiling, do not assume anything
    ac_cv_vm_protect_works="guessing no"
    )
  done
  AC_TRY_RUN([
    #define CONFIGURE_TEST_VM_MAP
    #define TEST_VM_PROT_RDWR_WRITE
    $OSdarwinReq
    #include "$srcdir/src/Unix/vm_alloc.cpp"
  ], , ac_cv_vm_protect_works=no,
  dnl When cross-compiling, do not assume anything
  ac_cv_vm_protect_works="guessing no"
  )
  AC_LANG_RESTORE
  ]
)

dnl Remove support for vm_allocate() if vm_protect() does not work
if [[ "x$have_mach_vm" = "xyes" ]]; then
  case $ac_cv_vm_protect_works in
    *yes) have_mach_vm=yes;;
    *no) have_mach_vm=no;;
  esac
fi
AC_TRANSLATE_DEFINE(HAVE_MACH_VM, "$have_mach_vm",
  [Define if your system has a working vm_allocate()-based memory allocator.])

fi dnl HAVE_MACH_VM

dnl Various checks if the system supports mmap() and the like functions.
dnl ... and Mach memory allocators are not supported
have_mmap_vm=no
if [[ "x$ac_cv_func_mmap" = "xyes" -a "x$ac_cv_func_munmap" = "xyes" -a \
      "x$ac_cv_func_mprotect" = "xyes" ]]; then
  if [[ "x$have_mach_vm" = "xno" ]]; then
    have_mmap_vm=yes
  fi
fi
AC_TRANSLATE_DEFINE(HAVE_MMAP_VM, "$have_mmap_vm",
  [Define if your system has a working mmap()-based memory allocator.])

dnl Check that mmap() and associated functions work.
if [[ "x$have_mmap_vm" = "xyes" ]]; then

dnl Check if we have a working anonymous mmap()
AC_CACHE_CHECK([whether mmap supports MAP_ANON],
  ac_cv_mmap_anon, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_RUN([
    #define HAVE_MMAP_ANON
    #define CONFIGURE_TEST_VM_MAP
    #define TEST_VM_MMAP_ANON
    #include "$srcdir/src/Unix/vm_alloc.cpp"
  ], ac_cv_mmap_anon=yes, ac_cv_mmap_anon=no,
  dnl When cross-compiling, do not assume anything.
  ac_cv_mmap_anon="guessing no"
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(HAVE_MMAP_ANON, "$ac_cv_mmap_anon",
  [Define if <sys/mman.h> defines MAP_ANON and mmap()'ing with MAP_ANON works.])

AC_CACHE_CHECK([whether mmap supports MAP_ANONYMOUS],
  ac_cv_mmap_anonymous, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_RUN([
    #define HAVE_MMAP_ANONYMOUS
    #define CONFIGURE_TEST_VM_MAP
    #define TEST_VM_MMAP_ANON
    #include "$srcdir/src/Unix/vm_alloc.cpp"
  ], ac_cv_mmap_anonymous=yes, ac_cv_mmap_anonymous=no,
  dnl When cross-compiling, do not assume anything.
  ac_cv_mmap_anonymous="guessing no"
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(HAVE_MMAP_ANONYMOUS, "$ac_cv_mmap_anonymous",
  [Define if <sys/mman.h> defines MAP_ANONYMOUS and mmap()'ing with MAP_ANONYMOUS works.])

AC_CACHE_CHECK([whether mprotect works],
  ac_cv_mprotect_works, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  ac_cv_mprotect_works=yes
  dnl First the tests that should segfault
  for test_def in NONE_READ NONE_WRITE READ_WRITE; do
    AC_TRY_RUN([
      #define CONFIGURE_TEST_VM_MAP
      #define TEST_VM_PROT_$test_def
      #include "$srcdir/src/Unix/vm_alloc.cpp"
    ], ac_cv_mprotect_works=no, rm -f core,
    dnl When cross-compiling, do not assume anything
    ac_cv_mprotect_works="guessing no"
    )
  done
  AC_TRY_RUN([
    #define CONFIGURE_TEST_VM_MAP
    #define TEST_VM_PROT_RDWR_WRITE
    #include "$srcdir/src/Unix/vm_alloc.cpp"
  ], , ac_cv_mprotect_works=no,
  dnl When cross-compiling, do not assume anything
  ac_cv_mprotect_works="guessing no"
  )
  AC_LANG_RESTORE
  ]
)

dnl Remove support for mmap() if mprotect() does not work
if [[ "x$have_mmap_vm" = "xyes" ]]; then
  case $ac_cv_mprotect_works in
    *yes) have_mmap_vm=yes;;
    *no) have_mmap_vm=no;;
  esac
fi
AC_TRANSLATE_DEFINE(HAVE_MMAP_VM, $have_mmap_vm,
  [Define if your system has a working mmap()-based memory allocator.])

fi dnl HAVE_MMAP_VM

dnl Check if we have POSIX shared memory support
AC_CACHE_CHECK([whether POSIX shared memory is working],
  ac_cv_have_posix_shm, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_RUN([
    #define HAVE_POSIX_SHM
    $OSdarwinReq
    #include "$srcdir/src/Unix/vm_alloc.cpp"
    int main(void) { /* returns 0 if we have working POSIX shm */
      if (vm_init() < 0) exit(2);
      char *m1 = (char *)vm_acquire(32768, VM_MAP_DEFAULT | VM_MAP_33BIT);
      if (m1 == VM_MAP_FAILED) exit(3);
      vm_exit(); exit(0);
    }
  ], ac_cv_have_posix_shm=yes, ac_cv_have_posix_shm=no,
  dnl When cross-compiling, do not assume anything.
  ac_cv_have_posix_shm="guessing no"
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(HAVE_POSIX_SHM, "$ac_cv_have_posix_shm",
  [Define if your system supports POSIX shared memory.])

dnl Check if we have working 33-bit memory addressing
AC_CACHE_CHECK([whether 33-bit memory addressing is working],
  ac_cv_have_33bit_addressing, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_RUN([
    #define USE_33BIT_ADDRESSING 1
    #include "$srcdir/src/Unix/vm_alloc.cpp"
    int main(void) { /* returns 0 if we have working 33-bit addressing */
      if (sizeof(void *) < 8) exit(1);
      if (vm_init() < 0) exit(2);
      char *m1 = (char *)vm_acquire(32768, VM_MAP_DEFAULT | VM_MAP_33BIT);
      if (m1 == VM_MAP_FAILED) exit(3);
      char *m2 = m1 + (1L << 32);
      m1[0] = 0x12; if (m2[0] != 0x12) exit(4);
      m2[0] = 0x34; if (m1[0] != 0x34) exit(5);
      vm_exit(); exit(0);
    }
  ], ac_cv_have_33bit_addressing=yes, ac_cv_have_33bit_addressing=no,
  dnl When cross-compiling, do not assume anything.
  ac_cv_have_33bit_addressing="guessing no"
  )
  AC_LANG_RESTORE
  ]
)

dnl Check signal handlers need to be reinstalled
AC_CACHE_CHECK([whether signal handlers need to be reinstalled],
  ac_cv_signal_need_reinstall, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_RUN([
    #include <stdlib.h>
    #ifdef HAVE_UNISTD_H
    #include <unistd.h>
    #endif
    #include <signal.h>
    static int handled_signal = 0;
    RETSIGTYPE sigusr1_handler(int) { handled_signal++; }
    int main(void) { /* returns 0 if signals need not to be reinstalled */
      signal(SIGUSR1, sigusr1_handler); raise(SIGUSR1); raise(SIGUSR1);
      exit(handled_signal == 2);
    }
  ], ac_cv_signal_need_reinstall=yes, ac_cv_signal_need_reinstall=no,
  dnl When cross-compiling, do not assume anything.
  ac_cv_signal_need_reinstall="guessing yes"
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(SIGNAL_NEED_REINSTALL, "$ac_cv_signal_need_reinstall",
  [Define if your system requires signals to be reinstalled.])

dnl Check if sigaction handlers need to be reinstalled
AC_CACHE_CHECK([whether sigaction handlers need to be reinstalled],
  ac_cv_sigaction_need_reinstall, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_RUN([
    #include <stdlib.h>
    #ifdef HAVE_UNISTD_H
    #include <unistd.h>
    #endif
    #include <signal.h>
    static int handled_signal = 0;
    RETSIGTYPE sigusr1_handler(int) { handled_signal++; }
    typedef RETSIGTYPE (*signal_handler)(int);
    static signal_handler mysignal(int sig, signal_handler handler) {
      struct sigaction old_sa;
      struct sigaction new_sa;
      new_sa.sa_handler = handler;
      return ((sigaction(sig,&new_sa,&old_sa) < 0) ? SIG_IGN : old_sa.sa_handler);
    }
    int main(void) { /* returns 0 if signals need not to be reinstalled */
      mysignal(SIGUSR1, sigusr1_handler); raise(SIGUSR1); raise(SIGUSR1);
      exit(handled_signal == 2);
    }
  ], ac_cv_sigaction_need_reinstall=yes, ac_cv_sigaction_need_reinstall=no,
  dnl When cross-compiling, do not assume anything.
  ac_cv_sigaction_need_reinstall="guessing yes"
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(SIGACTION_NEED_REINSTALL, "$ac_cv_sigaction_need_reinstall",
  [Define if your system requires sigactions to be reinstalled.])

dnl Check if we have vasprintf (_GNU_SOURCE)
AC_CACHE_CHECK([for vasprintf],
  ac_cv_have_vasprintf, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
    #define _GNU_SOURCE
    #include <stdarg.h>
    #include <stdio.h>
  ], [
    va_list a;
    vasprintf(NULL, NULL, a);
  ],
  [ac_cv_have_vasprintf=yes],
  [ac_cv_have_vasprintf=no]
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(HAVE_GNU_SOURCE, "$ac_cv_have_vasprintf",
  [Define if your system support GNU SOURCEs (vasprintf function)])
if [[ "x$ac_cv_have_vasprintf" = "xyes" ]]; then
  DEFINES="$DEFINES -D_GNU_SOURCE"
else SYSSRCS="$SYSSRCS Unix/vasprintf.c"
fi

AC_CACHE_CHECK([for sighandler_t],
  ac_cv_have_sighandler_t, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
    #define _GNU_SOURCE
    #include <signal.h>
    #include <stdio.h>
  ], [
    signal(0, (sighandler_t)NULL);
  ],
  [ac_cv_have_sighandler_t=yes],
  [ac_cv_have_sighandler_t=no]
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(HAVE_SIGHANDLER_T, "$ac_cv_have_sighandler_t",
  [Define if <signal.h> doesn't define sighandler_t])

dnl Check if we have getopt_long
AC_CACHE_CHECK([for getopt_long],
  ac_cv_have_getopt_long, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
    #define _GNU_SOURCE
    #include <unistd.h>
    #include <getopt.h>
  ], [
    getopt_long(0, NULL, NULL, NULL, NULL);
  ],
  [ac_cv_have_getopt_long=yes],
  [ac_cv_have_getopt_long=no]
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(HAVE_GETOPT_H, "$ac_cv_have_getopt_long")

if [[ "x$ac_cv_have_getopt_long" = "xno" ]]; then
  SYSSRCS="$SYSSRCS Unix/getopt_init.c Unix/getopt1.c Unix/getopt.c"
fi

case $CONTROL_KEY in
	meta)
		AC_DEFINE([MAP_META_TO_CONTROL], 1, [Define if you want to map the meta key to the emulated control key])
		;;
	both)
		AC_DEFINE([MAP_META_TO_CONTROL], 1, [Define if you want to map the meta key to the emulated control key])
		AC_DEFINE([MAP_CONTROL_TO_CONTROL], 1, [Define if you want to map the control key to the emulated control key])
		;;
	ctrl)
		AC_DEFINE([MAP_CONTROL_TO_CONTROL], 1, [Define if you want to map the control key to the emulated control key])
		;;
esac

if [[ "x$WANT_JIT" = "xyes" ]]; then
  WANT_EXSIGSEGV=yes
  MEMORY_CHECK=sseg
fi

case $MEMORY_CHECK in
  no)   AC_DEFINE([NOCHECKBOUNDARY], 1, [Define if you don't want to use check memory boundary])
        ;;
  page) AC_DEFINE([ARAM_PAGE_CHECK], 1, [Define if you want to try out a different memory check])
        ;;
  hwsp) ADDRESSING_TEST_ORDER="fixed"
        WANT_EXSIGSEGV="yes"
        AC_DEFINE([NOCHECKBOUNDARY], 1, [Define if you don't want to use check memory boundary])
	AC_DEFINE([EXTENDED_SIGSEGV], 1, [Define if you want to use extended segafault handler])
        AC_DEFINE([RAMENDNEEDED])
        ;;
  pagehwsp) ADDRESSING_TEST_ORDER="fixed"
        WANT_EXSIGSEGV="yes"
        AC_DEFINE([ARAM_PAGE_CHECK], 1, [Define if you want to try out a different memory check])
        AC_DEFINE([NOCHECKBOUNDARY], 1, [Define if you don't want to use check memory boundary])
	AC_DEFINE([EXTENDED_SIGSEGV], 1, [Define if you want to use extended segafault handler])
        AC_DEFINE([RAMENDNEEDED], 1, [Define if you want protect end of memory])
        ;;
   sseg) ADDRESSING_TEST_ORDER="fixed"
        WANT_EXSIGSEGV="yes"
        AC_DEFINE([NOCHECKBOUNDARY], 1, [Define if you don't want to use check memory boundary])
	AC_DEFINE([RAMENDNEEDED], 1,)
        AC_DEFINE([EXTENDED_SIGSEGV], 1, [Define if you want to use extended segafault handler])
	AC_DEFINE([HW_SIGSEGV], 1, [Define if you want to use extended segfault handler also for HW space])
        ;;
  *) ;;
esac

dnl Full opcode's history
if [[ "x$WANT_HISTORY" = "xyes" ]]; then
    AC_DEFINE([FULL_HISTORY], 1, [Define if you want to use full history])
fi

dnl CPU flight recorder
if [[ "x$WANT_FLIGHTRECORDER" = "xyes" ]]; then
     AC_DEFINE([FLIGHT_RECORDER], 1, [Define if you want to use CPU flight recorder])
fi

dnl ATA CD-ROM emulation
if [[ "x$WANT_CDROM" = "xyes" ]]; then
     AC_DEFINE([SUPPORT_CDROM], 1, [Define if you want to use ATA CD-ROM emulation])
fi

MMUSRCS="uae_cpu/cpummu.cpp"
dnl Full MMU
if [[ "x$WANT_MMU" = "xyes" ]]; then
    AC_DEFINE([FULLMMU], 1, [Define if using full MMU])
fi

dnl Small ATC
if [[ "x$ATC_TYPE" = "xsmall" ]]; then
    AC_DEFINE([SMALL_ATC], 1, [Define if using only small ATC])
fi

dnl Full ATC
if [[ "x$ATC_TYPE" = "xno" ]]; then
    AC_DEFINE([DISABLE_ATC], 1, [Define if not using full ATC])
fi

dnl DSP
DSPSRCS=""
if [[ "x$WANT_DSP" = "xyes" ]]; then
    AC_DEFINE([DSP_EMULATION], 1, [Define if using DSP])
    DSPSRCS="dsp_cpu.cpp"
    dnl DSP disassembler
    if [[ "x$WANT_DSP_DISASM" = "xyes" ]]; then
        AC_DEFINE([DSP_DISASM], 1, [Define if DSP disassembly])
        DSPSRCS="$DSPSRCS dsp_disasm.cpp"
    fi
fi

dnl protect first 2kB of RAM
if [[ "x$WANT_PROTECT2K" != "xno" ]]; then
  AC_DEFINE([PROTECT2K], 1, [Define if you want to protect first 2kB of RAM against user mode access])
fi

dnl fixed size FastRAM
if [[ "x$WANT_FIXEDFASTRAM" != "xno" ]]; then
  AC_DEFINE_UNQUOTED([FixedSizeFastRAM], $WANT_FIXEDFASTRAM, [Define if you want FastRAM of fixed size])
fi

dnl fixed start of VideoRAM
if [[ "x$WANT_FIXEDVIDEORAM" = "xyes" ]]; then
  AC_DEFINE([FIXED_VIDEORAM], 1, [Define if you want fixed position of VideoRAM])
else
  if [[ "x$WANT_MMU" = "xyes" ]]; then
    AC_MSG_WARN([Protection of first 2kB RAM enabled as well - won't hurt])
    AC_DEFINE([PROTECT2K], 1, [Define if you want to protect first 2kB of RAM against user mode access])
  fi
  if [[ "x$WANT_BLITSDLBLIT" = "xyes" ]]; then
    AC_MSG_WARN([Blitter SDL surface blitting support needs fixed position of VideoRAM])
    AC_DEFINE([FIXED_VIDEORAM], 1, [Define if you want fixed position of VideoRAM])
    WANT_FIXEDVIDEORAM="yes"
  fi
fi

dnl Full STOP instruction
if [[ "x$WANT_REALSTOP" = "xyes" ]]; then
  AC_DEFINE([ENABLE_REALSTOP], 1, [Define if you want real STOP instruction implementation])
  WANT_SPCFLAGS_EXCL=yes
fi

dnl Blitter memmove acceleration
if [[ "x$WANT_BLITMEMMOVE" = "xyes" ]]; then
  AC_DEFINE([BLITTER_MEMMOVE], 1, [Define if you want accelerated blitter output])
fi

  if [[ "x$WANT_BLITSDLBLIT" = "xyes" ]]; then
    AC_MSG_WARN([Blitter SDL surface blitting support needs fixed position of VideoRAM])
    AC_DEFINE([FIXED_VIDEORAM], 1, [Define if you want fixed position of VideoRAM])
    WANT_FIXEDVIDEORAM="yes"
  fi
dnl Blitter SDL surface blitting
if [[ "x$WANT_BLITSDLBLIT" = "xyes" ]]; then
  AC_DEFINE([BLITTER_SDLBLIT], 1, [Define if you want even more accelerated blitter output])
  AC_DEFINE([BLITTER_MEMMOVE], 1, [Define if you want accelerated blitter output])	dnl required for the SDLBLIT
fi

dnl Ethernet card emulation
HAVE_ETHERNET=no
if [[ "x$WANT_ETHERNET" = "xyes" ]]; then
  if [[ "x$ac_cv_tun_tap_support" = "xyes" ]]; then
    AC_DEFINE([ETHERNET_SUPPORT], 1, [Define if you want to use ethernet card emulation])
    SYSSRCS="$SYSSRCS natfeat/ethernet.cpp"
    TAPIF="aratapif"
    HAVE_ETHERNET="yes"
  elif [[ "x$OS_TYPE" = "xcygwin" -a "x$HAVE_I386" ]]; then
    AC_DEFINE([ETHERNET_SUPPORT], 1, [Define if you want to use ethernet card emulation])
    SYSSRCS="$SYSSRCS natfeat/ethernet.cpp"
    TAPIF=""
    HAVE_ETHERNET="yes"
  else
    AC_MSG_WARN([Ethernet interface not supported on your system])
    TAPIF=""
  fi
else
  TAPIF=""
fi

dnl Support for direct access to host's fs
if [[ "x$WANT_HOSTFS" = "xyes" ]]; then
  SYSSRCS="$SYSSRCS natfeat/hostfs.cpp"
  AC_DEFINE([HOSTFS_SUPPORT], 1, [Define if you want to use direct access to host's fs])
fi

dnl SDL GUI
GUISDLSRCS=""
if test "x$WANT_SDLGUI" = "xyes" ; then
  GUISDLSRCS="gui-sdl/sdlgui.cpp gui-sdl/dlgFileSelect.cpp gui-sdl/file.cpp gui-sdl/dlgMain.cpp gui-sdl/dlgDisk.cpp gui-sdl/dlgKeyboard.cpp gui-sdl/dlgHotkeys.cpp gui-sdl/dlgAlert.cpp gui-sdl/dlgOs.cpp gui-sdl/dlgVideo.cpp gui-sdl/dlgNetwork.cpp gui-sdl/dlgKeypress.cpp gui-sdl/dialog.cpp"
  CPPFLAGS="$CPPFLAGS -I\$(srcdir)/gui-sdl"
  AC_DEFINE([SDL_GUI], 1, [Define if you want to build SDL GUI])
fi

dnl C++ exceptions
if [[ "x$WANT_EXSIGSEGV" = "xyes" ]]; then
  WANT_CXX_EXCEPTIONS="no"
fi
if [[ "x$WANT_CXX_EXCEPTIONS" = "xno" ]]; then
  AC_DEFINE(EXCEPTIONS_VIA_LONGJMP, 1, [Define if you want to use setjmp/longjmp instead of C++ exceptions])
fi

dnl Extended signal segmentation fault handler
if [[ "x$WANT_EXSIGSEGV" = "xyes" ]]; then
  case "$host_os" in
    linux*)
      case "$CPU_TYPE" in
        i386)
	  SYSSRCS="$SYSSRCS Unix/linux/sigsegv_linux_x86.cpp"
	  ;;
	x86_64)
          SYSSRCS="$SYSSRCS Unix/linux/sigsegv_linux_x86_64.cpp"
          ;;
        *)
          AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
          ;;
      esac
      ;;
    openbsd*)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS Unix/xbsd/sigsegv_xbsd.cpp"
      else
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      fi
      ;;
    darwin*)
      if [[ "x$CPU_TYPE" = "powerpc" ]]; then
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      else
        SYSSRCS="$SYSSRCS Unix/darwin/sigsegv_darwin_x86.cpp"
        AC_DEFINE(NO_NESTED_SIGSEGV, 1 , [Define if OS needs to cleanly exit sigsegv handler])
      fi
      ;;
    netbsd*)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS Unix/xbsd/sigsegv_xbsd.cpp"
      else
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      fi
      ;;
    freebsd*|kfreebsd*-gnu)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS Unix/xbsd/sigsegv_xbsd.cpp"
      else
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      fi
      ;;
    cygwin*)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS Unix/cygwin/sigsegv_cygwin.cpp"
        AC_DEFINE(NO_NESTED_SIGSEGV, 1 , [Define if OS needs to cleanly exit sigsegv handler])
      else
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      fi
      ;;
    solaris*)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS Unix/solaris/sigsegv_solaris_x86.cpp"
      else
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      fi
      ;;
    *)
      AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      ;;
  esac
fi

dnl Determine the addressing mode to use
ADDRESSING_MODE=""
AC_MSG_CHECKING([for the addressing mode to use])
for am in $ADDRESSING_TEST_ORDER; do
  case $am in
  direct)
    if [[ "x$HAVE_MMAP_VM" = "xno" -a "x$HAVE_MACH_VM" = "xno" ]]; then
      continue
    fi
    dnl Problems with Solaris
    if [[ "x$OS_TYPE" = "xsolaris" ]]; then
      continue
    fi
    ADDRESSING_MODE="direct"
    DEFINES="$DEFINES -DDIRECT_ADDRESSING"
    break
    ;;
  fixed)
    if [[ "x$HAVE_MMAP_VM" = "xno" -a "x$HAVE_MACH_VM" = "xno" ]]; then
      continue
    fi
    if [[ "x$OS_TYPE" = "xlinux" -a "x$HAVE_I386" ]]; then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if [[ "x$OS_TYPE" = "xfreebsd" -a "x$HAVE_I386" ]]; then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if [[ "x$OS_TYPE" = "xopenbsd" -a "x$HAVE_I386" ]]; then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if [[ "x$OS_TYPE" = "xdarwin" -a "x$HAVE_I386" ]]; then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if [[ "x$OS_TYPE" = "xcygwin" -a "x$HAVE_I386" ]]; then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if [[ "x$OS_TYPE" = "xsolaris" -a "x$HAVE=I386" ]]; then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    ;;
  normal)
    ADDRESSING_MODE="normal"
    DEFINES="$DEFINES -DNORMAL_ADDRESSING"
    break
    ;;
  *)
    AC_MSG_ERROR([Internal configure.in script error for $am addressing mode])
  esac
done
AC_MSG_RESULT($ADDRESSING_MODE)
if [[ "x$ADDRESSING_MODE" = "x" ]]; then
  AC_MSG_ERROR([Sorry, no suitable addressing mode in $ADDRESSING_TEST_ORDER])
fi

dnl Check for GAS.
HAVE_GAS=no
AC_MSG_CHECKING(for GAS .p2align feature)
cat >conftest.S << EOF
	.text
	.p2align 5
EOF
if $CC conftest.S -c -o conftest.o >/dev/null 2>&1 ; then HAVE_GAS=yes; fi
AC_MSG_RESULT($HAVE_GAS)

dnl Check for GCC 2.7 or higher.
HAVE_GCC27=no
AC_MSG_CHECKING(for GCC 2.7 or higher)
AC_EGREP_CPP(xyes,
[#if __GNUC__ - 1 > 1 || __GNUC_MINOR__ - 1 > 5
  xyes
#endif
], [AC_MSG_RESULT(yes); HAVE_GCC27=yes], AC_MSG_RESULT(no))

dnl Check for GCC 3.0 or higher.
HAVE_GCC30=no
AC_MSG_CHECKING(for GCC 3.0 or higher)
AC_EGREP_CPP(xyes,
[#if __GNUC__ >= 3
  xyes
#endif
], [AC_MSG_RESULT(yes); HAVE_GCC30=yes], AC_MSG_RESULT(no))

dnl (gb) Do not merge constants since it breaks fpu/fpu_x86.cpp.
dnl As of 2001/08/02, this affects the following compilers:
dnl Official: probably gcc-3.1 (mainline CVS)
dnl Mandrake: gcc-2.96 >= 0.59mdk, gcc-3.0.1 >= 0.1mdk
dnl Red Hat : gcc-2.96 >= 89, gcc-3.0 >= 1
if [[ "x$HAVE_GCC27" = "xyes" -a "x$FPE_CORE" = "xx86" ]]; then
  SAVED_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -fno-merge-constants"
  AC_CACHE_CHECK([whether GCC supports constants merging], ac_cv_gcc_constants_merging, [
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_TRY_COMPILE([],[],[ac_cv_gcc_constants_merging=yes],[ac_cv_gcc_constants_merging=no])
    AC_LANG_RESTORE
  ])
  if [[ "x$ac_cv_gcc_constants_merging" != "xyes" ]]; then
    CXXFLAGS="$SAVED_CXXFLAGS"
  fi
fi

dnl Store motion was introduced in 3.3-hammer branch and any gcc >= 3.4
dnl However, there are some corner cases exposed on x86-64
if [[ "x$HAVE_GCC27" = "xyes" ]]; then
  SAVED_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -fno-gcse-sm"
  AC_CACHE_CHECK([whether GCC supports store motion], ac_cv_gcc_store_motion, [
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_TRY_COMPILE([],[],[ac_cv_gcc_store_motion=yes],[ac_cv_gcc_store_motion=no])
    AC_LANG_RESTORE
  ])
  if [[ "x$ac_cv_gcc_store_motion" != "xyes" ]]; then
    CXXFLAGS="$SAVED_CXXFLAGS"
  fi
fi

dnl Distributed dispatcher eliminates pipeline stall
dnl It needs computed gotos (supported by GNU C++)
dnl DODO - test for computed gotos
if [[ "x$WANT_DIS_DIP" = "xyes" ]]; then
  DEFINES="$DEFINES -DDISDIP"
fi

dnl And also remove functions calls costs
if [[ "x$WANT_SIGSEGV_GOTO" = "xyes" ]]; then
  AC_DEFINE(SIGSEGV_HANDLER_GOTO, 1, [Define if you want computed gotos for sigsegv handler])
fi

dnl Select appropriate CPU source and REGPARAM define.
CPUSRCS=
GENCPUSRCS=
ASM_OPTIMIZATIONS=none
if [[ "x$WANT_DIS_DIP" = "xyes" ]]; then
	GENCPUSRCS="cpuemu.cpp"
else
	GENCPUSRCS="cpuemu1.cpp cpuemu2.cpp cpuemu3.cpp cpuemu4.cpp cpuemu5.cpp cpuemu6.cpp cpuemu7.cpp cpuemu8.cpp"
fi

dnl gb-- JITSRCS and GENJITSRCS will be emptied later if the JIT is not available
dnl gb-- Other platforms should define their own set of noflags file variants
CAN_JIT=no
JITSRCS=
GENJITSRCS="compemu1.cpp compemu2.cpp compemu3.cpp compemu4.cpp compemu5.cpp compemu6.cpp compemu7.cpp compemu8.cpp"

if [[ "x$HAVE_GCC27" = "xyes" -a "x$HAVE_I386" = "xyes" ]]; then
  dnl i386 CPU
  DEFINES="$DEFINES -DUNALIGNED_PROFITABLE -DREGPARAM=\"__attribute__((regparm(3)))\""
  if [[ "x$HAVE_GAS" = "xyes" ]]; then
    ASM_OPTIMIZATIONS=i386
    DEFINES="$DEFINES -DX86_ASSEMBLY -DOPTIMIZED_FLAGS -DSAHF_SETO_PROFITABLE"
#    if [[ "x$WANT_DIS_DIP" = "xyes" ]]; then
#      GENCPUSRCS="cpufast.s"
#    else
#      GENCPUSRCS="cpufast1.s cpufast2.s cpufast3.s cpufast4.s cpufast5.s cpufast6.s cpufast7.s cpufast8.s"
#    fi
     GENJITSRCS="cpuemu1_nf.cpp cpuemu2_nf.cpp cpuemu3_nf.cpp cpuemu4_nf.cpp cpuemu5_nf.cpp cpuemu6_nf.cpp cpuemu7_nf.cpp cpuemu8_nf.cpp $GENJITSRCS"
#    GENJITSRCS="cpufast1_nf.s cpufast2_nf.s cpufast3_nf.s cpufast4_nf.s cpufast5_nf.s cpufast6_nf.s cpufast7_nf.s cpufast8_nf.s $GENJITSRCS"
    CAN_JIT=yes
  fi
elif [[ "x$HAVE_GCC30" = "xyes" -a "x$HAVE_X86_64" = "xyes" ]]; then
  dnl x86-64 CPU
  DEFINES="$DEFINES -DUNALIGNED_PROFITABLE"
  if [[ "x$HAVE_GAS" = "xyes" ]]; then
    ASM_OPTIMIZATIONS="x86-64"
    DEFINES="$DEFINES -DX86_64_ASSEMBLY -DOPTIMIZED_FLAGS"
    GENJITSRCS="cpuemu1_nf.cpp cpuemu2_nf.cpp cpuemu3_nf.cpp cpuemu4_nf.cpp cpuemu5_nf.cpp cpuemu6_nf.cpp cpuemu7_nf.cpp cpuemu8_nf.cpp $GENJITSRCS"
    CAN_JIT=yes
    WANT_33BIT_ADDRESSING=yes
  fi
elif [[ "x$HAVE_GCC27" = "xyes" -a "x$HAVE_SPARC" = "xyes" -a "x$HAVE_GAS" = "xyes" ]]; then
  dnl SPARC CPU
  case "$host_os" in
  solaris*)
    AC_MSG_CHECKING(SPARC CPU architecture)
    SPARC_TYPE=`solaris/which_sparc`
    AC_MSG_RESULT($SPARC_TYPE)
    case "$SPARC_TYPE" in
    SPARC_V8)
      ASM_OPTIMIZATIONS="SPARC V8 architecture"
      DEFINES="$DEFINES -DSPARC_V8_ASSEMBLY" dnl -DOPTIMIZED_FLAGS"
      CFLAGS="$CFLAGS -Wa,-Av8"
      CXXFLAGS="$CXXFLAGS -Wa,-Av8"
      ;;
    SPARC_V9)
      ASM_OPTIMIZATIONS="SPARC V9 architecture"
      DEFINES="$DEFINES -DSPARC_V9_ASSEMBLY" dnl -DOPTIMIZED_FLAGS"
      CFLAGS="$CFLAGS -Wa,-Av9"
      CXXFLAGS="$CXXFLAGS -Wa,-Av9"
      ;;
    esac
    ;;
  esac
fi

dnl JIT sources if available
if [[ "x$WANT_JIT" = "xyes" -a "x$CAN_JIT" = "xyes" ]]; then
  if [[ "x$WANT_MMU" = "xyes" ]]; then
      AC_MSG_WARN([!!!!! MMU&JIT in progress, dangerous !!!!!])
  fi
  JITSRCS="$JITSRCS uae_cpu/compiler/compemu_support.cpp"
  DEFINES="$DEFINES -DUSE_JIT=1"

  if [[ "x$WANT_JIT_DEBUG" = "xyes" ]]; then
    DEFINES="$DEFINES -DJIT_DEBUG=1"
  fi

  if [[ "x$WANT_JIT_FPU" = "xyes" ]]; then
    DEFINES="$DEFINES -DUSE_JIT_FPU=1"
    JITSRCS="$JITSRCS uae_cpu/compiler/compemu_fpp.cpp"
  fi

  GENJITSRCS="$GENJITSRCS compstbl.cpp cpustbl_nf.cpp"
else
  WANT_JIT=no
  WANT_JIT_FPU=no
  WANT_JIT_DEBUG=no
  JITSRCS=""
  GENJITSRCS=""
fi

dnl Use 33-bit memory addressing?
if [[ "$ac_cv_have_33bit_addressing:$WANT_33BIT_ADDRESSING" = "yes:yes" ]]; then
  use_33bit_addressing=yes
fi
AC_TRANSLATE_DEFINE(USE_33BIT_ADDRESSING, "$use_33bit_addressing",
  [Define to use 33-bit memory addressing on 64-bit JIT capable systems.])

dnl Select appropriate FPU source.
gcc_AC_C_FLOAT_FORMAT
AC_CHECK_HEADERS(ieee754.h ieeefp.h floatingpoint.h nan.h math.h)

if [[ "x$WANT_JIT_FPU" = "xyes" -a "x$FPE_CORE_TEST_ORDER" != "xieee" ]]; then
dnl  AC_MSG_WARN([Sorry, JIT-FPU supports only the "ieee" FPE implementation])
  FPE_CORE_TEST_ORDER="ieee"
  dnl Restore previous variables. FPE_CORE_STR and FPUSRCS are overwritten
fi

for fpe in $FPE_CORE_TEST_ORDER; do
  case $fpe in
  ieee)
    case $ac_cv_c_float_format in
    IEEE*)
      FPE_CORE="IEEE fpu core"
      DEFINES="$DEFINES -DFPU_IEEE"
      FPUSRCS="uae_cpu/fpu/fpu_ieee.cpp"
      dnl Math functions not mandated by C99 standard
      AC_CHECK_FUNCS(isnanl isinfl)
      dnl Math functions required by C99 standard, but probably not
      dnl implemented everywhere. In that case, we fall back to the
      dnl regular variant for doubles.
      AC_CHECK_FUNCS(logl log10l expl powl fabsl sqrtl)
      AC_CHECK_FUNCS(sinl cosl tanl sinhl coshl tanhl)
      AC_CHECK_FUNCS(asinl acosl atanl asinhl acoshl atanhl)
      AC_CHECK_FUNCS(floorl ceill)
      case $CPU_TYPE in
        alpha)
          dnl Maybe -mieee-with-inexact
	  CXXFLAGS="$CXXFLAGS -mieee"
	  LDFLAGS="$LDFLAGS -mieee"
	  break
	  ;;
        *)
          ;;
      esac
      break
      ;;
    esac
    ;;
  x86)
    if [[ ":$HAVE_GCC27:$HAVE_I386:$HAVE_GAS:" = ":yes:yes:yes:" ]]; then
      FPE_CORE="i387 fpu core"
      DEFINES="$DEFINES -DFPU_X86"
      FPUSRCS="uae_cpu/fpu/fpu_x86.cpp"
      break
    fi
    ;;
  uae)
    FPE_CORE="uae fpu core"
    DEFINES="$DEFINES -DFPU_UAE"
    FPUSRCS="uae_cpu/fpu/fpu_uae.cpp"
    break
    ;;
  *)
    AC_MSG_ERROR([Internal configure.in script error for $fpe fpu core])
    ;;
  esac
done
if [[ "x$FPE_CORE" = "x" ]]; then
  AC_MSG_ERROR([Sorry, no suitable FPU core found in $FPE_CORE_TEST_ORDER])
fi

dnl Check for certain math functions
AC_CHECK_FUNCS(atanh)
AC_CHECK_FUNCS(isnan isinf finite isnormal signbit)

dnl Select debugger
if [[ "x$WANT_JIT" = "xno" ]]; then
  dnl new debugger
  if [[ "x$WHICH_DEBUG" = "xndebug" -a "x$ac_cv_func_sigaction" = "xyes" ]]; then
    AC_DEFINE([DEBUGGER], 1, [Define if using some internal debugger])
    AC_DEFINE([NEWDEBUG], 1, [Define if using new debugger])
    CPUSRCS="$CPUSRCS uae_cpu/debug.cpp"
  fi

  dnl UAE debugger
  if [[ "x$WHICH_DEBUG" = "xuae" ]]; then
    AC_DEFINE([DEBUGGER], 1, [Define if using some internal debugger])
    AC_DEFINE([UAEDEBUG], 1, [Define if using UAE debugger])
    CPUSRCS="$CPUSRCS uae_cpu/debug.cpp"
  fi
fi

dnl Exclusive spcflags handling
if [[ "x$WANT_SPCFLAGS_EXCL" = "xyes" ]]; then
  DEFINES="$DEFINES -DENABLE_EXCLUSIVE_SPCFLAGS"
fi

dnl UAE CPU sources for all architectures.
CPUINCLUDES="-I\$(srcdir)/uae_cpu"
CPUSRCS="$CPUSRCS uae_cpu/aranym_glue.cpp uae_cpu/memory.cpp uae_cpu/newcpu.cpp uae_cpu/readcpu.cpp $JITSRCS $FPUSRCS $MMUSRCS"
GENCPUSRCS="$GENCPUSRCS cpudefs.cpp cpustbl.cpp $GENJITSRCS"

dnl Remove the "-g" option if set for GCC.
if [[ "x$HAVE_GCC27" = "xyes" ]]; then
  CFLAGS=`echo $CFLAGS | sed -e 's/-g\b//g'`
  CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-g\b//g'`
fi
if [[ "x$WANT_NATDEBUG" = "xyes" ]]; then
  DBGSP="-g"
  WFLAGS=""
  if [[ "x$HAVE_GCC27" = "xyes" ]]; then
    WFLAGS="$WFLAGS -W -Wall"
    DBGSP="-ggdb3"
  fi
else
  DBGSP=""
  WFLAGS=""
fi

if [[ "x$WANT_FULLDEBUG" = "xyes" ]]; then
  CFLAGS="$CFLAGS -DWHOLEDEBUG"
  CXXFLAGS="$CXXFLAGS -DWHOLEDEBUG"
fi

dnl Remove the "-O2" option if set for GCC and set the "-O3" option.
if [[ "x$HAVE_GCC27" = "xyes" ]]; then
  CFLAGS=`echo $CFLAGS | sed -e 's/-O2//g'`
  CFLAGS="-O3 $CFLAGS"
  CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-O2//g'`
  CXXFLAGS="-O3 $CXXFLAGS"
fi

dnl Remove -O[23] for cygwin gcc 3.x (gcc-3.3.3-3 found buggy).
if [[ "x$OS_TYPE" = "xcygwin" -a "x$HAVE_GCC30" = "xyes" ]]; then
  CFLAGS=`echo $CFLAGS | sed -e 's/-O[[23]]//g'`
  CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-O[[23]]//g'`
fi

dnl Or if we have -Ofast
if [[ "x$HAVE_OFAST" = "xyes" ]]; then
  CFLAGS="`echo $CFLAGS | sed -e 's/-g//g'` -Ofast"
  CXXFLAGS="`echo $CXXFLAGS | sed -e 's/-g//g'` -Ofast"
  CXXFLAGS="-LANG:std $CXXFLAGS"
  LDFLAGS="$LDFLAGS -ipa"
fi

dnl For computed gotos it is necessary no-gcse
if [[ "x$WANT_DIS_DIP" = "xyes" ]]; then
  if [[ "x$HAVE_GCC27" = "xyes" ]]; then
    CXXFLAGS="$CXXFLAGS -fno-gcse"
  fi
fi

dnl Check for SDL
SDL_CFLAGS=
SDL_LIBS=
if test "x$OS_TYPE" == "xdarwin" -a "x$WITH_FINK" = "xno"; then
	ARANYM_CHECK_FRAMEWORK(SDL, [])
fi
if test "x$have_framework_SDL" = "xyes" ; then
	ARANYM_CHECK_FRAMEWORK_LOCATION(SDL)
	CPPFLAGS="$CPPFLAGS -I$SDL_LOCATION/Headers"
	BUNDLE_FRAMEWORKS="$BUNDLE_FRAMEWORKS $SDL_LOCATION"
else
	dnl Try macro from sdl.m4
	AM_PATH_SDL(1.2.10, :, AC_MSG_ERROR([*** SDL version $SDL_VERSION or higher not found! ***]))
fi

dnl Check for SDL_Image
ARANYM_CHECK_FRAMEWORK(SDL_image, [])
if test "x$have_framework_SDL_image" = "xyes" ; then
	ARANYM_CHECK_FRAMEWORK_LOCATION(SDL_image)
	CPPFLAGS="$CPPFLAGS -I$SDL_image_LOCATION/Headers"
	BUNDLE_FRAMEWORKS="$BUNDLE_FRAMEWORKS $SDL_image_LOCATION"
	have_sdlimage_lib=yes
	have_sdlimage_h=yes
else
	tmp_cflags=$CFLAGS
	tmp_cppflags=$CPPFLAGS
	CFLAGS="$CFLAGS $SDL_CFLAGS"
	CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"

	AC_CHECK_HEADER(SDL_image.h, have_sdlimage_h=yes, havesdlimage_h=no)
	AC_CHECK_LIB(SDL_image, IMG_isJPG, have_sdlimage_lib=yes, have_sdlimage_lib=no)

	CFLAGS=$tmp_cflags
	CPPFLAGS=$tmp_cppflags

	if test "x$have_sdlimage_h" = "xyes" -a "x$have_sdlimage_lib" = "xyes"; then
		LIBS="$LIBS -lSDL_image"
	fi
fi
if test "x$have_sdlimage_h" = "xyes" -a "x$have_sdlimage_lib" = "xyes"; then
	AC_DEFINE(HAVE_SDL_IMAGE, 1, [Define if you have SDL_image])
fi

dnl Add OpenGL ?
if test "x$WANT_OPENGL" = "xyes"; then
	AC_MSG_CHECKING(for OpenGL support)
	have_opengl="no"
	GL_LIBS=""
	case "$host" in
		*-*-cygwin* | *-*-mingw*)
			GL_LIBS="-lopengl32"
			;;
		*-*-darwin*)
			GL_LIBS="-Wl,-framework,OpenGL"
			;;
		*)
			GL_LIBS="-lGL"
			;;
	esac
	tmp_cflags=$CFLAGS
	tmp_libs=$LIBS
	CFLAGS="$CFLAGS $SDL_CFLAGS"
	LIBS="$LIBS $SDL_LIBS $GL_LIBS"
	AC_TRY_COMPILE([
		 #include "SDL_opengl.h"
		],[],[have_opengl="yes"])
	AC_MSG_RESULT($have_opengl)
	CFLAGS=$tmp_cflags
	LIBS=$tmp_libs
	DYNGLSRCS=""
	if test "x$have_opengl" = "xyes"; then
		AC_DEFINE([ENABLE_OPENGL], 1, [Define if OpenGL enabled])
		DYNGLSRCS="dyngl.c"
	else
		WANT_OPENGL="no"
	fi

dnl	MDL_HAVE_OPENGL
dnl	if test "x$have_GL" = "xyes"; then
dnl		AC_DEFINE([ENABLE_OPENGL], 1, [Define if using OpenGL])
dnl		CFLAGS="$CFLAGS $GL_CFLAGS"
dnl		CXXFLAGS="$CXXFLAGS $GL_CFLAGS"
dnl		LIBS="$LIBS $GL_LIBS"
dnl	else
dnl		WANT_OPENGL="no"
dnl	fi
fi

dnl Check for zlib
if [[ "x$WANT_LILO" = "xyes" ]]; then
	CHECK_ZLIB()

	if [[ "x$have_zlib" = "xyes" ]]; then
		LIBS="$LIBS -lz"
	fi
else
	have_zlib="not required"
fi

dnl Linux loader

if [[ "x$WANT_LILO" = "xyes" ]]; then
	if [[ "x$WANT_MMU" = "xyes" ]]; then
		AC_DEFINE([ENABLE_LILO], 1, [Define if lilo enabled])
	else
		AC_MSG_ERROR([Linux kernel requires functional MMU emulation])
	fi
fi

dnl Natfeat CD-ROM driver

NFCDROMSRCS=""
if [[ "x$WANT_NFCDROM" = "xyes" ]]; then
	AC_DEFINE([NFCDROM_SUPPORT], 1, [Define if you want to use host CD-ROM driver])
	NFCDROMSRCS="natfeat/nfcdrom.cpp"
	case "$host_os" in
		linux*)
			NFCDROMSRCS="$NFCDROMSRCS natfeat/nfcdrom_linux.cpp"
			AC_DEFINE([NFCDROM_LINUX_SUPPORT], 1, [Define if you want to use Linux support as CD-ROM driver])
			;;
	esac
fi

dnl Natfeat PCI driver

NFPCISRCS=""
if [[ "x$WANT_NFPCI" = "xyes" ]]; then
	case "$host_os" in
		linux*)
			AC_DEFINE([NFPCI_SUPPORT], 1, [Define if you want to use host PCI driver])
			NFPCISRCS="natfeat/nfpci.cpp"
			AC_DEFINE([NFPCI_LINUX_SUPPORT], 1, [Define if you want to use Linux PCI driver])
			NFPCISRCS="$NFPCISRCS natfeat/nfpci_linux.cpp"
			;;
		*)
			AC_MSG_ERROR([*** NatFeats PCI not supported by this OS ***])
			;;
	esac
fi

dnl Natfeat OSMesa driver

NFOSMESASRCS=""
if test "x$WANT_NFOSMESA" = "xyes"; then
#	AC_CHECK_LIB(SDL, SDL_LoadObject, have_sdlloadso=yes, have_sdlloadso=no)
#	if test "x$have_sdlloadso" = "xyes"; then
		AC_DEFINE([NFOSMESA_SUPPORT], 1, [Define if you want to use host OSMesa driver])
		NFOSMESASRCS="natfeat/nfosmesa.cpp"
		if test "x$WITH_MESAINC" != "xno"; then
			CPPFLAGS="$CPPFLAGS -I$WITH_MESAINC"
		else
			case "$host_os" in
				dnl For darwin the OSMesa include path is by default not defined
				darwin*)
					CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include/"
				;;
			esac
		fi
#	else
#		AC_MSG_ERROR([*** SDL>=1.2.6 mandatory for nfosmesa ***])
#		WANT_NFOSMESA="no"
#	fi
fi

dnl Natfeat JPEG decoder

NFJPEGSRCS=""
if [[ "x$WANT_NFJPEG" = "xyes" ]]; then
	if test "x$have_sdlimage_h" = "xyes" -a "x$have_sdlimage_lib" = "xyes"; then
		AC_DEFINE([NFJPEG_SUPPORT], 1, [Define if you want to use host JPEG decoder])
		NFJPEGSRCS="natfeat/nfjpeg.cpp"
	else
		WANT_NFJPEG="no"
	fi
fi

dnl Natfeat VDI driver

NFVDISRCS=""
if [[ "x$WANT_NFVDI" = "xyes" ]]; then
	AC_DEFINE([NFVDI_SUPPORT], 1, [Define if you want to use NF VDI driver])
	NFVDISRCS="natfeat/nfvdi.cpp natfeat/nfvdi_soft.cpp"
	if [[ "x$WANT_OPENGL" = "xyes" ]]; then
		NFVDISRCS="$NFVDISRCS natfeat/nfvdi_opengl.cpp"
	fi
fi

dnl Natfeat Clipboard driver (so far Windows only)

NFCLIPBOARDSRCS=""
if [[ "x$WANT_NFCLIPBRD" = "xyes" ]]; then
	if [[ "x$OS_TYPE" = "xcygwin" ]]; then
		AC_DEFINE([NFCLIPBRD_SUPPORT], 1, [Define if you want to use MS-Windows Clipboard])
		NFCLIPBOARDSRCS="natfeat/nfclipbrd.cpp"
    	SYSSRCS="$SYSSRCS Unix/cygwin/clipbrd_cygwin.cpp"
	else
        AC_MSG_ERROR([Sorry, NatFeat Clipboard not implemented on your platform yet])
	fi
fi

dnl Exception per second limiter

if [[ "x$WANT_EPSLIMITER" = "xyes" ]]; then
	AC_DEFINE([ENABLE_EPSLIMITER], 1, [Define if you want to use EPS limiter])
fi

dnl Linux /dev/rtc source timer

if test "x$WANT_RTCTIMER" = "xyes" ; then
	if test "x$OS_TYPE" = "xlinux"; then
		AC_DEFINE([RTC_TIMER], 1, [Define if you want to use Linux /dev/rtc as source timer])
	else
		WANT_RTCTIMER="no"
	fi
fi

dnl Linux/X86 hardware access for parallel port

PARALLELX86SRCS=""
if test "x$WANT_PARALLELX86" = "xyes" ; then
	if test "x$OS_TYPE" = "xlinux" -a "x$HAVE_I386" = "xyes"; then
		AC_DEFINE([ENABLE_PARALLELX86], 1, [Define if you want to use Linux/X86 hardware access for parallel port])
		PARALLELX86SRCS="parallel_x86.cpp"
	else
		WANT_PARALLELX86="no"
	fi
fi

dnl Linux /dev/parport access for parallel port

PARALLELPARPORTSRCS=""
if test "x$WANT_PARALLELPARPORT" = "xyes" ; then
	if test "x$OS_TYPE" = "xlinux"; then
		AC_DEFINE([ENABLE_PARALLELPARPORT], 1, [Define if you want to use Linux /dev/parport access for parallel port])
		PARALLELPARPORTSRCS="parallel_parport.cpp"
	else
		WANT_PARALLELPARPORT="no"
	fi
fi

dnl Linux /dev/sequencer MIDI support

if test "x$OS_TYPE" = "xlinux"; then
	AC_DEFINE([ENABLE_MIDI_SEQUENCER], 1, [Define if you want to use Linux /dev/sequencer support])
	MIDISEQSRCS="midi_sequencer.cpp"
fi

dnl Data directory

APP=aranym
case "$host_os" in
	darwin*)
		if test "x$WITH_FINK" = "xyes"; then
			TARGET="\$(APP)"
		else
			TARGET="\$(APP_BUNDLE)"
		fi
		ARANYM_DATADIR="\"\\\"\${datadir}/\${APP}\\\"\""
		srcdir="../../.."
		;;
	mingw*) TARGET="\$(APP)"
		ARANYM_DATADIR="\"\\\"./\${APP}\\\"\""
		;;
	*)	TARGET="\$(APP)"
		ARANYM_DATADIR="\"\\\"\${datadir}/\${APP}\\\"\""
		;;
esac
CFLAGS="$CFLAGS -DARANYM_DATADIR=$ARANYM_DATADIR"
CXXFLAGS="$CXXFLAGS -DARANYM_DATADIR=$ARANYM_DATADIR"

dnl Remove the ARCHFLAGS again as they must be handled separately inside the Makefile.
if [[ "x$ARCHFLAGS" != "x" ]]; then
	CPPFLAGS=`echo $CPPFLAGS | sed -e "s:$ARCHFLAGS::g"`
	LDFLAGS=`echo $LDFLAGS | sed -e "s:$ARCHFLAGS::g"`
fi

dnl Generate Makefile.

AC_SUBST(WFLAGS)
AC_SUBST(SET_MAKE)
AC_SUBST(DEFINES)
AC_SUBST(SYSSRCS)
AC_SUBST(CPUINCLUDES)
AC_SUBST(CPUSRCS)
AC_SUBST(GENCPUSRCS)
AC_SUBST(DSPSRCS)
AC_SUBST(MIDISEQSRCS)
AC_SUBST(NFCDROMSRCS)
AC_SUBST(NFPCISRCS)
AC_SUBST(NFOSMESASRCS)
AC_SUBST(NFJPEGSRCS)
AC_SUBST(NFVDISRCS)
AC_SUBST(NFCLIPBOARDSRCS)
AC_SUBST(PARALLELX86SRCS)
AC_SUBST(PARALLELPARPORTSRCS)
AC_SUBST(GUISDLSRCS)
AC_SUBST(DYNGLSRCS)
AC_SUBST(DBGSP)
AC_SUBST(TAPIF)
AC_SUBST(TARGET)
AC_SUBST(CPU_TYPE)
AC_SUBST(ARCHFLAGS)
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)
AC_SUBST(BUNDLE_FRAMEWORKS)
AC_OUTPUT(Makefile)

dnl Print summary.
echo
echo ARAnyM configuration summary:
echo
echo Use exclusive spcflags handling ........ : $WANT_SPCFLAGS_EXCL
echo Use JIT compiler ....................... : $WANT_JIT
echo Use JIT compiler for FPU instructions .. : $WANT_JIT_FPU
echo JIT debug mode ......................... : $WANT_JIT_DEBUG
echo Floating-Point emulation core .......... : $FPE_CORE
echo Assembly optimizations ................. : $ASM_OPTIMIZATIONS
echo Addressing mode ........................ : $ADDRESSING_MODE
echo -n Full MMU support ....................... : $WANT_MMU
if [[ "x$WANT_MMU" = "xyes" ]]; then
    echo -n " ("
    case "$ATC_TYPE" in
		full)	echo -n "with ATC";;
		small)	echo -n "with small ATC";;
		no)		echo -n "without ATC";;
    esac
    echo -n \)
fi
echo
echo Linux /dev/rtc source timer ............ : $WANT_RTCTIMER
echo Real STOP implementation ............... : $WANT_REALSTOP
echo DSP 56001 support ...................... : $WANT_DSP
echo DSP 56001 disassembly support .......... : $WANT_DSP_DISASM
echo Distributed dispatcher ................. : $WANT_DIS_DIP
echo Debugger ............................... : $WHICH_DEBUG
echo Check memory ........................... : $MEMORY_CHECK
echo First 2kB of RAM Supervisor protected... : $WANT_PROTECT2K
echo FastRAM fixed size ..................... : $WANT_FIXEDFASTRAM
echo Fixed position of VideoRAM ............. : $WANT_FIXEDVIDEORAM
echo Blitter memmove acceleration ........... : $WANT_BLITMEMMOVE
echo Blitter SDL blit acceleration .......... : $WANT_BLITSDLBLIT
echo Support for hostfs ..................... : $WANT_HOSTFS
echo Support for ethernet ................... : $HAVE_ETHERNET
echo GUI .................................... : $WANT_SDLGUI
echo OpenGL ................................. : $WANT_OPENGL
echo Linux-m68k loader ...................... : $WANT_LILO
echo Zlib ................................... : $have_zlib
echo NatFeat CD-ROM driver .................. : $WANT_NFCDROM
echo NatFeat PCI driver ..................... : $WANT_NFPCI
echo NatFeat OSMesa driver .................. : $WANT_NFOSMESA
echo NatFeat JPEG decoder ................... : $WANT_NFJPEG
echo NatFeat Clipboard ...................... : $WANT_NFCLIPBRD
echo NatFeat VDI driver ..................... : $WANT_NFVDI
echo Exception per second limiter ........... : $WANT_EPSLIMITER
echo Linux/X86 h/w access for parallel port.. : $WANT_PARALLELX86
echo Linux /dev/parport for parallel port.... : $WANT_PARALLELPARPORT
echo Use C++ exceptions for m68k exceptions.. : $WANT_CXX_EXCEPTIONS
echo
echo "Configuration done. Now type \"make depend\" (or \"gmake depend\") then
\"make\" (or \"gmake\")."
