/*
 * parameters.cpp - parameter init/load/save code
 *
 * Copyright (c) 2001-2010 ARAnyM developer team (see AUTHORS)
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
#include "parameters.h"
#include "tools.h"		// for safe_strncpy()
#include "host.h"
#include "host_filesys.h"
#include "cfgopts.h"

#define DEBUG 0
#include "debug.h"

#include <cstdlib>

#ifdef OS_darwin
#  include <CoreFoundation/CoreFoundation.h>
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

#ifndef USES_FPU_CORE
# define USES_FPU_CORE "<undefined>"
#endif

#ifndef PROVIDES_NATFEATS
# define PROVIDES_NATFEATS "<undefined>"
#endif

static struct option const long_options[] =
{
#ifndef FixedSizeFastRAM
  {"fastram", required_argument, 0, 'F'},
#endif
  {"floppy", required_argument, 0, 'a'},
  {"resolution", required_argument, 0, 'r'},
#ifdef DEBUGGER
  {"debug", no_argument, 0, 'D'},
#endif
  {"fullscreen", no_argument, 0, 'f'},
  {"nomouse", no_argument, 0, 'N'},
  {"refresh", required_argument, 0, 'v'},
  {"monitor", required_argument, 0, 'm'},
#if HOSTFS_SUPPORT
  {"disk", required_argument, 0, 'd'},
#endif
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"config", required_argument, 0, 'c'},
  {"save", no_argument, 0, 's'},
#ifdef SDL_GUI
  {"gui", no_argument, 0, 'G'},
#endif
  {"swap-ide", no_argument, 0, 'S'},
  {"emutos", no_argument, 0, 'e'},
  {"locale", required_argument, 0, 'k'},
#ifdef ENABLE_LILO
  {"lilo", no_argument, 0, 'l'},
#endif
  {"display", required_argument, 0, 'P'},
  {NULL, 0, NULL, 0}
};

#define TOS_FILENAME		"ROM"
#define EMUTOS_FILENAME		"etos512k.img"
#define FREEMINT_FILENAME	"mintara.prg"

#ifndef DEFAULT_SERIAL
#define DEFAULT_SERIAL "/dev/ttyS0"
#endif

char *program_name;		// set by main()

#ifdef SDL_GUI
bool startupGUI = false;
#endif

bool boot_emutos = false;
bool boot_lilo = false;
bool halt_on_reboot = false;
bool ide_swap = false;
uint32 FastRAMSize;

static char config_file[512];

#if !defined(XIF_HOST_IP) && !defined(XIF_ATARI_IP) && !defined(XIF_NETMASK)
# define XIF_TYPE	"ptp"
# define XIF_TUNNEL	"tap0"
# define XIF_HOST_IP	"192.168.0.1"
# define XIF_ATARI_IP	"192.168.0.2"
# define XIF_NETMASK	"255.255.255.0"
# define XIF_MAC_ADDR	"00:41:45:54:48:30"		// just made up from \0AETH0
#endif

static bool saveConfigFile = false;

bx_options_t bx_options;

static bx_atadevice_options_t *diskc = &bx_options.atadevice[0][0];
static bx_atadevice_options_t *diskd = &bx_options.atadevice[0][1];


// configuration file 
/*************************************************************************/
struct Config_Tag global_conf[]={
	{ "FastRAM", Int_Tag, &bx_options.fastram, 0, 0},
	{ "Floppy", Path_Tag, bx_options.floppy.path, sizeof(bx_options.floppy.path), 0},
	{ "TOS", Path_Tag, bx_options.tos_path, sizeof(bx_options.tos_path), 0},
	{ "EmuTOS", Path_Tag, bx_options.emutos_path, sizeof(bx_options.emutos_path), 0},
	{ "Bootstrap", Path_Tag, bx_options.bootstrap_path, sizeof(bx_options.bootstrap_path), 0},
	{ "BootstrapArgs", String_Tag, bx_options.bootstrap_args, sizeof(bx_options.bootstrap_args), 0},
	{ "BootDrive", Char_Tag, &bx_options.bootdrive, 0, 0},
	{ "GMTime", Bool_Tag, &bx_options.gmtime, 0, 0},
#ifdef ENABLE_EPSLIMITER
	{ "EpsEnabled", Bool_Tag, &bx_options.cpu.eps_enabled, 0, 0},
	{ "EpsMax", Int_Tag, &bx_options.cpu.eps_max, 0, 0},
#endif
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_global()
{
  strcpy(bx_options.tos_path, TOS_FILENAME);
  strcpy(bx_options.emutos_path, EMUTOS_FILENAME);
  strcpy(bx_options.bootstrap_path, FREEMINT_FILENAME);
  bx_options.gmtime = false;	// use localtime by default
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

void postload_global()
{
#ifndef FixedSizeFastRAM
	FastRAMSize = bx_options.fastram * 1024 * 1024;
#endif
	if (!isalpha(bx_options.bootdrive))
		bx_options.bootdrive = 0;
}

void presave_global()
{
	bx_options.fastram = FastRAMSize / 1024 / 1024;
	if (bx_options.bootdrive == 0)
		bx_options.bootdrive = ' ';
}

/*************************************************************************/
struct Config_Tag startup_conf[]={
	{ "GrabMouse", Bool_Tag, &bx_options.startup.grabMouseAllowed, 0, 0},
#ifdef DEBUGGER
	{ "Debugger", Bool_Tag, &bx_options.startup.debugger, 0, 0},
#endif
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_startup()
{
  bx_options.startup.debugger = false;
  bx_options.startup.grabMouseAllowed = true;
}

void postload_startup()
{
}

void presave_startup()
{
}

/*************************************************************************/
struct Config_Tag jit_conf[]={
	{ "JIT", Bool_Tag, &bx_options.jit.jit, 0, 0},
	{ "JITFPU", Bool_Tag, &bx_options.jit.jitfpu, 0, 0},
	{ "JITCacheSize", Int_Tag, &bx_options.jit.jitcachesize, 0, 0},
	{ "JITLazyFlush", Int_Tag, &bx_options.jit.jitlazyflush, 0, 0},
	{ "JITBlackList", String_Tag, &bx_options.jit.jitblacklist, sizeof(bx_options.jit.jitblacklist), 0},
	{ "JITInline", Bool_Tag, &bx_options.jit.jitinline, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_jit()
{
	bx_options.jit.jit = true;
	bx_options.jit.jitfpu = true;
	bx_options.jit.jitcachesize = 8192;
	bx_options.jit.jitlazyflush = 1;
	strcpy(bx_options.jit.jitblacklist, "");
}

void postload_jit()
{
}

void presave_jit()
{
}

/*************************************************************************/
struct Config_Tag tos_conf[]={
	{ "Cookie_MCH", HexLong_Tag, &bx_options.tos.cookie_mch, 0, 0},
	{ "RedirConsole", Bool_Tag, &bx_options.tos.redirect_CON, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_tos()
{
  bx_options.tos.redirect_CON = false;
  bx_options.tos.cookie_mch = 0x00050000; // ARAnyM
  bx_options.tos.cookie_akp = -1;
}

void postload_tos()
{
}

void presave_tos()
{
}

/*************************************************************************/
struct Config_Tag video_conf[]={
	{ "FullScreen", Bool_Tag, &bx_options.video.fullscreen, 0, 0},
	{ "BootColorDepth", Byte_Tag, &bx_options.video.boot_color_depth, 0, 0},
	{ "VidelRefresh", Byte_Tag, &bx_options.video.refresh, 0, 0},
	{ "VidelMonitor", Byte_Tag, &bx_options.video.monitor, 0, 0},
	{ "SingleBlitComposing", Bool_Tag, &bx_options.video.single_blit_composing, 0, 0},
	{ "SingleBlitRefresh", Bool_Tag, &bx_options.video.single_blit_refresh, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_video()
{
  bx_options.video.fullscreen = false;		// Boot in Fullscreen
  bx_options.video.boot_color_depth = -1;	// Boot in color depth
  bx_options.video.monitor = -1;			// preserve default NVRAM monitor
  bx_options.video.refresh = 2;				// 25 Hz update
  strcpy(bx_options.video.window_pos, "");	// ARAnyM window position on screen
  bx_options.video.single_blit_composing = false;	// Use chunky screen composing
  bx_options.video.single_blit_refresh = false;	// Use chunky screen refreshing
}

void postload_video()
{
	if (bx_options.video.refresh < 1 || bx_options.video.refresh > 200)
		bx_options.video.refresh = 2;	// default if input parameter is insane
}

void presave_video()
{
}

/*************************************************************************/
struct Config_Tag opengl_conf[]={
	{ "Enabled", Bool_Tag, &bx_options.opengl.enabled, 0, 0},
	{ "Filtered", Bool_Tag, &bx_options.opengl.filtered, 0, 0},
	{ "Library", Path_Tag, bx_options.opengl.library, sizeof(bx_options.opengl.library), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_opengl()
{
  bx_options.opengl.enabled = false;
  bx_options.opengl.filtered = false;
  strcpy(bx_options.opengl.library, "");
}

void postload_opengl()
{
#ifndef ENABLE_OPENGL
  bx_options.opengl.enabled = false;
#endif
}

void presave_opengl()
{
}

/*************************************************************************/
#define BX_DISK_CONFIG(Disk)	struct Config_Tag Disk ## _configs[] = {	\
	{ "Present", Bool_Tag, &Disk->present, 0, 0},	\
	{ "IsCDROM", Bool_Tag, &Disk->isCDROM, 0, 0},	\
	{ "ByteSwap", Bool_Tag, &Disk->byteswap, 0, 0},	\
	{ "ReadOnly", Bool_Tag, &Disk->readonly, 0, 0},	\
	{ "Path", Path_Tag, Disk->path, sizeof(Disk->path), 0},	\
	{ "Cylinders", Int_Tag, &Disk->cylinders, 0, 0},	\
	{ "Heads", Int_Tag, &Disk->heads, 0, 0},	\
	{ "SectorsPerTrack", Int_Tag, &Disk->spt, 0, 0},	\
	{ "ModelName", String_Tag, Disk->model, sizeof(Disk->model), 0},	\
	{ NULL , Error_Tag, NULL, 0, 0 }	\
}

BX_DISK_CONFIG(diskc);
BX_DISK_CONFIG(diskd);

static void set_ide(unsigned int number, const char *dev_path, int cylinders, int heads, int spt, int byteswap, bool readonly, const char *model_name)
{
  // Autodetect ???
  if (cylinders == -1) {
    if ((cylinders = get_geometry(dev_path, geoCylinders)) == -1) {
      fprintf(stderr, "Disk %s has unknown geometry.\n", dev_path);
      exit(-1);
    }
  }

  if (heads == -1) {
    if ((heads = get_geometry(dev_path, geoHeads)) == -1) {
      fprintf(stderr, "Disk %s has unknown geometry.\n", dev_path);
      exit(-1);
    }
  }

  if (spt == -1) {
    if ((spt = get_geometry(dev_path, geoSpt)) == -1) {
      fprintf(stderr, "Disk %s has unknown geometry.\n", dev_path);
      exit(-1);
    }
  }

  if (byteswap == -1) {
    if ((byteswap = get_geometry(dev_path, geoByteswap)) == -1) {
      fprintf(stderr, "Disk %s has unknown geometry.\n", dev_path);
      exit(-1);
    }
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

void preset_ide()
{
  set_ide(0, "", 0, 0, 0, false, false, "Master");
  set_ide(1, "", 0, 0, 0, false, false, "Slave");

  bx_options.newHardDriveSupport = true;
}

void postload_ide()
{
}

void presave_ide()
{
}

/*************************************************************************/
#define DISK_CONFIG(Disk)	struct Config_Tag Disk ## _configs[] = {	\
	{ "Path", Path_Tag, Disk->path, sizeof(Disk->path), 0},	\
	{ "Present", Bool_Tag, &Disk->present, 0, 0},	\
	{ "PartID", String_Tag, Disk->partID, sizeof(Disk->partID), 0},	\
	{ "ByteSwap", Bool_Tag, &Disk->byteswap, 0, 0},	\
	{ "ReadOnly", Bool_Tag, &Disk->readonly, 0, 0},	\
	{ NULL , Error_Tag, NULL, 0, 0 }	\
}

static bx_scsidevice_options_t *disk0 = &bx_options.disks[0];
static bx_scsidevice_options_t *disk1 = &bx_options.disks[1];
static bx_scsidevice_options_t *disk2 = &bx_options.disks[2];
static bx_scsidevice_options_t *disk3 = &bx_options.disks[3];
static bx_scsidevice_options_t *disk4 = &bx_options.disks[4];
static bx_scsidevice_options_t *disk5 = &bx_options.disks[5];
static bx_scsidevice_options_t *disk6 = &bx_options.disks[6];
static bx_scsidevice_options_t *disk7 = &bx_options.disks[7];

DISK_CONFIG(disk0);
DISK_CONFIG(disk1);
DISK_CONFIG(disk2);
DISK_CONFIG(disk3);
DISK_CONFIG(disk4);
DISK_CONFIG(disk5);
DISK_CONFIG(disk6);
DISK_CONFIG(disk7);

void preset_disk()
{
	for(int i=0; i<DISKS; i++) {
		*bx_options.disks[i].path = '\0';
		bx_options.disks[i].present = false;
		strcpy(bx_options.disks[i].partID, "BGM");
		bx_options.disks[i].byteswap = false;
		bx_options.disks[i].readonly = false;
	}
}

void postload_disk()
{
}

void presave_disk()
{
}

/*************************************************************************/
#define HOSTFS_ENTRY(c,n) \
	{ c, Path_Tag, &bx_options.aranymfs[n].configPath, sizeof(bx_options.aranymfs[n].configPath), 0}

struct Config_Tag arafs_conf[]={
	HOSTFS_ENTRY("A", 0),
	HOSTFS_ENTRY("B", 1),
	HOSTFS_ENTRY("C", 2),
	HOSTFS_ENTRY("D", 3),
	HOSTFS_ENTRY("E", 4),
	HOSTFS_ENTRY("F", 5),
	HOSTFS_ENTRY("G", 6),
	HOSTFS_ENTRY("H", 7),
	HOSTFS_ENTRY("I", 8),
	HOSTFS_ENTRY("J", 9),
	HOSTFS_ENTRY("K", 10),
	HOSTFS_ENTRY("L", 11),
	HOSTFS_ENTRY("M", 12),
	HOSTFS_ENTRY("N", 13),
	HOSTFS_ENTRY("O", 14),
	HOSTFS_ENTRY("P", 15),
	HOSTFS_ENTRY("Q", 16),
	HOSTFS_ENTRY("R", 17),
	HOSTFS_ENTRY("S", 18),
	HOSTFS_ENTRY("T", 19),
	HOSTFS_ENTRY("U", 20),
	HOSTFS_ENTRY("V", 21),
	HOSTFS_ENTRY("W", 22),
	HOSTFS_ENTRY("X", 23),
	HOSTFS_ENTRY("Y", 24),
	HOSTFS_ENTRY("Z", 25),
	{ NULL , Error_Tag, NULL, 0, 0}
};

void preset_arafs()
{
	for(int i=0; i < 'Z'-'A'+1; i++) {
		bx_options.aranymfs[i].rootPath[0] = '\0';
		bx_options.aranymfs[i].configPath[0] = '\0';
		bx_options.aranymfs[i].halfSensitive = true;
	}
}

void postload_arafs()
{
	for(int i=0; i < 'Z'-'A'+1; i++) {
		safe_strncpy(bx_options.aranymfs[i].rootPath, bx_options.aranymfs[i].configPath, sizeof(bx_options.aranymfs[i].rootPath));
		int len = strlen(bx_options.aranymfs[i].configPath);
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

void presave_arafs()
{
	for(int i=0; i < 'Z'-'A'+1; i++) {
		safe_strncpy(bx_options.aranymfs[i].configPath, bx_options.aranymfs[i].rootPath, sizeof(bx_options.aranymfs[i].configPath));
		if ( strlen(bx_options.aranymfs[i].rootPath) > 0 &&
			 !bx_options.aranymfs[i].halfSensitive ) {
			// set the halfSensitive indicator
			strcat( bx_options.aranymfs[i].configPath, ":" );
		}
	}
}

/*************************************************************************/
// struct Config_Tag ethernet_conf[]={
#define ETH_CONFIG(Eth)	struct Config_Tag Eth ## _conf[] = {	\
	{ "Type", String_Tag, &Eth->type, sizeof(Eth->type), 0}, \
	{ "Tunnel", String_Tag, &Eth->tunnel, sizeof(Eth->tunnel), 0}, \
	{ "HostIP", String_Tag, &Eth->ip_host, sizeof(Eth->ip_host), 0}, \
	{ "AtariIP", String_Tag, &Eth->ip_atari, sizeof(Eth->ip_atari), 0}, \
	{ "Netmask", String_Tag, &Eth->netmask, sizeof(Eth->netmask), 0}, \
	{ "MAC", String_Tag, &Eth->mac_addr, sizeof(Eth->mac_addr), 0}, \
	{ NULL , Error_Tag, NULL, 0, 0 } \
}

static bx_ethernet_options_t *eth0 = &bx_options.ethernet[0];
static bx_ethernet_options_t *eth1 = &bx_options.ethernet[1];
static bx_ethernet_options_t *eth2 = &bx_options.ethernet[2];
static bx_ethernet_options_t *eth3 = &bx_options.ethernet[3];

ETH_CONFIG(eth0);
ETH_CONFIG(eth1);
ETH_CONFIG(eth2);
ETH_CONFIG(eth3);

#define ETH(i, x) bx_options.ethernet[i].x
void preset_ethernet()
{
	// ETH[0] with some default values
	safe_strncpy(ETH(0, type), XIF_TYPE, sizeof(ETH(0, type)));
	safe_strncpy(ETH(0, tunnel), XIF_TUNNEL, sizeof(ETH(0, tunnel)));
	safe_strncpy(ETH(0, ip_host), XIF_HOST_IP, sizeof(ETH(0, ip_host)));
	safe_strncpy(ETH(0, ip_atari), XIF_ATARI_IP, sizeof(ETH(0, ip_atari)));
	safe_strncpy(ETH(0, netmask), XIF_NETMASK, sizeof(ETH(0, netmask)));
	safe_strncpy(ETH(0, mac_addr), XIF_MAC_ADDR, sizeof(ETH(0, mac_addr)));

	// ETH[1] - ETH[MAX_ETH] are empty by default
	for(int i=1; i<MAX_ETH; i++) {
		*ETH(i, type) = '\0';
		*ETH(i, tunnel) = '\0';
		*ETH(i, ip_host) = '\0';
		*ETH(i, ip_atari) = '\0';
		*ETH(i, netmask) = '\0';
		*ETH(i, mac_addr) = '\0';
	}
}

void postload_ethernet()
{
}

void presave_ethernet()
{
}

/*************************************************************************/
#define LILO(x) bx_options.lilo.x

struct Config_Tag lilo_conf[]={
	{ "Kernel", Path_Tag, &LILO(kernel), sizeof(LILO(kernel)), 0},
	{ "Args", String_Tag, &LILO(args), sizeof(LILO(args)), 0},
	{ "Ramdisk", Path_Tag, &LILO(ramdisk), sizeof(LILO(ramdisk)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_lilo()
{
  safe_strncpy(LILO(kernel), "linux.bin", sizeof(LILO(kernel)));
  safe_strncpy(LILO(args), "root=/dev/ram video=atafb:vga16", sizeof(LILO(args)));
  safe_strncpy(LILO(ramdisk), "root.bin", sizeof(LILO(ramdisk)));
}

void postload_lilo()
{
}

void presave_lilo()
{
}

/*************************************************************************/
#define MIDI(x) bx_options.midi.x

struct Config_Tag midi_conf[]={
	{ "Type", String_Tag, &MIDI(type), sizeof(MIDI(type)), 0},
	{ "File", Path_Tag, &MIDI(file), sizeof(MIDI(file)), 0},
	{ "Sequencer", Path_Tag, &MIDI(sequencer), sizeof(MIDI(sequencer)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_midi() {
  safe_strncpy(MIDI(type), "none", sizeof(MIDI(type)));
  safe_strncpy(MIDI(file), "", sizeof(MIDI(file)));
  safe_strncpy(MIDI(sequencer), "/dev/sequencer", sizeof(MIDI(sequencer)));
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
struct Config_Tag autozoom_conf[]={
	{ "Enabled", Bool_Tag, &bx_options.autozoom.enabled, 0, 0},
	{ "IntegerCoefs", Bool_Tag, &bx_options.autozoom.integercoefs, 0, 0},
	{ "FixedSize", Bool_Tag, &bx_options.autozoom.fixedsize, 0, 0},
	{ "Width", Int_Tag, &bx_options.autozoom.width, 0, 0},
	{ "Height", Int_Tag, &bx_options.autozoom.height, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_autozoom() {
  bx_options.autozoom.enabled = false;
  bx_options.autozoom.integercoefs = false;
  bx_options.autozoom.fixedsize = false;
  bx_options.autozoom.width = 640;
  bx_options.autozoom.height = 480;
}

void postload_autozoom() {
}

void presave_autozoom() {
}

/*************************************************************************/
#define OSMESA_CONF(x) bx_options.osmesa.x

struct Config_Tag osmesa_conf[]={
	{ "ChannelSize", Int_Tag, &OSMESA_CONF(channel_size), 0, 0},
	{ "LibGL", String_Tag, &OSMESA_CONF(libgl), sizeof(OSMESA_CONF(libgl)), 0},
	{ "LibOSMesa", String_Tag, &OSMESA_CONF(libosmesa), sizeof(OSMESA_CONF(libosmesa)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_osmesa() {
	OSMESA_CONF(channel_size) = 0;
	safe_strncpy(OSMESA_CONF(libgl), "libGL.so", sizeof(OSMESA_CONF(libgl)));
	safe_strncpy(OSMESA_CONF(libosmesa), "libOSMesa.so", sizeof(OSMESA_CONF(libosmesa)));
}

void postload_osmesa() {
}

void presave_osmesa() {
}

/*************************************************************************/
#define PARALLEL_CONF(x) bx_options.parallel.x

struct Config_Tag parallel_conf[]={
	{ "Type", String_Tag, &PARALLEL_CONF(type), sizeof(PARALLEL_CONF(type)), 0},
	{ "File", String_Tag, &PARALLEL_CONF(file), sizeof(PARALLEL_CONF(file)), 0},
	{ "Parport", String_Tag, &PARALLEL_CONF(parport), sizeof(PARALLEL_CONF(parport)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_parallel() {
  safe_strncpy(PARALLEL_CONF(type), "file", sizeof(PARALLEL_CONF(type)));
  safe_strncpy(PARALLEL_CONF(file), "stderr", sizeof(PARALLEL_CONF(type)));
  safe_strncpy(PARALLEL_CONF(parport), "/dev/parport0", sizeof(PARALLEL_CONF(parport)));
}

void postload_parallel() {
}

void presave_parallel() {
}
/*************************************************************************/
#define SERIAL_CONF(x) bx_options.serial.x

struct Config_Tag serial_conf[]={
	{ "Serport", String_Tag, &SERIAL_CONF(serport), sizeof(SERIAL_CONF(serport)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_serial() {
  safe_strncpy(SERIAL_CONF(serport), DEFAULT_SERIAL, sizeof(SERIAL_CONF(serport)));
}

void postload_serial() {
}

void presave_serial() {
}

/*************************************************************************/
#define NATFEAT_CONF(x) bx_options.natfeats.x

struct Config_Tag natfeat_conf[]={
	{ "CDROM", String_Tag, &NATFEAT_CONF(cdrom_driver), sizeof(NATFEAT_CONF(cdrom_driver)), 0},
	{ "Vdi", String_Tag, &NATFEAT_CONF(vdi_driver), sizeof(NATFEAT_CONF(vdi_driver)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_natfeat() {
  safe_strncpy(NATFEAT_CONF(cdrom_driver), "sdl", sizeof(NATFEAT_CONF(cdrom_driver)));
  safe_strncpy(NATFEAT_CONF(vdi_driver), "soft", sizeof(NATFEAT_CONF(vdi_driver)));
}

void postload_natfeat()
{
#ifndef ENABLE_OPENGL
  safe_strncpy(NATFEAT_CONF(vdi_driver), "soft", sizeof(NATFEAT_CONF(vdi_driver)));
#endif
}

void presave_natfeat() {
}

/*************************************************************************/
#define NFVDI_CONF(x) bx_options.nfvdi.x

struct Config_Tag nfvdi_conf[]={
	{ "UseHostMouseCursor", Bool_Tag,  &NFVDI_CONF(use_host_mouse_cursor), 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_nfvdi() {
	NFVDI_CONF(use_host_mouse_cursor) = false;
}

void postload_nfvdi() {
}

void presave_nfvdi() {
}

/*************************************************************************/

#define HOTKEYS_STRING_SIZE		20
static char hotkeys[10][HOTKEYS_STRING_SIZE];
struct Config_Tag hotkeys_conf[]={
	{ "Setup", String_Tag, hotkeys[0], HOTKEYS_STRING_SIZE, 0},
	{ "Quit", String_Tag, hotkeys[1], HOTKEYS_STRING_SIZE, 0},
	{ "Reboot", String_Tag, hotkeys[2], HOTKEYS_STRING_SIZE, 0},
	{ "Ungrab", String_Tag, hotkeys[3], HOTKEYS_STRING_SIZE, 0},
	{ "Debug", String_Tag, hotkeys[4], HOTKEYS_STRING_SIZE, 0},
	{ "Screenshot", String_Tag, hotkeys[5], HOTKEYS_STRING_SIZE, 0},
	{ "Fullscreen", String_Tag, hotkeys[6], HOTKEYS_STRING_SIZE, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

typedef struct { char *string; SDL_keysym *keysym; } HOTKEYS_REL;

HOTKEYS_REL hotkeys_rel[]={
	{ hotkeys[0], &bx_options.hotkeys.setup },
	{ hotkeys[1], &bx_options.hotkeys.quit },
	{ hotkeys[2], &bx_options.hotkeys.reboot },
	{ hotkeys[3], &bx_options.hotkeys.ungrab },
	{ hotkeys[4], &bx_options.hotkeys.debug },
	{ hotkeys[5], &bx_options.hotkeys.screenshot },
	{ hotkeys[6], &bx_options.hotkeys.fullscreen }
};

void preset_hotkeys()
{
	// default values
#ifdef OS_darwin
	bx_options.hotkeys.setup.sym = SDLK_COMMA;
	bx_options.hotkeys.setup.mod = KMOD_LMETA;
	bx_options.hotkeys.quit.sym = SDLK_q;
	bx_options.hotkeys.quit.mod = KMOD_LMETA;
	bx_options.hotkeys.reboot.sym = SDLK_r;
	bx_options.hotkeys.reboot.mod = KMOD_LMETA;
	bx_options.hotkeys.ungrab.sym = SDLK_ESCAPE;
	bx_options.hotkeys.ungrab.mod = KMOD_LMETA;
	bx_options.hotkeys.debug.sym = SDLK_d;
	bx_options.hotkeys.debug.mod = KMOD_LMETA;
	bx_options.hotkeys.screenshot.sym = SDLK_s;
	bx_options.hotkeys.screenshot.mod = KMOD_LMETA;
	bx_options.hotkeys.fullscreen.sym = SDLK_f;
	bx_options.hotkeys.fullscreen.mod = KMOD_LMETA;
#else
	bx_options.hotkeys.setup.sym = SDLK_PAUSE;
	bx_options.hotkeys.setup.mod = KMOD_NONE;
	bx_options.hotkeys.quit.sym = SDLK_PAUSE;
	bx_options.hotkeys.quit.mod = KMOD_LSHIFT;
	bx_options.hotkeys.reboot.sym = SDLK_PAUSE;
	bx_options.hotkeys.reboot.mod = KMOD_LCTRL;
	bx_options.hotkeys.ungrab.sym = SDLK_ESCAPE;
	bx_options.hotkeys.ungrab.mod = (SDLMod)(KMOD_LSHIFT | KMOD_LCTRL | KMOD_LALT);
	bx_options.hotkeys.debug.sym = SDLK_PAUSE;
	bx_options.hotkeys.debug.mod = KMOD_LALT;
	bx_options.hotkeys.screenshot.sym = SDLK_PRINT;
	bx_options.hotkeys.screenshot.mod = KMOD_NONE;
	bx_options.hotkeys.fullscreen.sym = SDLK_SCROLLOCK;
	bx_options.hotkeys.fullscreen.mod = KMOD_NONE;
#endif
}

void postload_hotkeys() {
	// convert from string to pair of ints
	for(uint16 i=0; i<sizeof(hotkeys_rel)/sizeof(hotkeys_rel[0]); i++) {
		int sym, mod;
		if ( sscanf(hotkeys_rel[i].string, "%i:%i", &sym, &mod) != 2 ) continue;

		hotkeys_rel[i].keysym->sym = SDLKey(sym);
		hotkeys_rel[i].keysym->mod = SDLMod(mod);
	}
}

void presave_hotkeys() {
	// convert from pair of ints to string
	for(uint16 i=0; i<sizeof(hotkeys_rel)/sizeof(hotkeys_rel[0]); i++) {
		snprintf(hotkeys_rel[i].string, HOTKEYS_STRING_SIZE, "%d:%#x", hotkeys_rel[i].keysym->sym, hotkeys_rel[i].keysym->mod);
	}
}

/*************************************************************************/
struct Config_Tag ikbd_conf[]={
	{ "WheelEiffel", Bool_Tag, &bx_options.ikbd.wheel_eiffel, 0, 0},
	{ "AltGr", Bool_Tag, &bx_options.ikbd.altgr, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_ikbd()
{
	bx_options.ikbd.wheel_eiffel = false;
	bx_options.ikbd.altgr = true;
}

void postload_ikbd()
{
}

void presave_ikbd()
{
}

/*************************************************************************/
struct Config_Tag audio_conf[]={
	{ "Frequency", Int_Tag, &bx_options.audio.freq, 0, 0},
	{ "Channels", Int_Tag, &bx_options.audio.chans, 0, 0},
	{ "Bits", Int_Tag, &bx_options.audio.bits, 0, 0},
	{ "Samples", Int_Tag, &bx_options.audio.samples, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_audio() {
  bx_options.audio.freq = 22050;
  bx_options.audio.chans = 2;
  bx_options.audio.bits = 16;
  bx_options.audio.samples = 1024;
}

void postload_audio() {
}

void presave_audio() {
}

/*************************************************************************/
struct Config_Tag joysticks_conf[]={
	{ "Ikbd0", Int_Tag, &bx_options.joysticks.ikbd0, 0, 0},
	{ "Ikbd1", Int_Tag, &bx_options.joysticks.ikbd1, 0, 0},
	{ "JoypadA", Int_Tag, &bx_options.joysticks.joypada, 0, 0},
	{ "JoypadAButtons", String_Tag, &bx_options.joysticks.joypada_mapping,
		sizeof(bx_options.joysticks.joypada_mapping), 0},
	{ "JoypadB", Int_Tag, &bx_options.joysticks.joypadb, 0, 0},
	{ "JoypadBButtons", String_Tag, &bx_options.joysticks.joypadb_mapping,
		sizeof(bx_options.joysticks.joypadb_mapping), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_joysticks() {
	bx_options.joysticks.ikbd0 = -1;	/* This one is wired to mouse */
	bx_options.joysticks.ikbd1 = 0;
	bx_options.joysticks.joypada = -1;
	bx_options.joysticks.joypadb = -1;
	safe_strncpy(bx_options.joysticks.joypada_mapping,
		"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16",
		sizeof(bx_options.joysticks.joypada_mapping));
	safe_strncpy(bx_options.joysticks.joypadb_mapping,
		"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16",
		sizeof(bx_options.joysticks.joypadb_mapping));
}

void postload_joysticks() {
}

void presave_joysticks() {
}

/*************************************************************************/
void usage (int status) {
  printf ("Usage: %s [OPTIONS]\n", program_name);
  printf ("\
Options:\n\
  -a, --floppy NAME          floppy image file NAME\n\
  -e, --emutos               boot EmuTOS\n\
  -N, --nomouse              don't grab mouse at startup\n\
  -f, --fullscreen           start in fullscreen\n\
  -v, --refresh <X>          VIDEL refresh rate in VBL (default 2)\n\
  -r, --resolution <X>       boot in X color depth [1,2,4,8,16]\n\
  -m, --monitor <X>          attached monitor: 0 = VGA, 1 = TV\n\
  -c, --config FILE          read different configuration file\n\
  -s, --save                 save configuration file\n\
  -S, --swap-ide             swap IDE drives\n\
  -P <X,Y> or <center>       set window position\n\
  -k, --locale <XY>          set NVRAM keyboard layout and language\n\
  -h, --help                 display this help and exit\n\
  -V, --version              output version information and exit\n\
");
#ifdef SDL_GUI
  printf("  -G, --gui                  open GUI at startup\n");
#endif
#if HOSTFS_SUPPORT
  printf("  -d, --disk CHAR:PATH[:]    HostFS mapping, e.g. d:/atari/d_drive\n");
#endif
#ifdef ENABLE_LILO
  printf("  -l, --lilo                 boot a linux kernel\n");
  printf("  -H, --halt                 linux kernel halts on reboot\n");
#endif
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
  preset_ikbd();
  preset_hotkeys();
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
  preset_autozoom();
  preset_osmesa();
  preset_parallel();
  preset_serial();
  preset_natfeat();
  preset_nfvdi();
  preset_audio();
  preset_joysticks();
}

void postload_cfg() {
  postload_global();
  postload_ikbd();
  postload_hotkeys();
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
  postload_autozoom();
  postload_osmesa();
  postload_parallel();
  postload_serial();
  postload_natfeat();
  postload_nfvdi();
  postload_audio();
  postload_joysticks();
}

void presave_cfg() {
  presave_global();
  presave_ikbd();
  presave_hotkeys();
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
  presave_autozoom();
  presave_osmesa();
  presave_parallel();
  presave_serial();
  presave_natfeat();
  presave_nfvdi();
  presave_audio();
  presave_joysticks();
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
			infoprint("HW Configuration:");
			infoprint("CPU JIT compiler : %s", (USE_JIT == 1) ? "enabled" : "disabled");
			infoprint("Full MMU         : %s", (FULLMMU == 1) ? "enabled" : "disabled");
			infoprint("FPU              : %s", USES_FPU_CORE);
			infoprint("DSP              : %s", (DSP_EMULATION == 1) ? "enabled" : "disabled");
			infoprint("DSP disassembler : %s", (DSP_DISASM == 1) ? "enabled" : "disabled");
			infoprint("OpenGL support   : %s", (ENABLE_OPENGL == 1) ? "enabled" : "disabled");
			infoprint("Native features  : %s", PROVIDES_NATFEATS);

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
#ifdef ENABLE_LILO
							 "l"  /* boot lilo */
							 "H"  /* halt on reboot */
#endif
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
#if HOSTFS_SUPPORT
							 "d:" /* filesystem assignment */
#endif
							 "s"  /* save config file */

							 "c:" /* path to config file */
#ifdef SDL_GUI
							 "G"  /* GUI startup */
#endif
							 "P:" /* position of the window */
							 "S"  /* swap IDE drives */
							 "k:" /* keyboard layout */
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
#ifdef SDL_GUI
			case 'G':
				startupGUI = true;
				break;
#endif
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

#ifdef ENABLE_LILO
			case 'l':
				boot_lilo = true;
				break;

			case 'H':
				halt_on_reboot = true;
				break;
#endif

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
					fprintf(stderr, "Floppy image filename longer than %zu chars.\n", sizeof(bx_options.floppy.path));
				safe_strncpy(bx_options.floppy.path, optarg, sizeof(bx_options.floppy.path));
				break;

			case 'r':
				bx_options.video.boot_color_depth = atoi(optarg);
				break;

			case 'P':
				safe_strncpy(bx_options.video.window_pos, optarg, sizeof(bx_options.video.window_pos));
				break;

#if HOSTFS_SUPPORT
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
#endif

			case 's':
				saveConfigFile = true;
				break;

#ifndef FixedSizeFastRAM
			case 'F':
				bx_options.fastram = atoi(optarg);
				break;
#endif

			case 'k':
				{
					char countries[][3] = {{"us"},{"de"},{"fr"},{"uk"},{"es"},{"it"},{"se"},{"ch"},
							{"cd"},{"tr"},{"fi"},{"no"},{"dk"},{"sa"},{"nl"},{"cz"},{"hu"},{"sk"},{"gr"}};
					bx_options.tos.cookie_akp = -1;
					for(unsigned i=0; i<sizeof(countries)/sizeof(countries[0]); i++) {
						if (strcasecmp(optarg, countries[i]) == 0) {
							bx_options.tos.cookie_akp = i << 8 | i;
							break;
						}
					}
					if (bx_options.tos.cookie_akp == -1) {
						fprintf(stderr, "Error '%s', use one of:", optarg);
						for(unsigned i=0; i<sizeof(countries)/sizeof(countries[0]); i++) {
							fprintf(stderr, " %s", countries[i]);
						}
						fprintf(stderr, "\n");
						exit(0);
					}
				}
				break;
				
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
	Host::getConfFolder(buffer, bufsize);

	// Does the folder exist?
	struct stat buf;
	if (stat(buffer, &buf) == -1) {
		D(bug("Creating config folder '%s'", buffer));
		Host::makeDir(buffer);
	}

	return addFilename(buffer, file, bufsize);
}

// build a complete path to system wide data file
char *getDataFilename(const char *file, char *buffer, unsigned int bufsize)
{
	Host::getDataFolder(buffer, bufsize);
	return addFilename(buffer, file, bufsize);
}

static bool decode_ini_file(const char *rcfile)
{
	// Does the config exist?
	struct stat buf;
	if (stat(rcfile, &buf) == -1) {
		infoprint("Config file '%s' not found. It's been created with default values. Edit it to suit your needs.", rcfile);
		saveConfigFile = true;
#ifdef SDL_GUI
		startupGUI = true;
#endif
		return false;
	}

	infoprint("Using config file: '%s'", rcfile);

	bool verbose = false;

	char home_folder[1024];
	char data_folder[1024];
	Host::getHomeFolder(home_folder, sizeof(home_folder));
	Host::getDataFolder(data_folder, sizeof(data_folder));
	ConfigOptions cfgopts(rcfile, home_folder, data_folder);

	cfgopts.process_config(global_conf, "[GLOBAL]", verbose);
	cfgopts.process_config(startup_conf, "[STARTUP]", verbose);
	cfgopts.process_config(ikbd_conf, "[IKBD]", verbose);
	cfgopts.process_config(hotkeys_conf, "[HOTKEYS]", verbose);
	cfgopts.process_config(jit_conf, "[JIT]", verbose);
	cfgopts.process_config(video_conf, "[VIDEO]", verbose);
	cfgopts.process_config(tos_conf, "[TOS]", verbose);
	cfgopts.process_config(ide_swap ? diskd_configs : diskc_configs, "[IDE0]", verbose);
	cfgopts.process_config(ide_swap ? diskc_configs : diskd_configs, "[IDE1]", verbose);

	cfgopts.process_config(disk0_configs, "[PARTITION0]", verbose);
	cfgopts.process_config(disk1_configs, "[PARTITION1]", verbose);
	cfgopts.process_config(disk2_configs, "[PARTITION2]", verbose);
	cfgopts.process_config(disk3_configs, "[PARTITION3]", verbose);
	cfgopts.process_config(disk4_configs, "[PARTITION4]", verbose);
	cfgopts.process_config(disk5_configs, "[PARTITION5]", verbose);
	cfgopts.process_config(disk6_configs, "[PARTITION6]", verbose);
	cfgopts.process_config(disk7_configs, "[PARTITION7]", verbose);

	cfgopts.process_config(arafs_conf, "[HOSTFS]", verbose);
	cfgopts.process_config(opengl_conf, "[OPENGL]", verbose);
	cfgopts.process_config(eth0_conf, "[ETH0]", verbose);
	cfgopts.process_config(eth1_conf, "[ETH1]", verbose);
	cfgopts.process_config(eth2_conf, "[ETH2]", verbose);
	cfgopts.process_config(eth3_conf, "[ETH3]", verbose);
	cfgopts.process_config(lilo_conf, "[LILO]", verbose);
	cfgopts.process_config(midi_conf, "[MIDI]", verbose);
	cfgopts.process_config(nfcdroms_conf, "[CDROMS]", verbose);
	cfgopts.process_config(autozoom_conf, "[AUTOZOOM]", verbose);
	cfgopts.process_config(osmesa_conf, "[NFOSMESA]", verbose);
	cfgopts.process_config(parallel_conf, "[PARALLEL]", verbose);
	cfgopts.process_config(serial_conf, "[SERIAL]", verbose);
	cfgopts.process_config(natfeat_conf, "[NATFEATS]", verbose);
	cfgopts.process_config(nfvdi_conf, "[NFVDI]", verbose);
	cfgopts.process_config(audio_conf, "[AUDIO]", verbose);
	cfgopts.process_config(joysticks_conf, "[JOYSTICKS]", verbose);

	return true;
}

bool loadSettings(const char *cfg_file) {
	preset_cfg();
	bool ret = decode_ini_file(cfg_file);
	postload_cfg();
	return ret;
}

bool saveSettings(const char *fs)
{
	presave_cfg();

	char home_folder[1024];
	char data_folder[1024];
	Host::getHomeFolder(home_folder, sizeof(home_folder));
	Host::getDataFolder(data_folder, sizeof(data_folder));
	ConfigOptions cfgopts(fs, home_folder, data_folder);

	if (cfgopts.update_config(global_conf, "[GLOBAL]") < 0) {
		fprintf(stderr, "Error while writing the '%s' config file.\n", fs);
		return false;
	}
	cfgopts.update_config(startup_conf, "[STARTUP]");
	cfgopts.update_config(ikbd_conf, "[IKBD]");
	cfgopts.update_config(hotkeys_conf, "[HOTKEYS]");
	cfgopts.update_config(jit_conf, "[JIT]");
	cfgopts.update_config(video_conf, "[VIDEO]");
	cfgopts.update_config(tos_conf, "[TOS]");
	cfgopts.update_config(ide_swap ? diskd_configs : diskc_configs, "[IDE0]");
	cfgopts.update_config(ide_swap ? diskc_configs : diskd_configs, "[IDE1]");

	cfgopts.update_config(disk0_configs, "[PARTITION0]");
	if (strlen((char *)disk1_configs->buf))
		cfgopts.update_config(disk1_configs, "[PARTITION1]");
	if (strlen((char *)disk2_configs->buf))
		cfgopts.update_config(disk2_configs, "[PARTITION2]");
	if (strlen((char *)disk3_configs->buf))
		cfgopts.update_config(disk3_configs, "[PARTITION3]");
	if (strlen((char *)disk4_configs->buf))
		cfgopts.update_config(disk4_configs, "[PARTITION4]");
	if (strlen((char *)disk5_configs->buf))
		cfgopts.update_config(disk5_configs, "[PARTITION5]");
	if (strlen((char *)disk6_configs->buf))
		cfgopts.update_config(disk6_configs, "[PARTITION6]");
	if (strlen((char *)disk7_configs->buf))
		cfgopts.update_config(disk7_configs, "[PARTITION7]");

	cfgopts.update_config(arafs_conf, "[HOSTFS]");
	cfgopts.update_config(opengl_conf, "[OPENGL]");

	cfgopts.update_config(eth0_conf, "[ETH0]");
	if (strlen((char *)eth1_conf->buf))
		cfgopts.update_config(eth1_conf, "[ETH1]");
	if (strlen((char *)eth2_conf->buf))
		cfgopts.update_config(eth2_conf, "[ETH2]");
	if (strlen((char *)eth3_conf->buf))
		cfgopts.update_config(eth3_conf, "[ETH3]");

	cfgopts.update_config(lilo_conf, "[LILO]");
	cfgopts.update_config(midi_conf, "[MIDI]");
	cfgopts.update_config(nfcdroms_conf, "[CDROMS]");
	cfgopts.update_config(autozoom_conf, "[AUTOZOOM]");
	cfgopts.update_config(osmesa_conf, "[NFOSMESA]");
	cfgopts.update_config(parallel_conf, "[PARALLEL]");
	cfgopts.update_config(serial_conf, "[SERIAL]");
	cfgopts.update_config(natfeat_conf, "[NATFEATS]");
	cfgopts.update_config(nfvdi_conf, "[NFVDI]");
	cfgopts.update_config(audio_conf, "[AUDIO]");
	cfgopts.update_config(joysticks_conf, "[JOYSTICKS]");

	return true;
}

bool check_cfg()
{
	return true;
}

bool decode_switches(int argc, char **argv)
{
	getConfFilename(ARANYMCONFIG, config_file, sizeof(config_file));

	early_cmdline_check(argc, argv);
	preset_cfg();
	decode_ini_file(config_file);
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

/*
vim:ts=4:sw=4:
*/
