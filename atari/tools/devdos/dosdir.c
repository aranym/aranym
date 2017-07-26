#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <osbind.h>

#include "bosmeta.h"
#include "list.h"
#include "dosdir.h"

#if 0
#include "nfd.h"
#define TRACE(x) NFD(x)
#define DEBUG(x) NFD(x)
#else
#define TRACE(x)
#define DEBUG(x)
#endif


long __CDECL
sys_dl_opendir (DIR *dirh, LIST *list, int flag)
{
	DEBUG(("dl_opendir: %lx list=%lx\n", dirh, list));
	dirh->list = list;
	dirh->mode = flag;
	dirh->current = (FCOOKIE*)listRewind(dirh->list);
	return 0;
}

long __CDECL
sys_dl_readdir (DIR *dirh, char *buf, int len)
{
	dirh->current = (FCOOKIE *)listNext((LINKABLE*)dirh->current);
	DEBUG(("dl_readdir: %lx list=%lx: len %d, '%s'\n", dirh, dirh->list, len, dirh->current ? dirh->current->name : "ENMFILES"));
	if (!dirh->current)
		return -ENMFILES;

	/* Insert file index if needed */
	if (!dirh->mode) {
		*(long*)buf = (long)dirh->current;
		buf += 4;
		len -= 4;
	}
	if ( len <= 0 )
		return -ERANGE;

	strncpy(buf, dirh->current->name, len-1);
	return 0;
}

long __CDECL
sys_dl_closedir (DIR *dirh)
{
	DEBUG(("dl_closedir: %lx list=%lx\n", dirh, dirh->list));
	dirh->list = NULL;
	dirh->current = NULL;
	return 0;
}

long __CDECL
sys_dl_readlabel (char *buf, int buflen)
{
	strncpy(buf, "nfstderr", buflen-1);
	return 0;
}


long __CDECL
sys_d_free (MetaDOSDir long *buf, int d)
{
#if 1
	return -ENOSYS;
#else
	PROC *p = curproc;
	fcookie *dir = 0;

	d = (int)tolower(pathNameMD[0])-'a';
	dir = &p->p_cwd->root[d];
#endif
}

long __CDECL
sys_d_create (MetaDOSDir const char *path)
{
	return -EACCES;
}

long __CDECL
sys_d_delete (MetaDOSDir const char *path)
{
	return -EACCES;
}


long __CDECL
sys_f_xattr (MetaDOSFile int flag, const char *name, struct xattr *xattr)
{
	FCOOKIE *fc;
	long r = path2cookie (name, NULL, &fc);
	if (r)
	{
		DEBUG(("Fattrib(%s): error %ld", name, r));
		return r;
	}

	return getxattr( fc, xattr);
}

long __CDECL
sys_f_attrib (MetaDOSFile const char *name, int rwflag, int attr)
{
	FCOOKIE *fc;
	long r = path2cookie (name, NULL, &fc);
	if (r)
	{
		DEBUG(("Fattrib(%s): error %ld", name, r));
		return r;
	}

	return fc->attr;
}

long __CDECL
sys_f_delete (MetaDOSFile const char *name)
{
	return -EACCES;
}

long __CDECL
sys_f_rename (MetaDOSFile int junk, const char *old, const char *new)
{
	return -EACCES;
}

/*
 * GEMDOS extension: Dpathconf(name, which)
 *
 * returns information about filesystem-imposed limits; "name" is the name
 * of a file or directory about which the limit information is requested;
 * "which" is the limit requested, as follows:
 *  -1  max. value of "which" allowed
 *  0   internal limit on open files, if any
 *  1   max. number of links to a file  {LINK_MAX}
 *  2   max. path name length       {PATH_MAX}
 *  3   max. file name length       {NAME_MAX}
 *  4   no. of bytes in atomic write to FIFO {PIPE_BUF}
 *  5   file name truncation rules
 *  6   file name case translation rules
 *
 * unlimited values are returned as 0x7fffffffL
 *
 * see also Sysconf() in dos.c
 */
long __CDECL
sys_d_pathconf (MetaDOSDir const char *name, int which)
{
	/* FIXME? now from cookfs */
	switch (which)
	{
		case -1:	return 7;
		case 0:		return 0x7fffffffL;
		case 1:		return 1;
		case 2:		return PATH_MAX;
		case 3:		return PATH_MAX;
		case 4:		return 0;
		case 5:		return 0;
		case 6:		return 0;
		case 7:		return 0x00800000L;
		default:	return -ENOSYS;
	}
}

long __CDECL
sys_d_opendir (MetaDOSDir const char *name, int flag)
{
	DIR *dirh = (DIR *) dirMD;
	FCOOKIE *fc;
	long r = path2cookie( name, NULL, &fc);
	if (r)
	{
		DEBUG(("Dopendir(%s): path2cookie returned %ld", name, r));
		return r;
	}

	r = sys_dl_opendir( dirh, fc->folder, flag);
	if (r)
		return r;

	return (long)dirh;
}

long __CDECL
sys_d_readdir (MetaDOSDir int len, long handle, char *buf)
{
	DIR *dirh = (DIR *) dirMD;
	return sys_dl_readdir( dirh, buf, len);
}

long __CDECL
sys_d_xreaddir (MetaDOSDir int len, long handle, char *buf, struct xattr *xattr, long *xret)
{
	DIR *dirh = (DIR *) dirMD;
	long ret = sys_dl_readdir( dirh, buf, len);
	if ( ret )
		return ret;

	*xret = getxattr( dirh->current, xattr);
	return 0;
}

long __CDECL
sys_d_rewind (MetaDOSDir long handle)
{
	DIR *dirh = (DIR *) dirMD;
	dirh->current = (FCOOKIE *)listRewind(dirh->list);
	return 0;
}

long __CDECL
sys_d_closedir (MetaDOSDir long handle)
{
	return sys_dl_closedir( (DIR*)dirMD);
}

long __CDECL
sys_d_readlabel (MetaDOSDir const char *name, char *buf, int buflen)
{
	return sys_dl_readlabel(buf, buflen);
}

long __CDECL
sys_d_writelabel (MetaDOSDir const char *name, const char *label)
{
	return -ENOSYS;
}

