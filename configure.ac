dnl Process this file with autoconf to produce a configure script.
dnl Written in 1999 by Christian Bauer et al.
dnl Adapted for ARAnyM by Milan Jurik
dnl Adapted to automake by Thorsten Otto

m4_define(ARANYM_VERSION, m4_esyscmd([sed -n -e 's/#define.*VER_MAJOR.*\([0-9][0-9]*\).*$/\1./p
s/#define.*VER_MINOR.*\([0-9][0.9]*\).*$/\1./p
s/#define.*VER_MICRO.*\([0-9][0-9]*\).*$/\1/p' src/include/version.h | tr -d '\n']))

AC_INIT(aranym, ARANYM_VERSION, [aranym@googlegroups.com],,[https://github.com/aranym/aranym])
AC_PREREQ(2.52)
AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_SRCDIR([src/hostscreen.cpp])

dnl Define list of provided Native Features
PROVIDES_NF="audio bootstrap xhdi"

dnl Canonical system information.
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE(check-news m4_esyscmd([case `automake --version | head -n 1` in 
                             *1.10*);; 
                             *) echo subdir-objects color-tests;; 
                             esac]))
AM_MAINTAINER_MODE([disable])

dnl Silent rules
# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AH_TOP([
#ifndef __ARANYM_CONFIG_H__
#define __ARANYM_CONFIG_H__

#if defined(__linux__) && !defined(__ANDROID__)
#include "linux/libcwrap.h"
#endif

#ifndef _FORTIFY_SOURCE
#  define _FORTIFY_SOURCE 2
#endif

])

dnl Options.
AC_ARG_ENABLE(nat-debug,     [AC_HELP_STRING([--enable-nat-debug], [enable native debugger support (default=yes)])], [WANT_NATDEBUG=$enableval], [WANT_NATDEBUG=yes])

AC_ARG_ENABLE(full-debug,    [AC_HELP_STRING([--enable-full-debug], [enable full debug outputs (default=no)])], [WANT_FULLDEBUG=$enableval], [WANT_FULLDEBUG=no])

AC_ARG_ENABLE(debug,
[AC_HELP_STRING([--enable-debug=which], [specify wether to use internal debugger (default=no)])],
[ case "$enableval" in
    default | yes | uae | ndebug)  USE_DEBUGGER=yes;;
         no)  USE_DEBUGGER="no";;
	  *)  AC_MSG_ERROR([--enable-debug takes only one of the following values: default, uae, ndebug, no]);;
  esac
],
[USE_DEBUGGER=no
])

AC_ARG_ENABLE(fullhistory,     [AC_HELP_STRING([--enable-fullhistory], [enable full history for debugger (default=no)])], [WANT_HISTORY=$enableval], [WANT_HISTORY=no])

AC_ARG_ENABLE(flightrecorder,  [AC_HELP_STRING([--enable-flightrecorder], [enable CPU flight recorder (default=no)])], [WANT_FLIGHTRECORDER=$enableval], [WANT_FLIGHTRECORDER=no])

AC_ARG_ENABLE(fullmmu,         [AC_HELP_STRING([--enable-fullmmu], [enable full 68040 MMU (default=no)])], [WANT_MMU=$enableval], [WANT_MMU=no])

AC_ARG_ENABLE(atc,
[AC_HELP_STRING([--enable-atc=ATC], [select type of ATC for MMU (default=full)])],
[ case "$enableval" in
        full)  ATC_TYPE="full";;
       small)  ATC_type="small";;
          no)  ATC_TYPE="no";;
           *)  AC_MSG_ERROR([--enable-atc takes only one of the following values: full, small, no]);;
  esac
], [ATC_TYPE=full])

AC_ARG_ENABLE(realstop,        [AC_HELP_STRING([--enable-realstop], [enable real STOP instruction (default=yes)])], [WANT_REALSTOP=$enableval], [WANT_REALSTOP=yes])

AC_ARG_ENABLE(dsp,             [AC_HELP_STRING([--enable-dsp], [enable DSP 56001 (default=yes)])], [WANT_DSP=$enableval], [WANT_DSP=yes])

AC_ARG_ENABLE(dsp-disasm,      [AC_HELP_STRING([--enable-dsp-disasm], [enable DSP 56001 disassembler (default=no)])], [WANT_DSP_DISASM=$enableval], [WANT_DSP_DISASM=no])

AC_ARG_ENABLE(protect2k,       [AC_HELP_STRING([--enable-protect2k], [enable first 2kB of RAM Super protection (default=yes)])], [WANT_PROTECT2K=$enableval], [WANT_PROTECT2K=yes])

AC_ARG_ENABLE(fixedfastram,    [AC_HELP_STRING([--enable-fixedfastram=X], [use fixed size FastRAM of X MB (default=no)])], [WANT_FIXEDFASTRAM=$enableval], [WANT_FIXEDFASTRAM=no])

AC_ARG_ENABLE(fixedvideoram,   [AC_HELP_STRING([--enable-fixedvideoram], [use fixed start of VideoRAM (default=no)])], [WANT_FIXEDVIDEORAM=$enableval], [WANT_FIXEDVIDEORAM=no])

AC_ARG_ENABLE(blitmemmove,     [AC_HELP_STRING([--enable-blitmemmove], [enable Blitter memmove rout (default=no)])], [WANT_BLITMEMMOVE=$enableval], [WANT_BLITMEMMOVE=no])

AC_ARG_ENABLE(blitsdlblit,     [AC_HELP_STRING([--enable-blitsdlblit], [enable SDL blitting routine EXPERIMENTAL UNSUPPORTED BUGGY (default=no)])], [WANT_BLITSDLBLIT=$enableval], [WANT_BLITSDLBLIT=no])

AC_ARG_ENABLE(hostfs,          [AC_HELP_STRING([--enable-hostfs], [enable hostfs support (default=yes)])], [WANT_HOSTFS=$enableval], [WANT_HOSTFS=yes])

AC_ARG_ENABLE(ctrlkey,
[AC_HELP_STRING([--enable-ctrlkey=WHICH], [specify which key to use for control key emulation (default=both)])],
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

AC_ARG_ENABLE(gui,             [AC_HELP_STRING([--enable-gui], [enable GUI (default=yes)])], [WANT_SDLGUI=$enableval], [WANT_SDLGUI=yes])

AC_ARG_ENABLE(opengl,          [AC_HELP_STRING([--enable-opengl], [enable OpenGL (default=yes)])], [WANT_OPENGL=$enableval], [WANT_OPENGL=yes])

AC_ARG_WITH(mesa-headers,      [AC_HELP_STRING([--with-mesa-headers], [Path to Mesa header files])], [WITH_MESAINC=$withval], [WITH_MESAINC=no])

AC_ARG_ENABLE(nfpci,           [AC_HELP_STRING([--enable-nfpci], [enable NatFeat PCI (default=no)])], [WANT_NFPCI=$enableval], [WANT_NFPCI=no])

AC_ARG_ENABLE(usbhost,         [AC_HELP_STRING([--enable-usbhost], [enable NatFeat USB (default=no)])], [WANT_USBHOST=$enableval], [WANT_USBHOST=no])

AC_ARG_ENABLE(nfosmesa,        [AC_HELP_STRING([--enable-nfosmesa], [enable NatFeat OSMesa (default=no)])], [WANT_NFOSMESA=$enableval], [WANT_NFOSMESA=no])

AC_ARG_ENABLE(nfjpeg,          [AC_HELP_STRING([--enable-nfjpeg], [enable NatFeat JPEG (default=yes)])], [WANT_NFJPEG=$enableval], [WANT_NFJPEG=yes])

AC_ARG_ENABLE(nfclipbrd,       [AC_HELP_STRING([--enable-nfclipbrd], [enable NatFeat Clipboard (default=yes)])], [WANT_NFCLIPBRD=$enableval], [WANT_NFCLIPBRD=yes])

AC_ARG_ENABLE(nfvdi,           [AC_HELP_STRING([--enable-nfvdi], [enable NatFeat VDI (default=yes)])], [WANT_NFVDI=$enableval], [WANT_NFVDI=yes])

AC_ARG_ENABLE(scsi,            [AC_HELP_STRING([--enable-nfscsi], [enable NatFeat SCSI (default=yes)])], [WANT_NFSCSI=$enableval], [WANT_NFSCSI=yes])

AC_ARG_ENABLE(nfexec,          [AC_HELP_STRING([--enable-nfexec], [enable NatFeat host file execution (default=yes)])], [WANT_NFEXEC=$enableval], [WANT_NFEXEC=yes])

AC_ARG_ENABLE(ata-cdrom,       [AC_HELP_STRING([--enable-ata-cdrom], [enable ATA CD-ROM emulation (default=yes)])], [WANT_CDROM=$enableval], [WANT_CDROM=yes])

AC_ARG_ENABLE(epslimiter,      [AC_HELP_STRING([--enable-epslimiter], [enable Exception per second limiter (default=no)])], [WANT_EPSLIMITER=$enableval], [WANT_EPSLIMITER=no])

AC_ARG_ENABLE(lilo,            [AC_HELP_STRING([--enable-lilo], [enable Linux-m68k loader (default=no)])], WANT_LILO="$enableval", WANT_LILO="no")

AC_ARG_ENABLE(rtctimer,        [AC_HELP_STRING([--enable-rtctimer], [enable Linux /dev/rtc as source timer (default=yes)])], WANT_RTCTIMER="$enableval", WANT_RTCTIMER="yes")

AC_ARG_ENABLE(parallelx86,     [AC_HELP_STRING([--enable-parallelx86], [enable Linux X86 hw access for parallel port (default=no)])], [WANT_PARALLELX86=$enableval], [WANT_PARALLELX86=no])

AC_ARG_ENABLE(parport,         [AC_HELP_STRING([--enable-parport], [enable Linux /dev/parport access for parallel port (default=yes)])], [WANT_PARALLELPARPORT=$enableval], [WANT_PARALLELPARPORT=yes])

AC_ARG_ENABLE(serialport,      [AC_HELP_STRING([--enable-serialport], [enable Unix /dev/ttySn serial port support (default=yes)])], [WANT_UNIXSERIALPORT=$enableval], [WANT_UNIXSERIALPORT=yes])

AC_ARG_WITH(fink,              [AC_HELP_STRING([--with-fink], [Use Fink suite to build Aranym on MacOSX (default=no)])], [WITH_FINK=$withval], [WITH_FINK=no])

AC_ARG_WITH(classicconfig,     [AC_HELP_STRING([--with-classicconfig], [Use Classic config names on MacOSX (default=no)])], [WITH_CLASSIC_CONFIG=$withval], [WITH_CLASSIC_CONFIG=no])

dnl FPU emulation core.
if [[ "x$WANT_MMU" = "xyes" ]]; then
    SUGGESTED_AUTO_FPE_CORE_TEST_ORDER="mpfr ieee uae"
else
    SUGGESTED_AUTO_FPE_CORE_TEST_ORDER="ieee uae"
fi
AC_ARG_ENABLE(fpe,
[AC_HELP_STRING([--enable-fpe=FPE], [specify which fpu emulator to use (default=auto)])],
[ case "$enableval" in
    dnl default is always ieee, if architecture has this fp format
    auto)	FPE_CORE_TEST_ORDER=$SUGGESTED_AUTO_FPE_CORE_TEST_ORDER;;
    ieee)	FPE_CORE_TEST_ORDER="ieee";;
    uae)	FPE_CORE_TEST_ORDER="uae";;
    x86)	FPE_CORE_TEST_ORDER="x86";;
    mpfr)	FPE_CORE_TEST_ORDER="mpfr";;
    *)		AC_MSG_ERROR([--enable-fpe takes only one of the following values: auto, uae, x86, ieee, mpfr]);;
  esac
],
[ FPE_CORE_TEST_ORDER=$SUGGESTED_AUTO_FPE_CORE_TEST_ORDER
])

