dnl Process this file with autoconf to produce a configure script.
dnl Written in 1999 by Christian Bauer et al.
dnl Adapted for ARAnyM by Milan Jurik

AC_INIT(main_unix.cpp)
AC_PREREQ(2.52)
AC_CONFIG_HEADER(config.h)

dnl Options.
AC_ARG_ENABLE(nat-debug,     [  --enable-nat-debug      enable native debugger support [default=yes]], [WANT_NATDEBUG=$enableval], [WANT_NATDEBUG=yes])

AC_ARG_ENABLE(debug,
[  --enable-debug=which    specify which debugger to use [default=ndebug]],
[ case "$enableval" in
    default)  WHICH_DEBUG="ndebug";;
        uae)  WHICH_DEBUG="uae";;
     ndebug)  WHICH_DEBUG="ndebug";;
     gdebug)  WHICH_DEBUG="gdebug";;
         no)  WHICH_DEBUG="no";;   
	  *)  AC_MSG_ERROR([--enable-debug takes only one of the following values: default, uae, ndebug, no]);;
  esac
],
[WHICH_DEBUG="ndebug"
])
AC_ARG_WITH(mon,               [  --with-mon              use mon as debugger [default=no]], [WANT_MON=$withval], [WANT_MON=no])

AC_ARG_ENABLE(fullhistory,     [  --enable-fullhistory    enable full history for debugger (slower) [default=no]], [WANT_HISTORY=$enableval], [WANT_HISTORY=no])

AC_ARG_ENABLE(fullmmu,         [  --enable-fullmmu        enable full 68040 MMU [default=no]], [WANT_MMU=$enableval], [WANT_MMU=no])

AC_ARG_ENABLE(dsp,             [  --enable-dsp            enable DSP 56001 [default=yes]], [WANT_DSP=$enableval], [WANT_DSP=yes])

AC_ARG_ENABLE(dsp-disasm,      [  --enable-dsp-disasm     enable DSP 56001 disassembler [default=no]], [WANT_DSP_DISASM=$enableval], [WANT_DSP_DISASM=no])

AC_ARG_ENABLE(fixedfastram,    [  --enable-fixedfastram=X use fixed size FastRAM of X MB [default=no]], [WANT_FIXEDFASTRAM=$enableval], [WANT_FIXEDFASTRAM=no])

AC_ARG_ENABLE(fixedvideoram,   [  --enable-fixedvideoram  use fixed start of VideoRAM [default=no]], [WANT_FIXEDVIDEORAM=$enableval], [WANT_FIXEDVIDEORAM=no])

AC_ARG_ENABLE(directcolor,     [  --enable-directcolor    enable TOS direct truecolor [default=no]], [WANT_DIRECTCOLOR=$enableval], [WANT_DIRECTCOLOR=no])

AC_ARG_ENABLE(blitmemmove,     [  --enable-blitmemmove    enable Blitter memmove rout [default=no]], [WANT_BLITMEMMOVE=$enableval], [WANT_BLITMEMMOVE=no])

AC_ARG_ENABLE(blitsdlblit,     [  --enable-blitsdlblit    enable SDL blitting routine [default=no]], [WANT_BLITSDLBLIT=$enableval], [WANT_BLITSDLBLIT=no])

AC_ARG_ENABLE(hostfs,          [  --enable-hostfs         enable hostfs support [default=yes]], [WANT_HOSTFS=$enableval], [WANT_HOSTFS=yes])

AC_ARG_ENABLE(aranymfs,        [  --enable-aranymfs       enable aranymfs support [default=yes]], [WANT_ARANYMFS=$enableval], [WANT_ARANYMFS=yes])

AC_ARG_ENABLE(gui,             [  --enable-gui            enable GUI [default=yes]], [WANT_SDLGUI=$enableval], [WANT_SDLGUI=yes])

AC_ARG_ENABLE(opengl,          [  --enable-opengl         enable OpenGL [default=no]], [WANT_OPENGL=$enableval], [WANT_OPENGL=no])

dnl FPU emulation core.
AC_ARG_ENABLE(fpe,
[  --enable-fpe=FPE        specify which fpu emulator to use [default=auto]],
[ case "$enableval" in
    auto)	FPE_CORE_TEST_ORDER="ieee uae";; dnl ieee
    uae)	FPE_CORE_TEST_ORDER="uae";;
    i386)	FPE_CORE_TEST_ORDER="x86";;
    ieee)	FPE_CORE_TEST_ORDER="ieee";;
    *)		AC_MSG_ERROR([--enable-fpe takes only one of the following values: auto, uae, x86, ieee]);;
  esac
],
[ FPE_CORE_TEST_ORDER="ieee uae"
])

dnl Addressing modes.
AC_ARG_ENABLE(addressing,
[  --enable-addressing=AM  specify the addressing mode to use [default=fastest]],
[ case "$enableval" in
    real) 	ADDRESSING_TEST_ORDER="real";;
    direct)	ADDRESSING_TEST_ORDER="direct";;
    normal)	ADDRESSING_TEST_ORDER="normal";;
    fixed)	ADDRESSING_TEST_ORDER="fixed";;
    *)		AC_MSG_ERROR([--enable-addressing takes only one of the following values: real, direct, fixed, normal]);;
  esac
],
[ ADDRESSING_TEST_ORDER="real direct normal"
])

