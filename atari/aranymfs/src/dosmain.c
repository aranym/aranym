/*
	@(#)cookiefs/dosmain.c

	Copyright (c) Julian F. Reschke, 28. November 1995
	All rights reserved
*/

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "errno.h"

extern initfun ();
extern fs_dfree (), fs_fsfirst (), fs_fsnext ();
extern fs_fopen (), fs_fclose (), fs_fdatime ();
extern fs_fread (), fs_fwrite (), fs_fattrib ();
extern fs_dcreate (), fs_ddelete (), fs_frename ();
extern fs_fcreate (), fs_fseek (), fs_fcntl ();
extern fs_dpathconf (), fs_dopendir (), fs_dreaddir ();
extern fs_dxreaddir (), fs_dclosedir (), fs_drewinddir ();
extern fs_fxattr (), fs_dreadlabel (), fs_fdelete ();

int errno;

#define DEVNAME	"Fanda040 Host-Filesystem"
#define VERSION	"1.00"

char DriverName[] = DEVNAME" "VERSION;

char *
sccsid (void)
{
	return "@(#)"DEVNAME" "VERSION", Copyright (c) ARAnyM Development Team, "__DATE__;
}

typedef struct
{
	int	dummy;
} LOGICAL_DEV;

typedef struct
{
	int index;
	int mode;
	int flags;
	long cookieval;
	long offset;
	int device;
} MYFILE;

#if (sizeof (MYFILE)) > (8 * sizeof (long))
#error MYFILE too big
#endif

#define FILESIZE	4

/* Diverse Utility-Funktionen */

static int
Bconws (char *str)
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

/* Zugriff auf den Cookie-Jar */

typedef struct
{
	union {
		char ascii[4];
		long val;
	} tag;
	long val;
} COOKIE;

#define _p_cookies	((COOKIE **) 0x5a0L)

static char *
cookie_file (int index, char *tmp)
{
	int cnt = 0;
	COOKIE *c = *_p_cookies;

	if (!c) return 0L;
	
	while (c[cnt].tag.val) cnt += 1;
	
	if (index >= cnt) return NULL;
	
	strncpy (tmp, c[index].tag.ascii, 4);
	tmp[4] = '\0';

	/* Maccel? */
	if (c[index].tag.val == 0xAA006EL)
		strcpy (tmp, "macc");

	return tmp;
}


typedef struct          /* used by Fsetdta, Fgetdta */
{
	unsigned char	ds_attr;
	unsigned char	ds_index;
	unsigned char	ds_ast;
    char            ds_name[18];
    unsigned char   d_attrib;
    unsigned int    d_time;
    unsigned int    d_date;
    unsigned long   d_length;
    char            d_fname[14];
} myDTA;

static char *
tune_pn (char *pathname, int *drive)
{
	if (drive) *drive = toupper(pathname[0]) - 'A';
	
	pathname += 2; /* skip : */
	if (pathname[0] == '\\') pathname += 1;

	return pathname;
}

/* Funktionen */

long cdecl
DiskFree (LOGICAL_DEV *ldp, char *pathname, void *drp,
	long ret, int opcode, DISKINFO *buf, int drive)
{
	COOKIE *c = *_p_cookies;
	int cnt = 0;
	long total;

	(void)ldp,pathname,drp,ret,opcode,drive;
	
	while (c[cnt].tag.val) cnt++;
	total = c[cnt].val;
	
	buf->b_free = total - cnt;
	buf->b_total = total;
	buf->b_clsiz = 2;
	buf->b_secsiz = 4;
	
	return E_OK;
}

long cdecl
CreateFile (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, char *pn, int mode)
{
	(void)ldp,ret,opcode,pn,mode,fp,pathname;

	return EACCDN;
}


static long
locate_file (const char *template, int case_sens, MYFILE *fp)
{
	int i = 0;
	char tmp[6];
	
	while (NULL != cookie_file (i, tmp))
	{
		int no_match = case_sens
			? strcmp (template, tmp)
			: stricmp (template, tmp);
	
		if (! no_match)
		{
			fp->index = i + 1;
			
			/* Dateiinhalt */
			{
				COOKIE *c = *_p_cookies;

				fp->cookieval = c[i].val;
			}
			
			return E_OK;
		}
		
		i += 1;
	}

	return EFILNF;
}


long cdecl
OpenFile (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, char *pn, int mode)
{
	int drive;

	(void)ldp,ret,opcode,pn;

	pathname = tune_pn (pathname, &drive);

	fp->mode = mode;
	fp->offset = 0;
	fp->device = drive;

	/* zuerst Case-sensitiv suchen */

	if (E_OK == locate_file (pathname, 1, fp))
		return E_OK;
	else
		return locate_file (pathname, 0, fp);
}

