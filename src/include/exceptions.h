/*
 *
 * exceptions
 *
 * Public domain
 */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "sysdeps.h"
#include <setjmp.h>

extern jmp_buf excep_env;
extern uint32  excep_no;
#endif
