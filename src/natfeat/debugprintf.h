/**
 * BasicNatFeats (Basic set of Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "nf_base.h"
#include "stdio.h"

class DebugPrintf : public NF_Base
{
public:
	const char *name() { return "DEBUGPRINTF"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);

private:
	uint32 debugprintf(FILE *, memptr, uint32);
	uint32 PUTC(FILE *f, int c, int width);
	uint32 PUTS(FILE *f, memptr s, int width);
	uint32 PUTL(FILE *f, uint32 u, int base, int width, int fill_char);
};
