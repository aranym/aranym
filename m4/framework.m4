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
      [AS_VAR_SET(ac_Framework, yes); $1_LIBS=" -framework $1"], [AS_VAR_SET(ac_Framework, no)]
    )
    LIBS="$saved_LIBS"
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

