/*
 * Example of NatFeat usage - detecting ARAnyM presence
 * Written by Petr Stehlik in 2003 and placed into Public Domain
 */
#include <stdio.h>				/* for printf */
#include <tos.h>				/* for gemdos and Super */
#include <string.h>				/* for strcmp */

/* compiler and library compatibility defines */
#if __PUREC__
#define CDECL cdecl
#define compare_strings strcmpi
#else
#define CDECL
#define compare_strings strcasecmp
#endif

#ifndef TRUE
#define TRUE	(1==1)
#define FALSE	(!TRUE)
#endif

/* TOS Cookie Jar */
typedef struct {
	long cookie;
	long value;
} COOKIE;

int getcookie(long target, long *p_value)
{
	if (gemdos(340, -1, 0L, 0L) == 0) {
		/* Ssystem is available */
		long result = gemdos(340, 8, target, NULL);
		if (result != -1L) {
			*p_value = result;
			return TRUE;
		}
		return FALSE;
	}
	else {
		char *oldssp;
		COOKIE *cookie_ptr;

		oldssp = (char *) Super(0L);

		cookie_ptr = *(COOKIE **) 0x5A0;

		if (oldssp)
			Super(oldssp);

		if (cookie_ptr != NULL) {
			do {
				if (cookie_ptr->cookie == target) {
					if (p_value != NULL)
						*p_value = cookie_ptr->value;

					return TRUE;
				}
			} while ((cookie_ptr++)->cookie != 0L);
		}
	}

	return FALSE;
}

/* NatFeat code */
typedef struct {
	long magic;
	long CDECL(*nfGetID) (const char *);
	long CDECL(*nfCall) (long ID, ...);
} NatFeatCookie;

/* return NatFeat pointer if NatFeat initialization was successful,
 * otherwise NULL
 */
NatFeatCookie *initNatFeats(void)
{
	NatFeatCookie *nf_ptr = NULL;

	if (getcookie(0x5f5f4e46L, &(long) nf_ptr) == 0) {	/* "__NF" */
		puts("NatFeat cookie not found");
		return NULL;
	}

	if (nf_ptr->magic != 0x20021021L) {		/* NatFeat magic constant */
		puts("NatFeat cookie magic value does not match");
		return NULL;
	}

	return nf_ptr;
}

/* get name (fill 'buffer' with the `NF_NAME` value, up to 'size' chars) */
long nfGetName(NatFeatCookie * nf_ptr, char *buffer, long size)
{
	static unsigned long nf_name_id = 0;
	if (nf_name_id == 0)
		nf_name_id = nf_ptr->nfGetID("NF_NAME");
	return nf_ptr->nfCall(nf_name_id, buffer, size);
}

/* return TRUE if ARAnyM was detected, FALSE otherwise */
int isARAnyM(void)
{
	NatFeatCookie *nf_ptr = NULL;
	char buf[80] = "";

	nf_ptr = initNatFeats();
	if (nf_ptr == NULL)
		return FALSE;

	nfGetName(nf_ptr, buf, sizeof(buf));

	return !compare_strings(buf, "ARAnyM");
}

/* example of use */
int main()
{
	if (isARAnyM()) {
		printf("This is ARAnyM\n");
		return 1;
	}

	return 0;
}
