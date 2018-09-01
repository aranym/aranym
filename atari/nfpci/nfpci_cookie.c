/*
	NatFeat host PCI driver, cookie management

	ARAnyM (C) 2004 Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*--- Includes ---*/

#include <stdlib.h>

#include <mint/osbind.h>
#include <mint/sysvars.h>
#include <mint/cookie.h>

/*--- Defines ---*/

#ifndef C_NULL
#define C_NULL	0x4e554c4cL
#endif

/*--- Functions ---*/

static unsigned long *read_cookie_jar_pointer(void)
{
	unsigned long *cookie_jar;
	void *old_stack=NULL;
	int super_flag;

	/* Check if already in supervisor mode */
	super_flag = (int) Super((void *)1);

	if (!super_flag) {
		old_stack=(void *)Super(NULL);
	}
	cookie_jar= (unsigned long *) *_p_cookies;
	if (!super_flag) {
		Super(old_stack);
	}

	return (cookie_jar);
}

int cookie_present(unsigned long cookie, unsigned long *value)
{
	unsigned long *cookie_jar;
	
	cookie_jar = read_cookie_jar_pointer();

	if (cookie_jar == NULL) {
		return 0;
	}

	while (*cookie_jar != 0) {
		if (*cookie_jar == cookie) {
			if (value) *value = cookie_jar[1];
			return 1;
		}

		cookie_jar+=2;
	}
	return 0;
}

int cookie_add(unsigned long cookie, unsigned long value)
{
	unsigned long *cookie_jar;
	unsigned long count;
	
	cookie_jar = read_cookie_jar_pointer();

	if (cookie_jar == NULL) {
		return 0;
	}

	count=0;
	while (*cookie_jar != 0) {
		cookie_jar += 2;
		count++;
	}

	/* Is there enough space ? */
	if (cookie_jar[1]>count) {
		cookie_jar[2] = 0;
		cookie_jar[3] = cookie_jar[1];
		cookie_jar[0] = cookie;
		cookie_jar[1] = value;
		return 1;
	}

	/* Not enough space in cookie jar */
	return 0;
}