dnl Addressing modes.
AC_ARG_ENABLE(addressing,
[AC_HELP_STRING([--enable-addressing=AM], [specify the addressing mode to use (default=fastest)])],
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
[AC_HELP_STRING([--enable-addr-check=AM], [specify the memory check to use (default=page)])],
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
AC_ARG_ENABLE(spcflags-excl, [AC_HELP_STRING([--enable-spcflags-excl], [enable exclusive spcflags handling (default=no)])], [WANT_SPCFLAGS_EXCL=$enableval], [WANT_SPCFLAGS_EXCL=no])

dnl JIT compiler options.
AC_ARG_ENABLE(jit-compiler, [AC_HELP_STRING([--enable-jit-compiler], [enable JIT compiler (default=no)])], [WANT_JIT=$enableval], [WANT_JIT=no])
AC_ARG_ENABLE(jit2, [AC_HELP_STRING([--enable-jit2], [enable JIT (v2) compiler (experimental/ARM only) (default=no)])], [WANT_JIT2=$enableval], [WANT_JIT2=no])

AC_ARG_ENABLE(jit-fpu,
[AC_HELP_STRING([--enable-jit-fpu], [enable JIT compiler for FPU instructions (default=no)])], [WANT_JIT_FPU=$enableval], [WANT_JIT_FPU=no])

AC_ARG_ENABLE(jit-debug,       [AC_HELP_STRING([--enable-jit-debug], [activate native code disassemblers (default=no)])], [WANT_JIT_DEBUG=$enableval], [WANT_JIT_DEBUG=no])

AC_ARG_ENABLE(ethernet,        [AC_HELP_STRING([--enable-ethernet], [enable ethernet card emulation (default=yes)])], [WANT_ETHERNET=$enableval], [WANT_ETHERNET=yes])

AC_ARG_ENABLE(bpf-ethernet,    [AC_HELP_STRING([--enable-bpf-ethernet], [enable ethernet card emulation using BPF (default=no)])], [WANT_BFP_ETHERNET=$enableval], [WANT_BFP_ETHERNET=no])

AC_ARG_ENABLE(cxx-exceptions,  [AC_HELP_STRING([--enable-cxx-exceptions], [enable the use of C++ try/catch instead of setjmp/longjmp (default=yes)])], [WANT_CXX_EXCEPTIONS=$enableval], [WANT_CXX_EXCEPTIONS=yes])

dnl Host OS type (host is build if not cross-compiling).
ANDROID_OS=no
case "$host_os" in
  *android*)	OS_TYPE=linux
		ANDROID_OS=yes
		LIBS="$LIBS -lgnustl_static -llog"
	;;
  linux*)	OS_TYPE=linux;;
  openbsd*)	OS_TYPE=openbsd
		CFLAGS="$CFLAGS -I/usr/local/include"
		LIBS="$LIBS -L/usr/local/lib"
		;;
  netbsd*)	OS_TYPE=netbsd;;
  freebsd*|kfreebsd*-gnu)	OS_TYPE=freebsd;;
  solaris*)	OS_TYPE=solaris;;
  darwin*)	OS_TYPE=darwin;;
  cygwin*)	OS_TYPE=cygwin;;
  irix*)	OS_TYPE=irix;;
  mint*)	OS_TYPE=mint;;
  beos*|haiku*)	OS_TYPE=beos;;
  mingw*)	OS_TYPE=mingw;;
  *)		OS_TYPE=unknown;;
esac
AM_CONDITIONAL([OS_TYPE_linux], test "$OS_TYPE" = "linux")
AM_CONDITIONAL([OS_TYPE_openbsd], test "$OS_TYPE" = "openbsd")
AM_CONDITIONAL([OS_TYPE_netbsd], test "$OS_TYPE" = "netbsd")
AM_CONDITIONAL([OS_TYPE_freebsd], test "$OS_TYPE" = "freebsd")
AM_CONDITIONAL([OS_TYPE_solaris], test "$OS_TYPE" = "solaris")
AM_CONDITIONAL([OS_TYPE_darwin], test "$OS_TYPE" = "darwin")
AM_CONDITIONAL([OS_TYPE_cygwin], test "$OS_TYPE" = "cygwin")
AM_CONDITIONAL([OS_TYPE_irix], test "$OS_TYPE" = "irix")
AM_CONDITIONAL([OS_TYPE_mint], test "$OS_TYPE" = "mint")
AM_CONDITIONAL([OS_TYPE_beos], test "$OS_TYPE" = "beos")
AM_CONDITIONAL([OS_TYPE_mingw], test "$OS_TYPE" = "mingw")
AM_CONDITIONAL([OS_TYPE_unknown], test "$OS_TYPE" = "unknown")
AM_CONDITIONAL([ANDROID_OS], test "$ANDROID_OS" = "yes")
AM_CONDITIONAL([WITH_FINK], test "$WITH_FINK" = "yes")
if test "$OS_TYPE" = "unknown"; then
  OS_TYPE=`echo $host_os | sed -e 's/-/_/g' | sed -e 's/\./_/g'`
fi
#
# provide some defaults on cygwin,
# where we use the MinGW version of the SDL library,
# to simplify standard configuration.
# But only if the user did not override it.
#
if test "$OS_TYPE" = "cygwin"; then
  if test "$prefix" = NONE; then
    prefix=/usr
  fi
fi
DEFINES="$DEFINES -DOS_$OS_TYPE"
AC_DEFINE_UNQUOTED([OS_TYPE], "$OS_TYPE", [Define OS type])

if test "$OS_TYPE" = "darwin"; then
	# Make sure MPFR (and other 3rd party libs installed by MacPorts) are found
	# Note that the order of these flags may be different
	# in the Xcode project, which is important atleast for the
	# SDL framework because we want to use the headers from
	# the official build rather than the ones from MacPorts,
	# which are only suitable for a single target architecture
	LDFLAGS="$LDFLAGS -L/opt/local/lib"
	CPPFLAGS="$CPPFLAGS -I/opt/local/include"
	# SOURCE_ROOT is set when running from Xcode project
	if test "$XCODE_VERSION_ACTUAL" != "" -a "$SOURCE_ROOT" != ""; then
		LDFLAGS="$LDFLAGS -L$SOURCE_ROOT/external/lib"
		CPPFLAGS="$CPPFLAGS -I$SOURCE_ROOT/external/include"
	fi
fi

dnl Target CPU type.
ARCHFLAGS=
HAVE_I386=false
HAVE_ARM=false
HAVE_SPARC=false
HAVE_POWERPC=false
HAVE_X86_64=false
case "$host_cpu" in
  i386* | i486* | i586* | i686* | i786* )
	CPU_TYPE=i386
	HAVE_I386=true
	if [[ "x$OS_TYPE" = "xdarwin" ]]; then
		ARCHFLAGS="-arch i386"
        if [[ "x$DEVELOPER_SDK_DIR" = "x" ]]; then
            DEVELOPER_SDK_DIR=/Developer/SDKs
        fi
        if [[ ! -d "$DEVELOPER_SDK_DIR" ]]; then
            AC_MSG_ERROR([Cannot find SDKs directory at $DEVELOPER_SDK_DIR])
        fi
		case "$host_os" in
			darwin8* | darwin9* )	ISYSROOT="-mmacosx-version-min=10.4 -isysroot $DEVELOPER_SDK_DIR/MacOSX10.4u.sdk";;
			darwin10* )	ISYSROOT="-mmacosx-version-min=10.4 -isysroot $DEVELOPER_SDK_DIR/MacOSX10.5.sdk";;
			darwin11* | darwin12* )	ISYSROOT="-mmacosx-version-min=10.5";;
			darwin13* )	ISYSROOT="-mmacosx-version-min=10.7";;
			darwin14* )	ISYSROOT="-mmacosx-version-min=10.10";;
            darwin15* )	ISYSROOT="-mmacosx-version-min=10.10";;
            darwin16* )	ISYSROOT="-mmacosx-version-min=10.10";;
            darwin17* )	ISYSROOT="-mmacosx-version-min=10.13";;
			*)
				AC_MSG_ERROR([Sorry, unknown Darwin version: $host_os])
				;;
		esac
		dnl Add the ARCHFLAGS here to let configure check for the target and not host capabilities.
		CPPFLAGS="$CPPFLAGS $ISYSROOT $ARCHFLAGS"
		LDFLAGS="$LDFLAGS $ISYSROOT $ARCHFLAGS"
	fi
	;;
  sparc* ) CPU_TYPE=sparc HAVE_SPARC=true;;
  arm* ) CPU_TYPE=arm HAVE_ARM=true
         CFLAGS="$CFLAGS -ffast-math"
         CXXFLAGS="$CXXFLAGS -ffast-math"          
  	;;
  powerpc* )
	CPU_TYPE=powerpc
	HAVE_POWERPC=true
	if [[ "x$OS_TYPE" = "xdarwin" ]]; then
		ARCHFLAGS="-arch ppc"
		ISYSROOT="-mmacosx-version-min=10.3"
		dnl Add the ARCHFLAGS here to let configure check for the target and not host capabilities.
		CPPFLAGS="$CPPFLAGS $ISYSROOT $ARCHFLAGS"
		LDFLAGS="$LDFLAGS $ISYSROOT $ARCHFLAGS"
	fi
	;;
  x86_64* ) CPU_TYPE=x86_64 HAVE_X86_64=true;;
  *) CPU_TYPE=unknown;;
esac
AM_CONDITIONAL([CPU_TYPE_x86], test "$CPU_TYPE" = "i386")
AM_CONDITIONAL([CPU_TYPE_x86_64], test "$CPU_TYPE" = "x86_64")
AM_CONDITIONAL([CPU_TYPE_sparc], test "$CPU_TYPE" = "sparc")
AM_CONDITIONAL([CPU_TYPE_arm], test "$CPU_TYPE" = "arm")
AM_CONDITIONAL([CPU_TYPE_powerpc], test "$CPU_TYPE" = "powerpc")
AM_CONDITIONAL([CPU_TYPE_unknown], test "$CPU_TYPE" = "unknown")
if test "$CPU_TYPE" = "unknown"; then
  CPU_TYPE=`echo $host_cpu | sed -e 's/-/_/g'`
