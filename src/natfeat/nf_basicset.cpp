/**
 * BasicNatFeats (Basic set of Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "cpu_emulation.h"
#include "nf_basicset.h"

#define DEBUG 0
#include "debug.h"

int32 NF_Name::dispatch(uint32 fncode)
{
	memptr name_ptr = getParameter(0);
	uint32 name_maxlen = getParameter(1);
	D(bug("NF_Name(%p, %d)", name_ptr, name_maxlen));

	if (! ValidAddr(name_ptr, true, name_maxlen))
		BUS_ERROR(name_ptr);

	char *name = (char *)Atari2HostAddr(name_ptr);	// use A2Hstrcpy

	char *text;
	switch(fncode) {
		case 0:/* get_pure_name(char *name, uint32 max_len) */
			text = NAME_STRING;
			break;

		case 1:	/* get_complete_name(char *name, uint32 max_len) */
			text = VERSION_STRING;
			break;

		default:
			text = "Unimplemented NF_NAME sub-id";
			break;
	}
	strncpy(name, text, name_maxlen-1);
	name[name_maxlen-1] = '\0';
	return strlen(text);
}

int32 NF_Version::dispatch(uint32 /*fncode*/)
{
	return 0x00010000UL;
}

int32 NF_Shutdown::dispatch(uint32 fncode)
{
	if (fncode == 0) {
		Quit680x0();
	}
	return 0;
}

int32 NF_StdErr::dispatch(uint32 /*fncode*/)
{
	memptr str_ptr = getParameter(0);
	D(bug("NF_StdErr(%d, %p)", fncode, str_ptr));

	if (! ValidAddr(str_ptr, false, 1))
		BUS_ERROR(str_ptr);

	char *str = (char *)Atari2HostAddr(str_ptr);	// use A2Hstrcpy

	uint32 ret = fprintf(stderr, "%s", str);
	fflush(stdout);
	return ret;
}
