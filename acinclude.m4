dnl Function to check for Mac OS X frameworks (stolen from Basilisk)
dnl ARANYM_CHECK_FRAMEWORK($1=NAME, $2=INCLUDES)
AC_DEFUN([ARANYM_CHECK_FRAMEWORK], [
  AS_VAR_PUSHDEF([ac_Framework], [have_framework_$1])dnl
  AC_CACHE_CHECK([whether compiler supports framework $1],
    ac_Framework, [
    saved_LIBS="$LIBS"
    LIBS="$LIBS -framework $1"
    AC_TRY_LINK(
      [$2], [],
      [AS_VAR_SET(ac_Framework, yes)], [AS_VAR_SET(ac_Framework, no); LIBS="$saved_LIBS"]
    )
  ])
  AS_IF([test AS_VAR_GET(ac_Framework) = yes],
    [AC_DEFINE(AS_TR_CPP(HAVE_FRAMEWORK_$1), 1, [Define if framework $1 is available.])]
  )
  AS_VAR_POPDEF([ac_Framework])dnl
])


dnl Find location of an existing Mac OS X framework
dnl ARANYM_CHECK_FRAMEWORK_LOCATION($1=NAME)
AC_DEFUN([ARANYM_CHECK_FRAMEWORK_LOCATION], [
	AS_VAR_PUSHDEF([ac_Framework_Location], [$1_LOCATION])dnl
	AC_CACHE_CHECK([location of $1 framework],
		ac_Framework_Location, [
		AS_VAR_SET(ac_Framework_Location, "")
		echo "#error dummy" > conftest.c
		pos_locations=`$CC -v conftest.c -framework $1 2>&1 | grep -e "> search" -A 999 | grep -e "End of " -B 999 | grep -e "^ "`
		rm -f conftest.c
		for pos_location in $FRAMEWORK_SEARCH_PATHS $pos_locations ; do
			pos_location="`echo $pos_location | sed 's/^"//' | sed 's/"$//'`"
			framework_loc="$pos_location/$1.framework"
			if test -d "$framework_loc" ; then
				AS_VAR_SET(ac_Framework_Location, "$framework_loc")
				break;
			fi
		done
	])
	if test -z AS_VAR_GET(ac_Framework_Location) ; then
		AC_MSG_ERROR([$1 framework not found])
	fi
	AS_VAR_POPDEF([ac_Framework_Location])dnl
])

dnl Check whether the compiler recognizes bool

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

dnl Define a macro that translates a yesno-variable into a C macro definition
dnl to be put into the config.h file
dnl $1 -- the macro to define
dnl $2 -- the value to translate
dnl $3 -- template name

AC_DEFUN([AC_TRANSLATE_DEFINE], [
    if [[ "x$2" = "xyes" -o "x$2" = "xguessing yes" ]]; then
        AC_DEFINE([$1], 1, [$3])
    fi
])

dnl Written by Laurynas Biveinis
dnl GNU Autoconf macro AC_CXX_OPT(option)
dnl Checks if C++ compiler supports specified option.
dnl If yes - adds that option to CXXFLAGS.

AC_DEFUN([AC_CXX_OPT],
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
	if test $ac_cv_c_bigendian = universal; then
	    fbigend=WORDS_BIGENDIAN
	fi
	;;
    'IEEE (little-endian)' )
	if test $ac_cv_c_bigendian = yes; then
	    fbigend=0
	fi
	if test $ac_cv_c_bigendian = universal; then
	    fbigend='!WORDS_BIGENDIAN'
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

dnl Check for Mesa or OpenGL libraries