AC_ARG_ENABLE(addr-check,
[  --enable-addr-check=AM  specify the memory check to use [default=page]],
[ case "$enableval" in
    no)         MEMORY_CHECK="no";;
    full)	MEMORY_CHECK="full";;
    page)	MEMORY_CHECK="page";;
    hwsp)       MEMORY_CHECK="hwsp";;
    pagehwsp)   MEMORY_CHECK="pagehwsp";;
    sseg)       MEMORY_CHECK="sseg";;
    *)		AC_MSG_ERROR([--enable-addr-check takes only one of the following values: no, full, page, hwsp, pagehwsp, sseg]);;
  esac
],
[ MEMORY_CHECK="page"
])

dnl Experimental hacks around spcflags handling.
AC_ARG_ENABLE(spcflags-excl, [  --enable-spcflags-excl  enable exclusive spcflags handling [default=no]], [WANT_SPCFLAGS_EXCL=$enableval], [WANT_SPCFLAGS_EXCL=no])

dnl JIT compiler options.
AC_ARG_ENABLE(jit-compiler,    [  --enable-jit-compiler   enable JIT compiler [default=no]], [WANT_JIT=$enableval], [WANT_JIT=no])

AC_ARG_ENABLE(jit-fpu,
[  --enable-jit-fpu        enable JIT compiler for FPU instructions [default=yes]],
[WANT_JIT_FPU=$enableval],
[ if [[ "x$WANT_JIT" = "xyes" ]]; then
    WANT_JIT_FPU=yes
      else
    WANT_JIT_FPU=no
  fi
])

AC_ARG_ENABLE(jit-debug,       [  --enable-jit-debug      activate native code disassemblers [default=no]], [WANT_JIT_DEBUG=$enableval], [WANT_JIT_DEBUG=no])

AC_ARG_ENABLE(disdip,          [  --enable-disdip         enable distrubuted dispatcher [default=no]], [WANT_DIS_DIP=$enableval], [WANT_DIS_DIP=no])

AC_ARG_ENABLE(ethernet,        [  --enable-ethernet       enable ethernet card emulation [default=no]], [WANT_ETHERNET=$enableval], [WANT_ETHERNET=no])

dnl Canonical system information.
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

dnl Target OS type (target is host if not cross-compiling).
case "$target_os" in
  linux*)	OS_TYPE=linux;;
  openbsd*)	OS_TYPE=openbsd;;
  netbsd*)	OS_TYPE=netbsd;;
  freebsd*)	OS_TYPE=freebsd;;
  solaris*)	OS_TYPE=solaris;;
  darwin*)	OS_TYPE=darwin;;
  cygwin*)	OS_TYPE=cygwin;;
  irix*)	OS_TYPE=irix;;
  mint*)	OS_TYPE=mint;;
  *)		OS_TYPE=`echo $target_os | sed -e 's/-/_/g' | sed -e 's/\./_/g'`;;
esac
DEFINES="$DEFINES -DOS_$OS_TYPE"

dnl Target CPU type.
HAVE_I386=no
HAVE_M68K=no
HAVE_SPARC=no
HAVE_POWERPC=no
HAVE_X86_64=no
case "$target_cpu" in
  i386* | i486* | i586* | i686* | i786* ) CPU_TYPE=i386 HAVE_I386=yes;;
  m68k* ) CPU_TYPE=m68k HAVE_M68K=yes;;
  sparc* ) CPU_TYPE=sparc HAVE_SPARC=yes;;
  powerpc* ) CPU_TYPE=powerpc HAVE_POWERPC=yes;;
  x86_64* ) CPU_TYPE=x86_64 HAVE_X86_64=yes;;
  *) CPU_TYPE=`echo $target_cpu | sed -e 's/-/_/g'`;;
esac
DEFINES="$DEFINES -DCPU_$CPU_TYPE"

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
AC_CHECK_HEADERS(unistd.h fcntl.h sys/mman.h sys/ioctl.h)
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_HEADERS(termios.h termio.h alloca.h sys/statfs.h sys/statvfs.h)
AC_CHECK_HEADERS(sys/types.h sys/stat.h sys/vfs.h utime.h sys/param.h)
AC_CHECK_HEADERS(sys/mount.h types.h stat.h)

dnl checks for new version of ANSI C++ includes
AC_DEFUN([AC_CXX_CHEADERS],
  [AC_CACHE_CHECK([for ANSI C++ headers],
  ac_cv_cxx_cheaders,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
  #include <cstdlib>
  #include <cstdio>
  #include <cerrno>
  #include <cstring>
  #include <new>
  ],[return 0;],
  ac_cv_cxx_cheaders=yes, ac_cv_cxx_cheaders=no)
  AC_LANG_RESTORE
  ])
  if test "$ac_cv_cxx_cheaders" = yes; then
    AC_DEFINE(HAVE_NEW_HEADERS, 1, [Define if using new version of ANSI C++ headers.])
  fi
])
AC_CXX_CHEADERS

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
AC_TYPE_SIGNAL
AC_HEADER_TIME
AC_STRUCT_TM

AC_DEFUN([AC_CXX_BOOL],
  [AC_CACHE_CHECK(whether the compiler recognizes bool as a built-in type,
    ac_cv_cxx_bool,
    [AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_TRY_COMPILE([
  int f(int  x){return 1;}
  int f(char x){return 1;}
  int f(bool x){return 1;}
  ],[bool b = true; return f(b);],
  ac_cv_cxx_bool=yes, ac_cv_cxx_bool=no)
  AC_LANG_RESTORE
  ])
  if test "$ac_cv_cxx_bool" = yes; then
    AC_DEFINE(HAVE_BOOL,,[define if bool is a built-in type])
  fi
])
AC_CXX_BOOL

