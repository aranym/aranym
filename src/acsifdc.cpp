/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "acsifdc.h"

#define FDC_VERBOSE 0

static const int HW = 0xff8600;

extern int dma_mode, dma_scr, dma_car, fdc_command, fdc_track, fdc_sector,
	fdc_data, fdc_int, fdc_status, dma_sr;
extern void fdc_exec_command(void);

ACSIFDC::ACSIFDC() {
	DMAfifo = DMAstatus = DMAxor = 0;
	DMAdiskctl = FDC_T = FDC_S = FDC_D = HDC_T = HDC_S = HDC_D = 0;
}

uae_u8 ACSIFDC::handleRead(uaecptr addr) {
	addr -= HW;
	if (addr < 0 || addr > 0x0d)
		return 0;

	switch(addr) {
		case 4:	return LOAD_B_ff8604();
		case 5: return LOAD_B_ff8605();
		case 6: return LOAD_B_ff8606();
		case 7: return LOAD_B_ff8607();
		case 9: return DMAaddr >> 16;
		case 0x0b: return DMAaddr >> 8;
		case 0x0d: return DMAaddr;
	}

	return 0;
}

void ACSIFDC::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= HW;
	if (addr < 0 || addr > 0x0d)
		return;

	switch(addr) {
		case 4: STORE_B_ff8604(value); break;
		case 5: STORE_B_ff8605(value); break;
		case 6: STORE_B_ff8606(value); break;
		case 7: STORE_B_ff8607(value); break;
		case 9: DMAaddr = (DMAaddr & 0x00ffff) | (value << 16); break;
		case 0x0b: DMAaddr = (DMAaddr & 0xff00ff) | (value << 8); break;
		case 0x0d: DMAaddr = (DMAaddr & 0xffff00) | value; break;
	}
	if (addr >= 9)
		fprintf(stderr, "ACSIFDC: DMA = %06x\n", DMAaddr);
}

uae_u8 ACSIFDC::LOAD_B_ff8604(void)
{
	if (dma_mode & 0x10)
	{
		return dma_scr>>8;
	}
	else
	{
		if (dma_mode & 8)
		{
			return dma_car>>8;
		}
		else
		{
			switch(dma_mode & 6)
			{
				case 0:
					if (!fdc_int)
					{
						uae_u8 GPIPdata = HWget_b(0xfffa01);
						GPIPdata |= 0x20;
						HWput_b(0xfffa01, GPIPdata);
					}
					return fdc_status>>8;
				case 2:
					return fdc_track>>8;
				case 4:
					return fdc_sector>>8;
				case 6:
					return fdc_data>>8;
				default:
					return 0;
			}
		}
	}
}

uae_u8 ACSIFDC::LOAD_B_ff8605(void)
{
	if (dma_mode & 0x10)
	{
		return dma_scr&0xff;
	}
	else
	{
		if (dma_mode & 8)
		{
			return dma_car&0xff;
		}
		else
		{
			switch(dma_mode & 6)
			{
				case 0:
					if (!fdc_int)
					{
						uae_u8 GPIPdata = HWget_b(0xfffa01);
						GPIPdata |= 0x20;
						HWput_b(0xfffa01, GPIPdata);
					}
					return fdc_status&0xff;
				case 2:
					return fdc_track&0xff;
				case 4:
					return fdc_sector&0xff;
				case 6:
					return fdc_data&0xff;
				default:
					return 0;
			}
		}
	}
}

uae_u8 ACSIFDC::LOAD_B_ff8606(void)
{
	return dma_sr>>8;
}

uae_u8 ACSIFDC::LOAD_B_ff8607(void)
{
	return dma_sr&0xff;
}

void ACSIFDC::STORE_B_ff8604(uae_u8 vv)
{
#if FDC_VERBOSE
	fprintf(stderr,"DMA car/scr hi <- %x (mode=%x)\n",vv,dma_mode);
#endif
	if (dma_mode&0x10)
	{
		dma_scr &= 0xff;
		dma_scr |= vv<<8;
	}
	else
	{
		if (dma_mode&8)
		{
			dma_car &= 0xff;
			dma_car |= vv<<8;
		}
		else
		{
			switch (dma_mode&6)
			{
				case 0:
					fdc_command &= 0xff;
					fdc_command |= vv<<8;
					break;
				case 2:
					fdc_track &= 0xff;
					fdc_track |= vv<<8;
					break;
				case 4:
					fdc_sector &= 0xff;
					fdc_sector |= vv<<8;
					break;
				case 6:
					fdc_data &= 0xff;
					fdc_data |= vv<<8;
					break;
			}
		}
	}
}

void ACSIFDC::STORE_B_ff8605(uae_u8 vv)
{
#if FDC_VERBOSE
	fprintf(stderr,"DMA car/scr lo <- %x (mode=%x)\n",vv,dma_mode);
#endif
	if (dma_mode&0x10)
	{
		dma_scr &= 0xff00;
		dma_scr |= vv;
	}
	else
	{
		if (dma_mode&8)
		{
			dma_car &= 0xff00;
			dma_car |= vv;
		}
		else
		{
			switch (dma_mode&6)
			{
				case 0:
					fdc_command &= 0xff00;
					fdc_command |= vv;
					fdc_exec_command();
					break;
				case 2:
					fdc_track &= 0xff00;
					fdc_track |= vv;
					break;
				case 4:
					fdc_sector &= 0xff00;
					fdc_sector |= vv;
					break;
				case 6:
					fdc_data &= 0xff00;
					fdc_data |= vv;
					break;
			}
		}
	}
}

void ACSIFDC::STORE_B_ff8606(uae_u8 vv)
{
	dma_mode &= 0xff;
	dma_mode |= vv<<8;
#if FDC_VERBOSE
	fprintf(stderr,"DMA mode <- %04x\n",dma_mode);
#endif
}

void ACSIFDC::STORE_B_ff8607(uae_u8 vv)
{
	dma_mode &= 0xff00;
	dma_mode |= vv;
#if FDC_VERBOSE
	fprintf(stderr,"DMA mode <- %04x\n",dma_mode);
#endif
	return;
#if BOOTSECTOR
	if ((vv&0xff) == 0x88 && DMAdiskctl == 1)
	{
		UL x = (LM_UB(MEM(0xff8609))<<16)|(LM_UB(MEM(0xff860b))<<8)
				|LM_UB(MEM(0xff860d));
		bcopy(bootsec, MEM(x), 512);
	}
#endif
}
