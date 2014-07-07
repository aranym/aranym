/*
	NFTinyGL LDG linker

	Copyright (C) 2004	Patrice Mandin

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

/*--- Includes ---*/

#include <stdlib.h>

#include <gem.h>
#include <ldg.h>
#include <osbind.h>

/*--- Functions prototypes ---*/

#include "lib-osmesa.h"
#include "lib-oldmesa.h"
#include "lib-misc.h"
#include "nfosmesa_nfapi.h"
#include "../natfeat/nf_ops.h"


#define WITH_PROTOTYPE_STRINGS 1

/*--- LDG functions ---*/

static PROC const LibFunc[]={ 
#if WITH_PROTOTYPE_STRINGS
#define GL_PROC(name, f, desc) { name, desc, f },
#else
#define GL_PROC(name, f, desc) { name, 0, f },
#endif
	#include "link-tinygl.h"	/* 83 functions */
	{NULL, NULL, NULL}
};

#include "versinfo.h"

char const __Ident_tinygl[] = "$NFTinyGL: TinyGL NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER " $";

int err_old_nfapi(void)
{
	/* not an error for TinyGL; the 83 functions should always be present */
	return 0;
}


static LDGLIB const LibLdg = { 
	/* library version */
	0x0100,
	/* count of functions in library */
	sizeof(LibFunc) / sizeof(LibFunc[0]) - 1,
	/* function addresses */
	LibFunc,
	/* Library information string */
	"TinyGL NFOSMesa API Version " __STRINGIFY(ARANFOSMESA_NFAPI_VERSION) " " ASCII_ARCH_TARGET " " ASCII_COMPILER,
	/* Library flags */
	LDG_NOT_SHARED,
	NULL,
	0
};

void APIENTRY tinyglinformation(void)
{
	(void) Cconws(LibLdg.infos);
	(void) Cconws("\r\n");
}

/*
 * Main part : communication with LDG-system
 */

int main(void)
{
	if (ldg_init(&LibLdg) != 0)
	{
		err_init("This program is a LDG library");
	}
	return 0;
}
