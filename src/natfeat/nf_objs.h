#ifndef _NFOBJS_H
#define _NFOBJS_H

#include "nf_base.h"

typedef NF_Base * pNatFeat;

extern pNatFeat nf_objects[];
extern unsigned int nf_objs_cnt;
extern void NFReset();

#endif