dnl Checks for libraries
AC_CHECK_LIB(m, sin)

dnl Checks for library functions.
AC_CHECK_FUNCS(strdup)
AC_CHECK_FUNCS(sigaction signal)
AC_CHECK_FUNCS(mmap mprotect munmap)
AC_CHECK_FUNCS(vm_allocate vm_deallocate vm_protect)

dnl Darwin seems to define mach_task_self() instead of task_self().
AC_CHECK_FUNCS(mach_task_self task_self)

CAN_NATIVE_M68K=no
dnl Use 68k CPU natively?
WANT_NATIVE_M68K=no
if [[ "x$HAVE_M68K" = "xyes" -a "x$CAN_NATIVE_M68K" = "xyes" ]]; then
  AC_DEFINE([ENABLE_NATIVE_M68K], 1, [Define if using native 68k mode.])
  WANT_NATIVE_M68K=yes
fi

case "$target_os" in
  linux*)
    EXTRASYSSRCS=linux/parameters_linux.cpp
    ;;
  cygwin*)
    DEFINES="$DEFINES -fpermissive"
    LIBS="$LIBS -lSDL -lSDLmain -liberty"
    EXTRASYSSRCS=parameters_unix.cpp
    ;;
  solaris*)
    AC_DEFINE([_XOPEN_SOURCE], 500, [Define if needed])
    DEFINES="$DEFINES -DBSD_COMP -fpermissive -D__EXTENSIONS__"
    EXTRASYSSRCS=parameters_unix.cpp