long cdecl
CloseFile (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, int handle)
{
	(void)ret,opcode,handle,pathname,ldp,fp;

	return E_OK;
}

long cdecl
FDelete (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, char *pn)
{
	int drive;
	MYFILE F;

	(void)ldp,ret,opcode,pn,fp;

	pathname = tune_pn (pathname, &drive);
	
	/* zuerst Case-sensitiv suchen */

	ret = locate_file (pathname, 1, &F);
	if (ret != E_OK) ret = locate_file (pathname, 0, &F);
	
	if (ret != E_OK) return EFILNF;
	
	/* dann wirklich l”schen */
	{
		COOKIE *p = *_p_cookies;
		int index = F.index - 1;
		
		do
		{
			p[index] = p[index + 1];
					
			index += 1;
		} while (p[index].tag.val != 0);
	}

	return E_OK;
}


long cdecl
ReadFile (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, int handle,
	long count, void *buffer)
{
	long tc = count;

	(void)ldp,ret,opcode,handle,pathname;
	
	if (tc > FILESIZE - fp->offset) tc = FILESIZE - fp->offset;
	
	memcpy (buffer, (void *)(&fp->cookieval + fp->offset), tc);
	fp->offset += tc;

	return tc;
}

long cdecl
WriteFile (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, int handle,
	long count, void *buffer)
{
	(void)ldp,ret,opcode,handle,pathname;
	(void)buffer,count,fp;
	
	return 0L;
}

long cdecl
SeekFile (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, long offset, int handle,
	int seekmode)
{
	long newoff;

	(void)ldp,ret,opcode,handle,pathname;
	
	switch (seekmode)
	{
		case 0:
			newoff = offset;
			break;
					
		case 1:
			newoff = fp->offset + offset;
			break;
			
		case 2:
			newoff = FILESIZE + offset;
			break;

		default:
			return EINVFN;
	}
	
	if (newoff < 0 || newoff > FILESIZE)
		return ERANGE;
	
	return fp->offset = newoff;
}

long cdecl
FileAttributes (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, char *pn, int wflag, int attr)
{
	(void)ldp,ret,opcode;
	(void)attr,pn,wflag,fp,pathname;
	
	return 0;
}

static int
matchname (const char *str1, const char *str2, int flg)
{
	int i;
	
	for (i = 0; i < strlen (str1); i++)
	{
		if (str1[i] != '?')
			if (toupper (str1[i]) != toupper (str2[i])) return 0;
	}
	
	if (i == strlen (str2)) return 1;
	
	return flg;
}

static long
search (LOGICAL_DEV *ldp, myDTA *dta)
{
	char tmp[6];

	(void) ldp;

	if (dta->ds_attr == FA_VOLUME) return ENMFIL;

	while (NULL != cookie_file (dta->ds_index++, tmp))
	{
		if (matchname (dta->ds_name, tmp, dta->ds_ast))
		{
			strcpy (dta->d_fname, tmp);
			strupr (dta->d_fname);	
			
			dta->d_attrib = 0;
			dta->d_time = 0;
			dta->d_date = 0;
			dta->d_length = FILESIZE;
				
			return E_OK;
		}
	}
	
	return ENMFIL;
}

long cdecl
SearchFirst (LOGICAL_DEV *ldp, char *pathname, myDTA *dta,
	long ret, int opcode, const char *pn, int attribs)
{
	char *c;
	int drive;

	(void)ldp,ret,opcode,pn;

	dta->ds_attr = attribs;
	dta->ds_index = 0;
	dta->ds_ast = 0;
	
	pathname = tune_pn (pathname, &drive);
	
	if (strchr (pathname, '\\'))
		return EFILNF;
	
	c = strchr (pathname, '*');
	if (c) {
		*c = 0;
		dta->ds_ast = 1;
	}

	strcpy (dta->ds_name, pathname);
	
	return search (ldp, dta);
}

long cdecl
SearchNext (LOGICAL_DEV *ldp, char *pathname, myDTA *dta,
	long ret, int opcode)
{
	(void)ldp,ret,opcode,pathname;

	return search (ldp, dta);
}

