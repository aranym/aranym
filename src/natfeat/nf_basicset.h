/**
 * BasicNatFeats (Basic set of Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "nf_base.h"
#include "version.h"

class NF_Name : public NF_Base
{
public:
	char *name() { return "NF_NAME"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};

class NF_Version : public NF_Base
{
public:
	char *name() { return "NF_VERSION"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};

class NF_Shutdown : public NF_Base
{
public:
	char *name() { return "NF_SHUTDOWN"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

class NF_StdErr : public NF_Base
{
public:
	char *name() { return "NF_STDERR"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};