AC_DEFUN([MDL_HAVE_OPENGL],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PATH_X])
  AC_REQUIRE([AC_PATH_XTRA])

  dnl Check for OpenGL framework
  saved_LIBS="$LIBS"
  ARANYM_CHECK_FRAMEWORK(OpenGL, [#include <OpenGL/OpenGL.h>])
  LIBS="$saved_LIBS"

  AC_CACHE_CHECK([for OpenGL], mdl_cv_have_OpenGL,
  [
    if test "x$have_framework_OpenGL" = "xyes" ; then
      have_GL=yes
      have_GLU=yes
      have_GLX=no
      GL_LIBS="-framework OpenGL"

      AC_SUBST(GL_CFLAGS)
      AC_SUBST(GL_LIBS)
	
    elif test x"$OS_TYPE" = xcygwin; then
      mdl_cv_have_OpenGL=yes

      have_GL=yes
      have_GLU=yes
      have_GLX=no
      GL_LIBS="-lopengl32 -lglu32"

      AC_SUBST(GL_CFLAGS)
      AC_SUBST(GL_LIBS)
    else 

    if test x"$use_Mesa" = xyes; then
      GL_search_list="MesaGL   GL"
      GLU_search_list="MesaGLU GLU"
      GLX_search_list="MesaGLX GLX"
    else
      GL_search_list="GL MesaGL"
      GLU_search_list="GLU MesaGLU"
      GLX_search_list="GLX MesaGLX"
    fi

    AC_LANG_SAVE
    AC_LANG_C

dnl If we are running under X11 then add in the appropriate libraries.
if test x"$no_x" != xyes; then
dnl Add everything we need to compile and link X programs to GL_X_CFLAGS
dnl and GL_X_LIBS.
  GL_CFLAGS="$X_CFLAGS"
  GL_X_LIBS="$X_PRE_LIBS $X_LIBS -lX11 -lXext -lXmu -lXt -lXi $X_EXTRA_LIBS"
fi

    GL_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$GL_CFLAGS"

    GL_save_LIBS="$LIBS"
    LIBS="$GL_X_LIBS"


    # Save the "AC_MSG_RESULT file descriptor" to FD 8.
    exec 8>&AC_FD_MSG

    # Temporarily turn off AC_MSG_RESULT so that the user gets pretty
    # messages.
    exec AC_FD_MSG>/dev/null

    AC_SEARCH_LIBS(glAccum,          $GL_search_list, have_GL=yes,   have_GL=no)
    AC_SEARCH_LIBS(gluBeginCurve,   $GLU_search_list, have_GLU=yes,  have_GLU=no)
    AC_SEARCH_LIBS(glXChooseVisual, $GLX_search_list, have_GLX=yes,  have_GLX=no)

    # Restore pretty messages.
    exec AC_FD_MSG>&8

    if test -n "$LIBS"; then
      mdl_cv_have_OpenGL=yes
      GL_LIBS="$LIBS"
      AC_SUBST(GL_CFLAGS)
      AC_SUBST(GL_LIBS)
    else
      mdl_cv_have_OpenGL=no
      GL_CFLAGS=
    fi

dnl Reset GL_X_LIBS regardless, since it was just a temporary variable
dnl and we don't want to be global namespace polluters.
    GL_X_LIBS=

    LIBS="$GL_save_LIBS"
    CPPFLAGS="$GL_save_CPPFLAGS"

    AC_LANG_RESTORE

dnl if !cygwin
    fi
   
dnl bugfix: dont forget to cache this variables, too
    mdl_cv_GL_CFLAGS="$GL_CFLAGS"
    mdl_cv_GL_LIBS="$GL_LIBS"
    mdl_cv_have_GL="$have_GL"
    mdl_cv_have_GLU="$have_GLU"
    mdl_cv_have_GLX="$have_GLX"
  ])
  GL_CFLAGS="$mdl_cv_GL_CFLAGS"
  GL_LIBS="$mdl_cv_GL_LIBS"
  have_GL="$mdl_cv_have_GL"
  have_GLU="$mdl_cv_have_GLU"
  have_GLX="$mdl_cv_have_GLX"
dnl endof bugfix -ainan
])

dnl Check for zlib
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/check_zlib.html

AC_DEFUN([CHECK_ZLIB],
#
# Handle user hints
#
[AC_MSG_CHECKING(if zlib is wanted)
AC_ARG_WITH(zlib,
[  --with-zlib=DIR         root directory path of zlib installation [defaults 
                          to /usr/local or /usr if not found in /usr/local]
  --without-zlib          disable zlib usage completely],
[if test "$withval" != no ; then
  AC_MSG_RESULT(yes)
  ZLIB_HOME="$withval"
  have_zlib="yes"
else
  AC_MSG_RESULT(no)
  have_zlib="no"
fi], [
AC_MSG_RESULT(yes)
have_zlib="yes"
ZLIB_HOME=/usr/local
if test ! -f "${ZLIB_HOME}/include/zlib.h"
then
        ZLIB_HOME=/usr
fi
])

#
# Locate zlib, if wanted
#
if test -n "${ZLIB_HOME}"
then
        ZLIB_OLD_LDFLAGS=$LDFLAGS
        ZLIB_OLD_CPPFLAGS=$LDFLAGS
        LDFLAGS="$LDFLAGS -L${ZLIB_HOME}/lib"
        CPPFLAGS="$CPPFLAGS -I${ZLIB_HOME}/include"
        AC_LANG_SAVE
        AC_LANG_C
        AC_CHECK_LIB(z, inflateEnd, [zlib_cv_libz=yes], [zlib_cv_libz=no])
        AC_CHECK_HEADER(zlib.h, [zlib_cv_zlib_h=yes], [zlib_cvs_zlib_h=no])
        AC_LANG_RESTORE
        if test "$zlib_cv_libz" = "yes" -a "$zlib_cv_zlib_h" = "yes"
        then
                #
                # If both library and header were found, use them
                #
                AC_CHECK_LIB(z, inflateEnd)
                AC_MSG_CHECKING(zlib in ${ZLIB_HOME})
                AC_MSG_RESULT(ok)
		have_zlib="yes"
        else
                #
                # If either header or library was not found, revert and bomb
                #
                AC_MSG_CHECKING(zlib in ${ZLIB_HOME})
                LDFLAGS="$ZLIB_OLD_LDFLAGS"
                CPPFLAGS="$ZLIB_OLD_CPPFLAGS"
                AC_MSG_RESULT(failed)
		have_zlib="no"
                AC_MSG_ERROR(either specify a valid zlib installation with --with-zlib=DIR or disable zlib usage with --without-zlib)
        fi
fi

])
