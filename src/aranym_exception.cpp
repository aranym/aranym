#include <stdarg.h>
#include <stdio.h>

#include "aranym_exception.h"

AranymException::AranymException(char *fmt, ...)
{
	va_list argptr;
	
	va_start(argptr, fmt);
	vsnprintf(errMsg, sizeof(errMsg)-1, fmt, argptr);
	va_end(argptr);

	errMsg[sizeof(errMsg)-1]='\0';
}

AranymException::~AranymException()
{
}

char *AranymException::getErrorMessage(void)
{
	return &errMsg[0];
}
