#include "sysdeps.h"

#ifdef NFCONFIG_SUPPORT
#include "nf_config.h"
#include "toserror.h"

#include <algorithm>
#include <cstdlib>
#include <vector>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include "maptab.h"
#include "nf_objs.h"
#include "main.h"

#define DEBUG 0
#include "debug.h"

// The driver interface version
#define INTERFACE_VERSION 0

enum NFCONFIG_OPERATIONS {
	NFCONFIG_INTERFACE_VERSION = 0,
	NFCONFIG_GETVALUE,
	NFCONFIG_SETVALUE,
	NFCONFIG_GETNAMES,
	NFCONFIG_LINEA,
};

NF_Config *NF_Config::nf_config;


static long gmtoff(void)
{
	struct tm *tp;
	time_t t;

	t = time(NULL);
	if ((tp = localtime(&t)) == NULL)
		return 0;
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
	return tp->tm_gmtoff;
#else

#define SHR(a, b)       \
  (-1 >> 1 == -1        \
   ? (a) >> (b)         \
   : (a) / (1 << (b)) - ((a) % (1 << (b)) < 0))

	{
		struct tm gtm;
		struct tm ltm;
		time_t lt;
		long a4, b4, a100, b100, a400, b400, intervening_leap_days, years, days;
		ltm = *tp;
		lt = mktime(&ltm);
	
		if (lt == (time_t) -1)
		{
			/* mktime returns -1 for errors, but -1 is also a
			   valid time_t value.  Check whether an error really
			   occurred.  */
			struct tm tm;
	
			if ((tp = localtime(&lt)) == NULL)
				return 0;
			tm = *tp;
			if ((ltm.tm_sec ^ tm.tm_sec) ||
				(ltm.tm_min ^ tm.tm_min) ||
				(ltm.tm_hour ^ tm.tm_hour) ||
				(ltm.tm_mday ^ tm.tm_mday) ||
				(ltm.tm_mon ^ tm.tm_mon) ||
				(ltm.tm_year ^ tm.tm_year))
				return 0;
		}
	
		if ((tp = gmtime(&lt)) == NULL)
			return 0;
		gtm = *tp;
		
		a4 = SHR(ltm.tm_year, 2) + SHR(1900, 2) - !(ltm.tm_year & 3);
		b4 = SHR(gtm.tm_year, 2) + SHR(1900, 2) - !(gtm.tm_year & 3);
		a100 = a4 / 25 - (a4 % 25 < 0);
		b100 = b4 / 25 - (b4 % 25 < 0);
		a400 = SHR(a100, 2);
		b400 = SHR(b100, 2);
		intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);
		years = ltm.tm_year - gtm.tm_year;
		days = (365 * years + intervening_leap_days + (ltm.tm_yday - gtm.tm_yday));
	
		return (60 * (60 * (24 * days + (ltm.tm_hour - gtm.tm_hour)) + (ltm.tm_min - gtm.tm_min)) + (ltm.tm_sec - gtm.tm_sec));
	}
#undef SHR
#endif
}


