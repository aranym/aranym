/**
 * NatFeats (Native Features main dispatcher)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "natfeats.h"
#include "nf_objs.h"

#define DEBUG 1
#include "debug.h"

#define ID_SHIFT	20
#define IDX2MASTERID(idx)	(((idx)+1) << ID_SHIFT)
#define MASTERID2IDX(id)	(((id) >> ID_SHIFT)-1)
#define MASKOUTMASTERID(id)	((id) & ((1L << ID_SHIFT)-1))

static memptr context = 0;

uint32 nf_get_id(memptr stack)
{
	memptr name_ptr = ReadInt32(stack);
	if (! ValidAddr(name_ptr, false, 1))
		BUS_ERROR(name_ptr);

	char *name = (char *)Atari2HostAddr(name_ptr);	// FIXME replace with special strcpy
	D(bug("nf_get_id '%s'", name));


	for(unsigned int i=0; i < nf_objs_cnt; i++) {
		if (strcasecmp(name, nf_objects[i]->name()) == 0) {
			D(bug("Found the NatFeat at %d", i));
			return IDX2MASTERID(i);
		}
	}

	return 0;		/* ID with given name not found */
}

uint32 nf_call(memptr stack, bool inSuper)
{
	uint32 fncode = ReadInt32(stack);
	unsigned int idx = MASTERID2IDX(fncode);
	if (idx >= nf_objs_cnt) {
		D(bug("nf_call: wrong ID %d", idx));
		return 0;	/* FIXME: is this a good answer for wrong ID call? */
	}

	fncode = MASKOUTMASTERID(fncode);
	context = stack + 4;	/* parameters follow on the stack */

	pNatFeat obj = nf_objects[idx];
	D(bug("nf_call(%s, %d)", obj->name(), fncode));

	if (obj->isSuperOnly() && !inSuper) {
		longjmp(excep_env, 8);	// privilege exception
	}

	return obj->dispatch(fncode);
}

uint32 nf_getparameter(int i)
{
	if (i < 0)
		return 0;

	return ReadInt32(context + i*4);
}
