/**
 * $Header$
 *
 * 2001 STanda
 **/

#include <tos.h>

extern initfun ();
extern fs_dfree (), fs_fsfirst (), fs_fsnext ();
extern fs_fopen (), fs_fclose (), fs_fdatime ();
extern fs_fread (), fs_fwrite (), fs_fattrib ();
extern fs_dcreate (), fs_ddelete (), fs_frename ();
extern fs_fcreate (), fs_fseek (), fs_fcntl ();
extern fs_dpathconf (), fs_dopendir (), fs_dreaddir ();
extern fs_dgetpath (), fs_dsetpath ();
extern fs_dxreaddir (), fs_dclosedir (), fs_drewinddir ();
extern fs_fxattr (), fs_dreadlabel (), fs_fdelete ();


#define DEVNAME	"ARAnyM HostOS Filesystem"
#define VERSION	"0.50"

char DriverName[] = DEVNAME" "VERSION;

/*
char* sccsid(void)
{
	return "@(#)"DEVNAME" "VERSION", Copyright (c) ARAnyM Development Team, "__DATE__;
}
*/

typedef struct
{
	int	dummy;
} LOGICAL_DEV;


/* Diverse Utility-Funktionen */

static int Bconws( char *str )
{
	int cnt = 0;
	
	while (*str)
	{
		cnt++;
		
		if (*str == '\n') {
			Bconout (2, '\r');
			cnt++;
		}
		
		Bconout (2, *str++);
	}
	
	return cnt;
}


long FunctionTable[] =
{
	'MAGI', 'CMET', 349,
	(long) initfun,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L, /* 49 */
	-1L, -1L, -1L, -1L, (long) fs_dfree,
	-1L, -1L, (long)fs_dcreate, (long)fs_ddelete, (long)fs_dsetpath, /* 59 */
	(long) fs_fcreate, (long) fs_fopen, (long) fs_fclose,
	(long) fs_fread, (long) fs_fwrite,
	(long) fs_fdelete, (long) fs_fseek, (long) fs_fattrib, -1L, -1L, /* 69 */
	-1L, (long)fs_dgetpath, -1L, -1L, -1L,
	-1L, -1L, -1L, (long) fs_fsfirst, (long) fs_fsnext,
	-1L, -1L, -1L, -1L, -1L,
	-1L, (long)fs_frename, (long) fs_fdatime, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 99 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 109 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 119 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 129 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 139 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 149 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 159 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 169 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 179 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 189 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 199 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 209 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 219 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 229 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 239 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 249 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 259 */
	(long) fs_fcntl, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 269 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 279 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 289 */
	-1L, -1L, (long) fs_dpathconf, -1L, -1L, -1L, (long) fs_dopendir,
	(long) fs_dreaddir, (long) fs_drewinddir, (long) fs_dclosedir, /* 299 */
	(long) fs_fxattr, -1L, -1L, -1L, -1L,
	-1L, -1L, -1L, -1L, -1L, /* 309 */
	-1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 319 */
	-1L, -1L, (long) fs_dxreaddir, -1L, -1L, -1L, -1L, -1L, -1L, -1L, /* 329 */
	-1L, -1L, -1L, -1L, -1L,  /* 334 */
	-1L, -1L, -1L, (long) fs_dreadlabel, -1,  /* 339 */
	-1L, -1L, -1L, -1L, -1L,  /* 344 */
	-1L, -1L, -1L, -1L, -1L,  /* 349 */
};

void ShowBanner( void )
{
	Bconws (
		"\n\033p "DEVNAME" "VERSION" \033q "
		"\nCopyright (c) ARAnyM Development Team, "__DATE__"\n"
	);
}

void* cdecl InitDevice( int deviceid )
{
	LOGICAL_DEV *ldp;

	(void)deviceid;

	ldp = Malloc (sizeof (LOGICAL_DEV));
	if (!ldp) {
		Bconws ("Not enough memory for buffers\n");
		return (void *)-1L;
	}
	
	return ldp;
}



/**
 * $Log$
 *
 **/