int32 NF_Config::dispatch(uint32 fncode)
{
	int32 ret = TOS_EINVFN;
	uint32_t name;
	uint32_t value;
	memptr nameptr;

	switch (fncode)
	{
	case NFCONFIG_INTERFACE_VERSION:
		ret = INTERFACE_VERSION;
		break;

	case NFCONFIG_GETVALUE:
		name = getParameter(0);
		if (((name >> 24) & 0xff) == '_')
		{
			switch (name)
			{
			case 0x5f435055: /* '_CPU' */
				ret = CPUType;
				break;
			case 0x5f465055: /* '_FPU' */
				ret = FPUType;
				break;
			case 0x5f4d4d55: /* '_MMU' */
#ifdef FULLMMU
				ret = 1;
#else
				ret = 0;
#endif
				break;
			case 0x5f4a4954: /* '_JIT' */
#ifdef USE_JIT
				ret = bx_options.jit.jit;
#else
				ret = 0;
#endif
				break;
			case 0x5f4a4946: /* '_JIF' */
#ifdef USE_JIT_FPU
				ret = bx_options.jit.jitfpu;
#else
				ret = 0;
#endif
				break;
			case 0x5f445350: /* '_DSP' */
#ifdef DSP_EMULATION
				ret = 1;
#else
				ret = 0;
#endif
				break;
			case 0x5f4d454d: /* '_MEM' */
				ret = RAMSize >> 20;
				break;
			case 0x5f52414d: /* '_RAM' */
				ret = FastRAMSize >> 20;
				break;
			case 0x5f4d4348: /* '_MCH' */
				ret = bx_options.tos.cookie_mch;
				break;
			case 0x5f414b50: /* '_AKP' */
				ret = bx_options.tos.cookie_akp;
				break;
			case 0x5f474d54: /* '_GMT' */
				ret = bx_options.gmtime;
				break;
			case 0x5f474d4f: /* '_GMO' */
				ret = gmtoff();
				break;
			default:
				ret = TOS_EFILNF;
				break;
			}
		} else
		{
			if (!ValidName(name))
			{
				ret = TOS_EIHNDL;
			} else
			{
				UserValues::iterator it = values.find(name);
				ret = it != values.end() ? it->second : TOS_EFILNF;
			}
		}
		break;

	case NFCONFIG_SETVALUE:
		name = getParameter(0);
		value = getParameter(1);
		if (((name >> 24) & 0xff) == '_')
		{
			switch (name)
			{
			case 0x5f435055: /* '_CPU' */
				ret = TOS_EACCES;
				break;
			case 0x5f465055: /* '_FPU' */
				ret = TOS_EACCES;
				break;
			case 0x5f4d4d55: /* '_MMU' */
				ret = TOS_EACCES;
				break;
			case 0x5f4a4954: /* '_JIT' */
#ifdef USE_JIT
				changed |= bx_options.jit.jit != (value != 0);
				bx_options.jit.jit = value != 0;
				ret = 0;
#else
				ret = TOS_EFILNF;
#endif
				break;
			case 0x5f4a4946: /* '_JIF' */
#ifdef USE_JIT_FPU
				changed |= bx_options.jit.jitfpu != (value != 0);
				bx_options.jit.jitfpu = value != 0;
				ret = 0;
#else
				ret = TOS_EFILNF;
#endif
				break;
			case 0x5f445350: /* '_DSP' */
#ifdef DSP_EMULATION
				/* no configuration option to enable at runtime yet */
				ret = 0;
#else
				ret = TOS_EFILNF;
#endif
				break;
			case 0x5f4d454d: /* '_MEM' */
				ret = TOS_EACCES;
				break;
			case 0x5f52414d: /* '_RAM' */
				if (value > MAX_FASTRAM)
				{
					ret = TOS_EINVAL;
				} else
				{
					changed |= bx_options.fastram != value;
					bx_options.fastram = value;
					ret = 0;
				}
				break;
			case 0x5f4d4348: /* '_MCH' */
				changed |= bx_options.tos.cookie_mch != value;
				bx_options.tos.cookie_mch = value;
				ret = 0;
				break;
			case 0x5f414b50: /* '_AKP' */
				changed |= bx_options.tos.cookie_akp;
				bx_options.tos.cookie_akp = value;
				ret = 0;
				break;
			case 0x5f474d54: /* '_GMT' */
				changed |= bx_options.gmtime != (value != 0);
				bx_options.gmtime = value != 0;
				ret = 0;
				break;
			case 0x5f474d4f: /* '_GMO' */
				ret = TOS_EACCES;
				break;
			default:
				ret = TOS_EFILNF;
				break;
			}
		} else
		{
			ret = 0;
			if (!SetValue(name, value))
				ret = TOS_EIHNDL;
		}
		break;

	case NFCONFIG_GETNAMES:
		nameptr = getParameter(0);
		if (nameptr == 0)
		{
			ret = TOS_EINVAL;
		} else
		{
			name = ReadInt32(nameptr);
			if (ListValue(name, value))
			{
				ret = 0;
				WriteInt32(nameptr + 0, name);
				WriteInt32(nameptr + 4, value);
			} else
			{
				ret = TOS_ENMFIL;
			}
		}
		break;

	case NFCONFIG_LINEA:
		{
			ARADATA *ara = getARADATA();
			if (ara)
				ara->setAbase(getParameter(0));
		}
		ret = 0;
		break;
	}

	return ret;
}


bool NF_Config::SetValue(uint32_t name, uint32_t value)
{
	if (!ValidName(name))
		return false;
	
	UserValues::iterator it = values.find(name);
	if (it != values.end())
	{
		changed |= it->second != value;
		it->second = value;
	} else
	{
		changed = true;
		values.insert(std::make_pair(name, value));
	}
	return true;
}


bool NF_Config::ValidName(uint32_t name)
{
	unsigned char c;
	
	c = (name >> 24) & 0xff;
	if (c <= 0x20 || c >= 0x7f)
		return false;
	c = (name >> 16) & 0xff;
	if (c <= 0x20 || c >= 0x7f)
		return false;
	c = (name >> 8) & 0xff;
	if (c <= 0x20 || c >= 0x7f)
		return false;
	c = (name) & 0xff;
	if (c <= 0x20 || c >= 0x7f)
		return false;
	return true;
}


bool NF_Config::ListValue(uint32_t &name, uint32_t &value)
{
	UserValues::iterator it;
	
	if (name == 0)
	{
		it = values.begin();
	} else
	{
		it = values.find(name);
		it++;
	}
	if (it == values.end())
		return false;
	name = it->first;
	value = it->second;
	return true;
}


void NF_Config::reset()
{
	changed = false;
}


void NF_Config::clear()
{
	values.clear();
	changed = false;
}


NF_Config::NF_Config()
	: changed(false)
{
}


NF_Config::~NF_Config()
{
}


NF_Config *NF_Config::GetNFConfig()
{
	if (nf_config == NULL)
	{
		nf_config = new NF_Config;
	}
	return nf_config;
}

#endif /* NFCONFIG_SUPPORT */
