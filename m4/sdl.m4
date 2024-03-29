# Configure paths for SDL
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

# serial 1

dnl AM_PATH_SDL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL, and define SDL_CFLAGS and SDL_LIBS
dnl
AC_DEFUN([AM_PATH_SDL],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sdl-prefix,[AS_HELP_STRING([--with-sdl-prefix=PFX], [Prefix where SDL is installed (optional)])],
            sdl_prefix="$withval", sdl_prefix="")
AC_ARG_WITH(sdl-exec-prefix,[AS_HELP_STRING([--with-sdl-exec-prefix=PFX Exec prefix where SDL is installed (optional)])],
            sdl_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdltest, [AS_HELP_STRING([--disable-sdltest], [Do not try to compile and run a test SDL program])],
		    , enable_sdltest=yes)

  as_save_PATH="$PATH"
  if test "x$prefix" != xNONE; then
    PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  fi

#
# provide some defaults on cygwin,
# where we use the MinGW version of the SDL library,
# to simplify standard configuration.
# But only if the user did not override it.
#
case $host in
  i686-*-cygwin*)
  MINGW_ROOT=$prefix/i686-w64-mingw32/sys-root/mingw
  if test -d "$MINGW_ROOT"; then
     if test "$sdl_prefix" = "" ; then
        sdl_prefix="$MINGW_ROOT"
     fi
  fi
  ;;
  x86_64-*-cygwin*)
  MINGW_ROOT=$prefix/x86_64-w64-mingw32/sys-root/mingw
  if test -d "$MINGW_ROOT"; then
     if test "$sdl_prefix" = "" ; then
        sdl_prefix="$MINGW_ROOT"
     fi
  fi
  ;;
esac

  if test x$sdl_exec_prefix != x ; then
    sdl_config_args="$sdl_config_args --exec-prefix=$sdl_exec_prefix"
    PATH="$sdl_exec_prefix:$PATH"
  fi
  if test x$sdl_prefix != x ; then
    sdl_config_args="$sdl_config_args --prefix=$sdl_prefix"
    PATH="$sdl_prefix/bin:$PATH"
  fi

  if test x${SDL_CONFIG+set} != xset ; then
    AC_PATH_PROG(SDL_CONFIG, sdl-config, no, [$PATH])
  else
    test -f "$SDL_CONFIG" || SDL_CONFIG=no
  fi
  PATH="$as_save_PATH"
  min_sdl_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL_CONFIG" = "no" ; then
    no_sdl=yes
  else
    SDL_CFLAGS=`$SDL_CONFIG $sdl_config_args --cflags`
    SDL_LIBS=`$SDL_CONFIG $sdl_config_args --libs`

case $host in
  *-*-cygwin*)
  # switches that must be removed for the mixed cygwin/MinGW32 platform
  # also replaces -L... -lSDL by the absolute pathname of the library,
  # because the -L points to mingw libraries instead of cygwin libraries
  nosdlswitch='s/-Dmain=SDL_main//;
s=-I/usr/include.*/SDL='-I${includedir}/SDL'=;
s=-I/mingw/include.*/SDL='-I${includedir}/SDL'=;
s/-DWIN32//;
s/-Uunix//;
s/-mno-cygwin//;
s/-lmingw32//;
s/-lSDLmain//;
s/-mwindows//;
s/-mms-bitfields//;
s=-L\([[^ ]]*\).*-l\(SDL[[^ ]]*\)=\1/lib\2.dll.a=;
'
	SDL_CFLAGS=`echo $SDL_CFLAGS | sed -e "$nosdlswitch"`
	SDL_LIBS=`echo $SDL_LIBS | sed -e "$nosdlswitch"`
        enable_sdltest=no
	;;
  *-*-mingw*)
  # switches that must be removed because we dont link SDLmain
  nosdlswitch='s/-Dmain=SDL_main//;
s=-I/usr/include.*/SDL='-I${includedir}/SDL'=;
s=-I/mingw/include.*/SDL='-I${includedir}/SDL'=;
s/-DWIN32//;
s/-Uunix//;
s/-lmingw32//;
s/-lSDLmain//;
s/-mwindows//;
'
	SDL_CFLAGS=`echo $SDL_CFLAGS | sed -e "$nosdlswitch"`
	SDL_LIBS=`echo $SDL_LIBS | sed -e "$nosdlswitch"`
        enable_sdltest=no
	;;
esac

# remove -L/usr/lib{32,64} from SDL_LIBS; this is
# a default search directory anyways,
# and will cause trouble when compiling in 32-bit mode
# on 64-bit machines
SDL_LIBS=`echo $SDL_LIBS | sed -e 's,-L/usr/lib[[2346]]* ,,'`

    sdl_version=`$SDL_CONFIG $sdl_config_args --version`
    sdl_major_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl_minor_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl_micro_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_CXXFLAGS="$CXXFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      eval CFLAGS=\"$CFLAGS\"
      eval CFLAGS=\"$CFLAGS\"
      CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
      eval CXXFLAGS=\"$CXXFLAGS\"
      eval CXXFLAGS=\"$CXXFLAGS\"
      LIBS="$LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SDL is sufficiently new. (Also sanity
dnl checks the results of sdl-config to some extent
dnl
      rm -f conf.sdltest
      AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

]])],, no_sdl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       CXXFLAGS="$ac_save_CXXFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL_CONFIG" = "no" ; then
       echo "*** The sdl-config script installed by SDL could not be found"
       echo "*** If SDL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL_CONFIG environment variable to the"
       echo "*** full path to sdl-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL test program, checking why..."
          CFLAGS="$CFLAGS $SDL_CFLAGS"
          CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SDL_LIBS"
          AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
]],     [[ return 0; ]])],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL or finding the wrong"
          echo "*** version of SDL. If it is not finding SDL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL was incorrectly installed"
          echo "*** or that you have moved SDL since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl-config script: $SDL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          CXXFLAGS="$ac_save_CXXFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL_CFLAGS=""
     SDL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  rm -f conf.sdltest
])
