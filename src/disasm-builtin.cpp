#include "sysdeps.h"
#include <math.h>

#include "memory-uae.h"
#include "newcpu.h"
#include "m68k.h"
#include "cpu_emulation.h"
#include "disasm-glue.h"

#ifdef DISASM_USE_BUILTIN /* rest of file */


/*
opcode overview:

BITREV      0000 000 011 000 rrr
ORI         0000 000 0ss eee eee
BYTEREV     0000 001 011 000 rrr
ANDI        0000 001 0ss eee eee
FF1         0000 010 011 000 rrr
SUBI        0000 010 0ss eee eee
RTM         0000 011 011 00d rrr
CALLM       0000 011 011 eee eee
ADDI        0000 011 0ss eee eee
CMP2/CHK2   0000 0ss 011 eee eee
BTST        0000 100 000 eee eee
BCHG        0000 100 001 eee eee
BCLR        0000 100 010 eee eee
BSET        0000 100 011 eee eee
EORI        0000 101 0ss eee eee
CMPI        0000 110 0ss eee eee
MOVES       0000 111 0ss eee eee
CAS2        0000 1ss 011 111 100
CAS         0000 1ss 011 eee eee
MOVEP       0000 rrr 1oo 001 rrr
BTST        0000 rrr 100 eee eee
BCHG        0000 rrr 101 eee eee
BCLR        0000 rrr 110 eee eee
BSET        0000 rrr 111 eee eee
MOVE        0001 eee eee eee eee
MOVE        0010 eee eee eee eee
MOVE        0011 eee eee eee eee
MOVE SR     0100 000 011 eee eee
NEGX        0100 000 0ss eee eee
MOVE CCR    0100 001 011 eee eee
CLR         0100 001 0ss eee eee
MOVE CCR    0100 010 011 eee eee
NEG         0100 010 0ss eee eee
MOVE SR     0100 011 011 eee eee
NOT         0100 011 0ss eee eee
LINK.L      0100 100 000 001 rrr
NBCD        0100 100 000 eee eee
SWAP        0100 100 001 000 rrr
BKPT        0100 100 001 001 vvv
PEA         0100 100 001 eee eee
EXT         0100 100 01s 000 rrr
EXTB        0100 100 111 000 rrr
HALT        0100 101 011 001 000
PULSE       0100 101 011 001 100
BGND        0100 101 011 111 010
ILLEGAL     0100 101 011 111 100 ; sysV68: swbeg.w
SWBEGL      0100 101 011 111 101 ; sysV68
TAS         0100 101 011 eee eee
TST         0100 101 0ss eee eee
MULS/U      0100 110 000 eee eee
DIVS/U      0100 110 001 eee eee
SATS        0100 110 010 000 rrr
MOVEM       0100 1d0 01s eee eee
TRAP        0100 111 001 00v vvv
LINK        0100 111 001 010 rrr
UNLK        0100 111 001 011 rrr
MOVE USP    0100 111 001 10d rrr
RESET       0100 111 001 110 000
NOP         0100 111 001 110 001
STOP        0100 111 001 110 010
RTE         0100 111 001 110 011
RTD         0100 111 001 110 100
RTS         0100 111 001 110 101
TRAPV       0100 111 001 110 110
RTR         0100 111 001 110 111
MOVEC       0100 111 001 111 01d
JSR         0100 111 010 eee eee
JMP         0100 111 011 eee eee
LEA         0100 rrr 111 eee eee
CHK         0100 rrr ss0 eee eee
DBcc        0101 ccc c11 001 rrr
TRAPcc      0101 ccc c11 111 sss
Scc         0101 ccc c11 eee eee
ADDQ        0101 iii 0ss eee eee
SUBQ        0101 iii 1ss eee eee
BRA         0110 000 0dd ddd ddd
BSR         0110 000 1dd ddd ddd
Bcc         0110 ccc cdd ddd ddd
MVS         0111 rrr 10s eee eee
MVZ         0111 rrr 11s eee eee
MOVEQ       0111 rrr 0ii iii iii
DIVU        1000 rrr 011 eee eee
SBCD        1000 rrr 100 00m rrr
PACK        1000 rrr 101 00m rrr
UNPK        1000 rrr 110 00m rrr
DIVS        1000 rrr 111 eee eee
OR          1000 rrr ooo eee eee
SUBX        1001 rrr 1ss 00m rrr
SUB         1001 rrr oss eee eee
SUBA        1001 rrr s11 eee eee
MOV3Q       1010 iii 101 eee eee
MOVCLR      1010 0aa 111 00r rrr
MOVACC      1010 0aa 110 00r rrr
MOVMACSR    1010 100 100 eee eee
MOVACCEXT01 1010 101 110 00r rrr
MOVACCEXT23 1010 111 110 00r rrr
MOVMACSR    1010 100 110 00r rrr
MAC         1010 xxx 0ax 00y yyy
LINEA       1010 xxx xxx xxx xxx
CMPA        1011 rrr s11 eee eee
CMPM        1011 rrr 1ss 001 eee
EOR         1011 rrr 1ss eee eee
CMP         1011 rrr ooo eee eee
MULU        1100 rrr 011 eee eee
ABCD        1100 rrr 100 00m rrr
EXG         1100 rrr 101 000 rrr
EXG         1100 rrr 101 001 rrr
EXG         1100 rrr 110 001 rrr
MULS        1100 rrr 111 eee eee
AND         1100 rrr ooo eee eee
ADDX        1101 rrr 1ss 00m rrr
ADDA        1101 rrr s11 eee eee
ADD         1101 rrr oss eee eee
BFTST       1110 100 011 eee eee
BFEXTU      1110 100 111 eee eee
BFCHG       1110 101 011 eee eee
BFEXTS      1110 101 111 eee eee
BFCLR       1110 110 011 eee eee
BFFFO       1110 110 111 eee eee
BFSET       1110 111 011 eee eee
BFINS       1110 111 111 eee eee
ASR         1110 rrr 0ss i00 eee
ASL         1110 rrr 1ss i00 eee
LSR         1110 rrr 0ss i01 eee
LSL         1110 rrr 1ss i01 eee
ROXL        1110 rrr 0ss i10 eee
ROXR        1110 rrr 1ss i10 eee
ROL         1110 rrr 0ss i11 eee
ROR         1110 rrr 1ss i11 eee


MOVE16      1111 011 000 100 rrr
cpGEN       1111 ppp 000 eee eee
cpDBcc      1111 ppp 001 001 rrr
cpTRAPcc    1111 ppp 001 111 sss
cpScc       1111 ppp 001 eee eee
cpBcc       1111 ppp 01s ccc ccc
*/


#define dstreg(opcode) (((opcode) >> 9) & 7)
#define dstmod(opcode) (((opcode) >> 6) & 7)
#define srcmod(opcode) (((opcode) >> 3) & 7)
#define srcreg(opcode) (((opcode)     ) & 7)
#define insize(ocpode) (((opcode) >> 6) & 3)

#define sputc(c) *(((struct priv_data *)(info->disasm_data))->lp)++ = c
#define pcomma(info) sputc(',')

#define GETUW(a) ((uae_u16)phys_get_word(a))
#define GETW(a) ((uae_s16)phys_get_word(a))
#define GETL(a) ((uae_s32)phys_get_long(a))
#define GETUL(a) ((uae_u32)phys_get_long(a))

struct priv_data {
	memptr oldaddr;
	char *lp;
};

#define REG_PC 8

enum fpu_size {
	FPU_SIZE_LONG = 4,
	FPU_SIZE_SINGLE,
	FPU_SIZE_EXTENDED,
	FPU_SIZE_PACKED,
	FPU_SIZE_WORD,
	FPU_SIZE_DOUBLE,
	FPU_SIZE_BYTE,
	FPU_SIZE_PACKED_VARIABLE
};

static const char *const fpregs[8] = { "fp0","fp1","fp2","fp3","fp4","fp5","fp6","fp7" };
static const char *const fpcregs[3] = { "fpiar", "fpsr", "fpcr" };

#if defined(__WIN32__) || defined(_WIN32)
#  define NO_LONGDOUBLE_PRINTF
#endif

#if defined(SIZEOF_LONG_DOUBLE) && defined(SIZEOF_DOUBLE) && (SIZEOF_LONG_DOUBLE == SIZEOF_DOUBLE)
/*
 * Android currently has some functions commented out in the math.h header file,
 * even if autoconf detects them to be present.
 */
#undef logl
#define logl(x) ((long double)log((double)(x)))
#undef powl
#define powl(x, y) ((long double)pow((double)(x), (double)(y)))
#undef ldexpl
#define ldexpl(x, y) ((long double)ldexp((double)(x), y))
#endif

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static const char *const condtable[] =
{
	"t", "f", "hi", "ls", "cc", "cs", "ne", "eq",
	"vc", "vs", "pl", "mi", "ge", "lt", "gt", "le"
};

static const char *const pmmucondtable[64] =
{
	"bs", "bc", "ls", "lc", "ss", "sc", "as", "ac",
	"ws", "wc", "is", "ic", "gs", "gc", "cs", "cc",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??"
};

static const char *const cpcondtable[64] =
{
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??",
	"??", "??", "??", "??", "??", "??", "??", "??"
};

static const char *const fpucondtable[64] =
{
	"f",   "eq",  "ogt", "oge", "olt", "ole", "ogl", "or",
	"un",  "ueq", "ugt", "uge", "ult", "ule", "ne",  "t",
	"sf",  "seq", "gt",  "ge",  "lt",  "le",  "gl",  "gle",
	"ngle","ngl", "nle", "nlt", "nge", "ngt", "sne", "st",
	"???", "???", "???", "???", "???", "???", "???", "???",
	"???", "???", "???", "???", "???", "???", "???", "???",
	"???", "???", "???", "???", "???", "???", "???", "???",
	"???", "???", "???", "???", "???", "???", "???", "???"
};

static const char *const bittable[] =
{
	"btst", "bchg", "bclr", "bset"
};

static const char *const immedtable[] =
{
	"ori", "andi", "subi", "addi", "???", "eori", "cmpi", "???"
};

static const char *const shfttable[] =
{
	"as", "ls", "rox", "ro"
};

static const char *const bitfieldtable[] =
{
	"bftst", "bfextu", "bfchg", "bfexts", "bfclr", "bfffo", "bfset", "bfins"
};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void ps(m68k_disasm_info *info, const char *str)
{
	register char c;

	while ((c = *str++) != '\0')
	{
		sputc(c);
	}
}

/*** ---------------------------------------------------------------------- ***/

static void startoperands(m68k_disasm_info *info)
{
#if 0
	while (((info->lp - info->asmline) % 10) != 0)
		sputc(' ');
#else
	sputc('\0');
	((struct priv_data *)(info->disasm_data))->lp = info->operands;
#endif
}

/*** ---------------------------------------------------------------------- ***/

static void only_if(m68k_disasm_info *info, const char *cpu)
{
	if (*info->comments == '\0')
	{
		strcat(info->comments, cpu);
		strcat(info->comments, " only");
	}
}
#define only_68020() only_if(info, "68020+")
#define only_cf(rev) only_if(info, "ColdFire " rev)

/*** ---------------------------------------------------------------------- ***/

static void pudec(m68k_disasm_info *info, uae_u32 x)
{
	char buf[20];
	char *cp;

	cp = buf + sizeof(buf);
	*--cp = '\0';
	if (x != 0)
	{
		while (x)
		{
			*--cp = (char) (x % 10 + '0');
			x /= 10;
		}
	} else
	{
		*--cp = '0';
	}
	ps(info, cp);
}

/*** ---------------------------------------------------------------------- ***/

static void psdec(m68k_disasm_info *info, uae_s32 val)
{
	if (val < 0)
	{
		sputc('-');
		val = -val;
	}
	pudec(info, val);
}

/*** ---------------------------------------------------------------------- ***/

#if 0
static void pfloat(m68k_disasm_info *info, double val)
{
	gcvt(val, 6, ((struct priv_data *)(info->disasm_data))->lp);
	while (*(((struct priv_data *)(info->disasm_data))->lp) != '\0')
		(((struct priv_data *)(info->disasm_data))->lp)++;
}
#endif

/*** ---------------------------------------------------------------------- ***/

static void pbyte(m68k_disasm_info *info, unsigned short val)
{
	unsigned short c;

	val &= 0xff;
	c = val >> 4;
	c = c >= 10 ? (c + 'A' - 10) : (c + '0');
	sputc(c);
	c = val & 0x0f;
	c = c >= 10 ? (c + 'A' - 10) : (c + '0');
	sputc(c);
}

/*** ---------------------------------------------------------------------- ***/

static void p2hex(m68k_disasm_info *info, unsigned short val)
{
	sputc('$');
	pbyte(info, val);
}

/*** ---------------------------------------------------------------------- ***/

static void p4hex0(m68k_disasm_info *info, unsigned short val)
{
	pbyte(info, val >> 8);
	pbyte(info, val);
}

/*** ---------------------------------------------------------------------- ***/

static void p4hex(m68k_disasm_info *info, unsigned short val)
{
	sputc('$');
	p4hex0(info, val);
}

/*** ---------------------------------------------------------------------- ***/

static void p8hex(m68k_disasm_info *info, uae_u32 val)
{
	sputc('$');
	p4hex0(info, (unsigned short) (val >> 16));
	p4hex0(info, (unsigned short) (val));
}

/*** ---------------------------------------------------------------------- ***/

static void pcond(m68k_disasm_info *info, int cond)
{
	ps(info, condtable[cond]);
}

/*** ---------------------------------------------------------------------- ***/

static void odcw(m68k_disasm_info *info, int val)
{
	ps(info, "dc.w");
	startoperands(info);
	p4hex(info, val);
	info->num_oper = 1;
}

/*** ---------------------------------------------------------------------- ***/

static void od(m68k_disasm_info *info, int regnum)
{
	sputc('d');
	if (regnum >= 0 && regnum <= 7)
		sputc(regnum + '0');
	else
		sputc('?');
}

