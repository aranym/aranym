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

