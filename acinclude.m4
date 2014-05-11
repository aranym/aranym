dnl Function to check for Mac OS X frameworks (stolen from Basilisk)
dnl ARANYM_CHECK_FRAMEWORK($1=NAME, $2=INCLUDES)
AC_DEFUN([ARANYM_CHECK_FRAMEWORK], [
  AS_VAR_PUSHDEF([ac_Framework], [aranym_cv_have_framework_$1])dnl
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
  have_framework_$1=AS_VAR_GET(ac_Framework)
  AS_VAR_POPDEF([ac_Framework])dnl
])


dnl Find location of an existing Mac OS X framework
dnl ARANYM_CHECK_FRAMEWORK_LOCATION($1=NAME)
AC_DEFUN([ARANYM_CHECK_FRAMEWORK_LOCATION], [
	AS_VAR_PUSHDEF([ac_Framework_Location], [aranym_cv_$1_location])dnl
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
	$1_LOCATION=AS_VAR_GET(ac_Framework_Location)
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

AC_DEFUN([gl_TM_GMTOFF],
[
 AC_CHECK_MEMBERS([struct tm.tm_gmtoff],
                 [], [],
                 [#include <time.h>])
])

# Checks for stat-related time functions.
# Borrowed from gnulib module of coreutils

AC_DEFUN([gl_STAT_TIME],
[
  AC_CHECK_HEADERS([sys/time.h])

  AC_CHECK_MEMBERS([struct stat.st_atim.tv_nsec],
    [AC_CACHE_CHECK([whether struct stat.st_atim is of type struct timespec],
       [ac_cv_typeof_struct_stat_st_atim_is_struct_timespec],
       [AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
          [[
            #include <sys/types.h>
            #include <sys/stat.h>
            #if HAVE_SYS_TIME_H
            # include <sys/time.h>
            #endif
            #include <time.h>
            struct timespec ts;
            struct stat st;
          ]],
          [[
            st.st_atim = ts;
          ]])],
          [ac_cv_typeof_struct_stat_st_atim_is_struct_timespec=yes],
          [ac_cv_typeof_struct_stat_st_atim_is_struct_timespec=no])])
     if test $ac_cv_typeof_struct_stat_st_atim_is_struct_timespec = yes; then
       AC_DEFINE([TYPEOF_STRUCT_STAT_ST_ATIM_IS_STRUCT_TIMESPEC], [1],
         [Define to 1 if the type of the st_atim member of a struct stat is
          struct timespec.])
     fi],
    [AC_CHECK_MEMBERS([struct stat.st_atimespec.tv_nsec], [],
       [AC_CHECK_MEMBERS([struct stat.st_atimensec], [],
          [AC_CHECK_MEMBERS([struct stat.st_atim.st__tim.tv_nsec], [], [],
             [#include <sys/types.h>
              #include <sys/stat.h>])],
          [#include <sys/types.h>
           #include <sys/stat.h>])],
       [#include <sys/types.h>
        #include <sys/stat.h>])],
    [#include <sys/types.h>
     #include <sys/stat.h>])
])
