/**
 * BasicNatFeats (Basic set of Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "cpu_emulation.h"
#include "nf_name.h"

#define DEBUG 1
#include "debug.h"

uint32 NF_Name::dispatch(uint32 fncode)
{
	memptr name_ptr = getParameter(0);
	uint32 name_maxlen = getParameter(1);
	D(bug("NF_Name(%p, %d)", name_ptr, name_maxlen));

	if (! ValidAddr(name_ptr, true, name_maxlen))
		BUS_ERROR(name_ptr);

	char *name = (char *)Atari2HostAddr(name_ptr);	// use A2Hstrcpy

	uint32 ret = 0;
	switch(fncode) {
		default:/* get_pure_name(char *name, uint32 max_len) */
			strncpy(name, NAME_STRING, name_maxlen-1);
			name[name_maxlen-1] = '\0';
			break;

		case 1:	/* get_complete_name(char *name, uint32 max_len) */
			strncpy(name, VERSION_STRING, name_maxlen-1);
			name[name_maxlen-1] = '\0';
			break;
	}
	return ret;
}