dnl    WANT_ARANYMFS="no"
    ;;
  irix*)
    EXTRASYSSRCS="parameters_unix.cpp irix/unaligned.c"
    ;;
  freebsd*)
    AC_DEFINE([HAVE_O_FSYNC], 1, [Define if you want to use FreeBSD's imitation of O_SYNC])
    EXTRASYSSRCS=parameters_unix.cpp
    ;;	      
  darwin*)
    AC_DEFINE([HAVE_O_FSYNC], 1, [Define if you want to use FreeBSD's imitation of O_SYNC])
    EXTRASYSSRCS="parameters_unix.cpp darwin/cdrom_osx.cpp"
    DEFINES="$DEFINES -DDONTUSEUNIXCDROM"
    LIBS="$LIBS -lIOKit"
    ;;
  *)
    EXTRASYSSRCS=parameters_unix.cpp
    ;;
esac

SYSSRCS="$SYSSRCS $EXTRASYSSRCS"

dnl Define a macro that translates a yesno-variable into a C macro definition
dnl to be put into the config.h file
dnl $1 -- the macro to define
dnl $2 -- the value to translate
dnl $3 -- template name
AC_DEFUN(AC_TRANSLATE_DEFINE, [
    if [[ "x$2" = "xyes" -o "x$2" = "xguessing yes" ]]; then
        AC_DEFINE([$1], 1, [$3])
    fi
])

dnl Written by Laurynas Biveinis
dnl GNU Autoconf macro AC_CXX_OPT(option)
dnl Checks if C++ compiler supports specified option.
dnl If yes - adds that option to CXXFLAGS.

AC_DEFUN(AC_CXX_OPT,
[
   AC_REQUIRE([AC_PROG_CXX])
   AC_REQUIRE([AC_LANG_CPLUSPLUS])
   AC_MSG_CHECKING([if C++ compiler supports $1 option])
   old_cxxflags=$CXXFLAGS
   CXXFLAGS="$1 $CXXFLAGS"
   AC_TRY_COMPILE([ ], [ ], test_opt=yes)
   if test -z test_opt; then
      CXXFLAGS=$old_cxxflags
      AC_MSG_RESULT(no)
   else
      AC_MSG_RESULT(yes)
fi
])

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

dnl Various checks if the system supports vm_allocate() and the like functions.
have_mach_vm=no
if [[ "x$ac_cv_func_vm_allocate" = "xyes" -a "x$ac_cv_func_vm_deallocate" = "xyes" -a \
      "x$ac_cv_func_vm_protect" = "xyes" ]]; then
  have_mach_vm=yes
fi
AC_TRANSLATE_DEFINE(HAVE_MACH_VM, "$have_mach_vm",
  [Define if your system has a working vm_allocate()-based memory allocator.])

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
      #include "vm_alloc.cpp"
    ], ac_cv_vm_protect_works=no, rm -f core,
    dnl When cross-compiling, do not assume anything
    ac_cv_vm_protect_works="guessing no"
    )
  done
  AC_TRY_RUN([
    #define CONFIGURE_TEST_VM_MAP
    #define TEST_VM_PROT_RDWR_WRITE
    #include "vm_alloc.cpp"
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
    #include "vm_alloc.cpp"
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
    #include "vm_alloc.cpp"
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
      #include "vm_alloc.cpp"
    ], ac_cv_mprotect_works=no, rm -f core,
    dnl When cross-compiling, do not assume anything
    ac_cv_mprotect_works="guessing no"
    )
  done
  AC_TRY_RUN([
    #define CONFIGURE_TEST_VM_MAP
    #define TEST_VM_PROT_RDWR_WRITE
    #include "vm_alloc.cpp"
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

dnl Check if we can mmap 0x2000 bytes from 0x0000
AC_CACHE_CHECK([whether we can map Low Memory area 0x0000-0x2000],
  ac_cv_can_map_lm, [
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_RUN([
    #include "vm_alloc.cpp"
    int main(void) { /* returns 0 if we could map the lowmem globals */
      volatile char * lm = 0;
      if (vm_init() < 0) exit(1);
      if (vm_acquire_fixed(0, 0x2000) == false) exit(1);
      lm[0] = 'z';
      if (vm_release((char *)lm, 0x2000) < 0) exit(1);
      vm_exit(); exit(0);
    }
  ], ac_cv_can_map_lm=yes, ac_cv_can_map_lm=no,
  dnl When cross-compiling, do not assume anything.
  ac_cv_can_map_lm="guessing no"
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
else SYSSRCS="$SYSSRCS vasprintf.c"
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
  SYSSRCS="$SYSSRCS getopt_init.c getopt1.c getopt.c"
fi

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

dnl Full MMU
if [[ "x$WANT_MMU" = "xyes" ]]; then
    AC_DEFINE([FULLMMU], 1, [Define if using full MMU])
    MMUSRCS="../uae_cpu/cpummu.cpp"
else
    MMUSRCS=""
fi

dnl DSP
if [[ "x$WANT_DSP" = "xyes" ]]; then
    AC_DEFINE([DSP_EMULATION], 1, [Define if using DSP])
    DSPSRCS="../dsp_cpu.cpp"
else
    DSPSRCS=""
fi

dnl DSP disassembler
if [[ "x$WANT_DSP_DISASM" = "xyes" ]]; then
    AC_DEFINE([DSP_DISASM], 1, [Define if DSP disassembly])
	DSPDISSRCS="../dsp_disasm.cpp"
else
	DSPDISSRCS=""
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
    AC_MSG_WARN([MMU needs fixed position of VideoRAM])
    AC_DEFINE([FIXED_VIDEORAM], 1, [Define if you want fixed position of VideoRAM])
  fi
fi

dnl Direct Truecolor
if [[ "x$WANT_DIRECTCOLOR" = "xyes" ]]; then
  if [[ "x$WANT_FIXEDVIDEORAM" = "xno" ]]; then
    AC_MSG_WARN([Direct truecolor needs fixed position of VideoRAM, ignored])
  else
    AC_DEFINE([DIRECT_TRUECOLOR], 1, [Define if you want direct truecolor output])
  fi
fi

dnl Blitter memmove acceleration
if [[ "x$WANT_BLITMEMMOVE" = "xyes" ]]; then
  AC_DEFINE([BLITTER_MEMMOVE], 1, [Define if you want accelerated blitter output])
fi

dnl Blitter SDL surface blitting
if [[ "x$WANT_BLITSDLBLIT" = "xyes" ]]; then
  AC_DEFINE([BLITTER_SDLBLIT], 1, [Define if you want even more accelerated blitter output])
  AC_DEFINE([BLITTER_MEMMOVE], 1, [Define if you want accelerated blitter output])	dnl required for the SDLBLIT
fi

dnl Ethernet card emulation
if [[ "x$WANT_ETHERNET" = "xyes" ]]; then
  AC_DEFINE([ETHERNET_SUPPORT], 1, [Define if you want to use ethernet card emulation])
  AC_CHECK_HEADERS(linux/if_tun.h)
  SYSSRCS="$SYSSRCS ../natfeat/ece.cpp"
fi

dnl Support for direct access to host's fs
if [[ "x$WANT_HOSTFS" = "xyes" ]]; then
  SYSSRCS="$SYSSRCS ../natfeat/hostfs.cpp"
  AC_DEFINE([HOSTFS_SUPPORT], 1, [Define if you want to use direct access to host's fs])
fi

dnl Support for direct access to host's fs
if [[ "x$WANT_ARANYMFS" = "xyes" ]]; then
  SYSSRCS="$SYSSRCS ../extfs.cpp"
  AC_DEFINE([EXTFS_SUPPORT], 1, [Define if you want to use direct access to host's fs])
fi

dnl SDL GUI
if [[ "x$WANT_SDLGUI" = "xyes" ]]; then
  SYSSRCS="$SYSSRCS ../sdlgui.cpp ../dialog.cpp ../file.cpp"
  AC_DEFINE([SDL_GUI], 1, [Define if you want to build SDL GUI])
fi

dnl Extended signal segmentation fault handler
if [[ "x$WANT_EXSIGSEGV" = "xyes" ]]; then
  case "$target_os" in
    linux*)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS linux/sigsegv_linux.cpp"
      else
        AC_MSG_ERROR([Sorry, extended segfault handler supported on your platform])
      fi
      ;;
    openbsd*)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS xbsd/sigsegv_xbsd.cpp"
      else
        AC_MSG_ERROR([Sorry, extended segfault handler supported on your platform])
      fi
      ;;
    netbsd*)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS xbsd/sigsegv_xbsd.cpp"
      else
        AC_MSG_ERROR([Sorry, extended segfault handler supported on your platform])
      fi
      ;;
    freebsd*)
      if [[ "x$HAVE_I386" = "xyes" ]]; then
        SYSSRCS="$SYSSRCS xbsd/sigsegv_xbsd.cpp"
      else
        AC_MSG_ERROR([Sorry, extended segfault handler supported on your platform])
      fi
      ;;					
    *)
      AC_MSG_ERROR([Sorry, extended segfault handler supported on your platform])
      ;;
  esac
fi

dnl Determine the addressing mode to use
if [[ "x$WANT_NATIVE_M68K" = "xyes" ]]; then
  ADDRESSING_MODE="real"
else
  ADDRESSING_MODE=""
  AC_MSG_CHECKING([for the addressing mode to use])
  for am in $ADDRESSING_TEST_ORDER; do
    case $am in
    real)
      if [[ "x$HAVE_MMAP" = "xno" ]]; then
        continue
      fi
      dnl Requires ability to mmap() Low Memory globals
      if [[ "x$ac_cv_can_map_lm" = "xno" ]]; then
        continue
      fi
      dnl Problems with Cygwin
      if [[ "x$OS_TYPE" = "xcygwin" ]]; then
        continue
      fi
      dnl Problems with Solaris
      if [[ "x$OS_TYPE" = "xsolaris" ]]; then
        continue
      fi
      dnl Problems with Irix
      if [[ "x$OS_TYPE" = "xirix" ]]; then
        continue
      fi
      dnl Real addressing will probably work.
      ADDRESSING_MODE="real"
      DEFINES="$DEFINES -DREAL_ADDRESSING"
      break
      ;;
    direct)
      if [[ "x$HAVE_MMAP" = "xno" ]]; then
        continue
      fi
      dnl Problems with Cygwin
      if [[ "x$OS_TYPE" = "xcygwin" ]]; then
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
      if [[ "x$OS_TYPE" = "xlinux" -a "x$HAVE_I386" ]]; then
        ADDRESSING_MODE="fixed (0x51000000)"
        DEFINES="$DEFINES -DFIXED_ADDRESSING -DFMEMORY=0x51000000"
        break
      fi
      if [[ "x$OS_TYPE" = "xopenbsd" -a "x$HAVE_I386" ]]; then
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
AC_EGREP_CPP(yes,
[#if __GNUC__ - 1 > 1 || __GNUC_MINOR__ - 1 > 5
  yes
#endif
], [AC_MSG_RESULT(yes); HAVE_GCC27=yes], AC_MSG_RESULT(no))

dnl Check for GCC 3.0 or higher.
HAVE_GCC30=no
AC_MSG_CHECKING(for GCC 3.0 or higher)
AC_EGREP_CPP(yes,
[#if __GNUC__ >= 3
  yes
#endif
], [AC_MSG_RESULT(yes); HAVE_GCC30=yes], AC_MSG_RESULT(no))

dnl Set "-fomit-frame-pointer" on i386 GCC 2.7 or higher.
dnl No RTTI
if [[ "x$HAVE_GCC27" = "xyes" -a "x$HAVE_I386" = "xyes" ]]; then
  CFLAGS="$CFLAGS -fomit-frame-pointer"
  CXXFLAGS="$CXXFLAGS -fomit-frame-pointer -fno-rtti"
fi

dnl (gb) Do not merge constants since it breaks fpu/fpu_x86.cpp.
dnl As of 2001/08/02, this affects the following compilers:
dnl Official: probably gcc-3.1 (mainline CVS)
dnl Mandrake: gcc-2.96 >= 0.59mdk, gcc-3.0.1 >= 0.1mdk
dnl Red Hat : gcc-2.96 >= 89, gcc-3.0 >= 1
if [[ "x$HAVE_GCC27" = "xyes" -a "x$FPE_CORE" = "xi386" ]]; then
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

dnl Distributed dispatcher eliminates pipeline stall
dnl It needs computed gotos (supported by GNU C++)
dnl DODO - test for computed gotos
if [[ "x$WANT_DIS_DIP" = "xyes" ]]; then
  DEFINES="$DEFINES -DDISDIP"
fi			
				
dnl Select appropriate CPU source and REGPARAM define.
ASM_OPTIMIZATIONS=none
if [[ "x$WANT_DIS_DIP" = "xyes" ]]; then
  CPUSRCS="cpuemu.cpp"
else
  CPUSRCS="cpuemu1.cpp cpuemu2.cpp cpuemu3.cpp cpuemu4.cpp cpuemu5.cpp cpuemu6.cpp cpuemu7.cpp cpuemu8.cpp"
fi

dnl gb-- JITSRCS will be emptied later if the JIT is not available
dnl gb-- Other platforms should define their own set of noflags file variants
CAN_JIT=no
JITSRCS="compemu1.cpp compemu2.cpp compemu3.cpp compemu4.cpp compemu5.cpp compemu6.cpp compemu7.cpp compemu8.cpp"

if [[ "x$HAVE_GCC27" = "xyes" -a "x$HAVE_I386" = "xyes" ]]; then
  dnl i386 CPU
  DEFINES="$DEFINES -DUNALIGNED_PROFITABLE -DREGPARAM=\"__attribute__((regparm(3)))\""
  if [[ "x$HAVE_GAS" = "xyes" ]]; then
    ASM_OPTIMIZATIONS=i386
    DEFINES="$DEFINES -DX86_ASSEMBLY -DOPTIMIZED_FLAGS -DSAHF_SETO_PROFITABLE"
#    if [[ "x$WANT_DIS_DIP" = "xyes" ]]; then
#      CPUSRCS="cpufast.s"
#    else
#      CPUSRCS="cpufast1.s cpufast2.s cpufast3.s cpufast4.s cpufast5.s cpufast6.s cpufast7.s cpufast8.s"
#    fi
    JITSRCS="cpuemu1_nf.cpp cpuemu2_nf.cpp cpuemu3_nf.cpp cpuemu4_nf.cpp cpuemu5_nf.cpp cpuemu6_nf.cpp cpuemu7_nf.cpp cpuemu8_nf.cpp $JITSRCS"
#    JITSRCS="cpufast1_nf.s cpufast2_nf.s cpufast3_nf.s cpufast4_nf.s cpufast5_nf.s cpufast6_nf.s cpufast7_nf.s cpufast8_nf.s $JITSRCS"
    CAN_JIT=yes
  fi
elif [[ "x$HAVE_GCC30" = "xyes" -a "x$HAVE_X86_64" = "xyes" ]]; then
  dnl x86-64 CPU
  DEFINES="$DEFINES -DUNALIGNED_PROFITABLE"
  if [[ "x$HAVE_GAS" = "xyes" ]]; then
    ASM_OPTIMIZATIONS="x86-64"
    DEFINES="$DEFINES -DX86_64_ASSEMBLY -DOPTIMIZED_FLAGS"
  fi
elif [[ "x$HAVE_GCC27" = "xyes" -a "x$HAVE_SPARC" = "xyes" -a "x$HAVE_GAS" = "xyes" ]]; then
  dnl SPARC CPU
  case "$target_os" in
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
elif [[ "x$WANT_NATIVE_M68K" = "xyes" ]]; then
  dnl Native m68k, no emulation
  CPUINCLUDES="-I../native_cpu"
  CPUSRCS="asm_support.s"
fi

dnl JIT sources if available
if [[ "x$WANT_JIT" = "xyes" -a "x$CAN_JIT" ]]; then
  if [[ "x$WANT_MMU" = "xyes" ]]; then
      AC_MSG_WARN([!!!!! MMU&JIT in progress, dangerous !!!!!])
  fi
  JITSRCS="$JITSRCS ../uae_cpu/compiler/compemu_support.cpp"
  DEFINES="$DEFINES -DUSE_JIT"

  if [[ "x$WANT_JIT_DEBUG" = "xyes" ]]; then
#    if [[ "x$WANT_MON" = "xyes" ]]; then
      DEFINES="$DEFINES -DJIT_DEBUG=1"
#    else
#      AC_MSG_WARN([cxmon not found, ignoring --enable-jit-debug])
#      WANT_JIT_DEBUG=no
#    fi
  fi

  if [[ "x$WANT_JIT_FPU" = "xyes" ]]; then
    DEFINES="$DEFINES -DUSE_JIT_FPU"
    JITSRCS="$JITSRCS ../uae_cpu/compiler/compemu_fpp.cpp"
  fi

  JITSRCS="$JITSRCS compstbl.o cpustbl_nf.o"
else
  WANT_JIT=no
  WANT_JIT_FPU=no
  WANT_JIT_DEBUG=no
  JITSRCS=""
fi

dnl Utility macro used by next two tests.
dnl AC_EXAMINE_OBJECT(C source code,
dnl	commands examining object file,
dnl	[commands to run if compile failed]):
dnl
dnl Compile the source code to an object file; then convert it into a
dnl printable representation.  All unprintable characters and
dnl asterisks (*) are replaced by dots (.).  All white space is
dnl deleted.  Newlines (ASCII 0x10) in the input are preserved in the
dnl output, but runs of newlines are compressed to a single newline.
dnl Finally, line breaks are forcibly inserted so that no line is
dnl longer than 80 columns and the file ends with a newline.  The
dnl result of all this processing is in the file conftest.dmp, which
dnl may be examined by the commands in the second argument.
dnl
AC_DEFUN([gcc_AC_EXAMINE_OBJECT],
[AC_LANG_SAVE
AC_LANG_C
dnl Next bit cribbed from AC_TRY_COMPILE.
cat > conftest.$ac_ext <<EOF
[#line __oline__ "configure"
#include "confdefs.h"
$1
]EOF
if AC_TRY_EVAL(ac_compile); then
  od -c conftest.o |
    sed ['s/^[0-7]*[ 	]*/ /
	  s/\*/./g
	  s/ \\n/*/g
	  s/ [0-9][0-9][0-9]/./g
	  s/  \\[^ ]/./g'] |
    tr -d '
 ' | tr -s '*' '
' | fold | sed '$a\
' > conftest.dmp
  $2
ifelse($3, , , else
  $3
)dnl
fi
rm -rf conftest*
AC_LANG_RESTORE])

dnl Floating point format probe.
dnl The basic concept is the same as the above: grep the object
dnl file for an interesting string.  We have to watch out for
dnl rounding changing the values in the object, however; this is
dnl handled by ignoring the least significant byte of the float.
dnl
dnl Does not know about VAX G-float or C4x idiosyncratic format.
dnl It does know about PDP-10 idiosyncratic format, but this is
dnl not presently supported by GCC.  S/390 "binary floating point"
dnl is in fact IEEE (but maybe we should have that in EBCDIC as well
dnl as ASCII?)
dnl
AC_DEFUN([gcc_AC_C_FLOAT_FORMAT],
[AC_CACHE_CHECK(floating point format, ac_cv_c_float_format,
[gcc_AC_EXAMINE_OBJECT(
[/* This will not work unless sizeof(double) == 8.  */
extern char sizeof_double_must_be_8 [sizeof(double) == 8 ? 1 : -1];

/* This structure must have no internal padding.  */
struct possibility {
  char prefix[8];
  double candidate;
  char postfix[8];
};

#define C(cand) { "\nformat:", cand, ":tamrof\n" }
struct possibility table [] =
{
  C( 3.25724264705901305206e+01), /* @@IEEEFP - IEEE 754 */
  C( 3.53802595280598432000e+18), /* D__float - VAX */
  C( 5.32201830133125317057e-19), /* D.PDP-10 - PDP-10 - the dot is 0x13a */
  C( 1.77977764695171661377e+10), /* IBMHEXFP - s/390 format, ascii */
  C(-5.22995989424860458374e+10)  /* IBMHEXFP - s/390 format, EBCDIC */
};],
 [if   grep 'format:.@IEEEF.:tamrof' conftest.dmp >/dev/null 2>&1; then
    ac_cv_c_float_format='IEEE (big-endian)'
  elif grep 'format:.I@@PFE.:tamrof' conftest.dmp >/dev/null 2>&1; then
    ac_cv_c_float_format='IEEE (big-endian)'
  elif grep 'format:.FEEEI@.:tamrof' conftest.dmp >/dev/null 2>&1; then
    ac_cv_c_float_format='IEEE (little-endian)'
  elif grep 'format:.EFP@@I.:tamrof' conftest.dmp >/dev/null 2>&1; then
    ac_cv_c_float_format='IEEE (little-endian)'
  elif grep 'format:.__floa.:tamrof' conftest.dmp >/dev/null 2>&1; then
    ac_cv_c_float_format='VAX D-float'
  elif grep 'format:..PDP-1.:tamrof' conftest.dmp >/dev/null 2>&1; then
    ac_cv_c_float_format='PDP-10'
  elif grep 'format:.BMHEXF.:tamrof' conftest.dmp >/dev/null 2>&1; then
    ac_cv_c_float_format='IBM 370 hex'
  else
    AC_MSG_ERROR(Unknown floating point format)
  fi],
  [AC_MSG_ERROR(compile failed)])
])
# IEEE is the default format.  If the float endianness isn't the same
# as the integer endianness, we have to set FLOAT_WORDS_BIG_ENDIAN
# (which is a tristate: yes, no, default).  This is only an issue with
# IEEE; the other formats are only supported by a few machines each,
# all with the same endianness.
format=IEEE_FLOAT_FORMAT
fbigend=
case $ac_cv_c_float_format in
    'IEEE (big-endian)' )
	if test $ac_cv_c_bigendian = no; then
	    fbigend=1
	fi
	;;
    'IEEE (little-endian)' )
	if test $ac_cv_c_bigendian = yes; then
	    fbigend=0
	fi
	;;
    'VAX D-float' )
	format=VAX_FLOAT_FORMAT
	;;
    'PDP-10' )
	format=PDP10_FLOAT_FORMAT
	;;
    'IBM 370 hex' )
	format=IBM_FLOAT_FORMAT
	;;
