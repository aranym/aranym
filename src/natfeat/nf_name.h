/**
 * BasicNatFeats (Basic set of Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "nf_base.h"

class NF_Name : public NF_Base
{
public:
	char *name() { return "NF_NAME"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};
