/**
 * NatFeats (Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "natfeats.h"
#include "araobjs.h"

#define DEBUG 0
#include "debug.h"

#define ID_SHIFT	20

enum { NF_NAME = 1, NF_XHDI, NF_FVDI, NF_LAST };

uint32 identify_dispatch_params(uint32 *params)
{
	uint32 fncode = params[0];
	memptr name_ptr = params[1];
	uint32 name_maxlen = params[2];

	if (name_ptr < 0 || name_ptr > (RAMSize + ROMSize + FastRAMSize - name_maxlen))
		return 0;	/* illegal pointer to name */
	char *name = (char *)Atari2HostAddr(name_ptr);

	uint32 ret = 0;
	switch(fncode) {
		case 0:	/* get_name_and_version(char *name, uint32 max_len) */
			strncpy(name, VERSION_STRING, name_maxlen-1);
			name[name_maxlen-1] = '\0';
			break;

		case 1:	/* get_name(char *name, uint32 max_len) */
			strncpy(name, "ARAnyM", name_maxlen-1);
			name[name_maxlen-1] = '\0';
			break;

		case 2:	/* get_version_num(void) */
			ret = VERSION_MAJOR << 16 | VERSION_MINOR;
			break;

		default:
			ret = 1;	/* FIXME illegal subfunc ID */
			break;
	}
	return ret;
}

uint32 nf_get_id(memptr stack)
{
	D(bug("nf_get_id"));

	typedef struct NATFEATS { const char *name; int id; };
	NATFEATS nat_feats[] = {
		{"NAME", NF_NAME},
		{"XHDI", NF_XHDI},
		{"fVDI", NF_FVDI},
		{NULL, NF_LAST}
	};

	memptr name_ptr = ReadAtariInt32(stack);
	if (name_ptr < 0 || name_ptr >= (RAMSize + ROMSize + FastRAMSize))
		return 0;	/* illegal pointer to name */
	char *name = (char *)Atari2HostAddr(name_ptr);

	for(unsigned int i=0; i < sizeof(nat_feats) / sizeof(nat_feats[0]); i++) {
		if (strcasecmp(name, nat_feats[i].name) == 0) {
			D(bug("Found NatFeat %d", nat_feats[i].id));
			return nat_feats[i].id << ID_SHIFT;
		}
	}

	return 0;		/* ID with given name not found */
}

uint32 nf_rcall(memptr stack)
{
	D(bug("nf_rcall"));

	uint32 fncode = ReadInt32(stack);
	uint32 parameters[32];

	parameters[0] = fncode & ((1 << ID_SHIFT)-1); /* mask out master ID */

	// copy parameters from stack to a local array
	for(unsigned int i = 1; i < sizeof(parameters) / sizeof(parameters[0]); i++)
		parameters[i] = ReadInt32(stack + i * 4);

	switch(fncode >> ID_SHIFT) {
		case NF_NAME: return identify_dispatch_params(parameters); break;

		case NF_XHDI: return Xhdi.dispatch_params(parameters); break;

		// case NF_FVDI: return fVDI.dispatch_params(parameters); break;

		default: return 0;	/* FIXME: is this a good answer for wrong ID call? */
	}
}
