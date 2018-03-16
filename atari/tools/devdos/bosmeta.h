/*
 * Copyright (c) 2005 STanda Opichal
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef ___bosmeta_h__
#define ___bosmeta_h__


#include "list.h"
#include "metados.h"


typedef struct {
	LINKABLE	item;
	char		*name;
	short		attr;
	LIST		*folder;

	/* BOS device entries */
	short		bos_dev;
	unsigned long	bos_info_flags;
} FCOOKIE;

/* old stat structure */
struct xattr
{
	unsigned short	mode;
	long		index;
	unsigned short	dev;
	unsigned short	rdev;
	unsigned short	nlink;
	unsigned short	uid;
	unsigned short	gid;
	long		size;
	long		blksize;
	long		nblocks;
	unsigned short	mtime, mdate;
	unsigned short	atime, adate;
	unsigned short	ctime, cdate;
	short		attr;
	short		reserved2;
	long		reserved3[2];
};

static inline long
is_terminal( FCOOKIE *fc) {
	return fc->bos_info_flags & BOS_INFO_ISTTY;
}

long bosfs_initialize(void);

long getxattr (FCOOKIE *fc, struct xattr *res);
long name2cookie (LIST *folder, const char *name, FCOOKIE **res);
long relpath2cookie (FCOOKIE *dir, const char *path, char *lastname, FCOOKIE **res);
long path2cookie (const char *path, char *lastname, FCOOKIE **res);


#endif /* ___bosmeta_h__ */

