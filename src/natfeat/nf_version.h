/**
 * BasicNatFeats (Basic set of Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "nf_base.h"
#include "version.h"

class NF_Version : public NF_Base
{
public:
	char *name() { return "NF_VERSION"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode) { return VERSION_MAJOR << 16 | VERSION_MINOR; }
};
