#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "bosmeta.h"
#include "dosdir.h"

#if 0
#include "nfd.h"
#define TRACE(x) NFD(x)
#define DEBUG(x) NFD(x)
#else
#define TRACE(x)
#define DEBUG(x)
#endif


/* x:\dev\phys folder lists respectively */
LIST root;

static LIST root_dev;
static LIST root_dev_phys;

/* cookie release/dup */
static
void release_cookie( FCOOKIE *fc);

static
void release_folder( LIST *f) {
	LINKABLE *fc = listFirst(f);
	while (fc) {
		FCOOKIE *trash = (FCOOKIE*)fc;
		fc = listNext(fc);
		release_cookie( trash);
	}
}

static
void release_cookie( FCOOKIE *fc) {
	release_folder( fc->folder);
	free( fc->name);
	free( fc);
}

static
FCOOKIE *dup_cookie( FCOOKIE *s) {
	FCOOKIE *fc;
	fc = malloc(sizeof(FCOOKIE));
	fc->name = strdup(s->name);
	fc->attr = s->attr;
	fc->folder = s->folder;
	fc->bos_dev = s->bos_dev;
	fc->bos_info_flags = s->bos_info_flags;
	return fc;
}

long bosfs_initialize(void) {
	int i;
	metainit_t metainit={0,0,0,0};
	FCOOKIE *fc;

	Metainit(&metainit);
	TRACE(("MetaDOS version=%s, drives=%x\n", metainit.version, metainit.drives_map));
	if (metainit.version == NULL) {
		DEBUG(("MetaDOS not installed\n"));
		return -1;
	}

	if (metainit.drives_map == 0) {
		DEBUG(("No MetaDOS devices present\n"));
		return -1;
	}

	listInit( &root);

	{ /* create the x:\dev folder */
		fc = malloc(sizeof(FCOOKIE));
		fc->name = strdup("dev");
		fc->attr = FA_DIR;
		fc->folder = &root_dev;
		fc->bos_dev = 0;

		/* add it to the folder */
		listInsert( root.head.next, (LINKABLE*)fc);
	}

	listInit( &root_dev);

	{ /* create the x:\dev\phys folder */
		fc = malloc(sizeof(FCOOKIE));
		fc->name = strdup("bos");
		fc->attr = FA_DIR;
		fc->folder = &root_dev_phys;
		fc->bos_dev = 0;

		/* add it to the folder */
		listInsert( root_dev.head.next, (LINKABLE*)fc);
	}

	listInit( &root_dev_phys);

	for (i='A'; i<='Z'; i++) if (metainit.drives_map & (1<<(i-'A'))) {
		metaopen_t metaopen;
		int handle;
		char name[] = "A";
		name[0] = i;

		fc = malloc(sizeof(FCOOKIE));
		fc->name = strdup(name);
		fc->attr = 0;
		fc->folder = NULL;
		fc->bos_dev = i;
		fc->bos_info_flags = BOS_INFO_DEVHIDDEN; /* by default hide from U:\dev */

		DEBUG(("dev: %s->%s\n", name, fc->name));

		/* attempt to get the bos_info */
		handle = Metaopen(i, &metaopen);
		if (handle == 0) {
			bos_info_t info;
			if ( ! Metaioctl(i, METADOS_IOCTL_MAGIC, METADOS_IOCTL_BOSINFO, &info)) {
				DEBUG(("ioctl: %s->%s %lx\n", name, fc->name, info.flags));
				fc->bos_info_flags = info.flags;
			}
			Metaclose(i);

			DEBUG(("dev: %s->%s %lx\n", name, fc->name, fc->bos_info_flags));

			/* the BOSINFO states that the device should not be visible
			 * in x:\dev\bos nor in x:\dev\bos */
			if ( fc->bos_info_flags & BOS_INFO_BOSHIDDEN ) {
				release_cookie( fc );
				continue;
			}

			/* insert the devices that have a decent name also to u:\dev */
			if ( ! (fc->bos_info_flags & BOS_INFO_DEVHIDDEN) && *metaopen.name ) {
				FCOOKIE *cfc = dup_cookie(fc);
				free( cfc->name);
				cfc->name = strdup(metaopen.name);
				listInsert( &root_dev.tail, (LINKABLE*)cfc);
			}
		}

		/* add it to the folder */
		listInsert( root_dev_phys.head.next, (LINKABLE*)fc);
	}

	return 0;
}