fi
DEFINES="$DEFINES -DCPU_$CPU_TYPE"
AC_DEFINE_UNQUOTED([CPU_TYPE], "$CPU_TYPE", [Define CPU type])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_OBJC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AX_PROG_CC_FOR_BUILD
AX_PROG_CXX_FOR_BUILD
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_RANLIB
AC_PATH_PROG(PERL, perl)
AC_CHECK_TOOLS(WINDRES, [windres], :)
PKG_PROG_PKG_CONFIG
m4_ifndef([PKG_CHECK_MODULES], [AC_DEFUN([PKG_CHECK_MODULES], [true])])

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS(unistd.h fcntl.h sys/mman.h mach/mach.h sys/ioctl.h)
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_HEADERS(termios.h termio.h alloca.h sys/statfs.h sys/statvfs.h)
AC_CHECK_HEADERS(sys/types.h sys/stat.h sys/vfs.h utime.h sys/param.h)
AC_CHECK_HEADERS(sys/mount.h types.h stat.h ext2fs/ext2_fs.h)
AC_CHECK_HEADERS(sys/socket.h)
AC_CHECK_HEADERS(linux/if.h linux/if_tun.h net/if.h net/if_tun.h, [], [], [
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])
AC_CHECK_HEADERS(net/bpf.h)

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
gl_TM_GMTOFF
gl_STAT_TIME

AC_CXX_BOOL

dnl Checks for libraries
AC_CHECK_LIB(m, sin, [LIBS="$LIBS -lm"])

dnl Checks for library functions.
AC_CHECK_FUNCS(strdup)
AC_CHECK_FUNCS(sigaction signal sigsetjmp)
AC_CHECK_FUNCS(mmap mprotect munmap)
AC_CHECK_FUNCS(vm_allocate vm_deallocate vm_protect sbrk)
AC_CHECK_FUNCS(strchr memcpy bcopy)
AC_CHECK_FUNCS(usleep gettimeofday)
AC_CHECK_FUNCS(fseeko fsync futimes futimens link readlink symlink lstat truncate pathconf)
AC_CHECK_FUNCS(canonicalize_file_name realpath)

AC_CACHE_CHECK([whether sigsetjmp is supported],
  ac_cv_have_sigsetjmp, [
  AC_TRY_LINK([
    #include <signal.h>
    #include <setjmp.h>
  ], [
    sigjmp_buf b;
    sigsetjmp(b, 1);
    siglongjmp(b, 1);
  ],
  ac_cv_have_sigsetjmp=yes, ac_cv_have_sigsetjmp=no
  )
])
if test "$ac_cv_have_sigsetjmp" = "yes"; then
  AC_DEFINE([HAVE_SIGSETJMP], 1)
fi

dnl Darwin seems to define mach_task_self() instead of task_self().
AC_CHECK_FUNCS(mach_task_self task_self)

AC_DEFINE([DIRSEPARATOR], "/", [Default directory separator])