esac
AC_DEFINE_UNQUOTED(HOST_FLOAT_FORMAT, $format,
  [Define to the floating point format of the host machine.])
if test -n "$fbigend"; then
	AC_DEFINE_UNQUOTED(HOST_FLOAT_WORDS_BIG_ENDIAN, $fbigend,
  [Define to 1 if the host machine stores floating point numbers in
   memory with the word containing the sign bit at the lowest address,
   or to 0 if it does it the other way around.

   This macro should not be defined if the ordering is the same as for
   multi-word integers.])
fi
])

dnl Select appropriate FPU source.
gcc_AC_C_FLOAT_FORMAT
AC_CHECK_HEADERS(ieee754.h ieeefp.h floatingpoint.h nan.h)

if [[ "x$WANT_JIT_FPU" = "xyes" -a "x$FPE_CORE_TEST_ORDER" != "xieee" ]]; then
dnl  AC_MSG_WARN([Sorry, JIT-FPU supports only the "ieee" FPE implementation])
  FPE_CORE="ieee"
  dnl Restore previous variables. FPE_CORE_STR and FPUSRCS are overwritten
fi

for fpe in $FPE_CORE_TEST_ORDER; do
  case $fpe in
  ieee)
    case $ac_cv_c_float_format in
    IEEE*)
      FPE_CORE="IEEE fpu core"
      DEFINES="$DEFINES -DFPU_IEEE"
      FPUSRCS="../uae_cpu/fpu/fpu_ieee.cpp"
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
      FPUSRCS="../uae_cpu/fpu/fpu_x86.cpp"
      break
    fi
    ;;
  uae)
    FPE_CORE="uae fpu core"
    DEFINES="$DEFINES -DFPU_UAE"
    FPUSRCS="../uae_cpu/fpu/fpu_uae.cpp"
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
  if [[ "x$WHICH_DEBUG" = "xndebug" ]]; then
    AC_DEFINE([DEBUGGER], 1, [Define if using some debugger])
    AC_DEFINE([NEWDEBUG], 1, [Define if using New Debugger])
    CPUSRCS="$CPUSRCS ../uae_cpu/debug.cpp"
  fi

  dnl UAE debugger
  if [[ "x$WHICH_DEBUG" = "xuae" ]]; then
    AC_DEFINE([DEBUGGER], 1, [Define if using some debugger])
    CPUSRCS="$CPUSRCS ../uae_cpu/debug.cpp"
  fi

  dnl We use mon if possible.
  MONSRCS=
  if [[ "x$WANT_MON" = "xyes" ]]; then
    if [[ "x$WHICH_DEBUG" = "xndebug" ]]; then
      AC_MSG_WARN([ndebug and mon couldn't cooperated today]);
      WANT_MON=no
    else
      AC_MSG_CHECKING(for mon)
      mon_srcdir=../../../mon/src
      if grep mon_init $mon_srcdir/mon.h >/dev/null 2>/dev/null; then
        AC_MSG_RESULT(yes)
        AC_DEFINE([ENABLE_MON], 1, [Define if using "mon".])
        MONSRCS="$mon_srcdir/mon.cpp $mon_srcdir/mon_6502.cpp $mon_srcdir/mon_z80.cpp $mon_srcdir/mon_cmd.cpp $mon_srcdir/mon_disass.cpp $mon_srcdir/mon_ppc.cpp $mon_srcdir/mon_lowmem.cpp $mon_srcdir/disass/floatformat.c $mon_srcdir/disass/i386-dis.c $mon_srcdir/disass/m68k-dis.c $mon_srcdir/disass/m68k-opc.c"
        CXXFLAGS="$CXXFLAGS -I$mon_srcdir -I$mon_srcdir/disass"
        AC_CHECK_LIB(readline, readline)
        AC_CHECK_LIB(termcap, tputs)
        AC_CHECK_HEADERS(readline.h history.h readline/readline.h readline/history.h)
      else
        AC_MSG_RESULT(no)
        AC_MSG_WARN([Could not find mon, ignoring --with-mon.])
        WANT_MON=no
      fi
    fi
  fi
fi

SYSSRCS="$SYSSRCS $MONSRCS"

AC_CHECK_FUNCS(isnan isinf)		dnl C99
AC_CHECK_FUNCS(isnanl isinfl)	dnl IEEE ?

dnl Exclusive spcflags handling
if [[ "x$WANT_SPCFLAGS_EXCL" = "xyes" ]]; then
  DEFINES="$DEFINES -DENABLE_EXCLUSIVE_SPCFLAGS"
fi

dnl UAE CPU sources for all non-m68k-native architectures.
if [[ "x$WANT_NATIVE_M68K" = "xno" ]]; then
  CPUINCLUDES="-I../uae_cpu"
  CPUSRCS="../uae_cpu/aranym_glue.cpp ../uae_cpu/memory.cpp ../uae_cpu/newcpu.cpp ../uae_cpu/readcpu.cpp $FPUSRCS cpudefs.cpp cpustbl.cpp $CPUSRCS $JITSRCS $MMUSRCS $DSPSRCS $DSPDISSRCS"
fi

dnl Remove the "-g" option if set for GCC.
if [[ "x$HAVE_GCC27" = "xyes" ]]; then
  CFLAGS=`echo $CFLAGS | sed -e 's/-g//g'`
  CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-g//g'`
fi
if [[ "x$WANT_NATDEBUG" = "xyes" ]]; then
  DBGSP="-g"
else
  DBGSP=""
fi

dnl Remove the "-O2" option if set for GCC and set the "-O3" option.
if [[ "x$HAVE_GCC27" = "xyes" ]]; then
  CFLAGS=`echo $CFLAGS | sed -e 's/-O2//g'`
  CFLAGS="-O3 $CFLAGS"
  CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-O2//g'`
  CXXFLAGS="-O3 $CXXFLAGS"
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

dnl Add OpenGL ?
if [[ "x$WANT_OPENGL" = "xyes" ]]; then
        AC_DEFINE([ENABLE_OPENGL], 1, [Define if using OpenGL])
	LIBS="$LIBS -lGL -lGLU"
fi

SDL_CFLAGS=
SDL_LIBS=
AM_PATH_SDL(1.2.0, )
LIBS="$LIBS $SDL_LIBS"
CFLAGS="$CFLAGS $SDL_CFLAGS"
CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"

dnl Generate Makefile.
AC_SUBST(SET_MAKE)
AC_SUBST(DEFINES)
AC_SUBST(SYSSRCS)
AC_SUBST(CPUINCLUDES)
AC_SUBST(CPUSRCS)
AC_SUBST(DBGSP)
AC_OUTPUT(Makefile)

dnl Print summary.
echo
echo ARAnyM configuration summary:
echo
echo Running m68k code natively ............. : $WANT_NATIVE_M68K
echo Use exclusive spcflags handling ........ : $WANT_SPCFLAGS_EXCL
echo Use JIT compiler ....................... : $WANT_JIT
echo Use JIT compiler for FPU instructions .. : $WANT_JIT_FPU
echo JIT debug mode ......................... : $WANT_JIT_DEBUG
echo Floating-Point emulation core .......... : $FPE_CORE
echo Assembly optimizations ................. : $ASM_OPTIMIZATIONS
echo Addressing mode ........................ : $ADDRESSING_MODE
echo Full MMU support ....................... : $WANT_MMU
echo DSP 56001 support ...................... : $WANT_DSP
echo DSP 56001 disassembly support .......... : $WANT_DSP_DISASM
echo Distributed dispatcher ................. : $WANT_DIS_DIP
echo Debugger ............................... : $WHICH_DEBUG
echo Monitor ................................ : $WANT_MON
echo Check memory ........................... : $MEMORY_CHECK
echo FastRAM fixed size ..................... : $WANT_FIXEDFASTRAM
echo Fixed position of VideoRAM ............. : $WANT_FIXEDVIDEORAM
echo TOS Direct Truecolor.................... : $WANT_DIRECTCOLOR
echo Blitter memmove acceleration ........... : $WANT_BLITMEMMOVE
echo Blitter SDL blit acceleration .......... : $WANT_BLITSDLBLIT
echo Support for aranymfs ................... : $WANT_ARANYMFS
echo Support for hostfs ..................... : $WANT_HOSTFS
echo Support for ECE ........................ : $WANT_ETHERNET
echo GUI .................................... : $WANT_SDLGUI
echo OpenGL ................................. : $WANT_OPENGL
echo
echo "Configuration done. Now type \"make\" (or \"gmake\")."
