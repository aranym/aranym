/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 *
 * The simple FDC emulation was derived from FAST's FDC code. FAST is
 * an Atari ST emulator written by Joachim Hoenig
 * (hoenig@informatik.uni-erlangen.de). Bugs are probably implemented by
 * me (nino@complang.tuwien.ac.at), so don't bother him with questions
 * regarding this code!
 *
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include <stdio.h>
#include <unistd.h>

#define FDC_DEBUG 0

struct disk_geom
{
	int head, sides, tracks, sectors, secsize;
} disk[2];

extern int drive_fd[];

int dma_mode,dma_scr,dma_car,dma_sr;
int fdc_command,fdc_track,fdc_sector,fdc_data,fdc_int,fdc_status;

void init_fdc(void)
{
	int i;
	unsigned char buf[512];
	for (i=0; i<2; i++)
	{
		int fd=drive_fd[i];
		if (fd > 0)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, buf, 512);
			disk[i].head=0;
			disk[i].sides=2;
			disk[i].sectors=18;
			disk[i].secsize=512;
			disk[i].tracks=80;
			// if MS-DOS or Atari TOS boot sector sum valid
			if (true) {
				disk[i].sides=(int)buf[26];
				disk[i].sectors=(int)buf[24];
				disk[i].secsize=(int)(buf[12]<<8)|buf[11];
				int delitel = (disk[i].sides*disk[i].sectors);
				if (delitel != 0)
					disk[i].tracks=((int)(buf[20]<<8)|buf[19])/
				fprintf(stderr,"FDC %c: %d/%d/%d %d bytes/sector\n",
					'A'+i,disk[i].sides,disk[i].tracks,disk[i].sectors,
					disk[i].secsize);
			}
		}
	}
}