AH_TEMPLATE([ARANYMHOME], [Default ARAnyM directory])
AH_TEMPLATE([HAVE_O_FSYNC], [Define if you want to use FreeBSD's imitation of O_SYNC])
AH_TEMPLATE([ARANYMCONFIG], [Default ARAnyM config file])
AH_TEMPLATE([ARANYMNVRAM], [Default ARAnyM NVRAM file])
AH_TEMPLATE([ARANYMKEYMAP], [Default ARAnyM keymap])
AH_TEMPLATE([HAVE_LIBUDEV], [Define if you have <libudev.h> and libraries])

if test "$OS_TYPE" = "linux"; then
   AC_CHECK_HEADERS([libudev.h])
   if test "$ac_cv_header_libudev_h" = yes; then
      AC_CHECK_LIB(udev, udev_monitor_new_from_netlink, [LIBS="$LIBS -ludev"])
      if test $ac_cv_lib_udev_udev_monitor_new_from_netlink = yes; then
        AC_DEFINE([HAVE_LIBUDEV])
      fi
   fi
fi

case "$host_os" in
  linux*)
    ;;
  cygwin*|mingw*)
    AC_DEFINE([ARANYMHOME], "aranym")
    ;;
  solaris*)
    AC_DEFINE([_XOPEN_SOURCE], 500, [Define if needed])
    AC_DEFINE([_POSIX_C_SOURCE], 199506L, [Define if needed])
    DEFINES="$DEFINES -DBSD_COMP -D__EXTENSIONS__"
    ;;
  freebsd*|kfreebsd*-gnu)
    AC_DEFINE([HAVE_O_FSYNC], 1)
    ;;
  darwin*)
	AC_DEFINE([HAVE_O_FSYNC], 1)
	if test "x$WITH_FINK" = "xyes"; then
		AC_DEFINE([ARANYMHOME], "aranym")
	else
		AC_DEFINE([ARANYMHOME], "ARAnyM")
		
		if test "x$WITH_CLASSIC_CONFIG" = "xyes"; then
			echo "Using classic config"
			AC_DEFINE([ARANYMCONFIG], "config")
			AC_DEFINE([ARANYMNVRAM], "nvram")
			AC_DEFINE([ARANYMKEYMAP], "keymap")
		else	
			echo "Using preference files"
			AC_DEFINE([ARANYMCONFIG], "org.aranym.amcfg")
			AC_DEFINE([ARANYMNVRAM], "org.aranym.nvram")
			AC_DEFINE([ARANYMKEYMAP], "org.aranym.keymap")
		fi
		
		BUNDLE_FRAMEWORKS= dnl Empty list of frameworks to bundle

		dnl Check for some important frameworks
		ARANYM_CHECK_FRAMEWORK(AppKit, [])
		ARANYM_CHECK_FRAMEWORK(CoreFoundation, [#include <CoreFoundation/CoreFoundation.h>])
		ARANYM_CHECK_FRAMEWORK(IOKit, [#include <IOKit/IOKitLib.h>])
		ARANYM_CHECK_FRAMEWORK(Security, [#include <Security/Security.h>])
		        
        dnl We (Mac OS X target) don't want to use PKG_CONFIG even if it exists (e.g. in /opt)
        PKG_CONFIG=""
        
	fi
	;;
  beos*|haiku*)
    dnl Find tools needed to add resources
    AC_PATH_PROG(BEOS_XRES, xres, [AC_MSG_ERROR([xres not found.])])
    AC_PATH_PROG(BEOS_RC, rc, [AC_MSG_ERROR([rc not found.])])
    AC_PATH_PROG(BEOS_SETVERSION, setversion, [AC_MSG_ERROR([setversion not found.])])
    WANT_UNIXSERIALPORT=no
    ac_cv_tun_tap_support=no
    ;;
  *)
    ;;
esac

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

dnl Check that the host supports BPF devices
AC_CACHE_CHECK([whether BPF is supported],
  ac_cv_bpf_support, [
  AC_TRY_COMPILE([
    #if defined(HAVE_NET_BPF_H)
    #include <net/bpf.h>
    #endif
  ], [
    struct bpf_program filter;
  ],
  ac_cv_bpf_support=yes, ac_cv_bpf_support=no
  )
])

  
dnl Various checks if the system supports vm_allocate() and the like functions.
have_mach_vm=no
if [[ "x$ac_cv_func_vm_allocate" = "xyes" -a "x$ac_cv_func_vm_deallocate" = "xyes" -a \
      "x$ac_cv_func_vm_protect" = "xyes" ]]; then
  have_mach_vm=yes
fi
AC_TRANSLATE_DEFINE(HAVE_MACH_VM, $have_mach_vm,
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
AC_TRANSLATE_DEFINE(HAVE_MACH_VM, $have_mach_vm,
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
AC_TRANSLATE_DEFINE(HAVE_MMAP_VM, $have_mmap_vm,
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
AC_TRANSLATE_DEFINE(HAVE_MMAP_ANON, $ac_cv_mmap_anon,
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
AC_TRANSLATE_DEFINE(HAVE_MMAP_ANONYMOUS, $ac_cv_mmap_anonymous,
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

AH_TEMPLATE(HAVE_WIN32_VM, [Define if your system has a working VirtualAlloc()-based memory allocator.])
if test "$OS_TYPE" = mingw; then
	AC_DEFINE([HAVE_WIN32_VM])
        if $HAVE_I386; then
		LDFLAGS="$LDFLAGS -Wl,--large-address-aware"
	fi
fi

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
AC_TRANSLATE_DEFINE(SIGNAL_NEED_REINSTALL, $ac_cv_signal_need_reinstall,
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
AC_TRANSLATE_DEFINE(SIGACTION_NEED_REINSTALL, $ac_cv_sigaction_need_reinstall,
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
AC_TRANSLATE_DEFINE(HAVE_VASPRINTF, $ac_cv_have_vasprintf,
  [Define if your system supports vasprintf function with GNU SOURCEs])
if [[ "x$ac_cv_have_vasprintf" = "xyes" ]]; then
  DEFINES="$DEFINES -D_GNU_SOURCE"
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
AC_TRANSLATE_DEFINE(HAVE_SIGHANDLER_T, $ac_cv_have_sighandler_t,
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
AC_TRANSLATE_DEFINE(HAVE_GETOPT_H, $ac_cv_have_getopt_long)

AM_CONDITIONAL([NEED_GETOPT], test "$ac_cv_have_getopt_long" = "no")


dnl Check if we have __builtin_bswap16/32
AC_CACHE_CHECK([for __builtin_bswap16],
  ac_cv_have___builtin_bswap16, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
    #include <stdarg.h>
    #include <stdio.h>
    unsigned short v;
  ], [
    v = __builtin_bswap16(v);
  ],
  [ac_cv_have___builtin_bswap16=yes],
  [ac_cv_have___builtin_bswap16=no]
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(HAVE___BUILTIN_BSWAP16, $ac_cv_have___builtin_bswap16,
  [Define if your compiler supports __builtin_bswap16])
AC_CACHE_CHECK([for __builtin_bswap32],
  ac_cv_have___builtin_bswap32, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
    #include <stdarg.h>
    #include <stdio.h>
    unsigned int v;
  ], [
    v = __builtin_bswap32(v);
  ],
  [ac_cv_have___builtin_bswap32=yes],
  [ac_cv_have___builtin_bswap32=no]
  )
  AC_LANG_RESTORE
  ]
)
AC_TRANSLATE_DEFINE(HAVE___BUILTIN_BSWAP32, $ac_cv_have___builtin_bswap32,
  [Define if your compiler supports __builtin_bswap32])


AH_TEMPLATE([MAP_META_TO_CONTROL], [Define if you want to map the meta key to the emulated control key])
AH_TEMPLATE([MAP_CONTROL_TO_CONTROL], [Define if you want to map the control key to the emulated control key])
case $CONTROL_KEY in
	meta)
		AC_DEFINE([MAP_META_TO_CONTROL], 1)
		;;
	both)
		AC_DEFINE([MAP_META_TO_CONTROL], 1)
		AC_DEFINE([MAP_CONTROL_TO_CONTROL], 1)
		;;
	ctrl)
		AC_DEFINE([MAP_CONTROL_TO_CONTROL], 1)
		;;
esac

if [[ "x$WANT_JIT" = "xyes" ]]; then
  WANT_EXSIGSEGV=yes
  MEMORY_CHECK=sseg
fi

AH_TEMPLATE([ARAM_PAGE_CHECK], [Define if you want to try out a different memory check])
AH_TEMPLATE([NOCHECKBOUNDARY], [Define if you don't want to use check memory boundary])
AH_TEMPLATE([EXTENDED_SIGSEGV], [Define if you want to use extended segfault handler])
AH_TEMPLATE([RAMENDNEEDED], [Define if you want protect end of memory])
AH_TEMPLATE([HW_SIGSEGV], [Define if you want to use extended segfault handler also for HW space])
case $MEMORY_CHECK in
  no)   AC_DEFINE([NOCHECKBOUNDARY], 1)
        ;;
  page) AC_DEFINE([ARAM_PAGE_CHECK], 1)
        ;;
  hwsp) ADDRESSING_TEST_ORDER="fixed"
        WANT_EXSIGSEGV="yes"
        AC_DEFINE([NOCHECKBOUNDARY], 1)
	AC_DEFINE([EXTENDED_SIGSEGV], 1)
        AC_DEFINE([RAMENDNEEDED])
        ;;
  pagehwsp) ADDRESSING_TEST_ORDER="fixed"
        WANT_EXSIGSEGV="yes"
        AC_DEFINE([ARAM_PAGE_CHECK], 1)
        AC_DEFINE([NOCHECKBOUNDARY], 1)
	AC_DEFINE([EXTENDED_SIGSEGV], 1)
        AC_DEFINE([RAMENDNEEDED], 1)
        ;;
   sseg) ADDRESSING_TEST_ORDER="fixed"
        WANT_EXSIGSEGV="yes"
        AC_DEFINE([NOCHECKBOUNDARY], 1)
	AC_DEFINE([RAMENDNEEDED], 1)
        AC_DEFINE([EXTENDED_SIGSEGV], 1)
	AC_DEFINE([HW_SIGSEGV], 1)
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

dnl Full MMU
if [[ "x$WANT_MMU" = "xyes" ]]; then
    AC_DEFINE([FULLMMU], 1, [Define if using full MMU])
fi
AM_CONDITIONAL([FULLMMU], test "$WANT_MMU" = "yes")

dnl Small ATC
if [[ "x$ATC_TYPE" = "xsmall" ]]; then
    AC_DEFINE([SMALL_ATC], 1, [Define if using only small ATC])
fi

dnl Full ATC
if [[ "x$ATC_TYPE" = "xno" ]]; then
    AC_DEFINE([DISABLE_ATC], 1, [Define if not using full ATC])
fi

dnl DSP
if [[ "x$WANT_DSP" = "xyes" ]]; then
    AC_DEFINE([DSP_EMULATION], 1, [Define if using DSP])
    dnl DSP disassembler
    if [[ "x$WANT_DSP_DISASM" = "xyes" ]]; then
        AC_DEFINE([DSP_DISASM], 1, [Define if DSP disassembly])
    fi
fi
AM_CONDITIONAL([WANT_DSP], test "$WANT_DSP" = "yes")
AM_CONDITIONAL([WANT_DSP_DISASM], test "$WANT_DSP_DISASM" = "yes")

AH_TEMPLATE([FIXED_VIDEORAM], [Define if you want fixed position of VideoRAM])
AH_TEMPLATE([PROTECT2K], [Define if you want to protect first 2kB of RAM against user mode access])

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
  AC_DEFINE([FIXED_VIDEORAM], 1)
else
  if [[ "x$WANT_MMU" = "xyes" ]]; then
    AC_MSG_WARN([Protection of first 2kB RAM enabled as well - won't hurt])
    AC_DEFINE([PROTECT2K], 1)
  fi
  if [[ "x$WANT_BLITSDLBLIT" = "xyes" ]]; then
    AC_MSG_WARN([Blitter SDL surface blitting support needs fixed position of VideoRAM])
    AC_DEFINE([FIXED_VIDEORAM], 1)
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
    AC_DEFINE([FIXED_VIDEORAM], 1)
    WANT_FIXEDVIDEORAM="yes"
  fi
dnl Blitter SDL surface blitting
if [[ "x$WANT_BLITSDLBLIT" = "xyes" ]]; then
  AC_DEFINE([BLITTER_SDLBLIT], 1, [Define if you want even more accelerated blitter output])
  AC_DEFINE([BLITTER_MEMMOVE], 1, [Define if you want accelerated blitter output])	dnl required for the SDLBLIT
fi

AH_TEMPLATE([ETHERNET_SUPPORT], [Define if you want to use ethernet card emulation])
dnl Ethernet card emulation
HAVE_ETHERNET=no
NEED_ARATAPIF=no
if [[ "x$WANT_ETHERNET" = "xyes" ]]; then
  if [[ "x$ac_cv_tun_tap_support" = "xyes" ]]; then
    AC_DEFINE([ETHERNET_SUPPORT], 1)
    NEED_ARATAPIF="yes"
    HAVE_ETHERNET="yes"
  elif test "x$OS_TYPE" = "xcygwin" && ( $HAVE_I386 || $HAVE_X86_64 ); then
    AC_DEFINE([ETHERNET_SUPPORT], 1)
    HAVE_ETHERNET="yes"
  elif [[ "x$OS_TYPE" = "xdarwin" ]]; then
    AC_DEFINE([ETHERNET_SUPPORT], 1)
    if [[ "x$WANT_BFP_ETHERNET" = "xyes" ]]; then
      AC_TRANSLATE_DEFINE(ENABLE_BPF, $ac_cv_bpf_support,
        [Define if your system supports BPF devices.])
    else
      AC_MSG_WARN([Ethernet won't work without TunTap driver for Mac OS X. See http://tuntaposx.sourceforge.net/])
    fi
    HAVE_ETHERNET="yes"
  else
    AC_MSG_WARN([Ethernet interface not supported on your system])
  fi
fi
AM_CONDITIONAL([NEED_ARATAPIF], test "$NEED_ARATAPIF" = "yes")
AM_CONDITIONAL([ETHERNET_SUPPORT], test "$HAVE_ETHERNET" = "yes")


if [[ "x$HAVE_ETHERNET" = "xyes" ]]; then
  PROVIDES_NF="$PROVIDES_NF ethernet"
fi

dnl Support for direct access to host's fs
if [[ "x$WANT_HOSTFS" = "xyes" ]]; then
  AC_DEFINE([HOSTFS_SUPPORT], 1, [Define if you want to use direct access to host's fs])
  PROVIDES_NF="$PROVIDES_NF hostfs"
fi
AM_CONDITIONAL([HOSTFS_SUPPORT], test "$WANT_HOSTFS" = "yes")


dnl SDL GUI
if test "x$WANT_SDLGUI" = "xyes" ; then
  AC_DEFINE([SDL_GUI], 1, [Define if you want to build SDL GUI])
fi
AM_CONDITIONAL([SDL_GUI], test "$WANT_SDLGUI" = yes)


dnl C++ exceptions
if [[ "x$WANT_EXSIGSEGV" = "xyes" ]]; then
  WANT_CXX_EXCEPTIONS="no"
fi
if [[ "x$WANT_CXX_EXCEPTIONS" = "xno" ]]; then
  AC_DEFINE(EXCEPTIONS_VIA_LONGJMP, 1, [Define if you want to use setjmp/longjmp instead of C++ exceptions])
fi

AH_TEMPLATE(NO_NESTED_SIGSEGV, [Define if OS needs to cleanly exit sigsegv handler])


dnl Extended signal segmentation fault handler
dnl if you add support for os/cpu combinations here,
dnl don't forget to update the list of source files
dnl in src/Makefile.am, inside "if EXTENDED_SIGSEGV"
if [[ "x$WANT_EXSIGSEGV" = "xyes" ]]; then
  case "$host_os" in
    linux*)
      case "$CPU_TYPE" in
        i386)
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)
	  ;;
	x86_64)
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)
          ;;
	arm)
          CFLAGS="$CFLAGS -marm"
          CXXFLAGS="$CXXFLAGS -marm"          
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)
          ;;
        *)
          AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
          ;;
      esac
      ;;
    openbsd*)
      case "$CPU_TYPE" in
        i386)
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)
	  ;;
	x86_64)
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)
          ;;
        *)
          AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
          ;;
      esac
      ;;
    darwin*)
      if [[ "x$CPU_TYPE" = "powerpc" ]]; then
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      else
        AC_DEFINE(NO_NESTED_SIGSEGV, 1)
      fi
      ;;
    netbsd*)
      if $HAVE_I386 || $HAVE_X86_64; then
        :
      else
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      fi
      ;;
    freebsd*|kfreebsd*-gnu)
      case "$CPU_TYPE" in
        i386)
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)
	  ;;
	x86_64)
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)

	  dnl FreeBSD is using clang which requires additional options, may be to be set in
	  dnl general for clang in the future
  	  case "$host_os" in
          freebsd*)
            CFLAGS="$CFLAGS -mstackrealign"
	    CXXFLAGS="$CXXFLAGS -mstackrealign"
            ;;
          *) :
            ;;
          esac
          ;;
        *)
          AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
          ;;
      esac
      ;;
    cygwin*|mingw*)
      if $HAVE_I386 || $HAVE_X86_64; then
        AC_DEFINE(NO_NESTED_SIGSEGV, 1)
      else
        AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      fi
      ;;
    solaris*)
      case "$CPU_TYPE" in
        i386)
	  ;;
	x86_64)
	  ;;
        *)
          AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
          ;;
      esac
      ;;
    beos* | haiku*)
      case "$CPU_TYPE" in
        i386)
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)
	      ;;
	x86_64)
          AC_DEFINE(NO_NESTED_SIGSEGV, 1)
          ;;
        *)
          AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
          ;;
       esac
       dnl disabled for now: compiles, but panics kernel :(
       AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
       ;;
    *)
      AC_MSG_ERROR([Sorry, extended segfault handler not supported on your platform])
      ;;
  esac
fi
AM_CONDITIONAL([EXTENDED_SIGSEGV], test "$WANT_EXSIGSEGV" = yes)

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
    if test "$OS_TYPE" = "linux" && ( $HAVE_I386 || $HAVE_X86_64 || $HAVE_ARM ); then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if test "$OS_TYPE" = "freebsd" && ( $HAVE_I386 || $HAVE_X86_64 ); then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if test "$OS_TYPE" = "openbsd" && ( $HAVE_I386 || $HAVE_X86_64 ); then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if test "$OS_TYPE" = "darwin" && ( $HAVE_I386 || $HAVE_X86_64 ); then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if test "$OS_TYPE" = "cygwin" && ( $HAVE_I386 || $HAVE_X86_64 ); then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if test "$OS_TYPE" = "mingw" && ( $HAVE_I386 || $HAVE_X86_64 ); then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if test "$OS_TYPE" = "solaris" && ( $HAVE_I386 || $HAVE_X86_64 ); then
      ADDRESSING_MODE="fixed (0x51000000)"
      DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
      break
    fi
    if test "$OS_TYPE" = "beos" && ( $HAVE_I386 || $HAVE_X86_64 ); then
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