/*** ---------------------------------------------------------------------- ***/

static void oa(m68k_disasm_info *info, int regnum)
{
	if (regnum == REG_PC)
	{
		ps(info, "pc");
	} else
	{
		sputc('a');
		sputc(regnum + '0');
	}
}

/*** ---------------------------------------------------------------------- ***/

static void oad(m68k_disasm_info *info, int regnum)
{
	if (regnum & 0xfff8)
		oa(info, regnum & 7);
	else
		od(info, regnum & 7);
}

/*** ---------------------------------------------------------------------- ***/

static void oddstreg(m68k_disasm_info *info, int opcode)
{
	od(info, dstreg(opcode));
}

/*** ---------------------------------------------------------------------- ***/

static void oai(m68k_disasm_info *info, int regnum)
{
	sputc('(');
	oa(info, regnum);
	sputc(')');
}

/*** ---------------------------------------------------------------------- ***/

static void oadi(m68k_disasm_info *info, int regnum)
{
	sputc('(');
	oad(info, regnum);
	sputc(')');
}

/*** ---------------------------------------------------------------------- ***/

static int os(m68k_disasm_info *info, int opcode)
{
	register int size;

	size = insize(opcode);
	sputc('.');
	sputc(size == 2 ? 'l' :
		  size == 1 ? 'w' :
		  size == 0 ? 'b' : '?');
	startoperands(info);
	return size;
}

/*** ---------------------------------------------------------------------- ***/

static int os2(m68k_disasm_info *info, int opcode)
{
	register int size;

	size = (opcode >> 9) & 3;
	sputc('.');
	sputc(size == 3 ? 'l' :
		  size == 2 ? 'w' :
		  size == 1 ? 'b' : '?');
	startoperands(info);
	return size;
}

/*** ---------------------------------------------------------------------- ***/

static int osa(m68k_disasm_info *info, int opcode, int mask)
{
	sputc('.');
	sputc(opcode & mask ? 'l' : 'w');
	startoperands(info);
	return opcode & mask ? 2 : 1;
}

/*** ---------------------------------------------------------------------- ***/

static float make_single(uae_u32 value)
{
	float frac, result;
	int sign = (value & 0x80000000) != 0;
	
	if ((value & 0x7fffffff) == 0)
		return sign ? -0.0 : 0.0;
	
	if ((value & 0x7f800000) == 0x7f800000)
	{
		if ((value & 0x003fffff) == 0)
			return sign ? -1.0 / 0.0 : 1.0 / 0.0;
		return sign ? -log(-1) : log(-1);
	}
		
	frac = (float) ((value & 0x7fffff) | 0x800000) / 8388608.0;
	if (sign)
		frac = -frac;
	
	result = ldexp(frac, (int)((value >> 23) & 0xff) - 127);
	
	return result;
}

static double make_double(uae_u32 wrd1, uae_u32 wrd2)
{
	double frac, result;
	int sign = (wrd1 & 0x80000000) != 0;
	
	if ((wrd1 & 0x7fffffff) == 0 && wrd2 == 0)
		return sign ? -0.0 : 0.0;
	
	if ((wrd1 & 0x7ff00000) == 0x7ff00000)
	{
		if ((wrd1 & 0x0007ffff) == 0)
			return sign ? -1.0 / 0.0 : 1.0 / 0.0;
		return sign ? -log(-1) : log(-1);
	}
		
	frac =
		(double) ((wrd1 & 0x000fffff) | 0x100000) / 1048576.0 +
		(double) wrd2 / 4503599627370496.0;
	
	if (wrd1 & 0x80000000)
		frac = -frac;
	
	result = ldexp(frac, (int)((wrd1 >> 20) & 0x7ff) - 1023);

	return result;
}


static long double make_extended(uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
	long double frac, result;
	int sign = (wrd1 & 0x80000000) != 0;
	uae_u32 exp  = (wrd1 >> 16) & 0x7fff;

	if (exp == 0 && wrd2 == 0 && wrd3 == 0)
		return sign ? -0.0L : 0.0L;
	
	if (exp == 0x7fff)
	{
		if ((wrd2 & 0x3fffffff) == 0)
			return sign ? -1.0L / 0.0L : 1.0L / 0.0L;
		return sign ? -logl(-1) : logl(-1);
	}
		
	frac =
		(long double) ((wrd2 & 0x7fffffff) | 0x80000000) / 2147483648.0L +
		(long double) wrd3 / 9223372036854775808.0L;
	if (sign)
		frac = -frac;

	result = ldexpl(frac, (int)exp - 16383);

	return result;
}


static long double make_packed(uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
	long double d;
	bool sign = (wrd1 & 0x80000000) != 0;
	bool se = (wrd1 & 0x40000000) != 0;
	int exp = (wrd1 & 0x0fff0000) >> 16;
	unsigned int dig;
	long double pwr;
	
	if (exp == 0xfff)
	{
		if (wrd2 == 0 && wrd3 == 0)
		{
			return sign ? -1.0L / 0.0L : 1.0L / 0.0L;
		} else
		{
			return sign ? -logl(-1) : logl(-1);
		}
	}
	dig = wrd1 & 0x0000000f;
	if (dig == 0 && wrd2 == 0 && wrd3 == 0)
	{
		return sign ? -0.0L : 0.0L;
	}

	/*
	 * Convert the bcd exponent to binary by successive adds and
	 * muls. Set the sign according to SE. Subtract 16 to compensate
	 * for the mantissa which is to be interpreted as 17 integer
	 * digits, rather than 1 integer and 16 fraction digits.
	 * Note: this operation can never overflow.
	 */
	exp = ((wrd1 >> 24) & 0xf);
	exp = exp * 10 + ((wrd1 >> 20) & 0xf);
	exp = exp * 10 + ((wrd1 >> 16) & 0xf);
	if (se)
		exp = -exp;
	/* sub to compensate for shift of mant */
	exp = exp - 16;
	
	/*
	 * Convert the bcd mantissa to binary by successive
	 * adds and muls. Set the sign according to SM.
	 * The mantissa digits will be converted with the decimal point
	 * assumed following the least-significant digit.
	 * Note: this operation can never overflow.
	 */
	d = wrd1 & 0xf;
	d = d * 10.0L + ((wrd2 >> 28) & 0xf);
	d = d * 10.0L + ((wrd2 >> 24) & 0xf);
	d = d * 10.0L + ((wrd2 >> 20) & 0xf);
	d = d * 10.0L + ((wrd2 >> 16) & 0xf);
	d = d * 10.0L + ((wrd2 >> 12) & 0xf);
	d = d * 10.0L + ((wrd2 >>  8) & 0xf);
	d = d * 10.0L + ((wrd2 >>  4) & 0xf);
	d = d * 10.0L + ((wrd2      ) & 0xf);
	d = d * 10.0L + ((wrd3 >> 28) & 0xf);
	d = d * 10.0L + ((wrd3 >> 24) & 0xf);
	d = d * 10.0L + ((wrd3 >> 20) & 0xf);
	d = d * 10.0L + ((wrd3 >> 16) & 0xf);
	d = d * 10.0L + ((wrd3 >> 12) & 0xf);
	d = d * 10.0L + ((wrd3 >>  8) & 0xf);
	d = d * 10.0L + ((wrd3 >>  4) & 0xf);
	d = d * 10.0L + ((wrd3      ) & 0xf);

	/* Check the sign of the mant and make the value in fp0 the same sign. */
	if (sign)
		d = -d;
	
	/*
	 * Calculate power-of-ten factor from exponent.
	 */
	if (exp < 0)
	{
		exp = -exp;
		pwr = powl(10.0L, exp);
		d = d / pwr;
	} else
	{
		pwr = powl(10.0L, exp);
		d = d * pwr;
	}

	return d;
}

/*** ---------------------------------------------------------------------- ***/

static void oi(m68k_disasm_info *info, int size, bool immed)
{
	char str[100];
	
	if (immed)
		sputc('#');
	switch (size)
	{
	case 2:
		p8hex(info, GETL(info->memory_vma));
		info->memory_vma += 4;
		break;
	case 1:
		p4hex(info, GETUW(info->memory_vma));
		info->memory_vma += 2;
		break;
	case 0:
		p2hex(info, (uae_s32) ((signed char) (GETW(info->memory_vma) & 0xff)));
		info->memory_vma += 2;
		break;
	case FPU_SIZE_LONG:
		psdec(info, GETL(info->memory_vma));
		info->memory_vma += 4;
		break;
	case FPU_SIZE_SINGLE:
		sprintf(str, "%.17e", make_single(GETUL(info->memory_vma)));
		ps(info, str);
		info->memory_vma += 4;
		break;
	case FPU_SIZE_EXTENDED:
#ifdef NO_LONGDOUBLE_PRINTF
		sprintf(str, "%.17e", (double) make_extended(GETUL(info->memory_vma), GETUL(info->memory_vma + 4), GETUL(info->memory_vma + 8)));
#else
		sprintf(str, "%.17Le", make_extended(GETUL(info->memory_vma), GETUL(info->memory_vma + 4), GETUL(info->memory_vma + 8)));
#endif
		ps(info, str);
		info->memory_vma += 12;
		break;
	case FPU_SIZE_PACKED:
#ifdef NO_LONGDOUBLE_PRINTF
		sprintf(str, "%.17e", (double) make_packed(GETUL(info->memory_vma), GETUL(info->memory_vma + 4), GETUL(info->memory_vma + 8)));
#else
		sprintf(str, "%.17Le", make_packed(GETUL(info->memory_vma), GETUL(info->memory_vma + 4), GETUL(info->memory_vma + 8)));
#endif
		ps(info, str);
		info->memory_vma += 12;
		break;
	case FPU_SIZE_WORD:
		psdec(info, GETW(info->memory_vma));
		info->memory_vma += 2;
		break;
	case FPU_SIZE_DOUBLE:
		sprintf(str, "%.17e", make_double(GETUL(info->memory_vma), GETUL(info->memory_vma + 4)));
		ps(info, str);
		info->memory_vma += 8;
		break;
	case FPU_SIZE_BYTE:
		psdec(info, (uae_s32) ((signed char) (GETW(info->memory_vma) & 0xff)));
		info->memory_vma += 2;
		break;
	case FPU_SIZE_PACKED_VARIABLE:
#ifdef NO_LONGDOUBLE_PRINTF
		sprintf(str, "%.17e", (double) make_packed(GETUL(info->memory_vma), GETUL(info->memory_vma + 4), GETUL(info->memory_vma + 8)));
#else
		sprintf(str, "%.17Le", make_packed(GETUL(info->memory_vma), GETUL(info->memory_vma + 4), GETUL(info->memory_vma + 8)));
#endif
		ps(info, str);
		info->memory_vma += 12;
		break;
	}
}

/*** ---------------------------------------------------------------------- ***/

static void pscale(m68k_disasm_info *info, int opcode)
{
	switch (opcode & 0x600)
	{
		case 0x0200: ps(info, "*2"); only_68020(); break;
		case 0x0400: ps(info, "*4"); only_68020(); break;
		case 0x0600: ps(info, "*8"); only_68020(); break;
	}
}

/*** ---------------------------------------------------------------------- ***/

static void doextended(m68k_disasm_info *info, int opcode2, int srcr)
{
	uae_s32 disp;
	uae_s32 outer;
	bool con2 = false;
	int bd = (opcode2 & 0x30) >> 4;
	bool bs = (opcode2 & 0x80) != 0;
	int iis;
	uae_u32 relpc = info->reloffset + ((struct priv_data *)(info->disasm_data))->oldaddr;
	
	only_68020();
	switch (bd)
	{
	case 2:
		disp = GETW(info->memory_vma);
		info->memory_vma += 2;
		break;
	case 3:
		disp = GETL(info->memory_vma);
		info->memory_vma += 4;
		break;
	case 1:
		disp = 0;
		break;
	case 0:
	default:
		disp = 0;
		strcat(info->comments, "; reserved BD=0");
		break;
	}
	switch (opcode2 & 0x47)
	{
	case 0x00:
		/* no memory indirect */
		outer = 0;
		iis = 0;
		break;
	case 0x01:
		/* Indirect Preindexed with Null Outer Displacement */
		outer = 0;
		iis = 1;
		break;
	case 0x02:
		iis = 2;
		/* Indirect Preindexed with Word Outer Displacement */
		outer = GETW(info->memory_vma);
		info->memory_vma += 2;
		break;
	case 0x03:
		iis = 3;
		/* Indirect Preindexed with Long Outer Displacement */
		outer = GETL(info->memory_vma);
		info->memory_vma += 4;
		break;
	case 0x04:
		iis = 4;
		/* reserved */
		outer = 0;
		strcat(info->comments, "; reserved OD=0");
		break;
	case 0x05:
		iis = 5;
		/* Indirect Postindexed with Null Outer Displacement */
		outer = 0;
		break;
	case 0x06:
		iis = 6;
		/* Indirect Postindexed with Word Outer Displacement */
		outer = GETW(info->memory_vma);
		info->memory_vma += 2;
		break;
	case 0x07:
		iis = 7;
		/* Indirect Postindexed with Long Outer Displacement */
		outer = GETL(info->memory_vma);
		info->memory_vma += 4;
		break;
	case 0x40:
		iis = 0;
		/* no memory indirect */
		outer = 0;
		break;
	case 0x41:
		iis = 1;
		/* Memory Indirect with Null Outer Displacement */
		outer = 0;
		break;
	case 0x42:
		iis = 2;
		/* Memory Indirect with Word Outer Displacement */
		outer = GETW(info->memory_vma);
		info->memory_vma += 2;
		break;
	case 0x43:
		iis = 3;
		/* Memory Indirect with Long Outer Displacement */
		outer = GETL(info->memory_vma);
		info->memory_vma += 4;
		break;
	case 0x44:
	default:
		iis = 4;
		outer = 0;
		strcat(info->comments, "; reserved OD=0");
		break;
	case 0x45:
		iis = 5;
		outer = 0;
		strcat(info->comments, "; reserved OD=1");
		break;
	case 0x46:
		iis = 6;
		outer = GETW(info->memory_vma);
		info->memory_vma += 2;
		strcat(info->comments, "; reserved OD=2");
		break;
	case 0x47:
		iis = 7;
		outer = GETL(info->memory_vma);
		info->memory_vma += 4;
		strcat(info->comments, "; reserved OD=3");
		break;
	}
	sputc('(');
	if (iis != 0)
		sputc('[');
	if (bd == 3)
	{
		if (srcr != REG_PC || (srcr == REG_PC && bs))
			p8hex(info, disp);
		else
			p8hex(info, relpc + disp);
		con2 = true;
	} else if (bd == 2)
	{
		if (srcr != REG_PC || (srcr == REG_PC && bs))
			p4hex(info, disp);
		else
			p8hex(info, relpc + disp);
		con2 = true;
	}
	if (srcr == REG_PC)
	{
		if (con2)
			pcomma(info);
		if (bs)
			sputc('z');
		oa(info, srcr);
		con2 = true;
	} else
	{
		if (!bs)
		{
			if (con2)
				pcomma(info);
			oa(info, srcr);
			con2 = true;
		}
	}
	if (iis >= 0x4)
	{
		sputc(']');
		con2 = true;
	}
	if ((opcode2 & 0x40) == 0)
	{
		if (con2)
			pcomma(info);
		oad(info, (opcode2 >> 12) & 0x0f);
		ps(info, opcode2 & 0x0800 ? ".l" : ".w");
	}
	pscale(info, opcode2);
	if (iis < 0x4 && iis > 0)
	{
		sputc(']');
	}
	if ((opcode2 & 0x03) >= 0x02)
	{
		sputc(',');
		if ((opcode2 & 0x03) >= 0x03)
			p8hex(info, outer);
		else
			p4hex(info, outer);
	}
	sputc(')');
}


