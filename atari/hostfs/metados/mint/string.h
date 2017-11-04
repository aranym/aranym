/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000-2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

# ifndef _mint_string_h
# define _mint_string_h

#include <stdarg.h>

#define bzero			_mint_bzero
#define strcpy			_mint_strcpy
#define strncpy			_mint_strncpy
#define strlen			_mint_strlen
#define strupr			_mint_strupr

void	_cdecl bzero	(void *dst, unsigned long size);
char *	_cdecl strcpy	(char *dst, const char *src);
char *	_cdecl strncpy	(char *dst, const char *src, long len);
long	_cdecl strlen	(const char *s);
char *	_cdecl strupr	(char *s);

long        _cdecl kvsprintf        (char *buf, long buflen, const char *fmt, va_list args) __attribute__((format(printf, 3, 0)));
long        _cdecl ksprintf         (char *buf, long buflen, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

void *memcpy(void *dest, const void *src, size_t n);

#endif