dnl (gb) Do not merge constants since it breaks fpu/fpu_x86.cpp.
dnl As of 2001/08/02, this affects the following compilers:
dnl Official: probably gcc-3.1 (mainline CVS)
dnl Mandrake: gcc-2.96 >= 0.59mdk, gcc-3.0.1 >= 0.1mdk
dnl Red Hat : gcc-2.96 >= 89, gcc-3.0 >= 1
if [[ "x$GCC" = "xyes" -a "x$FPE_CORE" = "xx86" ]]; then
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

dnl Select appropriate CPU source and REGPARAM define.
ASM_OPTIMIZATIONS=none

dnl gb-- JITSRCS and GENJITSRCS will be emptied later if the JIT is not available
dnl gb-- Other platforms should define their own set of noflags file variants
CAN_JIT=no
JITSRCS=
GENJITSRCS="compemu1.cpp compemu2.cpp compemu3.cpp compemu4.cpp compemu5.cpp compemu6.cpp compemu7.cpp compemu8.cpp"
GENCOMP_ARCH=

AH_TEMPLATE([HAVE_CFLAG_NO_REDZONE], [define if your compiler understand -mno-red-zone])

if test "$GCC" = yes && $HAVE_I386; then
  dnl i386 CPU
  DEFINES="$DEFINES -DUNALIGNED_PROFITABLE -DREGPARAM=\"__attribute__((regparm(3)))\""
  if [[ "x$HAVE_GAS" = "xyes" ]]; then
    ASM_OPTIMIZATIONS=i386
    DEFINES="$DEFINES -DX86_ASSEMBLY -DOPTIMIZED_FLAGS -DSAHF_SETO_PROFITABLE"
    CAN_JIT=yes
    GENCOMP_ARCH=x86
  fi
elif test "$GCC" = yes && $HAVE_X86_64; then
  dnl x86-64 CPU
  DEFINES="$DEFINES -DUNALIGNED_PROFITABLE"
  if [[ "x$HAVE_GAS" = "xyes" ]]; then
    ASM_OPTIMIZATIONS="x86-64"
    DEFINES="$DEFINES -DX86_64_ASSEMBLY -DOPTIMIZED_FLAGS"
    CAN_JIT=yes
    GENCOMP_ARCH=x86
	SAVED_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS -mno-red-zone"
	  AC_CACHE_CHECK([whether GCC supports -mno-red-zone], ac_cv_gcc_no_red_zone, [
	    AC_LANG_SAVE
	    AC_LANG_CPLUSPLUS
	    AC_TRY_COMPILE([],[],[ac_cv_gcc_no_red_zone=yes],[ac_cv_gcc_no_red_zone=no])
	    AC_LANG_RESTORE
	  ])
	  CXXFLAGS="$SAVED_CXXFLAGS"
	  if [[ "x$ac_cv_gcc_no_red_zone" = "xyes" ]]; then
	     AC_DEFINE([HAVE_CFLAG_NO_REDZONE], 1)
	  fi
  fi
elif test "$GCC" = yes && $HAVE_SPARC; then
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
elif test "$GCC" = yes && $HAVE_ARM; then
  dnl ARM CPU
  if [[ "x$HAVE_GAS" = "xyes" ]]; then
    case "$host_cpu" in
	armv6*|armv7*|armv8*|armv9*)
           ASM_OPTIMIZATIONS="ARM/V6 architecture w optimized flags"
	   DEFINES="$DEFINES -DARMV6_ASSEMBLY -DARM_ASSEMBLY -DOPTIMIZED_FLAGS"
           CFLAGS="$CFLAGS -march=armv6"
           CXXFLAGS="$CXXFLAGS -march=armv6"
	   ;;
	*)
	   dnl Hack until we find a better way
           case "$host_os" in
                *-androideabi)
                        ASM_OPTIMIZATIONS="ARM/V6 architecture w optimized flags"
                        DEFINES="$DEFINES -DARMV6_ASSEMBLY -DARM_ASSEMBLY -DOPTIMIZED_FLAGS"
                        ;;
                *)
                        ASM_OPTIMIZATIONS="ARM/generic w optimized flags"
                        DEFINES="$DEFINES -DARM_ASSEMBLY -DOPTIMIZED_FLAGS"
                        ;;
           esac
	   ;;
    esac
    GENCOMP_ARCH=arm
    CAN_JIT=yes
  fi
fi

dnl Check for JIT Version 2
if [[ "x$WANT_JIT2" = "xyes" ]]; then
  if $HAVE_ARM; then
    DEFINES="$DEFINES -DUSE_JIT2=1"
  else
    AC_MSG_ERROR([JIT Version 2 is only available for ARM])
  fi
fi

AM_CONDITIONAL([GENCOMP_ARCH_X86], test "$GENCOMP_ARCH" = "x86")
AM_CONDITIONAL([GENCOMP_ARCH_ARM], test "$GENCOMP_ARCH" = "arm")


dnl JIT sources if available
if [[ "x$WANT_JIT" = "xyes" -a "x$CAN_JIT" = "xyes" ]]; then
  if [[ "x$WANT_MMU" = "xyes" ]]; then
      AC_MSG_ERROR([MMU&JIT in combination are not supported])
  fi
  JITSRCS="$JITSRCS uae_cpu/compiler/compemu_support.cpp"
  DEFINES="$DEFINES -DUSE_JIT=1"

  dnl JIT is buggy with strict-aliasing, it seems (FIXME!)
  CXXFLAGS="$CXXFLAGS -fno-strict-aliasing"

  if [[ "x$WANT_JIT_DEBUG" = "xyes" ]]; then
    DEFINES="$DEFINES -DJIT_DEBUG=1"
  fi

  if [[ "x$WANT_JIT_FPU" = "xyes" ]]; then
    DEFINES="$DEFINES -DUSE_JIT_FPU=1"
  fi
  if test "$OS_TYPE" != darwin && $HAVE_X86_64; then
     # Force text address; needed at least on ArchLinux
     # where it is otherwise set to a high memory address
     # which is not in the range for the JIT addressing scheme
     LDFLAGS="$LDFLAGS -Wl,-Ttext-segment=0x10000000"
  fi
else
  WANT_JIT=no
  WANT_JIT_FPU=no
  WANT_JIT_DEBUG=no
  JITSRCS=""
  GENJITSRCS=""
fi
AM_CONDITIONAL([USE_JIT], test "$WANT_JIT" = "yes")
AM_CONDITIONAL([USE_JIT_FPU], test "$WANT_JIT_FPU" = "yes")
AM_CONDITIONAL([USE_JIT2], test "$WANT_JIT2" = "yes")


dnl Select appropriate FPU source.
gcc_AC_C_FLOAT_FORMAT
AC_CHECK_HEADERS(ieee754.h ieeefp.h floatingpoint.h nan.h math.h)

if [[ "x$WANT_JIT_FPU" = "xyes" -a "x$FPE_CORE_TEST_ORDER" != "xieee" ]]; then
dnl  AC_MSG_WARN([Sorry, JIT-FPU supports only the "ieee" FPE implementation])
  FPE_CORE_TEST_ORDER="ieee"
  dnl Restore previous variables. FPE_CORE_STR and FPUSRCS are overwritten
fi

AH_TEMPLATE([USES_FPU_CORE], [FPU core])

for fpe in $FPE_CORE_TEST_ORDER; do
  case $fpe in
  ieee)
    case $ac_cv_c_float_format in
    IEEE*)
      FPE_CORE="IEEE fpu core"
      AC_DEFINE([USES_FPU_CORE], "IEEE")
      DEFINES="$DEFINES -DFPU_IEEE"
      dnl Math functions not mandated by C99 standard
      AC_CHECK_FUNCS(isnanl isinfl)
      dnl Math functions required by C99 standard, but probably not
      dnl implemented everywhere. In that case, we fall back to the
      dnl regular variant for doubles.
      AC_CHECK_FUNCS(logl log10l expl powl exp2l pow2l exp10l pow10l fabsl sqrtl log1pl expm1l exp2 pow2 pow10)
      AC_CHECK_FUNCS(sinl cosl tanl sinhl coshl tanhl)
      AC_CHECK_FUNCS(asinl acosl atanl asinhl acoshl atanhl)
      AC_CHECK_FUNCS(floorl ceill truncl roundl)
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
    if test ":$GCC:$HAVE_I386:$HAVE_GAS:" = ":yes:true:yes:" -o ":$GCC:$HAVE_X86_64:$HAVE_GAS:" = ":yes:true:yes:"; then
      FPE_CORE="i387 fpu core"
      DEFINES="$DEFINES -DFPU_X86"
      break
    fi
    ;;
  uae)
    FPE_CORE="uae fpu core"
    AC_DEFINE([USES_FPU_CORE], "UAE")
    DEFINES="$DEFINES -DFPU_UAE"
    break
    ;;
  mpfr)
    AC_CHECK_LIB(mpfr, mpfr_set_zero, [LIBS="$LIBS -lmpfr -lgmp"], [], [-lgmp])
    if test $ac_cv_lib_mpfr_mpfr_set_zero = yes; then
      FPE_CORE="mpfr fpu core"
      AC_DEFINE([USES_FPU_CORE], "MPFR")
      DEFINES="$DEFINES -DFPU_MPFR"
      break
    fi
    ;;
  *)
    AC_MSG_ERROR([Internal configure.in script error for $fpe fpu core])
    ;;
  esac
done
if [[ "x$FPE_CORE" = "x" ]]; then
  AC_MSG_ERROR([Sorry, no suitable FPU core found in $FPE_CORE_TEST_ORDER])
fi
AM_CONDITIONAL([FPU_IEEE], test "$fpe" = "ieee")
AM_CONDITIONAL([FPU_X86], test "$fpe" = "x86")
AM_CONDITIONAL([FPU_UAE], test "$fpe" = "uae")
AM_CONDITIONAL([FPU_MPFR], test "$fpe" = "mpfr")


dnl Check for certain math functions
AC_CHECK_FUNCS(atanh)
AC_CHECK_FUNCS(isnan isinf finite isnormal signbit)

AH_TEMPLATE([DEBUGGER], [Define if using some internal debugger])
dnl Select debugger
if [[ "x$WANT_JIT" = "xno" ]]; then
  dnl new debugger
  if [[ "$USE_DEBUGGER" = "yes" ]]; then
    AC_DEFINE([DEBUGGER], 1)
  fi
else
  if [[ "$USE_DEBUGGER" = "yes" ]]; then
    AC_MSG_WARN([interactive debugger not usable with JIT])
    USE_DEBUGGER=no
  fi
fi
AM_CONDITIONAL([USE_DEBUGGER], test "$USE_DEBUGGER" = yes)

