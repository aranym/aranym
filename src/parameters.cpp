/*
 * parameters.cpp - parameter init/load/save code
 *
 * Copyright (c) 2001-2004 ARAnyM developer team (see AUTHORS)
 *
 * Authors:
 *  MJ		Milan Jurik
 *  Joy		Petr Stehlik
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "sysdeps.h"
#include "config.h"
#include "parameters.h"
#include "tools.h"		// for safe_strncpy()

#define DEBUG 0
#include "debug.h"

#ifdef HAVE_NEW_HEADERS
# include <cstdlib>
#else
# include <stdlib.h>
#endif

#ifndef USE_JIT
# define USE_JIT 0
#endif

#ifndef FULLMMU
# define FULLMMU 0
#endif

#ifndef DSP_EMULATION
# define DSP_EMULATION 0
#endif

#ifndef DSP_DISASM
# define DSP_DISASM 0
#endif

#ifndef ENABLE_OPENGL
# define ENABLE_OPENGL 0
#endif

#ifndef HOSTFS_SUPPORT
# define HOSTFS_SUPPORT 0
#endif

static struct option const long_options[] =
{
#ifndef FixedSizeFastRAM
  {"fastram", required_argument, 0, 'F'},
#endif
  {"floppy", required_argument, 0, 'a'},
  {"resolution", required_argument, 0, 'r'},
  {"debug", no_argument, 0, 'D'},
  {"fullscreen", no_argument, 0, 'f'},
  {"nomouse", no_argument, 0, 'N'},
  {"refresh", required_argument, 0, 'v'},
  {"monitor", required_argument, 0, 'm'},
  {"disk", required_argument, 0, 'd'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"config", required_argument, 0, 'c'},
  {"save", no_argument, 0, 's'},
  {"gui", no_argument, 0, 'G'},
  {"swap-ide", no_argument, 0, 'S'},
  {"emutos", no_argument, 0, 'e'},
  {"lilo", no_argument, 0, 'l'},
  {NULL, 0, NULL, 0}
};

#define TOS_FILENAME		"ROM"
#define EMUTOS_FILENAME		"etos512k.img"

bool startupGUI = false;

bool boot_emutos = false;
bool boot_lilo = false;
bool ide_swap = false;
uint32 FastRAMSize;

static char config_file[512];

#if !defined(XIF_HOST_IP) && !defined(XIF_ATARI_IP) && !defined(XIF_NETMASK)
# define XIF_HOST_IP	"192.168.0.1"
# define XIF_ATARI_IP	"192.168.0.2"
# define XIF_NETMASK	"255.255.255.0"
#endif

static bool saveConfigFile = false;

bx_options_t bx_options;

static bx_atadevice_options_t *diskc = &bx_options.atadevice[0][0];
static bx_atadevice_options_t *diskd = &bx_options.atadevice[0][1];


// configuration file 
/*************************************************************************/
struct Config_Tag global_conf[]={
	{ "FastRAM", Int_Tag, &bx_options.fastram, 0, 0},
	{ "Floppy", String_Tag, bx_options.floppy.path, sizeof(bx_options.floppy.path), 0},
	{ "TOS", String_Tag, bx_options.tos_path, sizeof(bx_options.tos_path), 0},
	{ "EmuTOS", String_Tag, bx_options.emutos_path, sizeof(bx_options.emutos_path), 0},
	{ "AutoGrabMouse", Bool_Tag, &bx_options.autoMouseGrab, 0, 0},
#ifdef ENABLE_EPSLIMITER
	{ "EpsEnabled", Bool_Tag, &bx_options.cpu.eps_enabled, 0, 0},
	{ "EpsMax", Int_Tag, &bx_options.cpu.eps_max, 0, 0},
#endif
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_global() {
  bx_options.autoMouseGrab = true;
  strcpy(bx_options.floppy.path, "");
#ifdef FixedSizeFastRAM
  FastRAMSize = FixedSizeFastRAM * 1024 * 1024;
#else
  FastRAMSize = 0;
#endif
#ifdef ENABLE_EPSLIMITER
	bx_options.cpu.eps_enabled = false;
	bx_options.cpu.eps_max = 20;
#endif
}

void postload_global() {
#ifndef FixedSizeFastRAM
	FastRAMSize = bx_options.fastram * 1024 * 1024;
#endif
}

void presave_global() {
	bx_options.fastram = FastRAMSize / 1024 / 1024;
}

/*************************************************************************/
struct Config_Tag startup_conf[]={
	{ "GrabMouse", Bool_Tag, &bx_options.startup.grabMouseAllowed, 0, 0},
#ifdef DEBUGGER
	{ "Debugger", Bool_Tag, &bx_options.startup.debugger, 0, 0},
#endif
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_startup() {
  bx_options.startup.debugger = false;
  bx_options.startup.grabMouseAllowed = true;
}

void postload_startup() {
}

void presave_startup() {
}

/*************************************************************************/
struct Config_Tag jit_conf[]={
	{ "JIT", Bool_Tag, &bx_options.jit.jit, 0, 0},
	{ "JITFPU", Bool_Tag, &bx_options.jit.jitfpu, 0, 0},
	{ "TuneAlignment", Bool_Tag, &bx_options.jit.tunealign, 0, 0},
	{ "TuneNOPfill", Bool_Tag, &bx_options.jit.tunenop, 0, 0},
	{ "JITCacheSize", Int_Tag, &bx_options.jit.jitcachesize, 0, 0},
	{ "JITLazyFlush", Int_Tag, &bx_options.jit.jitlazyflush, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_jit() {
	bx_options.jit.jit = false;
	bx_options.jit.jitfpu = true;
	bx_options.jit.jitcachesize = 8192;
	bx_options.jit.jitlazyflush = 1;
	bx_options.jit.tunealign = true;
	bx_options.jit.tunenop = true;
}

void postload_jit() {
}

void presave_jit() {
}

/*************************************************************************/
struct Config_Tag tos_conf[]={
	{ "Cookie_MCH", HexLong_Tag, &bx_options.tos.cookie_mch, 0, 0},
	{ "RedirConsole", Bool_Tag, &bx_options.tos.redirect_CON, 0, 0},
	{ "RedirPrinter", Bool_Tag, &bx_options.tos.redirect_PRT, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_tos() {
  bx_options.tos.redirect_CON = false;
  bx_options.tos.redirect_PRT = false;
  bx_options.tos.cookie_mch = 0x00050000; // ARAnyM
}

void postload_tos() {
}

void presave_tos() {
}

/*************************************************************************/
struct Config_Tag video_conf[]={
	{ "FullScreen", Bool_Tag, &bx_options.video.fullscreen, 0, 0},
	{ "BootColorDepth", Byte_Tag, &bx_options.video.boot_color_depth, 0, 0},
	{ "VidelRefresh", Byte_Tag, &bx_options.video.refresh, 0, 0},
	{ "VidelMonitor", Byte_Tag, &bx_options.video.monitor, 0, 0},
	{ "AutoZoom", Bool_Tag, &bx_options.video.autozoom, 0, 0},
	{ "AutoZoomInteger", Bool_Tag, &bx_options.video.autozoomint, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_video() {
  bx_options.video.fullscreen = false;		// Boot in Fullscreen
  bx_options.video.boot_color_depth = -1;	// Boot in color depth
  bx_options.video.monitor = -1;			// preserve default NVRAM monitor
  bx_options.video.refresh = 2;			// 25 Hz update
  bx_options.video.autozoom = false;
  bx_options.video.autozoomint = false;
}

void postload_video() {
	if (bx_options.video.refresh < 1 || bx_options.video.refresh > 200)
		bx_options.video.refresh = 2;	// default if input parameter is insane
}

void presave_video() {
}

/*************************************************************************/
struct Config_Tag opengl_conf[]={
	{ "Enabled", Bool_Tag, &bx_options.opengl.enabled, 0, 0},
	{ "Width", Int_Tag, &bx_options.opengl.width, 0, 0},
	{ "Height", Int_Tag, &bx_options.opengl.height, 0, 0},
	{ "Bpp", Int_Tag, &bx_options.opengl.bpp, 0, 0},
	{ "Filtered", Bool_Tag, &bx_options.opengl.filtered, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_opengl() {
  bx_options.opengl.enabled = false;
  bx_options.opengl.width = 640;
  bx_options.opengl.height = 480;
  bx_options.opengl.bpp = 16;
  bx_options.opengl.filtered = false;
}

void postload_opengl() {
}

void presave_opengl() {
}

/*************************************************************************/
#define BX_DISK_CONFIG(Disk)	struct Config_Tag Disk ## _configs[] = {	\
	{ "Present", Bool_Tag, &Disk->present, 0, 0},	\
	{ "IsCDROM", Bool_Tag, &Disk->isCDROM, 0, 0},	\
	{ "ByteSwap", Bool_Tag, &Disk->byteswap, 0, 0},	\
	{ "ReadOnly", Bool_Tag, &Disk->readonly, 0, 0},	\
	{ "Path", String_Tag, Disk->path, sizeof(Disk->path), 0},	\
	{ "Cylinders", Int_Tag, &Disk->cylinders, 0, 0},	\
	{ "Heads", Int_Tag, &Disk->heads, 0, 0},	\
	{ "SectorsPerTrack", Int_Tag, &Disk->spt, 0, 0},	\
	{ "ModelName", String_Tag, Disk->model, sizeof(Disk->model), 0},	\
	{ NULL , Error_Tag, NULL, 0, 0 }	\
}

BX_DISK_CONFIG(diskc);
BX_DISK_CONFIG(diskd);

void set_ide(unsigned int number, char *dev_path, int cylinders, int heads, int spt, int byteswap, bool readonly, const char *model_name) {
  // Autodetect ???
  if (cylinders == -1)
    if ((cylinders = get_geometry(dev_path, geoCylinders)) == -1) {
      fprintf(stderr, "Disk %s has unknown geometry.\n", dev_path);
      exit(-1);
    }

  if (heads == -1)
    if ((heads = get_geometry(dev_path, geoHeads)) == -1) {
      fprintf(stderr, "Disk %s has unknown geometry.\n", dev_path);
      exit(-1);
    }

  if (spt == -1)
    if ((spt = get_geometry(dev_path, geoSpt)) == -1) {
      fprintf(stderr, "Disk %s has unknown geometry.\n", dev_path);
      exit(-1);
    }

  if (byteswap == -1)
    if ((byteswap = get_geometry(dev_path, geoByteswap)) == -1) {
      fprintf(stderr, "Disk %s has unknown geometry.\n", dev_path);
      exit(-1);
    }

	bx_atadevice_options_t *disk;
	switch(number) {
		case 0: disk = diskc; break;
		case 1: disk = diskd; break;
		default: disk = NULL; break;
	}
  if (disk != NULL) {
    disk->present = strlen(dev_path) > 0;
    disk->isCDROM = false;
    disk->byteswap = byteswap;
    disk->readonly = readonly;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->spt = spt;
    strcpy(disk->path, dev_path);
	safe_strncpy(disk->model, model_name, sizeof(disk->model));
  }

  if (cylinders && heads && spt) {
    D(bug("IDE%d CHS geometry: %d/%d/%d %d", number, cylinders, heads, spt, byteswap));
  }

}

void preset_ide() {
  set_ide(0, "", 0, 0, 0, false, false, "Master");
  set_ide(1, "", 0, 0, 0, false, false, "Slave");

  bx_options.newHardDriveSupport = true;
}

void postload_ide() {
}

void presave_ide() {
}

/*************************************************************************/
#define DISK_CONFIG(Disk)	struct Config_Tag Disk ## _configs[] = {	\
	{ "Path", String_Tag, bx_options.Disk.path, sizeof(bx_options.Disk.path), 0},	\
	{ "Present", Bool_Tag, &bx_options.Disk.present, 0, 0},	\
	{ "PartID", String_Tag, bx_options.Disk.partID, sizeof(bx_options.Disk.partID), 0},	\
	{ "ByteSwap", Bool_Tag, &bx_options.Disk.byteswap, 0, 0},	\
	{ "ReadOnly", Bool_Tag, &bx_options.Disk.readonly, 0, 0},	\
	{ NULL , Error_Tag, NULL, 0, 0 }	\
}

DISK_CONFIG(disk0);
DISK_CONFIG(disk1);
DISK_CONFIG(disk2);
DISK_CONFIG(disk3);
DISK_CONFIG(disk4);
DISK_CONFIG(disk5);
DISK_CONFIG(disk6);
DISK_CONFIG(disk7);

void preset_disk() {
}

void postload_disk() {
}

void presave_disk() {
}

/*************************************************************************/
struct Config_Tag arafs_conf[]={
	{ "A", String_Tag, &bx_options.aranymfs[0].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "B", String_Tag, &bx_options.aranymfs[1].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "C", String_Tag, &bx_options.aranymfs[2].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "D", String_Tag, &bx_options.aranymfs[3].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "E", String_Tag, &bx_options.aranymfs[4].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "F", String_Tag, &bx_options.aranymfs[5].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "G", String_Tag, &bx_options.aranymfs[6].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "H", String_Tag, &bx_options.aranymfs[7].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "I", String_Tag, &bx_options.aranymfs[8].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "J", String_Tag, &bx_options.aranymfs[9].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "K", String_Tag, &bx_options.aranymfs[10].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "L", String_Tag, &bx_options.aranymfs[11].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "M", String_Tag, &bx_options.aranymfs[12].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "N", String_Tag, &bx_options.aranymfs[13].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "O", String_Tag, &bx_options.aranymfs[14].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "P", String_Tag, &bx_options.aranymfs[15].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "Q", String_Tag, &bx_options.aranymfs[16].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "R", String_Tag, &bx_options.aranymfs[17].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "S", String_Tag, &bx_options.aranymfs[18].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "T", String_Tag, &bx_options.aranymfs[19].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "U", String_Tag, &bx_options.aranymfs[20].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "V", String_Tag, &bx_options.aranymfs[21].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "W", String_Tag, &bx_options.aranymfs[22].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "X", String_Tag, &bx_options.aranymfs[23].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "Y", String_Tag, &bx_options.aranymfs[24].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ "Z", String_Tag, &bx_options.aranymfs[25].rootPath, sizeof(bx_options.aranymfs[0].rootPath), 0},
	{ NULL , Error_Tag, NULL, 0, 0}
};

void preset_arafs() {
	for(int i=0; i < 'Z'-'A'+1; i++) {
		bx_options.aranymfs[i].rootPath[0] = '\0';
		bx_options.aranymfs[i].halfSensitive = true;
	}
}

void postload_arafs() {
	for(int i=0; i < 'Z'-'A'+1; i++) {
		int len = strlen(bx_options.aranymfs[i].rootPath);
		bx_options.aranymfs[i].halfSensitive = true;
		if (len > 0) {
			char *ptrLast = bx_options.aranymfs[i].rootPath + len-1;
			if (*ptrLast == ':') {
				*ptrLast = '\0';
				bx_options.aranymfs[i].halfSensitive = false;
			}
		}
	}
}

void presave_arafs() {
	for(int i=0; i < 'Z'-'A'+1; i++) {
		if ( strlen(bx_options.aranymfs[i].rootPath) > 0 &&
			 bx_options.aranymfs[i].halfSensitive ) {
			// set the halfSensitive indicator
			// NOTE: I can't add more chars here due to the fixed char[] width
			strcat( bx_options.aranymfs[i].rootPath, ":" );
		}
	}
}

/*************************************************************************/
#define ETH(x) bx_options.ethernet.x
struct Config_Tag ethernet_conf[]={
	{ "HostIP", String_Tag, &ETH(ip_host), sizeof(ETH(ip_host)), 0},
	{ "AtariIP", String_Tag, &ETH(ip_atari), sizeof(ETH(ip_atari)), 0},
	{ "Netmask", String_Tag, &ETH(netmask), sizeof(ETH(netmask)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_ethernet() {
  safe_strncpy(ETH(ip_host), XIF_HOST_IP, sizeof(ETH(ip_host)));
  safe_strncpy(ETH(ip_atari ), XIF_ATARI_IP, sizeof(ETH(ip_atari)));
  safe_strncpy(ETH(netmask), XIF_NETMASK, sizeof(ETH(netmask)));
}

void postload_ethernet() {
}

void presave_ethernet() {
}

/*************************************************************************/
#define LILO(x) bx_options.lilo.x

struct Config_Tag lilo_conf[]={
	{ "Kernel", String_Tag, &LILO(kernel), sizeof(LILO(kernel)), 0},
	{ "Args", String_Tag, &LILO(args), sizeof(LILO(args)), 0},
	{ "Ramdisk", String_Tag, &LILO(ramdisk), sizeof(LILO(ramdisk)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_lilo() {
  safe_strncpy(LILO(kernel), "linux.bin", sizeof(LILO(kernel)));
  safe_strncpy(LILO(args), "root=/dev/ram video=atafb:vga16", sizeof(LILO(args)));
  safe_strncpy(LILO(ramdisk), "root.bin", sizeof(LILO(ramdisk)));
}

void postload_lilo() {
}

void presave_lilo() {
}

/*************************************************************************/
#define MIDI(x) bx_options.midi.x

struct Config_Tag midi_conf[]={
	{ "Enabled", Bool_Tag, &MIDI(enabled), 0, 0},
	{ "Output", String_Tag, &MIDI(output), sizeof(MIDI(output)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_midi() {
  MIDI(enabled)=false;
  safe_strncpy(MIDI(output), "/tmp/aranym-midi.txt", sizeof(MIDI(output)));
}

void postload_midi() {
}

void presave_midi() {
}

/*************************************************************************/
#define NFCDROM_ENTRY(c,n) \
	{ c, Int_Tag, &bx_options.nfcdroms[n].physdevtohostdev, sizeof(bx_options.nfcdroms[n].physdevtohostdev), 0}

struct Config_Tag nfcdroms_conf[]={
	NFCDROM_ENTRY("A", 0),
	NFCDROM_ENTRY("B", 1),
	NFCDROM_ENTRY("C", 2),
	NFCDROM_ENTRY("D", 3),
	NFCDROM_ENTRY("E", 4),
	NFCDROM_ENTRY("F", 5),
	NFCDROM_ENTRY("G", 6),
	NFCDROM_ENTRY("H", 7),
	NFCDROM_ENTRY("I", 8),
	NFCDROM_ENTRY("J", 9),
	NFCDROM_ENTRY("K", 10),
	NFCDROM_ENTRY("L", 11),
	NFCDROM_ENTRY("M", 12),
	NFCDROM_ENTRY("N", 13),
	NFCDROM_ENTRY("O", 14),
	NFCDROM_ENTRY("P", 15),
	NFCDROM_ENTRY("Q", 16),
	NFCDROM_ENTRY("R", 17),
	NFCDROM_ENTRY("S", 18),
	NFCDROM_ENTRY("T", 19),
	NFCDROM_ENTRY("U", 20),
	NFCDROM_ENTRY("V", 21),
	NFCDROM_ENTRY("W", 22),
	NFCDROM_ENTRY("X", 23),
	NFCDROM_ENTRY("Y", 24),
	NFCDROM_ENTRY("Z", 25),
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_nfcdroms() {
	for(int i=0; i < 'Z'-'A'+1; i++) {
		bx_options.nfcdroms[i].physdevtohostdev = -1;
	}
}

void postload_nfcdroms() {
}

void presave_nfcdroms() {
}

/*************************************************************************/
void usage (int status) {
  printf ("Usage: aranym [OPTIONS]\n");
  printf ("\
Options:\n\
  -a, --floppy NAME          floppy image file NAME\n\
  -e, --emutos               boot EmuTOS\n\
  -l, --lilo                 boot a linux kernel\n\
  -N, --nomouse              don't grab mouse at startup\n\
  -f, --fullscreen           start in fullscreen\n\
  -v, --refresh <X>          VIDEL refresh rate in VBL (default 2)\n\
  -r, --resolution <X>       boot in X color depth [1,2,4,8,16]\n\
  -m, --monitor <X>          attached monitor: 0 = VGA, 1 = TV\n\
  -d, --disk CHAR:PATH[:]    HostFS mapping, e.g. d:/atari/d_drive\n\
  -c, --config FILE          read different configuration file\n\
  -s, --save                 save configuration file\n\
  -G, --gui                  open GUI at startup\n\
  -S, --swap-ide             swap IDE drives\n\
  -h, --help                 display this help and exit\n\
  -V, --version              output version information and exit\n\
");
#ifndef FixedSizeFastRAM
  printf("  -F, --fastram SIZE         FastRAM size (in MB)\n");
#endif
#ifdef DEBUGGER
  printf("  -D, --debug                start debugger\n");
#endif
  exit (status);
}

void preset_cfg() {
  preset_global();
  preset_ide();
  preset_disk();
  preset_arafs();
  preset_video();
  preset_tos();
  preset_startup();
  preset_jit();
  preset_opengl();
  preset_ethernet();
  preset_lilo();
  preset_midi();
  preset_nfcdroms();
}

void postload_cfg() {
  postload_global();
  postload_ide();
  postload_disk();
  postload_arafs();
  postload_video();
  postload_tos();
  postload_startup();
  postload_jit();
  postload_opengl();
  postload_ethernet();
  postload_lilo();
  postload_midi();
  postload_nfcdroms();
}

void presave_cfg() {
  presave_global();
  presave_ide();
  presave_disk();
  presave_arafs();
  presave_video();
  presave_tos();
  presave_startup();
  presave_jit();
  presave_opengl();
  presave_ethernet();
  presave_lilo();
  presave_midi();
  presave_nfcdroms();
}

void early_cmdline_check(int argc, char **argv) {
	for (int c = 0; c < argc; c++) {
		char *p = argv[c];
		if (strcmp(p, "-S") == 0  || strcmp(p, "--swap-ide") == 0)
			ide_swap = true;

		else if ((strcmp(p, "-c") == 0) || (strcmp(p, "--config") == 0)) {
			if ((c + 1) < argc) {
				safe_strncpy(config_file, argv[c + 1], sizeof(config_file));
			} else {
				fprintf(stderr, "config switch requires one parameter\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(p, "-h") == 0) || (strcmp(p, "--help") == 0)) {
			usage(0);
			exit(0);
		} else if ((strcmp(p, "-V") == 0) || (strcmp(p, "--version") == 0)) {
			// infoprint("%s\n", VERSION_STRING);
			infoprint("Capabilities:");
			infoprint("JIT compiler     : %s", (USE_JIT == 1) ? "enabled" : "disabled");
			infoprint("Full MMU         : %s", (FULLMMU == 1) ? "enabled" : "disabled");
			infoprint("DSP              : %s", (DSP_EMULATION == 1) ? "enabled" : "disabled");
			infoprint("DSP disassembler : %s", (DSP_DISASM == 1) ? "enabled" : "disabled");
			infoprint("OpenGL support   : %s", (ENABLE_OPENGL == 1) ? "enabled" : "disabled");
			infoprint("HOSTFS support   : %s", (HOSTFS_SUPPORT == 1) ? "enabled" : "disabled");

			exit (0);
		}
	}
}

int process_cmdline(int argc, char **argv)
{
	int c;
	while ((c = getopt_long (argc, argv,
							 "a:" /* floppy image file */
							 "e"  /* boot emutos */
							 "l"  /* boot lilo */
#ifdef DEBUGGER
							 "D"  /* debugger */
#endif
#ifndef FixedSizeFastRAM
							 "F:" /* FastRAM */
#endif
							 "N"  /* no mouse */
							 "f"  /* fullscreen */
							 "v:" /* VIDEL refresh */
							 "r:" /* resolution */
							 "m:" /* attached monitor */
							 "d:" /* filesystem assignment */
							 "s"  /* save config file */

							 "c:" /* path to config file */
							 "G"  /* GUI startup */
							 "S"  /* swap IDE drives */
							 "h"  /* help */
							 "V"  /* version */,
							 long_options, (int *) 0)) != EOF) {
		switch (c) {
			case 'V':
			case 'h':
			case 'c':
			case 'S':
				/* processed in early_cmdline_check already */
				break;

			case 'G':
				startupGUI = true;
				break;

#ifdef DEBUGGER
			case 'D':
				bx_options.startup.debugger = true;
				break;
#endif

			case 'e':
				boot_emutos = true;
				break;
	
			case 'f':
				bx_options.video.fullscreen = true;
				break;

			case 'l':
				boot_lilo = true;
				break;

			case 'v':
				bx_options.video.refresh = atoi(optarg);
				break;
	
			case 'N':
				bx_options.startup.grabMouseAllowed = false;
				break;
	
			case 'm':
				bx_options.video.monitor = atoi(optarg);
				break;
	
			case 'a':
				if ((strlen(optarg)-1) > sizeof(bx_options.floppy.path))
					fprintf(stderr, "Floppy image filename longer than %lu chars.\n", sizeof(bx_options.floppy.path));
				safe_strncpy(bx_options.floppy.path, optarg, sizeof(bx_options.floppy.path));
				break;

			case 'r':
				bx_options.video.boot_color_depth = atoi(optarg);
				break;

			case 'd':
				if ( strlen(optarg) < 4 || optarg[1] != ':') {
					fprintf(stderr, "Not enough parameters for -d\n");
					break;
				}
				// set the drive
				{
					int8 i = toupper(optarg[0]) - 'A';
					if (i <= 0 || i>('Z'-'A')) {
						fprintf(stderr, "Drive out of [A-Z] range for -d\n");
						break;
					}

					safe_strncpy( bx_options.aranymfs[i].rootPath, optarg+2,
								sizeof(bx_options.aranymfs[i].rootPath) );
					// Note: tail colon processing (case sensitivity flag) is 
					// done later by calling postload_cfg.
					// Just make sure postload_cfg is called after this.
				}
				break;

			case 's':
				saveConfigFile = true;
				break;

#ifndef FixedSizeFastRAM
			case 'F':
				bx_options.fastram = atoi(optarg);
				break;
#endif

			default:
				usage (EXIT_FAILURE);
		}
	}
	return optind;
}

// append a filename to a path
char *addFilename(char *buffer, const char *file, unsigned int bufsize)
{
	int dirlen = strlen(buffer);
	if ((dirlen + 1 + strlen(file) + 1) < bufsize) {
		if (dirlen > 0) {
			char *ptrLast = buffer + dirlen - 1;
			if (*ptrLast != '/' && *ptrLast != '\\')
				strcat(buffer, DIRSEPARATOR);
		}
		strcat(buffer, file);
	}
	else {
		panicbug("addFilename(\"%s\") - buffer too small!", file);
		safe_strncpy(buffer, file, bufsize);	// at least the filename
	}

	return buffer;
}

// build a complete path to an user-specific file
char *getConfFilename(const char *file, char *buffer, unsigned int bufsize)
{
	getConfFolder(buffer, bufsize);

	// Does the folder exist?
	struct stat buf;
	if (stat(buffer, &buf) == -1) {
		D(bug("Creating config folder '%s'", buffer));
		mkdir(buffer, 0755);
	}

	return addFilename(buffer, file, bufsize);
}

// build a complete path to system wide data file
char *getDataFilename(const char *file, char *buffer, unsigned int bufsize)
{
	getDataFolder(buffer, bufsize);
	return addFilename(buffer, file, bufsize);
}

static int process_config(FILE *f, const char *filename, struct Config_Tag *conf, char *title, bool verbose)
{
	int status = input_config(filename, conf, title);
	if (verbose) {
		if (status >= 0)
			fprintf(f, "%s configuration: found %d valid directives.\n", title, status);
		else
			fprintf(f, "Error while reading/processing the '%s' config file.\n", filename);
	}
	return status;
}

static bool decode_ini_file(FILE *f, const char *rcfile)
{
	// Does the config exist?
	struct stat buf;
	if (stat(rcfile, &buf) == -1) {
		fprintf(f, "Config file '%s' not found.\nThe config file is created with default values. Edit it to suit your needs.\n", rcfile);
		saveConfigFile = true;
		return false;
	}

	fprintf(f, "Using config file: '%s'\n", rcfile);

	process_config(f, rcfile, global_conf, "[GLOBAL]", true);
	process_config(f, rcfile, startup_conf, "[STARTUP]", true);
#ifdef USE_JIT
	process_config(f, rcfile, jit_conf, "[JIT]", true);
#endif
	process_config(f, rcfile, video_conf, "[VIDEO]", true);
	process_config(f, rcfile, tos_conf, "[TOS]", true);
	process_config(f, rcfile, ide_swap ? diskd_configs : diskc_configs, "[IDE0]", true);
	process_config(f, rcfile, ide_swap ? diskc_configs : diskd_configs, "[IDE1]", true);

/*
	process_config(f, rcfile, disk0_configs, "[DISK0]", true);
	process_config(f, rcfile, disk1_configs, "[DISK1]", true);
	process_config(f, rcfile, disk2_configs, "[DISK2]", true);
	process_config(f, rcfile, disk3_configs, "[DISK3]", true);
	process_config(f, rcfile, disk4_configs, "[DISK4]", true);
	process_config(f, rcfile, disk5_configs, "[DISK5]", true);
	process_config(f, rcfile, disk6_configs, "[DISK6]", true);
	process_config(f, rcfile, disk7_configs, "[DISK7]", true);
*/
	if (process_config(f, rcfile, arafs_conf, "[HOSTFS]", true) == 0) {
		// fallback to obsolete [ARANYMFS]
		fprintf(stderr, "[HOSTFS] section in config file not found.\n"
						"falling back to obsolete [ARANYMFS]\n");
		process_config(f, rcfile, arafs_conf, "[ARANYMFS]", true);
	}
	process_config(f, rcfile, opengl_conf, "[OPENGL]", true);
	process_config(f, rcfile, ethernet_conf, "[ETH0]", true);
	process_config(f, rcfile, lilo_conf, "[LILO]", true);
	process_config(f, rcfile, midi_conf, "[MIDI]", true);
	process_config(f, rcfile, nfcdroms_conf, "[CDROMS]", true);

	return true;
}

bool loadSettings(const char *cfg_file) {
	preset_cfg();
	bool ret = decode_ini_file(stderr, cfg_file);
	postload_cfg();
	return ret;
}

bool saveSettings(const char *fs)
{
	presave_cfg();

	if (update_config(fs, global_conf, "[GLOBAL]") < 0) {
		fprintf(stderr, "Error while writing the '%s' config file.\n", fs);
		return false;
	}
	update_config(fs, startup_conf, "[STARTUP]");
#ifdef USE_JIT
	update_config(fs, jit_conf, "[JIT]");
#endif
	update_config(fs, video_conf, "[VIDEO]");
	update_config(fs, tos_conf, "[TOS]");
	update_config(fs, ide_swap ? diskd_configs : diskc_configs, "[IDE0]");
	update_config(fs, ide_swap ? diskc_configs : diskd_configs, "[IDE1]");
/*
	update_config(fs, disk0_configs, "[DISK0]");
	update_config(fs, disk1_configs, "[DISK1]");
	update_config(fs, disk2_configs, "[DISK2]");
	update_config(fs, disk3_configs, "[DISK3]");
	update_config(fs, disk4_configs, "[DISK4]");
	update_config(fs, disk5_configs, "[DISK5]");
	update_config(fs, disk6_configs, "[DISK6]");
	update_config(fs, disk7_configs, "[DISK7]");
*/
	/// update_config(fs, arafs_conf, "[ARANYMFS]");
	update_config(fs, arafs_conf, "[HOSTFS]");
	update_config(fs, opengl_conf, "[OPENGL]");
	update_config(fs, ethernet_conf, "[ETH0]");
	update_config(fs, lilo_conf, "[LILO]");
	update_config(fs, midi_conf, "[MIDI]");
	update_config(fs, nfcdroms_conf, "[CDROMS]");

	return true;
}

bool check_cfg()
{
#if REAL_ADDRESSING
# if defined(__i386__) && defined(OS_linux)
	if (bx_options.fastram > (128 - 16))
	{
		panicbug("Maximum Fast RAM size for real addressing on x86/Linux is 112 MB");
		panicbug("If you need bigger Fast RAM, you must recompile ARAnyM");
		panicbug("./configure --enable-addressing=direct");
		return false;
	}
# endif
#endif
	return true;
}

bool decode_switches(FILE *f, int argc, char **argv)
{
	getConfFilename(ARANYMCONFIG, config_file, sizeof(config_file));
	getDataFilename(TOS_FILENAME, bx_options.tos_path, sizeof(bx_options.tos_path));
	getDataFilename(EMUTOS_FILENAME, bx_options.emutos_path, sizeof(bx_options.emutos_path));

	early_cmdline_check(argc, argv);
	preset_cfg();
	decode_ini_file(f, config_file);
	process_cmdline(argc, argv);
	postload_cfg();

	if (saveConfigFile) {
		D(bug("Storing configuration to file '%s'", config_file));
		saveSettings(config_file);
	}

	return check_cfg();
}

const char *getConfigFile() {
	return config_file;
}
