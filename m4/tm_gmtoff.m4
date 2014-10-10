
AC_DEFUN([gl_TM_GMTOFF],
[
 AC_CHECK_MEMBERS([struct tm.tm_gmtoff],
                 [], [],
                 [#include <time.h>])
])
