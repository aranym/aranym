/*
 * tools.cpp - non-32bit CPU and miscelany utilities
 *
 * Copyright (c) 2001-2003 STanda of ARAnyM developer team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cstring>

#include "tools.h"

#if SIZEOF_VOID_P != 4 || DEBUG_NON32BIT

// the instance for the memptr <-> void* conversions
// usefull for systems where the sizeof(void*) != 4
NativeTypeMapper<void*> memptrMapper;

#endif  // SIZEOF_VOID_P != 4

char *safe_strncpy(char *dest, const char *src, size_t size)
{
	if (dest == NULL) return NULL;
	if (size > 0) {
		strncpy(dest, src != NULL ? src : "", size);
		dest[size-1] = '\0';
	}
	return dest;
}