long
getxattr (FCOOKIE *fc, struct xattr *res)
{
	res->mode = 0666;
        res->mode |= fc->attr & FA_DIR ? (S_IFDIR|0111) : 0;

	res->index = (long)fc;
	res->dev = (long)&root;
	res->rdev = (long)&root;
	res->nlink = 1;
	res->uid = 0;
	res->gid = 0;
	res->size = 0;
	res->blksize = 512;
	res->nblocks = ( res->size + res->blksize - 1) >> 9;
	// FIXME!
	res->mtime = 0;
	res->mdate = 0;
	res->atime = 0;
	res->adate = 0;
	res->ctime = 0;
	res->cdate = 0;
	res->attr = fc->attr;
	res->reserved2 = 0;
	res->reserved3[0] = 0;
	res->reserved3[1] = 0;
	return 0;
}


long
name2cookie (LIST *folder, const char *name, FCOOKIE **res)
{
	char lowname[32];
	FCOOKIE* fc;

	DEBUG(( "name2cookie: name=%s\n", name));

	/* look for 'name' in the folder */
	listForEach( FCOOKIE*, fc, folder) {
		char *c = lowname;
		char *f = fc->name;
		while ( (*c++ = tolower(*f++)) ) ;

		DEBUG(( "name2cookie: fc->name=%s\n", fc->name));
		if ( ! strcmp( lowname, name) ) {
			*res = fc;
			return 0;
		}
	}

	return -ENOENT;
}

/*
 * routines for parsing path names
 */

/*
 * relpath2cookie converts a TOS file name into a file cookie representing
 * the directory the file resides in, and a character string representing
 * the name of the file in that directory. The character string is
 * copied into the "lastname" array. If lastname is NULL, then the cookie
 * returned actually represents the file, instead of just the directory
 * the file is in.
 *
 * note that lastname, if non-null, should be big enough to contain all the
 * characters in "path", since if the file system doesn't want the kernel
 * to do path name parsing we may end up just copying path to lastname
 * and returning the current or root directory, as appropriate
 *
 * "dir" is the directory relative to which the search should start.
 * if you just want the current directory, use path2cookie instead.
 *
 */

# define DIRSEP(p)  (((p) == '\\') || ((p) == '/'))

long
relpath2cookie (FCOOKIE *dir, const char *path, char *lastname, FCOOKIE **res)
{
	long r = 0;
	short do_last = (lastname == NULL);

	char temp[PATH_MAX];
	if ( ! lastname ) lastname = temp;
	
	DEBUG(( "relpath2cookie: dir=%s path=%s\n", dir->name, path));

	/* first, check for a drive letter
	 */
	if (path[1] == ':')
	{
		char c = tolower ((int)path[0] & 0xff);
		if (c >= 'a' && c <= 'z')
			path += 2;
		else if (c >= '1' && c <= '6')
			path += 2;
	}
	
	while (*path)
	{
		/* now we must have a directory, since there are more things
		 * in the path
		 */
		if ( ! (dir->attr & FA_DIR) )
			return -ENOTDIR;

		/*  skip slashes
		*/
		while (DIRSEP (*path))
			path++;

		/* if there's nothing left in the path, we can break here
		 */
		if (!*path)
		{
			*res = dir;
			*lastname = '\0';
			return 0;
		}

		/* next, peel off the next name in the path
		*/
		{
			register int len;
			register char c, *s;

			len = 0;
			s = lastname;
			c = *path;
			while (c && !DIRSEP (c))
			{
				if (len++ < PATH_MAX)
					*s++ = tolower(c);
				c = *++path;
			}

			*s = 0;
		}

		/* if there are no more names in the path, and we don't want
		 * to actually look up the last name, then we're done
		 */
		if (!do_last && !*path)
		{
			*res = dir;
			return 0;
		}

		r = name2cookie (dir->folder, lastname, res);
		if (r)
		{
			if (r == -ENOENT && *path)
			{
				/* the "file" we didn't find was treated as a directory */
				return -ENOTDIR;
			}
			return r;
		}

		dir = *res;
	}

	return 0;
}

long
path2cookie (const char *path, char *lastname, FCOOKIE **res)
{
	static FCOOKIE rootfc;
	long r;

	rootfc.name = "devdir:"; // FIXME: DEBUG!
	rootfc.attr = FA_DIR;
	rootfc.folder = &root;
	rootfc.bos_dev = 0;
	r  = relpath2cookie ( &rootfc, path, lastname, res);
	DEBUG(( "path2cookie: r=%d\n", r));
	return r;
}