void fdc_exec_command (void)
{
	static int dir=1,motor=1;
	int sides,d;
	uaecptr address;
	long offset;
	long count;
	uae_u8 *buffer;
	extern void linea_init(void);
	extern int linea_ok;

	address = (HWget_b(0xff8609)<<16)|(HWget_b(0xff860b)<<8)
			|HWget_b(0xff860d);
	buffer=do_get_real_address(address, true, false);	//?? Je to OK?
	int snd_porta = getFloppyStats();
#if FDC_DEBUG
	fprintf(stderr, "FDC DMA virtual address = %06x, physical = %08x, snd = %d\n", address, buffer, snd_porta);
#endif
	sides=(~snd_porta)&1;
	d=(~snd_porta)&6;
	switch(d)
	{
		case 2:
			d=0;
			break;
		case 4:
			d=1;
			break;
		case 6:
		case 0:
			d=-1;
			break;
	}
#if FDC_DEBUG
	fprintf(stderr,"FDC command 0x%04x drive=%d\n",fdc_command,d);
#endif
	fdc_status=0;
	if (fdc_command < 0x80)
	{
		if (d>=0)
		{
			switch(fdc_command&0xf0)
			{
				case 0x00:
#if FDC_DEBUG
					fprintf(stderr,"\tFDC RESTORE\n");
#endif
					disk[d].head=0;
					fdc_track=0;
					break;
				case 0x10:
#if FDC_DEBUG
					fprintf(stderr,"\tFDC SEEK to %d\n",fdc_data);
#endif
					disk[d].head += fdc_data-fdc_track;
					fdc_track=fdc_data;
					if (disk[d].head<0 || disk[d].head>=disk[d].tracks)
						disk[d].head=0;
					break;
				case 0x30:
					fdc_track+=dir;
				case 0x20:
					disk[d].head+=dir;
					break;
				case 0x50:
					fdc_track++;
				case 0x40:
					if (disk[d].head<disk[d].tracks)
						disk[d].head++;
					dir=1;
					break;
				case 0x70:
					fdc_track--;
				case 0x60:
					if (disk[d].head > 0)
						disk[d].head--;
					dir=-1;
					break;
			}
			if (disk[d].head==0)
				fdc_status |= 4;
			if (disk[d].head != fdc_track && (fdc_command & 4))
				fdc_status |= 0x10;
			if (motor)
				fdc_status |= 0x20;
		}
		else fdc_status |= 0x10;
	}
	else if ((fdc_command & 0xf0) == 0xd0)
	{
		if (fdc_command == 0xd8) fdc_int=1;
		else if (fdc_command == 0xd0) fdc_int=0;
	}
	else
	{
		if (d>=0)
		{
			offset=disk[d].secsize
				* (((disk[d].sectors*disk[d].sides*disk[d].head))
				+ (disk[d].sectors * sides) + (fdc_sector-1));
			switch(fdc_command & 0xf0)
			{
				case 0x80:
#if FDC_DEBUG
					fprintf(stderr,"\tFDC READ SECTOR %d to 0x%06lx\n",
						dma_scr,address);
#endif
					count=dma_scr*disk[d].secsize;
					if (lseek(drive_fd[d], offset, SEEK_SET)>=0)
					{
						if (count==read(drive_fd[d], buffer, count))
						{
							address += count;
							HWput_b(0xff8609, address>>16);
							HWput_b(0xff860b, address>>8);
							HWput_b(0xff860d, address);
							dma_scr=0;
							dma_sr=1;
							break;
						}
					}
					fdc_status |= 0x10;
					dma_sr=1;
					break;
				case 0x90:
#if FDC_DEBUG
					fprintf(stderr,"\tFDC READ SECTOR M. %d to 0x%06lx\n",
						dma_scr,address);
#endif
					count=dma_scr*disk[d].secsize;
					if (lseek(drive_fd[d], offset, SEEK_SET)>=0)
					{
						if (count==read(drive_fd[d], buffer, count))
						{
							address += count;
							HWput_b(0xff8609, address>>16);
							HWput_b(0xff860b, address>>8);
							HWput_b(0xff860d, address);
							dma_scr=0;
							dma_sr=1;
							fdc_sector += dma_scr; /* *(512/disk[d].secsize);*/
							break;
						}
					}
					fdc_status |= 0x10;
					dma_sr=1;
					break;
				case 0xa0:
					count=dma_scr*disk[d].secsize;
					if (lseek(drive_fd[d], offset, SEEK_SET)>=0)
					{
						if (count==write(drive_fd[d], buffer, count))
						{
							address += count;
							HWput_b(0xff8609, address>>16);
							HWput_b(0xff860b, address>>8);
							HWput_b(0xff860d, address);
							dma_scr=0;
							dma_sr=1;
							break;
						}
					}
					fdc_status |= 0x10;
					dma_sr=1;
					break;
				case 0xb0:
					count=dma_scr*disk[d].secsize;
					if (lseek(drive_fd[d], offset, SEEK_SET)>=0)
					{
						if (count==write(drive_fd[d], buffer, count))
						{
							address += count;
							HWput_b(0xff8609, address>>16);
							HWput_b(0xff860b, address>>8);
							HWput_b(0xff860d, address);
							dma_scr=0;
							dma_sr=1;
							fdc_sector += dma_scr; /* *(512/disk[d].secsize);*/
							break;
						}
					}
					fdc_status |= 0x10;
					dma_sr=1;
					break;
				case 0xc0:
					fdc_status |= 0x10;
					break;
				case 0xe0:
					fdc_status |= 0x10;
					break;
				case 0xf0:
					fdc_status |= 0x10;
					break;
			}
			if (disk[d].head != fdc_track) fdc_status |= 0x10;
		}
		else fdc_status |= 0x10;
	}
	if (motor)
		fdc_status |= 0x80;
	if (!(fdc_status & 1))
	{
		HWput_b(0xfffa0d, HWget_b(0xfffa0d) | (0x80 & HWget_b(0xfffa09)));
		HWput_b(0xfffa01, HWget_b(0xfffa01) & ~0x20);
	}
}


