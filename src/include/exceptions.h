/*
 *
 * exceptions
 *
 * Public domain
 */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

class bus_error {
    public:
        uaecptr where;
	uaecptr why;
	bus_error(uaecptr why) : why(why) { }
};

class access_error {
    public:
        uaecptr where;
	uaecptr why;
	access_error(uaecptr why) : why(why) { }
};

#include <setjmp.h>
#include "sysdeps.h"

extern jmp_buf excep_env;
extern uint32  excep_no;
#endif
