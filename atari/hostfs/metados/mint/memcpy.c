/*
 * memcpy.c
 */

#include "mint/kcompiler.h"
#include "mint/string.h"

void *memcpy(void *dest, const void *src, size_t n)
{
	register char *dp = dest;
	register const char *sp = src;
	while (n--)
		*dp++ = *sp++;
	return dest;
}
