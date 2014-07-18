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