long cdecl
FXAttr (LOGICAL_DEV *ldp, char *pathname, myDTA *dta,
	long ret, int opcode, int flag, const char *name, XATTR *xap)
{
	char tmp[5];
	int index = 0;
	int drive;

	(void)ldp,opcode,flag,name,ret,dta;

	pathname = tune_pn (pathname, &drive);

	if (pathname[0] == '\0') /* root dir? */
	{
		memset (xap, 0, sizeof (XATTR));
	
		xap->mode = S_IFDIR|S_IRUSR|S_IRGRP|S_IROTH;
		xap->index = 0;
		xap->dev = drive;
		xap->nlink = 1;
		xap->size = 0;
		xap->blksize = sizeof (COOKIE);
		xap->nblocks = 1;
		xap->attr = FA_READONLY|FA_VOLUME;
				
		return E_OK;
	}

	while (NULL != cookie_file (index++, tmp))
	{
		if (!strcmp (tmp, pathname))
		{
			memset (xap, 0, sizeof (XATTR));
	
			xap->mode = S_IFREG|S_IRUSR|S_IRGRP|S_IROTH;
			xap->index = index;
			xap->dev = drive;
			xap->nlink = 1;
			xap->size = FILESIZE;
			xap->blksize = sizeof (COOKIE);
			xap->nblocks = 1;
			xap->attr = FA_READONLY;
				
			return E_OK;
		}
	}

	return EFILNF;
}

long cdecl
DateAndTime (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, DOSTIME *timeptr, int handle, int wflag)
{
	(void)ldp,ret,opcode,pathname,handle,wflag,timeptr,fp;
	
	return EBADRQ;	
}

long cdecl
FCntl (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, int handle, void *arg, int cmd)
{
	(void)ldp,ret,opcode,pathname,handle;
	(void)cmd,arg,fp;
	
	return EINVFN;
}

long cdecl
DPathConf (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, const char *path, int cmd)
{
	(void) ldp,pathname,fp,ret,opcode,path;

	switch (cmd)
	{
		case -1:	return 7;
		case 0:		return 0x7fffffffL;
		case 1:		return 1;
		case 2:		return 4;
		case 3:		return 4;
		case 4:		return 0;
		case 5:		return 0;
		case 6:		return 0;
		case 7:		return 0x00800000L;
		default:	return EINVFN;
	}
}

long cdecl
DOpenDir (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, const char *path, int flag)
{
	int drive;

	(void) ldp,pathname,fp,ret,opcode,path;

	pathname = tune_pn (pathname, &drive);
	if (pathname[0] != '\0') return EPTHNF;	

	fp->index = 0;
	fp->mode = flag;
	fp->device = drive;

	return E_OK;
}

long cdecl
DCloseDir (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, long handle)
{
	(void) ldp,pathname,fp,ret,opcode,handle;

	return E_OK;
}


long cdecl
DRewindDir (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, long handle)
{
	(void) ldp,pathname,fp,ret,opcode,handle;

	fp->index = 0;

	return E_OK;
}

long cdecl
DXReadDir (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, int len, long handle, long *buf,
	XATTR *xap, long *xret)
{
	char tmp[5];
	int reclen = 5;
	
	(void) ldp,pathname,fp,ret,opcode,handle;

	if (NULL == cookie_file (fp->index++, tmp)) return ENMFIL;
	
	*xret = E_OK;
	memset (xap, 0, sizeof (XATTR));
	
	xap->mode = S_IFREG|S_IRUSR|S_IRGRP|S_IROTH;
	xap->index = fp->index;
	xap->dev = fp->device;
	xap->nlink = 1;
	xap->size = FILESIZE;
	xap->blksize = sizeof (COOKIE);
	xap->nblocks = 1;
	xap->attr = FA_READONLY;
	
	if (!fp->mode) reclen += (int) sizeof (long);
	if (reclen > len) return ERANGE;

	/* Insert file index if needed */
	if (!fp->mode) *buf++ = xap->index;

	if (fp->mode) strupr (tmp);
	strcpy ((char *)buf, tmp);

	return E_OK;
}

long cdecl
DReadDir (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, int len, long handle, long *buf)
{
	XATTR xa;
	long xr;

	return DXReadDir (ldp, pathname, fp,  ret, opcode,
		len, handle, buf, &xa, &xr);
}


long cdecl
DReadLabel (LOGICAL_DEV *ldp, char *pathname, MYFILE *fp,
	long ret, int opcode, const char *path, char *name, int size)
{
	(void) ldp,pathname,fp,ret,opcode,path;
#define LABEL "Cookies"

	strncpy (name, LABEL, size);
	name[size - 1] = '\0';
	
	return strlen (LABEL) >= size ? ERANGE : E_OK;
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
	-1L, -1L, (long)fs_dcreate, (long)fs_ddelete, -1L, /* 59 */
	(long) fs_fcreate, (long) fs_fopen, (long) fs_fclose,
	(long) fs_fread, (long) fs_fwrite,
	(long) fs_fdelete, (long) fs_fseek, (long) fs_fattrib, -1L, -1L, /* 69 */
	-1L, -1L, -1L, -1L, -1L,
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

void
ShowBanner (void)
{
	Bconws ("\033p "DEVNAME" "VERSION" \033q \n"
		"Copyright (c) ARAnyM Development Team, "__DATE__);
}

void * cdecl
InitDevice (int deviceid)
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

