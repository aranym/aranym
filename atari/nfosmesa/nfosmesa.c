/*
	OSMesa LDG linker

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

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

/*--- Functions prototypes ---*/

#include "lib-osmesa.h"
#include "lib-oldmesa.h"
#include "lib-misc.h"

/*--- LDG functions ---*/

PROC LibFunc[]={ 
	#include "link-osmesa.h"	/* 10 functions */
	#include "link-gl.h"		/* 465-118 functions */
	#include "link-glext.h"		/* 974 functions */
	#include "link-oldmesa.h"	/* 5 functions */
	{NULL, NULL, NULL}
};

LDGLIB LibLdg[]={ 
	{
		/* library version */
		0x0620,
		/* count of functions in library */
		10+465-118+974+5,
		/* function addresses */
		LibFunc,
		/* Library information string */
		"Mesa library",
		/* Library flags */
		LDG_NOT_SHARED,
		NULL,
		0
	}
};

/*
 * Main part : communication with LDG-system
 */

int main(int argc, char *argv[])
{
	if (ldg_init(LibLdg)==-1) {
		appl_init();
		form_alert(1, "[1][This program is a LDG library][End]");
		appl_exit();
	}
	return 0;
}
