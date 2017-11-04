/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * Copyright (C) 1998 by Guido Flohr.
 */

# ifndef _mint_time_h
# define _mint_time_h

typedef struct time TIME;

struct time
{
	long	high_time;
	long	time;		/* This has to be signed!  */
	ulong	nanoseconds;
};

#define dta_UTC_local_dos(dta,xattr,x)					\
{									\
	union { ushort s[2]; ulong l;} data;				\
									\
	/* UTC -> localtime -> DOS style */				\
	data.s[0]	= xattr.x##time;			\
	data.s[1]	= xattr.x##date;			\
	data.l		= dostime(data.l - timezone);			\
	dta->dta_time	= data.s[0];					\
	dta->dta_date	= data.s[1];					\
}

#define xtime_to_local_dos(a,x)				\
{							\
	union { ushort s[2]; ulong l;} data;		\
	data.s[0] = a->x##time;		\
	data.s[1] = a->x##date;		\
	data.l = dostime(data.l - timezone);		\
	a->x##time = data.s[0];		\
	a->x##date = data.s[1];		\
}

#define SET_XATTR_TD(a,x,ut)			\
{						\
	union { ushort s[2]; ulong l;} data;	\
	data.l = ut;				\
	a->x##time = data.s[0];	\
	a->x##date = data.s[1];	\
}

long        _cdecl dostime          (long tv_sec);
long        _cdecl unixtime         (unsigned short time, unsigned short date);
long        _cdecl unix2xbios       (long tv_sec);
void	_cdecl unix2calendar	(long tv_sec,
				 unsigned short *year, unsigned short *month,
				 unsigned short *day, unsigned short *hour,
				 unsigned short *minute, unsigned short *second);

# endif /* _mint_time_h */
