/**
 * BasicNatFeats (Basic set of Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#include "nf_base.h"
#include "stdio.h"

class NF_StdErr : public NF_Base
{
public:
	char *name() { return "NF_STDERR"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);

private:
	uint32 nf_fprintf(FILE *, const char *, uint32);
	uint32 PUTC(FILE *f, int c, int width);
	uint32 PUTS(FILE *f, const char *s, int width);
	uint32 PUTL(FILE *f, ulong u, int base, int width, int fill_char);
};