AH_TEMPLATE([DISASM_USE_OPCODES],[Define if using the opcodes library for disassembly])
AH_TEMPLATE([DISASM_USE_BUILTIN],[Define if using the builtin disassembler])

AC_ARG_WITH(opcodes,
[AC_HELP_STRING(--with-opcodes=DIR, [specify the directory where the opcodes library is located])],
with_opcodes=$withval, with_opcodes=)

AC_ARG_ENABLE(disasm,
[AC_HELP_STRING(--enable-disasm=builtin|opcodes, [Specify which library to use for disassembly @<:@none@:>@])],
enable_disasm=$enableval,
enable_disasm=no; enable_disasm_defaulted=yes)
case "$enable_disasm" in
no|builtin|opcodes)
	;;
yes|auto)
	enable_disasm=auto
	;;
*)
	AC_MSG_ERROR([unknown disassembler $enable_disasm])
	;;
esac

# check wether we need disassembly support, and which one
DISASM_CFLAGS=
DISASM_LIBS=
if test "$USE_DEBUGGER" != no -o "$WANT_HISTORY" = yes; then
   if test "$enable_disasm" = no; then
     if test "$enable_disasm_defaulted" = "yes"; then
        if test "$with_opcodes" != ""; then
          enable_disasm=opcodes
        else
          enable_disasm=auto
        fi
     else
        AC_MSG_WARN([you have requested debug support but disabled disassembly.
The debugger needs some disassembling support.])
     fi
   fi
fi
if test "$enable_disasm" = auto -o "$enable_disasm" = opcodes; then
   CPPFLAGS_save="$CPPFLAGS"
   LIBS_save="$LIBS"
   if test "$with_opcodes" != ""; then
      if test -f "$with_opcodes/build-dir/bfd/bfd.h" ; then
        OPCODES_CFLAGS="-I$with_opcodes/build-dir/bfd -I$with_opcodes/build-dir/libiberty -I$with_opcodes/build-dir/opcodes"
        OPCODES_LIBS="-L$with_opcodes/build-dir/bfd -L$with_opcodes/build-dir/libiberty -L$with_opcodes/build-dir/opcodes"
      else
        OPCODES_CFLAGS="-I$with_opcodes/include"
        OPCODES_LIBS="-L$with_opcodes/lib"
      fi
      CPPFLAGS="$CPPFLAGS $OPCODES_CFLAGS"
   fi
   OPCODES_LIBS="$OPCODES_LIBS -lopcodes -lbfd -lz -ldl"
   found=no
   AC_CHECK_HEADERS(bfd.h,found=yes,found=no)
   AC_CHECK_HEADERS(dis-asm.h,found=yes,found=no)
   CPPFLAGS="$CPPFLAGS_save"
   AC_CHECK_LIB([opcodes], print_insn_m68k, found=yes,
   	   [unset ac_cv_lib_opcodes_print_insn_m68k
            OPCODES_LIBS="$OPCODES_LIBS -liberty"
	    AC_CHECK_LIB([opcodes], print_insn_m68k, found=yes,
		found=no,
		[$OPCODES_LIBS])],
	[$OPCODES_LIBS])
   LIBS="$LIBS_save"
   AC_MSG_CHECKING([for opcodes usability])
   if test "$found" = "yes"; then
      AC_DEFINE(DISASM_USE_OPCODES)
      DISASM_CFLAGS="$OPCODES_CFLAGS"
      DISASM_LIBS="$OPCODES_LIBS"
      AC_MSG_RESULT($found)
      if test "$with_opcodes" != ""; then
        DISASM_LIBS="$DISASM_LIBS -Wl,-rpath,$with_opcodes/lib"
      fi
      enable_disasm=opcodes
   else
      AC_MSG_RESULT($found)
      if test "$enable_disasm" = "opcodes"; then
        AC_MSG_NOTICE([

Required libraries not found.
You may have to specify the path to the correct directory.
If your system isn't m68k-based, the libraries may be installed
in some unusual place (maybe something like
/usr/$host/m68k-unknown-linux-gnu).
In the meantime, i will use the builtin support.
])
      fi
      enable_disasm=auto
   fi
fi
if test "$enable_disasm" = auto -o "$enable_disasm" = builtin; then
   AC_DEFINE(DISASM_USE_BUILTIN)
   enable_disasm=builtin
fi
AC_SUBST(DISASM_CFLAGS)
AC_SUBST(DISASM_LIBS)
AM_CONDITIONAL(DISASM_USE_BUILTIN, test "$enable_disasm" = "builtin")
AM_CONDITIONAL(DISASM_USE_OPCODES, test "$enable_disasm" = "opcodes")
 

dnl Exclusive spcflags handling
if [[ "x$WANT_SPCFLAGS_EXCL" = "xyes" ]]; then
  DEFINES="$DEFINES -DENABLE_EXCLUSIVE_SPCFLAGS"
fi

dnl UAE CPU sources for all architectures.
CPUSRCS="$CPUSRCS $JITSRCS"
GENCPUSRCS="$GENCPUSRCS cpudefs.cpp cpustbl.cpp cpufunctbl.cpp $GENJITSRCS"

dnl Remove the "-g" option if set for GCC.
if [[ "x$GCC" = "xyes" ]]; then
  if test "$ac_test_CFLAGS" != set; then
    CFLAGS=`echo $CFLAGS | sed -e 's/-g\b//g'`
  fi
  if test "$ac_test_CXXFLAGS" != set; then
    CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-g\b//g'`
  fi
fi
if [[ "x$WANT_NATDEBUG" = "xyes" ]]; then
  DBGSP="-g"
  WFLAGS=""
  if [[ "x$GCC" = "xyes" ]]; then
    WFLAGS="$WFLAGS -W -Wall -Wundef"
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
dnl Do this only if it was defaulted by configure
if [[ "x$GCC" = "xyes" ]]; then
  if test "$ac_test_CFLAGS" != set; then
    CFLAGS=`echo $CFLAGS | sed -e 's/-O2//g'`
    CFLAGS="-O3 $CFLAGS"
  fi
  if test "$ac_test_CXXFLAGS" != set; then
    CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-O2//g'`
    CXXFLAGS="-O3 $CXXFLAGS"
  fi
fi

dnl Or if we have -Ofast
if [[ "x$HAVE_OFAST" = "xyes" ]]; then
  CFLAGS="`echo $CFLAGS | sed -e 's/-g//g'` -Ofast"
  CXXFLAGS="`echo $CXXFLAGS | sed -e 's/-g//g'` -Ofast"
  CXXFLAGS="-LANG:std $CXXFLAGS"
  LDFLAGS="$LDFLAGS -ipa"
fi

dnl Check for SDL and/or SDL2
m4_define(use_sdl2_default, yes)
m4_case(use_sdl2_default, yes,
[AC_ARG_ENABLE(sdl2,    [AC_HELP_STRING([--disable-sdl2], [disable use of SDL2 even if available (default=no)])], , [enable_sdl2=auto])],
[AC_ARG_ENABLE(sdl2,    [AC_HELP_STRING([--enable-sdl2], [use SDL2 if available (default=no)])], , [enable_sdl2=no])])

SDL_CFLAGS=
SDL_LIBS=
no_sdl=yes
no_sdl2=yes
if test "$OS_TYPE" == darwin -a "$WITH_FINK" = no; then
	ARANYM_CHECK_FRAMEWORK(SDL, [])
	if test "$have_framework_SDL" = yes ; then
		ARANYM_CHECK_FRAMEWORK_LOCATION(SDL)
        	sdl_version="`defaults read $SDL_LOCATION/Resources/Info.plist CFBundleShortVersionString`"
		# SDL 1.2.15 has still 1.2.14 in the Info.plist file
		if test "$sdl_version" = "1.2.14"; then
			if test `defaults read $SDL_LOCATION/Resources/Info.plist BuildMachineOSBuild` = "12F2518"; then
				sdl_version=1.2.15
			fi
		fi
        	no_sdl=no
	fi
    if test "$enable_sdl2" != no; then
		ARANYM_CHECK_FRAMEWORK(SDL2, [])
		if test "$have_framework_SDL2" = yes ; then
			ARANYM_CHECK_FRAMEWORK_LOCATION(SDL2)
            sdl2_version="`defaults read $SDL2_LOCATION/Resources/Info.plist CFBundleShortVersionString`"
            no_sdl2=no
		fi
	fi
else
	dnl Try macro from sdl.m4 and sdl2.m4
	AM_PATH_SDL(1.2.12)
	if test "$enable_sdl2" != no; then
		AM_PATH_SDL2(2.0.3)
	fi
fi
if test "$no_sdl" = yes -a "$no_sdl2" = yes; then
	AC_MSG_ERROR([*** neither SDL nor SDL2 found! ***])
else
	AS_CASE("$host",
		[arm*-*-linux*],[on_rpi=`awk -v r=0 '/^Hardware@<:@ \t@:>@+:@<:@ \t@:>@+BCM2708/ { ++r;} /^Serial@<:@ \t@:>@+:/ { ++r; } END { print ((r > 1) ? "yes" : "no");}' /proc/cpuinfo`],
		[on_rpi="no"])
	
	if test "$on_rpi" = yes; then
		AC_DEFINE(USE_FIXED_CONSOLE_FBVIDEOMODE, 1, [Define if console framebuffer does not support changing host video mode])
	fi
fi
if test "$no_sdl2" = yes -a "$enable_sdl2" = yes; then
	AC_MSG_ERROR([*** SDL2 was requested, but was not found ***])
fi
if test "$no_sdl2" != yes -a "$enable_sdl2" != no; then
	enable_sdl2=yes
fi
if test "$have_framework_SDL" = yes -o "$have_framework_SDL2" = yes; then
	if test "$enable_sdl2" = yes; then
		SDL_CFLAGS="-I$SDL2_LOCATION/Headers"
		BUNDLE_FRAMEWORKS="$BUNDLE_FRAMEWORKS $SDL2_LOCATION"
		SDL_LIBS="$SDL2_LIBS"
		sdl_version="$sdl2_version"
	else
		SDL_CFLAGS="-I$SDL_LOCATION/Headers"
		BUNDLE_FRAMEWORKS="$BUNDLE_FRAMEWORKS $SDL_LOCATION"
	fi
else
	if test "$enable_sdl2" = yes; then
		SDL_CFLAGS="$SDL2_CFLAGS"
		SDL_LIBS="$SDL2_LIBS"
		sdl_version="$sdl2_version"
	fi
fi
AM_CONDITIONAL([ENABLE_SDL2], test "$enable_sdl2" = yes)
#
# SDL2 on macOS needs 10.6 or above
if test "$enable_sdl2" = yes -a "$OS_TYPE" == darwin; then
	export MACOSX_DEPLOYMENT_TARGET=10.6
fi

dnl Check for jpeg library
have_jpeglib_h=no
have_jpeglib_lib=no
AC_CHECK_HEADERS(jpeglib.h, have_jpeglib_h=yes)
if test "$have_jpeglib_h" = yes; then
	AC_CHECK_LIB(jpeg, jpeg_CreateDecompress, have_jpeglib_lib=yes)
