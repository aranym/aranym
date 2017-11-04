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

# ifndef _mint_ctype_h
# define _mint_ctype_h

#define _ctype _mint_ctype

extern unsigned char const _ctype[256];

#define _CTc		0x01	/* control character */
#define _CTd		0x02	/* numeric digit */
#define _CTu		0x04	/* upper case */
#define _CTl		0x08	/* lower case */
#define _CTs		0x10	/* whitespace */
#define _CTp		0x20	/* punctuation */
#define _CTx		0x40	/* hexadecimal */

#define isalnum(c)	(  _ctype[(unsigned char)(c)] & (_CTu|_CTl|_CTd))
#define isalpha(c)	(  _ctype[(unsigned char)(c)] & (_CTu|_CTl))
#define isascii(c)	(!((c) & ~0x7f))
#define iscntrl(c)	(  _ctype[(unsigned char)(c)] &  _CTc)
#define isdigit(c)	(  _ctype[(unsigned char)(c)] &  _CTd)
#define isgraph(c)	(!(_ctype[(unsigned char)(c)] & (_CTc|_CTs)) && (_ctype[(unsigned char)(c)]))
#define islower(c)	(  _ctype[(unsigned char)(c)] &  _CTl)
#define isprint(c)	(!(_ctype[(unsigned char)(c)] &  _CTc)       && (_ctype[(unsigned char)(c)]))
#define ispunct(c)	(  _ctype[(unsigned char)(c)] &  _CTp)
#define isspace(c)	(  _ctype[(unsigned char)(c)] &  _CTs)
#define isupper(c)	(  _ctype[(unsigned char)(c)] &  _CTu)
#define isxdigit(c)	(  _ctype[(unsigned char)(c)] &  _CTx)
#define iswhite(c)	(isspace (c))

#define toupper		_mint_toupper
#define tolower		_mint_tolower

int	_cdecl tolower	(int c);
int	_cdecl toupper	(int c);

#define _tolower(c)        ((c) ^ 0x20)
#define _toupper(c)        ((c) ^ 0x20)

#endif
