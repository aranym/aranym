#ifndef _NATFEATS_H
#define _NATFEATS_H

#include "sysdeps.h"

// NatFeats CPU handlers
extern uint32 nf_get_id(memptr);
extern uint32 nf_call(memptr, bool);

// NatFeats call for getting parameters
extern uint32 nf_getparameter(int);

#endif /* _NATFEATS_H */