fi
if test "$have_jpeglib_h" = yes -a "$have_jpeglib_lib" = yes; then
	AC_DEFINE(HAVE_JPEGLIB, 1, [Define if you have jpeglib])
	JPEG_LIBS="-ljpeg"
fi
AC_SUBST([JPEG_LIBS])

if test "$have_jpeglib_lib" = no; then
dnl Check for SDL_Image
if test "$enable_sdl2" = yes; then
	ARANYM_CHECK_FRAMEWORK(SDL2_image, [])
else
	ARANYM_CHECK_FRAMEWORK(SDL_image, [])
fi
if test "$have_framework_SDL2_image" = yes ; then
	ARANYM_CHECK_FRAMEWORK_LOCATION(SDL2_image)
	SDL_CFLAGS="$SDL_CFLAGS -I$SDL2_image_LOCATION/Headers"
	BUNDLE_FRAMEWORKS="$BUNDLE_FRAMEWORKS $SDL2_image_LOCATION"
	have_sdlimage_lib=yes
	have_sdlimage_h=yes
elif test "$have_framework_SDL_image" = yes ; then
	ARANYM_CHECK_FRAMEWORK_LOCATION(SDL_image)
	CPPFLAGS="$CPPFLAGS -I$SDL_image_LOCATION/Headers"
	BUNDLE_FRAMEWORKS="$BUNDLE_FRAMEWORKS $SDL_image_LOCATION"
	have_sdlimage_lib=yes
	have_sdlimage_h=yes
else
	tmp_cflags="$CFLAGS"
	tmp_cppflags="$CPPFLAGS"
	tmp_ldflags="$LDFLAGS"
	CFLAGS="$CFLAGS $SDL_CFLAGS"
	CPPFLAGS="$CPPFLAGS $SDL_CFLAGS"
	LDFLAGS="$LDFLAGS $SDL_LIBS"

	case "$host" in
		# the mixed environment on cygwin usually
		# causes link tests against SDL fail here,
		# just assume it is present
		*-*-cygwin*)
			have_sdlimage_lib=yes
			have_sdlimage_h=yes
			if test "$enable_sdl2" = yes; then
				SDL_LIBS="$SDL_LIBS `echo $SDL_LIBS | sed -e 's/SDL2.dll/SDL2_image.dll/'`"
			else
				SDL_LIBS="$SDL_LIBS `echo $SDL_LIBS | sed -e 's/SDL.dll/SDL_image.dll/'`"
			fi
			;;
		*-*-mingw*)
			have_sdlimage_lib=yes
			have_sdlimage_h=yes
			if test "$enable_sdl2" = yes; then
				SDL_LIBS="$SDL_LIBS -lSDL2_image"
			else
				SDL_LIBS="$SDL_LIBS -lSDL_image"
			fi
			;;
		*)
			AC_CHECK_HEADER(SDL_image.h, have_sdlimage_h=yes, havesdlimage_h=no)
			AC_CHECK_LIB(SDL_image, IMG_isJPG, have_sdlimage_lib=yes, have_sdlimage_lib=no)
			if test "$have_sdlimage_h" = yes -a "$have_sdlimage_lib" = yes; then
				if test "$enable_sdl2" = yes; then
					SDL_LIBS="$SDL_LIBS -lSDL2_image"
				else
					SDL_LIBS="$SDL_LIBS -lSDL_image"
				fi
			fi
			;;
	esac

	CFLAGS="$tmp_cflags"
	CPPFLAGS="$tmp_cppflags"
	LDFLAGS="$tmp_ldflags"
fi
if test "$have_sdlimage_h" = yes -a "$have_sdlimage_lib" = yes; then
	AC_DEFINE(HAVE_SDL_IMAGE, 1, [Define if you have SDL_image])
fi
fi

dnl Add OpenGL ?
if test "x$WANT_OPENGL" = "xyes"; then
	have_opengl="no"
	if test "x$PKG_CONFIG" != "x"; then
		PKG_CHECK_MODULES([GL], [gl], have_opengl="yes", have_opengl="no")
	fi
	if test "x$GL_LIBS" = "x"; then	
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
	fi
	case "$host" in
		# the mixed environment on cygwin usually
		# causes link tests against SDL fail here,
		# just assume it is present
		*-*-cygwin* | *-*-mingw*)
			have_opengl=yes
			;;
	esac
	AC_MSG_CHECKING(for OpenGL support)
	tmp_cflags="$CFLAGS"
	tmp_libs="$LIBS"
	if test "$have_opengl" = no; then
		CFLAGS="$CFLAGS $SDL_CFLAGS"
		LIBS="$LIBS $SDL_LIBS $GL_LIBS"
		AC_TRY_COMPILE([
			 #include "SDL_opengl.h"
			],[],[have_opengl="yes"])
	fi
	AC_MSG_RESULT($have_opengl)
	CFLAGS="$tmp_cflags"
	LIBS="$tmp_libs"
	if test "x$have_opengl" = "xyes"; then
		AC_DEFINE([ENABLE_OPENGL], 1, [Define if OpenGL enabled])
	else
		WANT_OPENGL="no"
	fi
fi
AM_CONDITIONAL([ENABLE_OPENGL], test "$WANT_OPENGL" = yes)


dnl Check for zlib
if [[ "x$WANT_LILO" = "xyes" ]]; then
	have_zlib=no
	# Search using pkg-config
	if test "x$PKG_CONFIG" != "x"; then
		PKG_CHECK_MODULES([ZLIB], [zlib], [have_zlib=yes], [have_zlib=no])
	fi

	# Fallback to autoconf (helps in Ubuntu 10.04 LTS)
	if test "$have_zlib" = "no"; then
		AC_CHECK_LIB(z, gzopen, [have_zlib=yes], [have_zlib=no])
	fi

	if test "$have_zlib" = "yes"; then
		AC_DEFINE([HAVE_LIBZ], 1, [Define if zlib enabled])
	fi
else
	have_zlib="not required"
fi

dnl Check for libusb-1.0
WITH_USBHOST="no"
if test "x$WANT_USBHOST" = "xyes"; then
  # Search using pkg-config
    if test "x$PKG_CONFIG" != "x"; then
      PKG_CHECK_MODULES([LIBUSB], [libusb-1.0], [WITH_USBHOST="yes"], [WITH_USBHOST="no"])
    fi
   
  # Search the library and headers directly
    if test "x$WITH_USBHOST" = "xno"; then
      AC_CHECK_HEADER(libusb-1.0/libusb.h, [WITH_USBHOST="yes"], [WITH_USBHOST="no"])
      if test "x$WITH_USBHOST" = "xyes"; then
        AC_CHECK_LIB(usb-1.0, libusb_init, [], [WITH_USBHOST="no"])
        LIBUSB_LIBS="-lusb-1.0"
      fi
    fi

    if test "x$WITH_USBHOST" = "xno"; then
      AC_MSG_WARN([libusb is missing, USB NF disabled])
    fi
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

AH_TEMPLATE([NFCDROM_SUPPORT], [Define if you want to use host CD-ROM driver])
NFCDROM_DRIVER=""
case "$host_os" in
	linux*)
		if [[ "$ANDROID_OS" != "yes" ]]; then
			AC_DEFINE([NFCDROM_LINUX_SUPPORT], 1, [Define if you want to use Linux support as CD-ROM driver])
			NFCDROM_DRIVER="linux"
		fi
		;;
	mingw*|cygwin*)
		AC_DEFINE([NFCDROM_WIN32_SUPPORT], 1, [Define if you want to use Win32 support as CD-ROM driver])
		NFCDROM_DRIVER="win32"
		;;
esac
if test "$enable_sdl2" != yes; then
	test "$NFCDROM_DRIVER" != "" && NFCDROM_DRIVER="${NFCDROM_DRIVER},"
	NFCDROM_DRIVER="${NFCDROM_DRIVER}sdl"
fi
if test "$NFCDROM_DRIVER" != ""; then
	AC_DEFINE([NFCDROM_SUPPORT], 1)
else
	NFCDROM_DRIVER=no
fi
PROVIDES_NF="$PROVIDES_NF cdrom($NFCDROM_DRIVER)"
AM_CONDITIONAL([NFCDROM_SUPPORT], test "$NFCDROM_DRIVER" != "no")


dnl Natfeat PCI driver

if [[ "x$WANT_NFPCI" = "xyes" ]]; then
	case "$host_os" in
		linux*)
			AC_DEFINE([NFPCI_SUPPORT], 1, [Define if you want to use host PCI driver])
			AC_DEFINE([NFPCI_LINUX_SUPPORT], 1, [Define if you want to use Linux PCI driver])
			PROVIDES_NF="$PROVIDES_NF pci"
			;;
		*)
			AC_MSG_ERROR([*** NatFeats PCI not supported by this OS ***])
			WANT_NFPCI="no"
			;;
	esac
fi
AM_CONDITIONAL([NFPCI_SUPPORT], test "$WANT_NFPCI" = "yes")
AM_CONDITIONAL([NFPCI_LINUX_SUPPORT], test "$WANT_NFPCI" = "yes" -a "$OS_TYPE" = "linux")

dnl Natfeat USB driver

if [[ "x$WANT_USBHOST" = "xyes" -a "x$WITH_USBHOST" = "xyes" ]]; then
	AC_DEFINE([USBHOST_SUPPORT], 1, [Define if you want to use host USB driver])
	LIBS="$LIBS $LIBUSB_LIBS"
	PROVIDES_NF="$PROVIDES_NF usb"
else
	WANT_USBHOST=no
fi
AM_CONDITIONAL([USBHOST_SUPPORT], test "$WANT_USBHOST" = yes)

dnl Natfeat SCSI driver

AH_TEMPLATE([NFSCSI_SUPPORT], [Define if you want to use host SCSI driver])
if [[ "x$WANT_NFSCSI" = "xyes" ]]; then
	case "$host_os" in
		linux*)
			AC_CHECK_HEADERS(scsi/sg.h)
			if test "$ac_cv_header_scsi_sg_h" != yes; then
				WANT_NFSCSI=no
			fi
			;;
		*)
			WANT_NFSCSI=no
			;;
	esac
	if test "$WANT_NFSCSI" = "no"; then
		AC_MSG_WARN([*** NatFeats SCSI not supported by this OS ***])
	else
		AC_DEFINE([NFSCSI_SUPPORT], 1)
		PROVIDES_NF="$PROVIDES_NF scsi"
	fi
fi

dnl Natfeat OSMesa driver

AH_TEMPLATE([NFOSMESA_SUPPORT], [Define if you want to use host OSMesa driver])
if test "x$WANT_NFOSMESA" = "xyes"; then
	have_osmesa=no
	if test "x$PKG_CONFIG" != "x"; then
		PKG_CHECK_MODULES([OSMESA], [osmesa], have_osmesa=yes, :)
	fi
	if test "$have_osmesa" = "no"; then
		have_osmesa=yes
		AC_CHECK_HEADERS(GL/osmesa.h)
	fi
	if test "$have_osmesa" = "yes"; then
		CPPFLAGS="$CPPFLAGS $OSMESA_CFLAGS"
		AC_DEFINE([NFOSMESA_SUPPORT], 1)
	else
		WANT_NFOSMESA="no"
	fi