static void doea(m68k_disasm_info *info, int opcode, int size)
{
	register int srcr;
	register int opcode2;
	uae_s32 offset;
	register uae_s32 adr;

	srcr = srcreg(opcode);
	switch (srcmod(opcode))
	{
	case 0:							/* Datenregister direkt */
		od(info, srcr);
		break;
	case 1:							/* Adressregister direkt */
		oa(info, srcr);
		break;
	case 2:							/* Adressregister indirekt */
		oai(info, srcr);
		break;
	case 3:							/* Adressregister indirekt mit Postinkrement */
		oai(info, srcr);
		sputc('+');
		break;
	case 4:							/* Adressregister indirekt mit Predecrement */
		sputc('-');
		oai(info, srcr);
		break;
	case 5:							/* Adressregister indirekt mit Offset */
		psdec(info, (uae_s32) (GETW(info->memory_vma)));
		info->memory_vma += 2;
		oai(info, srcr);
		break;
	case 6:							/* Adressregister indirekt indiziert mit Offset */
		opcode2 = GETUW(info->memory_vma);
		info->memory_vma += 2;
		if (opcode2 & 0x100)
		{
			doextended(info, opcode2, srcr);
		} else
		{
			offset = opcode2 & 0xff;
			psdec(info, (uae_s32) (char) offset);
			sputc('(');
			oa(info, srcr);
			pcomma(info);
			oad(info, (opcode2 >> 12) & 0x0f);
			ps(info, opcode2 & 0x0800 ? ".l" : ".w");
			pscale(info, opcode2);
			sputc(')');
		}
		break;
	case 7:
		switch (srcr)
		{
		case 0:						/* Absolut kurz */
			sputc('(');
			adr = GETW(info->memory_vma);
			info->memory_vma += 2;
			p8hex(info, adr);
			ps(info, ").w");
			break;
		case 1:						/* Absolut lang */
			adr = GETL(info->memory_vma);
			info->memory_vma += 4;
			p8hex(info, adr);
			break;
		case 2:						/* PC-relative */
			adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETW(info->memory_vma) + info->reloffset;
			info->memory_vma += 2;
			p8hex(info, adr);
			oai(info, REG_PC);
			break;
		case 3:						/* PC-relative indiziert */
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			if (opcode2 & 0x100)
			{
				doextended(info, opcode2, REG_PC);
			} else
			{
				adr = (uae_s32)(signed char)(opcode2 & 0xff) + info->reloffset + ((struct priv_data *)(info->disasm_data))->oldaddr;
				p8hex(info, adr);
				sputc('(');
				oa(info, REG_PC);
				pcomma(info);
				oad(info, (opcode2 >> 12) & 0x0f);
				ps(info, opcode2 & 0x0800 ? ".l" : ".w");
				pscale(info, opcode2);
				sputc(')');
			}
			break;
		case 4:						/* immediate */
			oi(info, size, true);
			break;
		default:
			ps(info, "???");
			break;
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

static void reglist(m68k_disasm_info *info, int regmask)
{
	register int regnum;
	int liststart;
	int lastreg;
	int status;

	liststart = status = 0;
	lastreg = 20;
	for (regnum = 0; regnum < 16; regnum++)
	{
		if ((1 << regnum) & regmask)
		{
			if (regnum == (lastreg + 1) && regnum != 8)
			{
				if (liststart != 0)
					sputc('-');
				liststart = 0;
				status = 1;
				lastreg = regnum;
			} else
			{
				if (status != 0)
				{
					oad(info, lastreg);
					status = 0;
					liststart = 1;
				}
				if (liststart != 0)
					sputc('/');
				oad(info, regnum);
				lastreg = regnum;
				liststart = 1;
			}
		}
	}
	if (status != 0)
		oad(info, lastreg);
}

/*** ---------------------------------------------------------------------- ***/

static int revbits(int mask, int n)
{
	register int i;
	register int newmask;

	for (newmask = 0, i = n; i > 0; i--)
	{
		newmask = (newmask << 1) | (mask & 1);
		mask >>= 1;
	}
	return newmask;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void group0(m68k_disasm_info *info, int opcode)
{
	unsigned int opcode2;
	int size;
	
	if (opcode & 0x0100)
	{
		if (srcmod(opcode) != 1)
		{
			/* 0000 rrr 1 xx yyyyyy */
			ps(info, bittable[insize(opcode)]);
			startoperands(info);
			oddstreg(info, opcode);
			pcomma(info);
			doea(info, opcode, 0);
			info->num_oper = 2;
		} else
		{
			/* 0000 000 1xx 001 yyy */
			ps(info, "movep");
			ps(info, opcode & 0x0040 ? ".l" : ".w");
			startoperands(info);
			if (opcode & 0x0080)
			{
				oddstreg(info, opcode);
				pcomma(info);
				psdec(info, GETW(info->memory_vma));
				info->memory_vma += 2;
				oai(info, srcreg(opcode));
			} else
			{
				psdec(info, GETW(info->memory_vma));
				info->memory_vma += 2;
				oai(info, srcreg(opcode));
				pcomma(info);
				oddstreg(info, opcode);
			}
			info->num_oper = 2;
		}
	} else
	{
		switch (opcode2 = dstreg(opcode))
		{
		case 4:
			/* 0000 100 0ss yyy yyy */
			ps(info, bittable[insize(opcode)]);
			startoperands(info);
			sputc('#');
			psdec(info, GETW(info->memory_vma));
			info->memory_vma += 2;
			pcomma(info);
			doea(info, opcode, 0);
			info->num_oper = 2;
			break;
			
		default:
			if (insize(opcode) == 3)
			{
				switch (opcode2)
				{
				case 0:
					/* 0000 000 011 xxx yyy */
					switch (srcmod(opcode))
					{
					case 2:
					case 5:
					case 6:
					case 7:
						opcode2 = GETUW(info->memory_vma);
						info->memory_vma += 2;
						ps(info, opcode2 & 0x0800 ? "chk2.b" : "cmp2.b");
						startoperands(info);
						doea(info, opcode, 0);
						pcomma(info);
						oad(info, (opcode2 >> 12) & 0x0f);
						info->num_oper = 2;
						only_68020();
						break;
					case 0:
						ps(info, "bitrev.l");
						startoperands(info);
						od(info, srcreg(opcode));
						info->num_oper = 1;
						only_cf("isa_c");
						break;
					default:
						odcw(info, opcode);
						break;
					}
					break;
				case 1:
					/* 0000 001 011 xxx yyy */
					switch (srcmod(opcode))
					{
					case 2:
					case 5:
					case 6:
					case 7:
						opcode2 = GETUW(info->memory_vma);
						info->memory_vma += 2;
						ps(info, opcode2 & 0x0800 ? "chk2.w" : "cmp2.w");
						startoperands(info);
						doea(info, opcode, 0);
						pcomma(info);
						oad(info, (opcode2 >> 12) & 0x0f);
						info->num_oper = 2;
						only_68020();
						break;
					case 0:
						ps(info, "byterev.l");
						startoperands(info);
						od(info, srcreg(opcode));
						info->num_oper = 1;
						only_cf("isa_c");
						break;
					default:
						odcw(info, opcode);
						break;
					}
					break;
				case 2:
					/* 0000 010 011 xxx yyy */
					switch (srcmod(opcode))
					{
					case 2:
					case 5:
					case 6:
					case 7:
						opcode2 = GETUW(info->memory_vma);
						info->memory_vma += 2;
						ps(info, opcode2 & 0x0800 ? "chk2.l" : "cmpl.w");
						startoperands(info);
						doea(info, opcode, 1);
						pcomma(info);
						oad(info, (opcode2 >> 12) & 0x0f);
						info->num_oper = 2;
						only_68020();
						break;
					case 0:
						ps(info, "ff1.l");
						startoperands(info);
						od(info, srcreg(opcode));
						info->num_oper = 1;
						only_cf("isa_c");
						break;
					default:
						odcw(info, opcode);
						break;
					}
					break;
				case 3:
					/* 0000 011 011 xxx yyy */
					if ((opcode & 0x0030) == 0)
					{
						ps(info, "rtm");
						startoperands(info);
						oad(info, opcode);
						info->num_oper = 1;
						only_if(info, "68020");
					} else
					{
						ps(info, "callm");
						startoperands(info);
						oi(info, 1, true);
						pcomma(info);
						doea(info, opcode, 0);
						info->num_oper = 2;
						only_if(info, "68020");
					}
					break;
				case 5:
				case 6:
				case 7:
					/* 0000 101 011 xxx yyy */
					/* 0000 110 011 xxx yyy */
					/* 0000 111 011 xxx yyy */
					if ((opcode & 0x3f) == 0x3c)
					{
						unsigned int opcode3;
						
						opcode2 = GETUW(info->memory_vma);
						info->memory_vma += 2;
						opcode3 = GETUW(info->memory_vma);
						info->memory_vma += 2;
						ps(info, "cas2");
						os2(info, opcode);
						od(info, opcode2 & 0x07);
						sputc(':');
						od(info, opcode3 & 0x07);
						pcomma(info);
						od(info, (opcode2 >> 6) & 0x07);
						sputc(':');
						od(info, (opcode3 >> 6) & 0x07);
						pcomma(info);
						oadi(info, (opcode2 >> 12) & 0x0f);
						pcomma(info);
						oadi(info, (opcode3 >> 12) & 0x0f);
					} else
					{
						opcode2 = GETUW(info->memory_vma);
						info->memory_vma += 2;
						ps(info, "cas");
						os2(info, opcode);
						od(info, opcode2 & 0x07);
						pcomma(info);
						od(info, (opcode2 >> 6) & 0x07);
						pcomma(info);
						doea(info, opcode, 0);
					}
					only_68020();
					info->num_oper = 3;
					break;
				default:
					odcw(info, opcode);
					break;
				}
			} else if (opcode2 == 7)
			{
				/* 0000 111 0ss yyy yyy */
	
				ps(info, "moves");
				os(info, opcode);
				opcode2 = GETUW(info->memory_vma);
				info->memory_vma += 2;
				if (opcode2 & 0x0800)
				{
					oad(info, (opcode2 >> 12) & 0x0f);
					pcomma(info);
					doea(info, opcode, 0);
				} else
				{
					doea(info, opcode, 0);
					pcomma(info);
					oad(info, (opcode2 >> 12) & 0x0f);
				}
				info->num_oper = 2;
			} else
			{
				ps(info, immedtable[opcode2]);
				size = os(info, opcode);
				oi(info, size, true);
				pcomma(info);
				if ((opcode & 0x003f) == 0x003c)
				{
					ps(info, opcode & 0x0040 ? "sr" : "ccr");
				} else
				{
					doea(info, opcode, size);
				}
				info->num_oper = 2;
			}
			break;
		}
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void group11(m68k_disasm_info *info, int opcode)
{
	register int size;

	if ((opcode & 0x0100) != 0 && insize(opcode) != 3)
	{
		if (srcmod(opcode) == 1)
		{
			ps(info, "cmpm");
			os(info, opcode);
			oai(info, srcreg(opcode));
			sputc('+');
			pcomma(info);
			oai(info, dstreg(opcode));
			sputc('+');
		} else
		{
			ps(info, "eor");
			size = os(info, opcode);
			oddstreg(info, opcode);
			pcomma(info);
			doea(info, opcode, size);
		}
	} else
	{
		ps(info, "cmp");
		switch (dstmod(opcode))
		{
		case 3:
		case 7:
			sputc('a');
			size = osa(info, opcode, 0x0100);
			doea(info, opcode, size);
			pcomma(info);
			oa(info, dstreg(opcode));
			break;
		default:
			size = os(info, opcode);
			doea(info, opcode, size);
			pcomma(info);
			oddstreg(info, opcode);
			break;
		}
	}
	info->num_oper = 2;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void group1(m68k_disasm_info *info, int opcode)
{
	register int size = 0;

	ps(info, "move");
	if (dstmod(opcode) == 1)
		sputc('a');
	sputc('.');
	switch (opcode >> 12)
	{
	case 1:							/* move.b */
		sputc('b');
		size = 0;
		break;
	case 2:							/* move.l */
		sputc('l');
		size = 2;
		break;
	case 3:							/* move.w */
		sputc('w');
		size = 1;
		break;
	}
	startoperands(info);
	doea(info, opcode, size);
	pcomma(info);
	doea(info, (dstmod(opcode) << 3) | dstreg(opcode), size);
	info->num_oper = 2;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void group14(m68k_disasm_info *info, int opcode)
{
	register short size;
	unsigned int opcode2;
	unsigned int bf;
	
	if ((size = insize(opcode)) == 3 && (opcode & 0x0800) == 0)
	{
		ps(info, shfttable[(opcode >> 9) & 3]);
		sputc(opcode & 0x0100 ? 'l' : 'r');
		ps(info, ".b");
		startoperands(info);
		doea(info, opcode, size);
		info->num_oper = 1;
	} else if (size == 3 && (opcode & 0x0800) != 0)
	{
		uae_u16 cmd = (opcode >> 8) & 7;
		
		only_68020();
		ps(info, bitfieldtable[cmd]);
		startoperands(info);
		opcode2 = GETUW(info->memory_vma);
		info->memory_vma += 2;
		info->num_oper = 1;
		if (cmd == 0x7) /* bfins */
		{
			od(info, (opcode2 >> 12) & 0x7);
			pcomma(info);
			info->num_oper = 2;
		}
		doea(info, opcode, size);
		sputc('{');
		bf = (opcode2 >> 6) & 0x1f;
		if (opcode2 & 0x800)
			od(info, bf);
		else
			pudec(info, bf);
		sputc(':');
		bf = opcode2 & 0x1f;
		if (opcode2 & 0x20)
		{
			od(info, bf);
		} else
		{
			if (bf == 0)
				bf = 32;
			pudec(info, bf);
		}
		sputc('}');
		if ((opcode & 0x100) && cmd != 0x7)
		{
			pcomma(info);
			od(info, (opcode2 >> 12) & 0x7);
			info->num_oper = 2;
		}
	} else
	{
		ps(info, shfttable[(opcode >> 3) & 3]);
		sputc(opcode & 0x0100 ? 'l' : 'r');
		os(info, opcode);
		if (opcode & 0x0020)
		{
			oddstreg(info, opcode);
		} else
		{
			sputc('#');
			psdec(info, dstreg(opcode) != 0 ? dstreg(opcode) : 8);
		}
		pcomma(info);
		od(info, srcreg(opcode));
		info->num_oper = 2;
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void pmmureg(m68k_disasm_info *info, int preg, int num)
{
	switch (preg)
	{
	case 0x02:
		ps(info, "tt0");
		break;
	case 0x03:
		ps(info, "tt1");
		break;
	case 0x10:
		ps(info, "tc");
		break;
	case 0x11:
		ps(info, "drp");
		break;
	case 0x12:
		ps(info, "srp");
		break;
	case 0x13:
		ps(info, "crp");
		break;
	case 0x14:
		ps(info, "cal");
		break;
	case 0x15:
		ps(info, "val");
		break;
	case 0x16:
		ps(info, "scc");
		break;
	case 0x17:
		ps(info, "ac");
		break;
	case 0x18:
		ps(info, "psr");
		break;
	case 0x19:
		ps(info, "pcsr");
		break;
	case 0x1c:
		ps(info, "bad");
		sputc(num + '0');
		break;
	case 0x1d:
		ps(info, "bac");
		sputc(num + '0');
		break;
	default:
		ps(info, "???");
		break;
	}
}

/*** ---------------------------------------------------------------------- ***/

static void group_pmmu_030(m68k_disasm_info *info, int opcode)
{
	unsigned int opcode2;
	uae_s32 adr;
	
	switch ((opcode >> 6) & 0x07)
	{
	case 0:
		opcode2 = GETUW(info->memory_vma);
		info->memory_vma += 2;
		info->reloffset += 2;
		switch (opcode2 & 0xE000)
		{
		case 0x0000:
		case 0x4000:
		case 0x6000:
			{
				int r;
				
				r = (opcode2 >> 10) & 0x1f;
				ps(info, "pmove");
				if (opcode2 & 0x100)
					ps(info, "fd");
				if (r == 0x11 || r == 0x12 || r == 0x13)
					ps(info, ".q");
				else if (r == 0x18)
					ps(info, ".w");
				else
					ps(info, ".l");
				startoperands(info);
				if (opcode2 & 0x0200)
				{
					pmmureg(info, r, (opcode2 >> 2) & 0x07);
					pcomma(info);
					doea(info, opcode, 0);
				} else
				{
					doea(info, opcode, 0);
					pcomma(info);
					pmmureg(info, r, (opcode2 >> 2) & 0x07);
				}
			}
			info->num_oper = 2;
			break;
		case 0x2000:
			switch (opcode2 & 0x1c00)
			{
			case 0x0400:
				ps(info, "pflusha");
				info->num_oper = 0;
				break;
			case 0x1000:
			case 0x1400:
			case 0x1800:
			case 0x1c00:
				ps(info, "pflush");
				if (opcode2 & 0x400)
					sputc('s');
				startoperands(info);
				switch (opcode2 & 0x18)
				{
				case 0x10:
				case 0x18:
					sputc('#');
					p2hex(info, opcode2 & 0x0f);
					break;
				case 0x08:
					od(info, opcode2 & 0x07);
					break;
				case 0x00:
					ps(info, opcode2 & 1 ? "dfc" : "sfc");
					break;
				}
				pcomma(info);
				sputc('#');
				p2hex(info, (opcode2 >> 5) & 0x07);
				if (opcode2 & 0x0800)
				{
					pcomma(info);
					info->num_oper = 3;
					doea(info, opcode, 0);
				} else
				{
					info->num_oper = 2;
				}
				break;
			case 0x0000:
				if ((opcode2 & 0x01de0) == 0x0000)
				{
					ps(info, "pload");
					sputc(opcode2 & 0x200 ? 'r' : 'w');
					startoperands(info);
					switch (opcode2 & 0x18)
					{
					case 0x10:
					case 0x18:
						sputc('#');
						p2hex(info, opcode2 & 0x0f);
						break;
					case 0x08:
						od(info, opcode2 & 0x07);
						break;
					case 0x00:
						ps(info, opcode2 & 1 ? "dfc" : "sfc");
						break;
					}
					pcomma(info);
					doea(info, opcode, 0);
					info->num_oper = 2;
				} else
				{
					info->memory_vma -= 2;
					odcw(info, opcode);
				}
				break;
			case 0x0800:
				ps(info, "pvalid");
				startoperands(info);
				doea(info, opcode, 1);
				info->num_oper = 1;
				break;
			default:
				info->memory_vma -= 2;
				odcw(info, opcode);
				break;
			}
			break;
		case 0xa000:
			ps(info, "pflushr");
			startoperands(info);
			doea(info, opcode, 0);
			info->num_oper = 1;
			break;
		case 0x8000:
			ps(info, "ptest");
			sputc(opcode2 & 0x0200 ? 'r' : 'w');
			startoperands(info);
			info->num_oper = 3;
			switch ((opcode2 >> 3) & 0x03)
			{
			case 0x00:
				if ((opcode2 & 7) == 0)
					ps(info, "sfc");
				else
					ps(info, "dfc");
				break;
			case 0x01:
				od(info, opcode2 & 0x07);
				break;
			case 0x02:
				sputc('#');
				p2hex(info, opcode2 & 0x07);
				break;
			case 0x03:
				sputc('#');
				p2hex(info, opcode2 & 0x0f);
				break;
			}
			pcomma(info);
			doea(info, opcode, 1);
			pcomma(info);
			pudec(info, (opcode2 >> 10) & 0x07);
			if (opcode2 & 0x100)
			{
				pcomma(info);
				info->num_oper++;
				oa(info, (opcode2 >> 5) & 7);
			}
			break;
		default:
			info->memory_vma -= 2;
			odcw(info, opcode);
			break;
		}
		break;
	case 1:
		if (srcmod(opcode) == 1)
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, "pdb");
			/*
			 * The manual states:
			 * "The value of the program counter used in the branch address
			 *  calculation is the address of the PDBcc instruction plus two."
			 * I think this is wrong, for other co-pros its the address of the
			 * displacement word.
			 */
			adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETW(info->memory_vma) + info->reloffset;
			info->memory_vma += 2;
			ps(info, pmmucondtable[opcode2 & 0x3f]);
			startoperands(info);
			od(info, opcode & 0x07);
			pcomma(info);
			p8hex(info, adr);
			info->num_oper = 2;
		} else if (srcmod(opcode) == 7 && srcreg(opcode) > 1)
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, "ptrap");
			ps(info, pmmucondtable[opcode2 & 0x3f]);
			switch (srcreg(opcode))
			{
			case 2:
				startoperands(info);
				oi(info, 1, true);
				info->num_oper = 1;
				break;
			case 3:
				startoperands(info);
				oi(info, 2, true);
				info->num_oper = 1;
				break;
			}
		} else
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, "ps");
			ps(info, pmmucondtable[opcode2 & 0x3f]);
			startoperands(info);
			doea(info, opcode, 1);
			info->num_oper = 1;
		}
		break;
	case 2:
	case 3:
		if (opcode & 0x40)
		{
			adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETL(info->memory_vma) + info->reloffset;
			info->memory_vma += 4;
			ps(info, "pb");
			ps(info, pmmucondtable[opcode & 0x3f]);
			ps(info, ".l");
		} else
		{
			adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETW(info->memory_vma) + info->reloffset;
			info->memory_vma += 2;
			ps(info, "pb");
			ps(info, pmmucondtable[opcode & 0x3f]);
			ps(info, ".w");
		}
		startoperands(info);
		p8hex(info, adr);
		info->num_oper = 1;
		break;
	case 4:
		switch (srcmod(opcode))
		{
		case 0:
			ps(info, "pflushn");
			startoperands(info);
			oai(info, srcreg(opcode));
			info->num_oper = 1;
			break;
		case 1:
			ps(info, "pflush");
			startoperands(info);
			oai(info, srcreg(opcode));
			info->num_oper = 1;
			break;
		case 2:
			ps(info, "pflushan"); /* conflicts with psave (ax) */
			info->num_oper = 0;
			break;
		case 3:
			ps(info, "pflusha");
			info->num_oper = 0;
			break;
		default:
			ps(info, "psave");
			startoperands(info);
			doea(info, opcode, 1);
			info->num_oper = 1;
			break;
		}
		break;
	case 5:
		ps(info, "prestore");
		startoperands(info);
		doea(info, opcode, 1);
		info->num_oper = 1;
		break;
	default:
		odcw(info, opcode);
		break;
	}
}

/*** ---------------------------------------------------------------------- ***/

static void group_pmmu_040(m68k_disasm_info *info, int opcode)
{
	static const char *const caches[4] = { "nc", "dc", "ic", "bc" };
	
	if (opcode & 0x0100)
	{
		if ((opcode & 0xd8) == 0x48)
		{
			ps(info, "ptest");
			sputc(opcode & 0x20 ? 'r' : 'w');
			startoperands(info);
			oai(info, srcreg(opcode));
			info->num_oper = 1;
		} else if ((opcode & 0xc0) == 0)
		{
			switch (srcmod(opcode))
			{
			case 0x00:
				ps(info, "pflushn");
				startoperands(info);
				oai(info, srcreg(opcode));
				info->num_oper = 1;
				break;
			case 0x01:
				ps(info, "pflush");
				startoperands(info);
				oai(info, srcreg(opcode));
				info->num_oper = 1;
				break;
			case 0x02:
				ps(info, "pflushan");
				info->num_oper = 0;
				break;
			case 0x03:
				ps(info, "pflusha");
				info->num_oper = 0;
				break;
			default:
				odcw(info, opcode);
				break;
			}
		} else
		{
			odcw(info, opcode);
		}
	} else
	{
		switch (srcmod(opcode))
		{
		case 0x0001:
			ps(info, "cinvl");
			startoperands(info);
			ps(info, caches[(opcode >> 6) & 3]);
			pcomma(info);
			oai(info, srcreg(opcode));
			info->num_oper = 2;
			break;
		case 0x0002:
			ps(info, "cinvp");
			startoperands(info);
			ps(info, caches[(opcode >> 6) & 3]);
			pcomma(info);
			oai(info, srcreg(opcode));
			info->num_oper = 2;
			break;
		case 0x0003:
			ps(info, "cinva");
			startoperands(info);
			ps(info, caches[(opcode >> 6) & 3]);
			info->num_oper = 1;
			break;
		case 0x0005:
			if (((opcode >> 6) & 3) == 0)
			{
				ps(info, "intouch");
				startoperands(info);
				info->num_oper = 1;
			} else
			{
				ps(info, "cpushl");
				startoperands(info);
				ps(info, caches[(opcode >> 6) & 3]);
				pcomma(info);
				info->num_oper = 2;
			}
			oai(info, srcreg(opcode));
			break;
		case 0x0006:
			ps(info, "cpushp");
			startoperands(info);
			ps(info, caches[(opcode >> 6) & 3]);
			pcomma(info);
			oai(info, srcreg(opcode));
			info->num_oper = 2;
			break;
		case 0x0007:
			ps(info, "cpusha");
			startoperands(info);
			ps(info, caches[(opcode >> 6) & 3]);
			info->num_oper = 1;
			break;
		default:
			odcw(info, opcode);
			break;
		}
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void pfreg(m68k_disasm_info *info, int reg)
{
	ps(info, fpregs[reg]);
}

/*** ---------------------------------------------------------------------- ***/

static enum fpu_size print_fpsize(m68k_disasm_info *info, int mask)
{
	enum fpu_size sz;

	switch (mask)
	{
	default:
	case 0:
		sputc('l');
		sz = FPU_SIZE_LONG;
		break;
	case 1:
		sputc('s');
		sz = FPU_SIZE_SINGLE;
		break;
	case 2:
		sputc('x');
		sz = FPU_SIZE_EXTENDED;
		break;
	case 3:
		sputc('p');
		sz = FPU_SIZE_PACKED;
		break;
	case 4:
		sputc('w');
		sz = FPU_SIZE_WORD;
		break;
	case 5:
		sputc('d');
		sz = FPU_SIZE_DOUBLE;
		break;
	case 6:
		sputc('b');
		sz = FPU_SIZE_BYTE;
		break;
	case 7:
		sputc('p');
		sz = FPU_SIZE_PACKED_VARIABLE;
		break;
	}
	return sz;
}

/*** ---------------------------------------------------------------------- ***/

static void print_freglist(m68k_disasm_info *info, int regmask, int mode, bool cntl)
{
	const char *const * regs;
	register int regnum;
	int liststart;
	int lastreg;
	int status;
	int upper;
	
	regs = cntl ? fpcregs : fpregs;
	upper = cntl ? 3 : 8;
	if (!cntl && mode != 4)
	{
		regmask = revbits(regmask, upper);
	}
	liststart = status = 0;
	lastreg = 20;
	for (regnum = 0; regnum < upper; regnum++)
	{
		if ((1 << regnum) & regmask)
		{
			if (regnum == (lastreg + 1) && regnum != 8)
			{
				if (liststart != 0)
					sputc('-');
				liststart = 0;
				status = 1;
				lastreg = regnum;
			} else
			{
				if (status != 0)
				{
					ps(info, regs[lastreg]);
					status = 0;
					liststart = 1;
				}
				if (liststart != 0)
					sputc('/');
				ps(info, regs[regnum]);
				lastreg = regnum;
				liststart = 1;
			}
		}
	}
	if (status != 0)
		ps(info, regs[lastreg]);
}

/*** ---------------------------------------------------------------------- ***/

static void fpu_stdgen(m68k_disasm_info *info, unsigned int opcode, unsigned int opcode2, const char *name)
{
	enum fpu_size sz;

	ps(info, name);

	info->num_oper = 1;
	sputc('.');
	if ((opcode2 & 0x4000) != 0)
	{
		sz = print_fpsize(info, (opcode2 >> 10) & 0x07);
		startoperands(info);
		doea(info, opcode, sz);
		if (((opcode2 >> 3) & 0x0f) == 6) /* fsincos */
		{
			pcomma(info);
			pfreg(info, (opcode2 >> 0) & 0x07);
			sputc(':');
			pfreg(info, (opcode2 >> 7) & 0x07);
			info->num_oper = 2;
		} else if ((opcode2 & 0x3f) != 0x3a) /* ftst */
		{
			pcomma(info);
			pfreg(info, (opcode2 >> 7) & 0x07);
			info->num_oper = 2;
		}
	} else
	{
		sputc('x');
		startoperands(info);
		pfreg(info, (opcode2 >> 10) & 0x07);
		if (((opcode2 >> 3) & 0x0f) == 6) /* fsincos */
		{
			pcomma(info);
			pfreg(info, (opcode2 >> 0) & 0x07);
			sputc(':');
			pfreg(info, (opcode2 >> 7) & 0x07);
			info->num_oper = 2;
		} else if ((opcode2 & 0x3f) != 0x3a) /* ftst */
		{
			pcomma(info);
			pfreg(info, (opcode2 >> 7) & 0x07);
			info->num_oper = 2;
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

static void group_fpu(m68k_disasm_info *info, int opcode)
{
	unsigned int opcode2;
	uae_s32 adr;
	enum fpu_size sz;
	
	switch ((opcode >> 6) & 0x07)
	{
	case 0:
		opcode2 = GETUW(info->memory_vma);
		info->memory_vma += 2;
		info->reloffset += 2;
		switch ((opcode2 >> 13) & 0x07)
		{
		case 0:
		case 2:
			if ((opcode & 0x01ff) == 0x0000 && (opcode2 & 0xfc00) == 0x5c00)
			{
				ps(info, "fmovecr.x");
				startoperands(info);
				sputc('#');
				p2hex(info, opcode2 & 0x7f);
				pcomma(info);
				pfreg(info, (opcode2 >> 7) & 7);
				info->num_oper = 2;
				switch (opcode2 & 0x7f)
				{
					case 0x00: strcpy(info->comments, "pi"); break;
					case 0x0b: strcpy(info->comments, "log10(2)"); break;
					case 0x0c: strcpy(info->comments, "e"); break;
					case 0x0d: strcpy(info->comments, "log2(e)"); break;
					case 0x0e: strcpy(info->comments, "log10(e)"); break;
					case 0x0f: strcpy(info->comments, "0.0"); break;
					case 0x30: strcpy(info->comments, "ln(2)"); break;
					case 0x31: strcpy(info->comments, "ln(10)"); break;
					case 0x32: strcpy(info->comments, "10e0"); break;
					case 0x33: strcpy(info->comments, "10e1"); break;
					case 0x34: strcpy(info->comments, "10e2"); break;
					case 0x35: strcpy(info->comments, "10e4"); break;
					case 0x36: strcpy(info->comments, "10e8"); break;
					case 0x37: strcpy(info->comments, "10e16"); break;
					case 0x38: strcpy(info->comments, "10e32"); break;
					case 0x39: strcpy(info->comments, "10e64"); break;
					case 0x3a: strcpy(info->comments, "10e128"); break;
					case 0x3b: strcpy(info->comments, "10e256"); break;
					case 0x3c: strcpy(info->comments, "10e512"); break;
					case 0x3d: strcpy(info->comments, "10e1024"); break;
					case 0x3e: strcpy(info->comments, "10e2048"); break;
					case 0x3f: strcpy(info->comments, "10e4096"); break;
				}
			} else
			{
				switch (opcode2 & 0x7f)
				{
				case 0x00:
					fpu_stdgen(info, opcode, opcode2, "fmove");
					break;
				case 0x40:
					fpu_stdgen(info, opcode, opcode2, "fsmove");
					break;
				case 0x44:
					fpu_stdgen(info, opcode, opcode2, "fdmove");
					break;
				case 0x01:
					fpu_stdgen(info, opcode, opcode2, "fint");
					break;
				case 0x02:
					fpu_stdgen(info, opcode, opcode2, "fsinh");
					break;
				case 0x03:
					fpu_stdgen(info, opcode, opcode2, "fintrz");
					break;
				case 0x04:
					fpu_stdgen(info, opcode, opcode2, "fsqrt");
					break;
				case 0x45:
					fpu_stdgen(info, opcode, opcode2, "fdsqrt");
					break;
				case 0x41:
					fpu_stdgen(info, opcode, opcode2, "fssqrt");
					break;
				/* 0x05: illegal */
				case 0x06:
					fpu_stdgen(info, opcode, opcode2, "flognp1");
					break;
				case 0x46:
					fpu_stdgen(info, opcode, opcode2, "fdlognp1");
					break;
				/* 0x07: illegal */
				case 0x08:
					fpu_stdgen(info, opcode, opcode2, "fetoxm1");
					break;
				case 0x09:
					fpu_stdgen(info, opcode, opcode2, "ftanh");
					break;
				case 0x0a:
					fpu_stdgen(info, opcode, opcode2, "fatan");
					break;
				/* 0x0b: illegal */
				case 0x0c:
					fpu_stdgen(info, opcode, opcode2, "fasin");
					break;
				case 0x0d:
					fpu_stdgen(info, opcode, opcode2, "fatanh");
					break;
				case 0x0e:
					fpu_stdgen(info, opcode, opcode2, "fsin");
					break;
				case 0x0f:
					fpu_stdgen(info, opcode, opcode2, "ftan");
					break;
				case 0x10:
					fpu_stdgen(info, opcode, opcode2, "fetox");
					break;
				case 0x11:
					fpu_stdgen(info, opcode, opcode2, "ftwotox");
					break;
				case 0x12:
					fpu_stdgen(info, opcode, opcode2, "ftentox");
					break;
				/* 0x13: illegal */
				case 0x14:
					fpu_stdgen(info, opcode, opcode2, "flogn");
					break;
				case 0x15:
					fpu_stdgen(info, opcode, opcode2, "flog10");
					break;
				case 0x16:
					fpu_stdgen(info, opcode, opcode2, "flog2");
					break;
				/* 0x17: illegal */
				case 0x18:
					fpu_stdgen(info, opcode, opcode2, "fabs");
					break;
				case 0x58:
					fpu_stdgen(info, opcode, opcode2, "fsabs");
					break;
				case 0x5c:
					fpu_stdgen(info, opcode, opcode2, "fdabs");
					break;
				case 0x19:
					fpu_stdgen(info, opcode, opcode2, "fcosh");
					break;
				case 0x1a:
					fpu_stdgen(info, opcode, opcode2, "fneg");
					break;
				case 0x5a:
					fpu_stdgen(info, opcode, opcode2, "fsneg");
					break;
				case 0x5e:
					fpu_stdgen(info, opcode, opcode2, "fdneg");
					break;
				/* 0x1b: illegal */
				case 0x1c:
					fpu_stdgen(info, opcode, opcode2, "facos");
					break;
				case 0x1d:
					fpu_stdgen(info, opcode, opcode2, "fcos");
					break;
				case 0x1e:
					fpu_stdgen(info, opcode, opcode2, "fgetexp");
					break;
				case 0x1f:
					fpu_stdgen(info, opcode, opcode2, "fgetman");
					break;
				case 0x20:
					fpu_stdgen(info, opcode, opcode2, "fdiv");
					break;
				case 0x60:
					fpu_stdgen(info, opcode, opcode2, "fsdiv");
					break;
				case 0x64:
					fpu_stdgen(info, opcode, opcode2, "fddiv");
					break;
				case 0x21:
					fpu_stdgen(info, opcode, opcode2, "fmod");
					break;
				case 0x22:
					fpu_stdgen(info, opcode, opcode2, "fadd");
					break;
				case 0x62:
					fpu_stdgen(info, opcode, opcode2, "fsadd");
					break;
				case 0x66:
					fpu_stdgen(info, opcode, opcode2, "fdadd");
					break;
				case 0x23:
					fpu_stdgen(info, opcode, opcode2, "fmul");
					break;
				case 0x63:
					fpu_stdgen(info, opcode, opcode2, "fsmul");
					break;
				case 0x67:
					fpu_stdgen(info, opcode, opcode2, "fdmul");
					break;
				case 0x24:
					fpu_stdgen(info, opcode, opcode2, "fsgldiv");
					break;
				case 0x25:
					fpu_stdgen(info, opcode, opcode2, "frem");
					break;
				case 0x26:
					fpu_stdgen(info, opcode, opcode2, "fscale");
					break;
				case 0x27:
					fpu_stdgen(info, opcode, opcode2, "fsglmul");
					break;
				case 0x28:
					fpu_stdgen(info, opcode, opcode2, "fsub");
					break;
				case 0x68:
					fpu_stdgen(info, opcode, opcode2, "fssub");
					break;
				case 0x6c:
					fpu_stdgen(info, opcode, opcode2, "fdsub");
					break;
				/* 0x29: illegal */
				/* 0x2a: illegal */
				/* 0x2b: illegal */
				/* 0x2c: illegal */
				/* 0x2d: illegal */
				/* 0x2d: illegal */
				/* 0x2e: illegal */
				/* 0x2f: illegal */
				case 0x30:
				case 0x31:
				case 0x32:
				case 0x33:
				case 0x34:
				case 0x35:
				case 0x36:
				case 0x37:
					fpu_stdgen(info, opcode, opcode2, "fsincos");
					break;
				case 0x38:
					fpu_stdgen(info, opcode, opcode2, "fcmp");
					break;
				/* 0x39: illegal */
				case 0x3a:
					fpu_stdgen(info, opcode, opcode2, "ftst");
					break;
				/* 0x3b: illegal */
				/* 0x3c: illegal */
				/* 0x3d: illegal */
				/* 0x3e: illegal */
				/* 0x3f: illegal */
				default:
					odcw(info, opcode);
					pcomma(info);
					p4hex(info, opcode2);
					info->num_oper = 2;
					break;
				}
			}
			break;
		case 3:
			info->num_oper = 2;
			/* fmove r ==> m */
			ps(info, "fmove.");
			sz = print_fpsize(info, (opcode2 >> 10) & 0x07);
			startoperands(info);
			pfreg(info, (opcode2 >> 7) & 0x07);
			pcomma(info);
			doea(info, opcode, sz);
			if (sz == FPU_SIZE_PACKED_VARIABLE)
			{
				sputc('{');
				if ((opcode2 & 0x1000) != 0)
				{
					od(info, (opcode2 >> 4) & 0x7);
				} else
				{
					sputc('#');
					pudec(info, (opcode2 >> 4) & 0x07);
				}
				sputc('}');
			}
			break;
		case 4:
		case 5:
			info->num_oper = 2;
			switch ((opcode2 >> 10) & 0x07)
			{
			case 0x01:
			case 0x02:
			case 0x04:
				ps(info, "fmove.l");
				break;
			default:
				ps(info, "fmovem.l");
				break;
			}
			/* fmove[m] control reg */
			startoperands(info);
	
			if ((opcode2 & 0x2000) != 0)
			{
				print_freglist(info, (opcode2 >> 10) & 0x07, 4, true);
				pcomma(info);
			}
			doea(info, opcode, FPU_SIZE_LONG);
			if ((opcode2 & 0x2000) == 0)
			{
				pcomma(info);
				print_freglist(info, (opcode2 >> 10) & 0x07, 4, true);
			}
			info->num_oper = 2;
			break;
		case 6:
		case 7:
			info->num_oper = 2;
			ps(info, "fmovem.x");
			startoperands(info);
		
			if ((opcode2 & 0x0800) != 0)
			{
				if ((opcode2 & 0x2000) != 0)
				{
					od(info, (opcode2 >> 4) & 0x07);
					pcomma(info);
				}
				doea(info, opcode, FPU_SIZE_EXTENDED);
				if ((opcode2 & 0x2000) == 0)
				{
					pcomma(info);
					od(info, (opcode2 >> 4) & 0x07);
				}
			} else
			{
				if ((opcode2 & 0x2000) != 0)
				{
					print_freglist(info, opcode2 & 0xff, srcmod(opcode), false);
					pcomma(info);
				}
				doea(info, opcode, FPU_SIZE_EXTENDED);
				if ((opcode2 & 0x2000) == 0)
				{
					pcomma(info);
					print_freglist(info, opcode2 & 0xff, srcmod(opcode), false);
				}
			}
			break;
		}
		break;
	case 1:
		if (srcmod(opcode) == 1)
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, "fdb");
			/*
			 * The value of the program counter used in the branch
			 * address calculation is the address of the displacement word.
			 */
			adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETW(info->memory_vma) + info->reloffset;
			info->memory_vma += 2;
			ps(info, fpucondtable[opcode2 & 0x3f]);
			startoperands(info);
			od(info, opcode & 0x07);
			pcomma(info);
			p8hex(info, adr);
			info->num_oper = 2;
		} else if (srcmod(opcode) == 7 && srcreg(opcode) > 1)
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, "ftrap");
			ps(info, fpucondtable[opcode2 & 0x3f]);
			switch (srcreg(opcode))
			{
			case 2:
				startoperands(info);
				oi(info, 1, true);
				info->num_oper = 1;
				break;
			case 3:
				startoperands(info);
				oi(info, 2, true);
				info->num_oper = 1;
				break;
			}
		} else
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, "fs");
			ps(info, fpucondtable[opcode2 & 0x3f]);
			startoperands(info);
			doea(info, opcode, 1);
			info->num_oper = 1;
		}
		break;
	case 2:
	case 3:
		opcode2 = GETUW(info->memory_vma);
		if (((opcode >> 6) & 0x07) == 2 && (opcode & 0x3f) == 0 && opcode2 == 0)
		{
			ps(info, "fnop");
			return;
		}
		if (opcode & 0x40)
		{
			adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETL(info->memory_vma) + info->reloffset;
			info->memory_vma += 4;
			ps(info, "fb");
			ps(info, fpucondtable[opcode & 0x3f]);
			ps(info, ".l");
		} else
		{
			adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETW(info->memory_vma) + info->reloffset;
			info->memory_vma += 2;
			ps(info, "fb");
			ps(info, fpucondtable[opcode & 0x3f]);
			ps(info, ".w");
		}
		startoperands(info);
		p8hex(info, adr);
		info->num_oper = 1;
		break;
	case 4:
		ps(info, "fsave");
		startoperands(info);
		doea(info, opcode, 0);
		info->num_oper = 1;
		break;
	case 5:
		ps(info, "frestore");
		startoperands(info);
		doea(info, opcode, 0);
		info->num_oper = 1;
		break;
	default:
		odcw(info, opcode);
		break;
	}
}

/*** ---------------------------------------------------------------------- ***/

static void coprocessor_general(m68k_disasm_info *info, int opcode, const char *prefix)
{
	int adr;
	unsigned int opcode2;
	
	switch ((opcode >> 6) & 0x07)
	{
	case 0:
		opcode2 = GETUW(info->memory_vma);
		info->memory_vma += 2;
		info->reloffset += 2;
		ps(info, prefix);
		ps(info, "GEN");
		startoperands(info);
		oi(info, 1, true);
		pcomma(info);
		doea(info, opcode, 1);
		info->num_oper = 2;
		break;
	case 1:
		if (srcmod(opcode) == 1)
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, prefix);
			ps(info, "DB");
			/*
			 * The value of the scan PC is the address of the displacement word.
			 */
			adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETW(info->memory_vma) + info->reloffset;
			info->memory_vma += 2;
			ps(info, cpcondtable[opcode2 & 0x3f]);
			startoperands(info);
			od(info, opcode & 0x07);
			pcomma(info);
			p8hex(info, adr);
			info->num_oper = 2;
		} else if (srcmod(opcode) == 7 && srcreg(opcode) > 1)
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, prefix);
			ps(info, "TRAP");
			ps(info, cpcondtable[opcode2 & 0x3f]);
			switch (srcreg(opcode))
			{
			case 2:
				startoperands(info);
				oi(info, 1, true);
				info->num_oper = 1;
				break;
			case 3:
				startoperands(info);
				oi(info, 2, true);
				info->num_oper = 1;
				break;
			}
		} else
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, prefix);
			ps(info, "S");
			ps(info, cpcondtable[opcode2 & 0x3f]);
			startoperands(info);
			doea(info, opcode, 1);
			info->num_oper = 1;
		}
		break;
	case 2:
		adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETW(info->memory_vma) + info->reloffset;
		info->memory_vma += 2;
		ps(info, prefix);
		ps(info, "B");
		ps(info, cpcondtable[opcode & 0x3f]);
		ps(info, ".w");
		startoperands(info);
		p8hex(info, adr);
		info->num_oper = 1;
		break;
	case 3:
		adr = ((struct priv_data *)(info->disasm_data))->oldaddr + GETL(info->memory_vma) + info->reloffset;
		info->memory_vma += 4;
		ps(info, prefix);
		ps(info, "B");
		ps(info, cpcondtable[opcode & 0x3f]);
		ps(info, ".l");
		startoperands(info);
		p8hex(info, adr);
		info->num_oper = 1;
		break;
	case 4:
	case 5:
	case 6:
		if (((opcode >> 9) & 0x07) == 5)
		{
			ps(info, "wddata");
			os(info, opcode);
			doea(info, opcode, insize(opcode));
			info->num_oper = 1;
		} else
		{
			odcw(info, opcode);
		}
		break;
	case 7:
		if (((opcode >> 9) & 0x07) == 5)
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			ps(info, "wdebug");
			startoperands(info);
			doea(info, opcode, 2);
			info->num_oper = 1;
		} else
		{
			odcw(info, opcode);
		}
		break;
	default:
		odcw(info, opcode);
		break;
	}
}

/*** ---------------------------------------------------------------------- ***/

static void group15(m68k_disasm_info *info, int opcode)
{
	int opcode2;
	
	switch ((opcode >> 9) & 0x07)
	{
	case 0:
		/* ID 0: m68851 or 68030 onchip pmmu */
		if (info->cpu == CPU_68020 && info->mmu != MMU_NONE)
		{
			if ((opcode & 0x01c0) != 0)
				coprocessor_general(info, opcode, "p");
			else
				group_pmmu_030(info, opcode);
		} else
		{
			group_pmmu_030(info, opcode);
		}
		break;
	case 1:
		/* ID 1: m68881/2 fpu */
		group_fpu(info, opcode);
		break;
	case 2:
		/* ID 2: 68040/60 on chip pmmu */
		group_pmmu_040(info, opcode);
		break;
	case 3:
		/* ID 3: move16 */
		odcw(info, opcode);
		break;
	case 4:
		/* ID 4: */
		if ((opcode & 0x01c0) == 0 && (opcode & 0x003f) == 0 && GETUW(info->memory_vma) == 0x01c0)
		{
			ps(info, "lpstop");
			info->memory_vma += 2;
			info->reloffset += 2;
			startoperands(info);
			oi(info, 1, true);
			info->num_oper = 1;
		} else if ((opcode & 0x01c0) == 0 && (GETUW(info->memory_vma) & 0x8338) == 0x0100)
		{
			opcode2 = GETUW(info->memory_vma);
			info->memory_vma += 2;
			info->reloffset += 2;
			ps(info, "tbl");
			sputc(opcode2 & 0x0800 ? 's' : 'u');
			if (opcode2 & 0x400)
				sputc('n');
			os(info, opcode2);
			if (srcmod(opcode) == 0)
			{
				od(info, srcreg(opcode));
				sputc(':');
				od(info, srcreg(opcode2));
			} else
			{
				doea(info, opcode, insize(opcode2));
			}
			pcomma(info);
			od(info, (opcode2 >> 12) & 7);
			info->num_oper = 2;
		} else
		{
			coprocessor_general(info, opcode, "lp");
		}
		break;
	case 5:
		/* ID 5: ColdFire FPU */
		coprocessor_general(info, opcode, "cp5");
		break;
	case 6:
		/* ID 6: */
		coprocessor_general(info, opcode, "cp6");
		break;
	case 7:
		/* ID 7: */
		coprocessor_general(info, opcode, "nf");
		break;
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void pspreg(m68k_disasm_info *info, int mask)
{
	const char *reg = "???";
	
	switch (mask & 0xfff)
	{
	case 0x000:
		reg = "sfc";
		break;
	case 0x001:
		reg = "dfc";
		break;
	case 0x002:
		/* if (info->cpu >= CPU_68020 && info->cpu != CPU_CPU32) */
			reg = "cacr";
		break;
	case 0x003:
		/* if (info->mmu == MMU_68851 || info->cpu == CPU_68030 || info->cpu == CPU_68040 || info->cpu == CPU_68LC040 || info->cpu == CPU_68060) */
			reg = "tc";
		if (info->cpu >= CPU_CF_FIRST && info->cpu <= CPU_CF_LAST)
			reg = "asid";
		break;
	case 0x004:
		/* if (info->cpu == CPU_68040 || info->cpu == CPU_68LC040 || info->cpu == CPU_68060) */
			reg = "itt0";
		if (info->cpu == CPU_68EC040)
			reg = "iacr0";
		if (info->cpu >= CPU_CF_FIRST && info->cpu <= CPU_CF_LAST)
			reg = "acr0";
		break;
	case 0x005:
		/* if (info->cpu == CPU_68040 || info->cpu == CPU_68LC040 || info->cpu == CPU_68060) */
			reg = "itt1";
		if (info->cpu == CPU_68EC040)
			reg = "iacr1";
		if (info->cpu >= CPU_CF_FIRST && info->cpu <= CPU_CF_LAST)
			reg = "acr1";
		break;
	case 0x006:
		/* if (info->cpu == CPU_68040 || info->cpu == CPU_68LC040 || info->cpu == CPU_68060) */
			reg = "dtt0";
		if (info->cpu == CPU_68EC040)
			reg = "dacr0";
		if (info->cpu >= CPU_CF_FIRST && info->cpu <= CPU_CF_LAST)
			reg = "acr2";
		break;
	case 0x007:
		/* if (info->cpu == CPU_68040 || info->cpu == CPU_68LC040 || info->cpu == CPU_68060) */
			reg = "dtt1";
		if (info->cpu == CPU_68EC040)
			reg = "dacr1";
		if (info->cpu >= CPU_CF_FIRST && info->cpu <= CPU_CF_LAST)
			reg = "acr3";
		break;
	case 0x008:
		/* if (info->cpu == CPU_68060) */
			reg = "buscr";
		if (info->cpu >= CPU_CF_FIRST && info->cpu <= CPU_CF_LAST)
			reg = "mmubar";
		break;
	case 0x009:
		reg = "rgpiobar";
		break;
	case 0x00c:
		reg = "acr4";
		break;
	case 0x00d:
		reg = "acr5";
		break;
	case 0x00e:
		reg = "acr6";
		break;
	case 0x00f:
		reg = "acr7";
		break;
	case 0x800:
		reg = "usp";
		break;
	case 0x801:
		reg = "vbr";
		break;
	case 0x802:
		/* if (info->cpu == CPU_68020 || info->cpu == CPU_68030 || info->cpu == CPU_68EC030) */
			reg = "caar";
		break;
	case 0x803:
		reg = "msp";
		break;
	case 0x804:
		reg = "isp";
		break;
	case 0x805:
		/* if (info->mmu == MMU_68851 || info->cpu == CPU_68030 || info->cpu == CPU_68040 || info->cpu == CPU_68LC040) */
			reg = "psr"; /* sometimes named mmusr */
		break;
	case 0x806:
		/* if (info->cpu == CPU_68040 || info->cpu == CPU_68LC040 || info->cpu == CPU_68060) */
			reg = "urp";
		break;
	case 0x807:
		/* if (info->mmu == MMU_68851 || info->cpu == CPU_68030 || info->cpu == CPU_68040 || info->cpu == CPU_68LC040 || info->cpu == CPU_68060) */
			reg = "srp";
		break;
	case 0x808:
		/* if (info->cpu == CPU_68060) */
			reg = "pcr";
		break;
	case 0x80e:
		reg = "sr";
		break;
	case 0x80f:
		reg = "pc";
		break;
	case 0xc00:
		reg = "rombar0";
		break;
	case 0xc01:
		reg = "rombar1";
		break;
	case 0xc04:
		reg = "rambar0";
		break;
	case 0xc05:
		reg = "rambar1";
		break;
	case 0xc0c:
		reg = "mpcr";
		break;
	case 0xc0d:
		reg = "edrambar";
		break;
	case 0xc0e:
		reg = "secmbar"; /* sometimes called mbar0 or mbar2 */
		break;
	case 0xc0f:
		reg = "mbar"; /* sometimes called mbar1 */
		break;
	case 0xd02:
		reg = "pcr1u0";
		break;
	case 0xd03:
		reg = "pcr1l0";
		break;
	case 0xd04:
		reg = "pcr2u0";
		break;
	case 0xd05:
		reg = "pcr2l0";
		break;
	case 0xd06:
		reg = "pcr3u0";
		break;
	case 0xd07:
		reg = "pcr3l0";
		break;
	case 0xd0a:
		reg = "pcr1u1";
		break;
	case 0xd0b:
		reg = "pcr1l1";
		break;
	case 0xd0c:
		reg = "pcr2u1";
		break;
	case 0xd0d:
		reg = "pcr2l1";
		break;
	case 0xd0e:
		reg = "pcr3u1";
		break;
	case 0xd0f:
		reg = "pcr3l1";
		break;
	case 0xffe:
		reg = "cac";
		break;
	case 0xfff:
		reg = "mbo";
		break;
	}
	ps(info, reg);
}

/*** ---------------------------------------------------------------------- ***/

static void group4(m68k_disasm_info *info, int opcode)
{
	register int mask;
	register int size;

	if (opcode & 0x0100)
	{
		/* 0100 000 1 ss 000000 */
		switch (insize(opcode))
		{
		case 1:
		case 3:
			if ((opcode & 0xfff8) == 0x49c0)
			{
				ps(info, "extb.l");
				startoperands(info);
				od(info, srcreg(opcode));
				info->num_oper = 1;
				only_68020();
			} else
			{
				ps(info, "lea.l");
				startoperands(info);
				doea(info, opcode, 2);
				pcomma(info);
				oa(info, dstreg(opcode));
				info->num_oper = 2;
			}
			break;
		case 2:
			ps(info, "chk.w");
			startoperands(info);
			doea(info, opcode, 1);
			pcomma(info);
			oddstreg(info, opcode);
			info->num_oper = 2;
			break;
		case 0:
			ps(info, "chk.l");
			startoperands(info);
			doea(info, opcode, 1);
			pcomma(info);
			oddstreg(info, opcode);
			info->num_oper = 2;
			only_68020();
			break;
		default:
			odcw(info, opcode);
			break;
		}
	} else
	{
		switch (dstreg(opcode))
		{
		case 0:						/* move from sr, negx */
			if ((size = insize(opcode)) == 3)
			{
				/* 0100 000 011 xxxxxx */
				ps(info, "move.w");
				startoperands(info);
				ps(info, "sr");
				pcomma(info);
				doea(info, opcode, 1);
				info->num_oper = 2;
			} else
			{
				ps(info, "negx");
				os(info, opcode);
				doea(info, opcode, size);
				info->num_oper = 1;
			}
			break;
		case 1:						/* clr */
			if (insize(opcode) == 3)
			{
				/* 0100 001 011 xxxxxx */
				/* move.w from ccr, illegal for CPU_68000 */
				ps(info, "move.w");
				startoperands(info);
				ps(info, "ccr");
				pcomma(info);
				doea(info, opcode, 1);
				info->num_oper = 2;
			} else
			{
				ps(info, "clr");
				size = os(info, opcode);
				doea(info, opcode, size);
				info->num_oper = 1;
			}
			break;
		case 2:						/* move to ccr, neg */
			if ((size = insize(opcode)) == 3)
			{
				/* 0100 010 011 xxxxxx */
				ps(info, "move.b");
				startoperands(info);
				doea(info, opcode, 1);
				pcomma(info);
				ps(info, "ccr");
				info->num_oper = 2;
			} else
			{
				ps(info, "neg");
				os(info, opcode);
				doea(info, opcode, size);
				info->num_oper = 1;
			}
			break;
		case 3:						/* move to sr, not */
			if ((size = insize(opcode)) == 3)
			{
				ps(info, "move.w");
				startoperands(info);
				doea(info, opcode, 1);
				pcomma(info);
				ps(info, "sr");
				info->num_oper = 2;
			} else
			{
				ps(info, "not");
				os(info, opcode);
				doea(info, opcode, size);
				info->num_oper = 1;
			}
			break;
		case 4:						/* ext, movem to ram, nbcd, pea, swap */
			switch (insize(opcode))
			{
			case 0:
				/* 0100 100 000 yyyyyy */
				if (srcmod(opcode) == 1)
				{
					ps(info, "link.l");
					startoperands(info);
					doea(info, opcode, 2);
					pcomma(info);
					oi(info, 2, true);
					info->num_oper = 2;
					only_68020();
				} else
				{
					ps(info, "nbcd");
					startoperands(info);
					doea(info, opcode, 0);
					info->num_oper = 1;
				}
				break;
			case 1:
				/* 0100 100 001 yyyyyy */
				if (srcmod(opcode) != 0)
				{
					if (srcmod(opcode) == 1)
					{
						ps(info, "bkpt");
						startoperands(info);
						sputc('#');
						psdec(info, opcode & 7);
					} else
					{
						ps(info, "pea.l");
						startoperands(info);
						doea(info, opcode, 2);
					}
				} else
				{
					ps(info, "swap");
					startoperands(info);
					od(info, srcreg(opcode));
				}
				info->num_oper = 1;
				break;
			case 2:
			case 3:
				/* 0100 100 010 yyyyyy */
				/* 0100 100 011 yyyyyy */
				if (srcmod(opcode) == 0)
				{
					ps(info, "ext");
					osa(info, opcode, 0x0040);
					od(info, srcreg(opcode));
					info->num_oper = 1;
				} else
				{
					ps(info, "movem");
					size = osa(info, opcode, 0x0040);
					mask = GETW(info->memory_vma);
					info->memory_vma += 2;
					if (srcmod(opcode) == 4)
					{
						reglist(info, revbits(mask, 16));
					} else
					{
						reglist(info, mask);
					}
					pcomma(info);
					doea(info, opcode, size);
					info->num_oper = 2;
				}
				break;
			}
			break;
		case 5:						/* illegal, tas, tst */
			if ((size = insize(opcode)) == 3)
			{
				if (srcmod(opcode) == 7 && srcreg(opcode) > 1)
				{
					info->num_oper = 0;
					switch (srcreg(opcode))
					{
					case 2:
						ps(info, "bgnd");
						only_if(info, "cpu32 & fidoa");
						break;
					case 5:
						ps(info, "swbeg.l");
						only_if(info, "sysV68");
						startoperands(info);
						oi(info, 2, true);
						info->num_oper = 1;
						break;
					case 4:
						ps(info, "illegal");
						break;
					default:
						odcw(info, opcode);
						break;
					}
				} else if (srcmod(opcode) == 1)
				{
					info->num_oper = 0;
					switch (srcreg(opcode))
					{
					case 0:
						ps(info, "halt");
						only_if(info, "68060 & ColdFire");
						break;
					case 4:
						ps(info, "pulse");
						only_if(info, "68060 & ColdFire");
						break;
					default:
						odcw(info, opcode);
						break;
					}
				} else
				{
					ps(info, "tas.b");
					startoperands(info);
					doea(info, opcode, 1);
					info->num_oper = 1;
				}
			} else
			{
				ps(info, "tst");
				os(info, opcode);
				doea(info, opcode, size);
				info->num_oper = 1;
			}
			break;
		case 6:
			if (opcode & 0x0080)
			{
				switch (srcmod(opcode))
				{
				case 2:
				case 3:
				case 5:
				case 6:
				case 7:
					/* movem from ram */
					if (srcmod(opcode) != 7 || srcreg(opcode) <= 1)
					{
						ps(info, "movem");
						size = osa(info, opcode, 0x0040);
						mask = GETW(info->memory_vma);
						info->memory_vma += 2;
						doea(info, opcode, size);
						pcomma(info);
						reglist(info, mask);
						info->num_oper = 2;
					} else
					{
						odcw(info, opcode);
					}
					break;
				case 0:
					ps(info, "sats.l");
					startoperands(info);
					od(info, srcreg(opcode));
					break;
				default:
					odcw(info, opcode);
					break;
				}
			} else
			{
				uae_u16 opcode2;
				uae_u16 dr, dq;
				
				only_68020();
				opcode2 = GETUW(info->memory_vma);
				info->memory_vma += 2;
				dr = srcreg(opcode2);
				dq = (opcode2 >> 12) & 0x07;
				ps(info, opcode & 0x0040 ? "div" : "mul");
				sputc(opcode2 & 0x0800 ? 's' : 'u');
				if (opcode & 0x0040)
				{
					if (opcode2 & 0x0400)
					{
						/* div 64/32 -> dr/dq */
						info->num_oper = 3;
						ps(info, ".l");
						startoperands(info);
						doea(info, opcode, 2);
						pcomma(info);
						od(info, dr);
						pcomma(info);
						od(info, dq);
					} else
					{
						/* 32/32 -> dr/dq */
						info->num_oper = 3;
						ps(info, "l.l");
						startoperands(info);
						doea(info, opcode, 2);
						pcomma(info);
						od(info, dr);
						pcomma(info);
						od(info, dq);
						if (dr != dq)
						{
							if (opcode2 & 0x0800)
								strcpy(info->comments, "rems.l for ColdFire");
							else
								strcpy(info->comments, "remu.l for ColdFire");
						}
					}
				} else
				{
					if (opcode2 & 0x0400)
					{
						/* mul 32*32 -> dh-dl */
						info->num_oper = 3;
						ps(info, ".l");
						startoperands(info);
						doea(info, opcode, 2);
						pcomma(info);
						od(info, dr);
						pcomma(info);
						od(info, dq);
					} else
					{
						/* 32*32 -> dl */
						info->num_oper = 2;
						ps(info, ".l");
						startoperands(info);
						doea(info, opcode, 2);
						pcomma(info);
						od(info, dq);
					}
				}
			}
			break;
		case 7:						/* jmp, jsr, link, move usp, nop, reset, rte, rtr, rts, stop, trap, trapv, unlk */
			switch (dstmod(opcode))
			{
			case 1:
				switch (srcmod(opcode))
				{
				case 0:
				case 1:
					/* 0100 111 001 00 xxxx */
					ps(info, "trap");
					startoperands(info);
					sputc('#');
					psdec(info, opcode & 0x000f);
					info->num_oper = 1;
					break;
				case 2:
					/* 0100 111 001 010 xxx */
					ps(info, "link");
					startoperands(info);
					oa(info, srcreg(opcode));
					pcomma(info);
					oi(info, 1, true);
					info->num_oper = 2;
					break;
				case 3:
					/* 0100 111 001 011 xxx */
					ps(info, "unlk");
					startoperands(info);
					oa(info, srcreg(opcode));
					info->num_oper = 1;
					break;
				case 4:
					/* 0100 111 001 100 xxx */
					ps(info, "move.l");
					startoperands(info);
					oa(info, srcreg(opcode));
					pcomma(info);
					ps(info, "usp");
					info->num_oper = 2;
					break;
				case 5:
					/* 0100 111 001 101 xxx */
					ps(info, "move.l");
					startoperands(info);
					ps(info, "usp");
					pcomma(info);
					oa(info, srcreg(opcode));
					info->num_oper = 2;
					break;
				case 6:
					/* 0100 111 001 110 xxx */
					switch (srcreg(opcode))
					{
					case 0:
						ps(info, "reset");
						info->num_oper = 0;
						break;
					case 1:
						ps(info, "nop");
						info->num_oper = 0;
						break;
					case 2:
						ps(info, "stop");
						startoperands(info);
						sputc('#');
						psdec(info, GETW(info->memory_vma));
						info->memory_vma += 2;
						info->num_oper = 1;
						break;
					case 3:
						ps(info, "rte");
						info->num_oper = 0;
						break;
					case 4:
						ps(info, "rtd");
						startoperands(info);
						sputc('#');
						psdec(info, GETW(info->memory_vma));
						info->memory_vma += 2;
						info->num_oper = 1;
						break;
					case 5:
						ps(info, "rts");
						info->num_oper = 0;
						break;
					case 6:
						ps(info, "trapv");
						info->num_oper = 0;
						break;
					case 7:
						ps(info, "rtr");
						info->num_oper = 0;
						break;
					}
					break;
				case 7:
					/* 0100 111 001 111 xxx */
					switch (srcreg(opcode))
					{
					default:
						odcw(info, opcode);
						break;
					case 2:
						if (info->cpu >= CPU_68010)
						{
							ps(info, "movec");
							startoperands(info);
							mask = GETUW(info->memory_vma);
							info->memory_vma += 2;
							pspreg(info, mask);
							pcomma(info);
							oad(info, (mask >> 12) & 0x0f);
							info->num_oper = 2;
							only_68020();
						} else
						{
							odcw(info, opcode);
						}
						break;
					case 3:
						if (info->cpu >= CPU_68010)
						{
							ps(info, "movec");
							startoperands(info);
							mask = GETUW(info->memory_vma);
							info->memory_vma += 2;
							oad(info, (mask >> 12) & 0x0f);
							pcomma(info);
							pspreg(info, mask);
							info->num_oper = 2;
						} else
						{
							odcw(info, opcode);
						}
						break;
					}
					break;
				}
				break;
			case 2:
				ps(info, "jsr");
				startoperands(info);
				doea(info, opcode, 0);
				info->num_oper = 1;
				break;
			case 3:
				ps(info, "jmp");
				startoperands(info);
				doea(info, opcode, 0);
				info->num_oper = 1;
				break;
			default:
				odcw(info, opcode);
				break;
			}
			break;
		}
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void group5(m68k_disasm_info *info, int opcode)
{
	register uae_s32 adr;
	
	if (insize(opcode) == 3)
	{
		if ((opcode & 0x3f) == 0x3a ||
			(opcode & 0x3f) == 0x3b ||
			(opcode & 0x3f) == 0x3c)
		{
			ps(info, "trap");
			pcond(info, (opcode >> 8) & 0x000f);
			if (srcreg(opcode) == 2)
			{
				ps(info, ".w");
				startoperands(info);
				oi(info, 1, true);
				info->num_oper = 1;
			} else if (srcreg(opcode) == 3)
			{
				ps(info, ".l");
				startoperands(info);
				oi(info, 2, true);
				info->num_oper = 2;
			} else
			{
				info->num_oper = 0;
			}
		} else
		{
			ps(info, srcmod(opcode) == 1 ? "db" : "s");
			pcond(info, (opcode >> 8) & 0x000f);
			startoperands(info);
			if (srcmod(opcode) != 1)
			{
				doea(info, opcode, 0);
				info->num_oper = 1;
			} else
			{
				od(info, srcreg(opcode));
				pcomma(info);
				adr = GETW(info->memory_vma) + ((struct priv_data *)(info->disasm_data))->oldaddr + info->reloffset;
				info->memory_vma += 2;
				p8hex(info, adr);
				info->num_oper = 2;
			}
		}
	} else
	{
		ps(info, opcode & 0x0100 ? "subq" : "addq");
		os(info, opcode);
		sputc('#');
		psdec(info, dstreg(opcode) != 0 ? dstreg(opcode) : 8);
		pcomma(info);
		doea(info, opcode, 0);
		info->num_oper = 2;
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void group6(m68k_disasm_info *info, int opcode)
{
	register uae_s32 adr;
	register int cond;
	register signed char dist;

	/* 0110 cccc dddddddd */
	switch (cond = (opcode >> 8) & 0x000f)
	{
	case 1:
		ps(info, "bsr");
		break;
	case 0:
		ps(info, "bra");
		break;
	default:
		sputc('b');
		pcond(info, cond);
		break;
	}
	if ((dist = (signed char) opcode) != 0)
	{
		if (dist == -1)
		{
			ps(info, ".l");
			startoperands(info);
			adr = GETL(info->memory_vma) + ((struct priv_data *)(info->disasm_data))->oldaddr + info->reloffset;
			info->memory_vma += 4;
			p8hex(info, adr);
			only_68020();
		} else
		{
			ps(info, ".s");
			startoperands(info);
			adr = dist + ((struct priv_data *)(info->disasm_data))->oldaddr + info->reloffset;
			p8hex(info, adr);
		}
	} else
	{
		startoperands(info);
		adr = GETW(info->memory_vma) + ((struct priv_data *)(info->disasm_data))->oldaddr + info->reloffset;
		info->memory_vma += 2;
		p8hex(info, adr);
	}
	info->num_oper = 1;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void group7(m68k_disasm_info *info, int opcode)
{
	int size;
	
	if (opcode & 0x0100)
	{
		/* ColdFire only */
		only_if(info, "ColdFire");
		ps(info, opcode & 0x0080 ? "mvz" : "mvs");
		size = opcode & 0x0040 ? 1 : 0;
		sputc('.');
		sputc(size == 0 ? 'b' : 'w');
		startoperands(info);
		doea(info, opcode, size);
		pcomma(info);
		oddstreg(info, opcode);
	} else
	{
		ps(info, "moveq.l");
		startoperands(info);
		sputc('#');
		psdec(info, (int) ((char) (opcode & 0xff)));
		pcomma(info);
		oddstreg(info, opcode);
		info->num_oper = 2;
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void g812(m68k_disasm_info *info, int opcode, const char *andor, const char *muldiv, const char *as)
{
	register int size;

	info->num_oper = 2;
	if (dstmod(opcode) == 3)
	{
		ps(info, muldiv);
		ps(info, "u.w");
		startoperands(info);
		doea(info, opcode, 1);
		pcomma(info);
		oddstreg(info, opcode);
	} else if (dstmod(opcode) == 7)
	{
		ps(info, muldiv);
		ps(info, "s.w");
		startoperands(info);
		doea(info, opcode, 1);
		pcomma(info);
		oddstreg(info, opcode);
	} else if (dstmod(opcode) == 4 && ((opcode >> 4) & 3) == 0)
	{
		ps(info, as);
		ps(info, "bcd.b");
		startoperands(info);
		if (srcmod(opcode) != 0)
		{
			sputc('-');
			oai(info, srcreg(opcode));
			pcomma(info);
			sputc('-');
			oai(info, dstreg(opcode));
		} else
		{
			od(info, srcreg(opcode));
			pcomma(info);
			oddstreg(info, opcode);
		}
	} else if ((opcode & 0x01f0) == 0x140 || (opcode & 0x01f0) == 0x180)
	{
		ps(info, opcode & 0x08 ? "unpk" : "pack");
		startoperands(info);
		if (opcode & 0x08)
		{
			od(info, srcreg(opcode));
			pcomma(info);
			od(info, dstreg(opcode));
		} else
		{
			sputc('-');
			oa(info, srcreg(opcode));
			pcomma(info);
			sputc('-');
			oa(info, dstreg(opcode));
		}
		pcomma(info);
		oi(info, 1, true);
		info->num_oper = 3;
	} else
	{
		ps(info, andor);
		size = os(info, opcode);
		if (opcode & 0x0100)
		{
			oddstreg(info, opcode);
			pcomma(info);
			doea(info, opcode, size);
		} else
		{
			doea(info, opcode, size);
			pcomma(info);
			oddstreg(info, opcode);
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

static void group8(m68k_disasm_info *info, int opcode)
{
	g812(info, opcode, "or", "div", "s");
}

/*** ---------------------------------------------------------------------- ***/

static void group12(m68k_disasm_info *info, int opcode)
{
	register int d5;

	if ((d5 = (opcode >> 3) & 0x003f) == 0x0028)
	{
		ps(info, "exg");
		startoperands(info);
		oddstreg(info, opcode);
		pcomma(info);
		od(info, srcreg(opcode));
		info->num_oper = 2;
	} else if (d5 == 0x0029)
	{
		ps(info, "exg");
		startoperands(info);
		oa(info, dstreg(opcode));
		pcomma(info);
		oa(info, srcreg(opcode));
		info->num_oper = 2;
	} else if (d5 == 0x0031)
	{
		ps(info, "exg");
		startoperands(info);
		oddstreg(info, opcode);
		pcomma(info);
		oa(info, srcreg(opcode));
		info->num_oper = 2;
	} else if (d5 == 0x0030)
	{
		odcw(info, opcode);
	} else
	{
		g812(info, opcode, "and", "mul", "a");
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void g913(m68k_disasm_info *info, int opcode, const char *addsub)
{
	register int size;

	ps(info, addsub);
	if ((opcode & 0x0100) != 0 && insize(opcode) != 3 && ((opcode >> 4) & 3) == 0)
	{
		/*
		 * 1x01rrr1sseeeeee
		 * ss != 3
		 *
		 * addx/subx ea
		 */
		sputc('x');
		os(info, opcode);
		if (opcode & 0x0008)
		{
			sputc('-');
			oai(info, srcreg(opcode));
			pcomma(info);
			sputc('-');
			oai(info, dstreg(opcode));
		} else
		{
			od(info, srcreg(opcode));
			pcomma(info);
			oddstreg(info, opcode);
		}
	} else
	{
		switch (dstmod(opcode))
		{
		case 3:
		case 7:
			sputc('a');
			size = osa(info, opcode, 0x0100);
			doea(info, opcode, size);
			pcomma(info);
			oa(info, dstreg(opcode));
			break;
		default:
			size = os(info, opcode);
			if (opcode & 0x0100)
			{
				oddstreg(info, opcode);
				pcomma(info);
				doea(info, opcode, size);
			} else
			{
				doea(info, opcode, size);
				pcomma(info);
				oddstreg(info, opcode);
			}
			break;
		}
	}
	info->num_oper = 2;
}

/*** ---------------------------------------------------------------------- ***/

static void group9(m68k_disasm_info *info, int opcode)
{
	g913(info, opcode, "sub");
}

/*** ---------------------------------------------------------------------- ***/

static void group13(m68k_disasm_info *info, int opcode)
{
	g913(info, opcode, "add");
}


/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void pacc(m68k_disasm_info *info, int acc)
{
	ps(info, "acc");
	sputc('0' + acc);
}


static void group10(m68k_disasm_info *info, int iw)
{
	int x;
	
	switch ((iw >> 6) & 0x0007)
	{
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
		{
			uae_u16 opcode2 = GETUW(info->memory_vma);
			unsigned int acc = ((iw >> 7) & 1) | ((opcode2 >> 3) & 0x02);
			unsigned int rx, rxa, ry, rya, rw, rwa;
			uae_u16 mode;
			
			info->memory_vma += 2;
			info->num_oper = 4;
			mode = (iw >> 3) & 7;
			rx = ry = rw = -1;
			rxa = rya = rwa = 0;
			switch (mode)
			{
			case 0:
			case 1:
				rx = (iw >> 9) & 7;
				rxa = (iw >> 6) & 1;
				ry = (iw & 7);
				rya = (iw >> 3) & 1;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				rw = (iw >> 9) & 7;
				rwa = (iw >> 6) & 1;
				rx = (opcode2 >> 12) & 7;
				rxa = (opcode2 >> 15) & 1;
				ry = (opcode2 & 7);
				rya = (opcode2 >> 3) & 1;
				acc ^= 1; /* WTF */
				break;
			}
			if ((mode == 0 || mode == 1) && (opcode2 & 0x03) == 0x01)
				ps(info, opcode2 & 0x0100 ? "msaac" : "maaac");
			else if ((mode == 0 || mode == 1) && (opcode2 & 0x03) == 0x03)
				ps(info, opcode2 & 0x0100 ? "mssac" : "masac");
			else
				ps(info, opcode2 & 0x0100 ? "msac" : "mac");
			sputc('.');
			sputc(opcode2 & 0x0800 ? 'l' : 'w');
			startoperands(info);
			if (rya)
				oa(info, ry);
			else
				od(info, ry);
			if ((opcode2 & 0x0800) == 0)
			{
				sputc('.');
				sputc(opcode2 & 0x0040 ? 'u' : 'l');
			}
			pcomma(info);
			if (rxa)
				oa(info, rx);
			else
				od(info, rx);
			if ((opcode2 & 0x0800) == 0)
			{
				sputc('.');
				sputc(opcode2 & 0x0080 ? 'u' : 'l');
			}
			pcomma(info);
			switch ((opcode2 >> 9) & 3)
			{
				case 0x00: ps(info, "0"); break;
				case 0x01: ps(info, "<<1"); break;
				case 0x02: ps(info, "??"); break;
				case 0x03: ps(info, ">>1"); break;
			}
			switch (mode)
			{
			case 2:
			case 3:
			case 4:
			case 5:
				pcomma(info);
				doea(info, iw, 0);
				if (opcode2 & 0x0020)
					sputc('&');
				pcomma(info);
				if (rwa)
					oa(info, rw);
				else
					od(info, rw);
				info->num_oper += 2;
				break;
			}
			pcomma(info);
			pacc(info, acc);
			if ((mode == 0 || mode == 1) && ((opcode2 & 0x03) == 0x01 || (opcode2 & 0x03) == 0x03))
			{
				pcomma(info);
				pacc(info, ((opcode2 >> 2) & 3));
				info->num_oper += 1;
			}
		}
		break;
	case 0x04:
		if ((iw & 0x0800) == 0)
		{
			ps(info, "move.l");
			startoperands(info);
			pacc(info, ((iw >> 9) & 3));
			pcomma(info);
			pacc(info, ((iw) & 3));
			info->num_oper = 2;
		} else
		{
			ps(info, "move.l");
			startoperands(info);
			doea(info, iw, 2);
			pcomma(info);
			info->num_oper = 2;
			switch ((iw >> 9) & 3)
			{
			case 0x00:
				ps(info, "macsr");
				break;
			case 0x01:
				ps(info, "accext01");
				break;
			case 0x02:
				ps(info, "mask");
				break;
			case 0x03:
				ps(info, "accext23");
				break;
			}
		}
		break;
	case 0x05:
		ps(info, "mov3q.l");
		startoperands(info);
		x = dstreg(iw);
		sputc('#');
		psdec(info, x == 0 ? -1 : x);
		pcomma(info);
		doea(info, iw, 0);
		info->num_oper = 2;
		break;
	case 0x06:
		if ((iw & 0x0800) == 0)
		{
			ps(info, "move.l");
			startoperands(info);
			pacc(info, ((iw >> 9) & 3));
			pcomma(info);
			oad(info, iw & 0x0f);
			info->num_oper = 2;
		} else
		{
			ps(info, "move.l");
			startoperands(info);
			switch ((iw >> 9) & 3)
			{
			case 0x00:
				ps(info, "macsr");
				break;
			case 0x01:
				ps(info, "accext01");
				break;
			case 0x02:
				ps(info, "mask");
				break;
			case 0x03:
				ps(info, "accext23");
				break;
			}
			pcomma(info);
			oad(info, iw & 0x0f);
			info->num_oper = 2;
		}
		break;
	case 0x07:
		if ((iw & 0x0800) == 0)
		{
			ps(info, "movclr.l");
			startoperands(info);
			pacc(info, ((iw >> 9) & 3));
			pcomma(info);
			oad(info, iw & 0x0f);
			info->num_oper = 2;
		} else
		{
			switch ((iw >> 9) & 3)
			{
			case 0x00:
				ps(info, "move.l");
				startoperands(info);
				ps(info, "macsr,ccr");
				info->num_oper = 2;
				break;
			default:
				odcw(info, iw);
				break;
			}
		}
		break;
	default:
		odcw(info, iw);
		break;
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

static void (*const grphdlrs[])(m68k_disasm_info *, int) = {
    group0, group1, group1, group1,
    group4, group5, group6, group7,
    group8, group9, group10, group11,
    group12, group13, group14, group15
};

/*** ---------------------------------------------------------------------- ***/

int m68k_disasm_builtin(m68k_disasm_info *info)
{
    register int opcode;
    register void (*hdlr)(m68k_disasm_info *info, int opcode);
    unsigned int insn_size;
	struct priv_data *priv;
	
	if (info->disasm_data == NULL)
	{
		priv = (struct priv_data *)malloc(sizeof(*priv));
		info->disasm_data = priv;
	}
	priv = (struct priv_data *)info->disasm_data;
    priv->oldaddr = info->memory_vma;
    priv->lp = info->opcode;
    opcode = GETUW(info->memory_vma);
	info->memory_vma += 2;
	info->reloffset = 2;
    hdlr = grphdlrs[(opcode >> 12) & 15];
    (*hdlr)(info, opcode);
    sputc('\0');
    insn_size = (unsigned int)(info->memory_vma - priv->oldaddr);
    info->memory_vma = priv->oldaddr;
    return insn_size;
}

#else

extern int i_dont_care_that_ISOC_doesnt_like_empty_sourcefiles;

#endif /* DISASM_USE_BUILTIN */