fi
AM_CONDITIONAL([NFOSMESA_SUPPORT], test "$WANT_NFOSMESA" = yes)

if [[ "x$WANT_NFOSMESA" = "xyes" ]]; then
	PROVIDES_NF="$PROVIDES_NF osmesa"
fi

dnl Natfeat JPEG decoder

AH_TEMPLATE([NFJPEG_SUPPORT], [Define if you want to use host JPEG decoder])
if [[ "x$WANT_NFJPEG" = "xyes" ]]; then
	if test "$have_jpeglib_lib" = "yes"; then
		AC_DEFINE([NFJPEG_SUPPORT])
		PROVIDES_NF="$PROVIDES_NF jpeg"
	elif test "$have_sdlimage_h" = "yes" -a "$have_sdlimage_lib" = "yes"; then
		AC_DEFINE([NFJPEG_SUPPORT])
		PROVIDES_NF="$PROVIDES_NF jpeg"
	else
		WANT_NFJPEG="no"
                AC_MSG_WARN([no jpeg library found, JPEG NF disabled])
	fi
fi
AM_CONDITIONAL([NFJPEG_SUPPORT], test "$WANT_NFJPEG" = yes)


dnl Natfeat VDI driver

if [[ "x$WANT_NFVDI" = "xyes" ]]; then
	AC_DEFINE([NFVDI_SUPPORT], 1, [Define if you want to use NF VDI driver])
	NFVDI_DRIVER="sw"
	if [[ "x$WANT_OPENGL" = "xyes" ]]; then
		NFVDI_DRIVER="opengl,$NFVDI_DRIVER"
	fi
	PROVIDES_NF="$PROVIDES_NF vdi($NFVDI_DRIVER)"
fi
AM_CONDITIONAL([NFVDI_SUPPORT], test "$WANT_NFVDI" = "yes")

dnl Natfeat Clipboard driver

AH_TEMPLATE([NFCLIPBRD_SUPPORT], [Define if you want to use Clipboard])
if [[ "x$WANT_NFCLIPBRD" = "xyes" ]]; then
	case $OS_TYPE in
	cygwin | mingw)
		AC_DEFINE([NFCLIPBRD_SUPPORT], 1)
		;;
	darwin)
		AC_DEFINE([NFCLIPBRD_SUPPORT], 1)
		# X-Headers are not always available on macOS
		tmp_cppflags="$CPPFLAGS"
		# not nice to hardcode the path here,
		# but it is also hardcoded in the XCode project
		CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
		AC_CHECK_HEADERS(X11/Xlib.h)
		CPPFLAGS="$tmp_cppflags"
		;;
	beos | haiku)
		AC_DEFINE([NFCLIPBRD_SUPPORT], 1)
		LIBS="$LIBS -lbe"
		;;
	*)
		AC_CHECK_LIB(X11, XOpenDisplay,[SDL_LIBS="$SDL_LIBS -lX11"],,[-L/usr/X11R6/lib])
		if [[ "x$ac_cv_lib_X11_XOpenDisplay" = "xyes" ]]; then
			AC_DEFINE([NFCLIPBRD_SUPPORT], 1)
		else
		     	AC_MSG_ERROR([Sorry, NatFeat Clipboard not implemented on your platform yet])
			WANT_NFCLIPBRD="no"
		fi
		;;
	esac
fi
AM_CONDITIONAL([NFCLIPBRD_SUPPORT], test "$WANT_NFCLIPBRD" = yes)

dnl Natfeat host exec driver

if [[ "$WANT_NFEXEC" = yes -a "$WANT_HOSTFS" != yes ]]; then
	AC_MSG_WARN([NF exec disabled due to lack of support from HOSTFS])
	WANT_NFEXEC=no
fi
if [[ "$WANT_NFEXEC" = yes ]]; then
	AC_DEFINE([NFEXEC_SUPPORT], 1, [Define if you want to use NF host exec driver])
	PROVIDES_NF="$PROVIDES_NF exec"
fi
AM_CONDITIONAL([NFEXEC_SUPPORT], test "$WANT_NFEXEC" = "yes")


if [[ "x$WANT_NFCLIPBRD" = "xyes" ]]; then
	PROVIDES_NF="$PROVIDES_NF clipboard"
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

if test "x$WANT_PARALLELX86" = "xyes" ; then
	if test "x$OS_TYPE" = "xlinux" && $HAVE_I386; then
		AC_DEFINE([ENABLE_PARALLELX86], 1, [Define if you want to use Linux/X86 hardware access for parallel port])
	else
		WANT_PARALLELX86="no"
	fi
fi
AM_CONDITIONAL([ENABLE_PARALLELX86], test "$WANT_PARALLELX86" = yes)

dnl Linux /dev/parport access for parallel port

if test "x$WANT_PARALLELPARPORT" = "xyes" ; then
	if test "x$OS_TYPE" = "xlinux"; then
		AC_DEFINE([ENABLE_PARALLELPARPORT], 1, [Define if you want to use Linux /dev/parport access for parallel port])
	else
		WANT_PARALLELPARPORT="no"
	fi
fi
AM_CONDITIONAL([ENABLE_PARALLELPARPORT], test "$WANT_PARALLELPARPORT" = yes)

dnl Linux /dev/sequencer MIDI support

if test "x$OS_TYPE" = "xlinux"; then
	AC_DEFINE([ENABLE_MIDI_SEQUENCER], 1, [Define if you want to use Linux /dev/sequencer support])
fi

dnl Serial port support

if test "x$WANT_UNIXSERIALPORT" = "xyes" ; then
	if test "x$ac_cv_header_termios_h" = "xyes" -a "x$ac_cv_header_sys_ioctl_h" = "xyes"; then
		AC_DEFINE([ENABLE_SERIALUNIX], 1, [Define if you want to use /dev/ttyXXX access for serial port])
	else
		WANT_UNIXSERIALPORT="no"
	fi
fi
AM_CONDITIONAL([ENABLE_SERIALUNIX], test "$WANT_UNIXSERIALPORT" = yes)

AC_DEFINE_UNQUOTED([PROVIDES_NATFEATS], "$PROVIDES_NF", [List of Native Features])

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

AH_BOTTOM([
#endif /* __ARANYM_CONFIG_H__ */
])

dnl Generate Makefile.

ARCH="$CPU_TYPE"

AC_SUBST(ARCH)
AC_SUBST(WFLAGS)
AC_SUBST(DEFINES)
AC_SUBST(DBGSP)
AC_SUBST(TARGET)
AC_SUBST(CPU_TYPE)
AC_SUBST(ARCHFLAGS)
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)
AC_SUBST(BUNDLE_FRAMEWORKS)
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)
AC_SUBST(GENCOMP_ARCH)
AC_SUBST(CXX_FOR_BUILD)
AC_SUBST(CC_FOR_BUILD)
AC_SUBST(CFLAGS_FOR_BUILD)
AC_SUBST(CXXFLAGS_FOR_BUILD)
AC_SUBST(LDFLAGS_FOR_BUILD)

if test "$am__include" = "include"; then am__include=-include; fi

AC_CONFIG_FILES([
Makefile
src/Makefile
src/uae_cpu/Makefile
doc/Makefile
])

AC_OUTPUT

dnl Print summary.
echo
echo "ARAnyM configuration summary:"
echo
echo "SDL version ................... ........ : $sdl_version"
echo "Use exclusive spcflags handling ........ : $WANT_SPCFLAGS_EXCL"
echo "Use JIT compiler ....................... : $WANT_JIT"
echo "Use JIT compiler for FPU instructions .. : $WANT_JIT_FPU"
echo "JIT debug mode ......................... : $WANT_JIT_DEBUG"
echo "Floating-Point emulation core .......... : $FPE_CORE"
echo "Assembly optimizations ................. : $ASM_OPTIMIZATIONS"
echo "Addressing mode ........................ : $ADDRESSING_MODE"
ATC_TYPE_MSG=
if [[ "x$WANT_MMU" = "xyes" ]]; then
    case "$ATC_TYPE" in
		full)	ATC_TYPE_MSG=" (with ATC)";;
		small)	ATC_TYPE_MSG=" (with small ATC)";;
		no)	ATC_TYPE_MSG=" (without ATC)";;
    esac
fi
echo "Full MMU support ....................... : $WANT_MMU$ATC_TYPE_MSG"
echo "Linux /dev/rtc source timer ............ : $WANT_RTCTIMER"
echo "Real STOP implementation ............... : $WANT_REALSTOP"
echo "DSP 56001 support ...................... : $WANT_DSP"
echo "DSP 56001 disassembly support .......... : $WANT_DSP_DISASM"
echo "Debugger ............................... : $USE_DEBUGGER"
echo "Check memory ........................... : $MEMORY_CHECK"
echo "First 2kB of RAM Supervisor protected... : $WANT_PROTECT2K"
echo "FastRAM fixed size ..................... : $WANT_FIXEDFASTRAM"
echo "Fixed position of VideoRAM ............. : $WANT_FIXEDVIDEORAM"
echo "Blitter memmove acceleration ........... : $WANT_BLITMEMMOVE"
echo "Blitter SDL blit acceleration .......... : $WANT_BLITSDLBLIT"
echo "Support for hostfs ..................... : $WANT_HOSTFS"
echo "Support for ethernet ................... : $HAVE_ETHERNET"
echo "GUI .................................... : $WANT_SDLGUI"
echo "OpenGL ................................. : $WANT_OPENGL"
echo "Linux-m68k loader ...................... : $WANT_LILO"
echo "Zlib ................................... : $have_zlib"
echo "libusb-1.0.............................. : $WITH_USBHOST"
echo "NatFeat CD-ROM driver .................. : $NFCDROM_DRIVER"
echo "NatFeat PCI driver ..................... : $WANT_NFPCI"
echo "NatFeat USB driver ..................... : $WANT_USBHOST"
echo "NatFeat OSMesa driver .................. : $WANT_NFOSMESA"
echo "NatFeat JPEG decoder ................... : $WANT_NFJPEG"
echo "NatFeat Clipboard ...................... : $WANT_NFCLIPBRD"
echo "NatFeat VDI driver ..................... : $WANT_NFVDI"
echo "NatFeat SCSI driver .................... : $WANT_NFSCSI"
echo "NatFeat host exec driver ............... : $WANT_NFEXEC"
echo "Exception per second limiter ........... : $WANT_EPSLIMITER"
echo "Linux/X86 h/w access for parallel port.. : $WANT_PARALLELX86"
echo "Linux /dev/parport for parallel port.... : $WANT_PARALLELPARPORT"
echo "Unix /dev/ttySn for serial port......... : $WANT_UNIXSERIALPORT"
echo "Use C++ exceptions for m68k exceptions.. : $WANT_CXX_EXCEPTIONS"
echo
echo "Configuration done. Now type \"make\" (or \"gmake\")."
