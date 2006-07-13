/*
 * parameters.cpp - parameter init/load/save code
 *
 * Copyright (c) 2001-2006 ARAnyM developer team (see AUTHORS)
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
#include "gdbstub.h"
#include "host.h"

#define DEBUG 0
#include "debug.h"

# include <cstdlib>

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

#ifndef NFCDROM_SUPPORT
# define NFCDROM_SUPPORT 0
#endif

#ifndef NFCDROM_LINUX_SUPPORT
# define NFCDROM_LINUX_SUPPORT 0
#endif

#ifndef NFOSMESA_SUPPORT
# define NFOSMESA_SUPPORT 0
#endif

#ifndef NFJPEG_SUPPORT
# define NFJPEG_SUPPORT 0
#endif

static struct option const long_options[] =
{
#ifndef FixedSizeFastRAM
  {"fastram", required_argument, 0, 'F'},
#endif
  {"floppy", required_argument, 0, 'a'},
  {"resolution", required_argument, 0, 'r'},
#if (defined(DEBUGGER) || defined(GDBSTUB))
  {"debug", no_argument, 0, 'D'},
# ifdef GDBSTUB
  {"port", required_argument, 0, 'p'},
  {"base", required_argument, 0, 'b'},
# endif
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
#ifdef ENABLE_LILO
  {"lilo", no_argument, 0, 'l'},
#endif
  {"display", required_argument, 0, 'P'},
  {NULL, 0, NULL, 0}
};

#define TOS_FILENAME		"ROM"
#define EMUTOS_FILENAME		"etos512k.img"
#define FREEMINT_FILENAME	"mintara.prg"

char *program_name;		// set by main()

#ifdef SDL_GUI
bool startupGUI = false;
#endif

bool boot_emutos = false;
bool boot_lilo = false;
bool ide_swap = false;
uint32 FastRAMSize;

static char config_file[512];

#if !defined(XIF_HOST_IP) && !defined(XIF_ATARI_IP) && !defined(XIF_NETMASK)
# define XIF_TYPE	"ptp"
# define XIF_TUNNEL	"tap0"
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
	{ "Floppy", Path_Tag, bx_options.floppy.path, sizeof(bx_options.floppy.path), 0},
	{ "TOS", Path_Tag, bx_options.tos_path, sizeof(bx_options.tos_path), 0},
	{ "EmuTOS", Path_Tag, bx_options.emutos_path, sizeof(bx_options.emutos_path), 0},
	{ "Bootstrap", Path_Tag, bx_options.bootstrap_path, sizeof(bx_options.bootstrap_path), 0},
	{ "BootstrapArgs", String_Tag, bx_options.bootstrap_args, sizeof(bx_options.bootstrap_args), 0},
	{ "BootDrive", Char_Tag, &bx_options.bootdrive, 0, 0},
	{ "AutoGrabMouse", Bool_Tag, &bx_options.autoMouseGrab, 0, 0},
#ifdef ENABLE_EPSLIMITER
	{ "EpsEnabled", Bool_Tag, &bx_options.cpu.eps_enabled, 0, 0},
	{ "EpsMax", Int_Tag, &bx_options.cpu.eps_max, 0, 0},
#endif
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_global()
{
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

void postload_global()
{
#ifndef FixedSizeFastRAM
	FastRAMSize = bx_options.fastram * 1024 * 1024;
#endif
}

void presave_global()
{
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

void preset_startup()
{
  bx_options.startup.debugger = false;
  bx_options.startup.grabMouseAllowed = true;
#ifdef GDBSTUB
  bx_options.gdbstub.text_base = GDBSTUB_TEXT_BASE;
  bx_options.gdbstub.data_base = GDBSTUB_DATA_BASE;
  bx_options.gdbstub.bss_base = GDBSTUB_BSS_BASE;
#endif
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
	bx_options.jit.jit = false;
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
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_video()
{
  bx_options.video.fullscreen = false;		// Boot in Fullscreen
  bx_options.video.boot_color_depth = -1;	// Boot in color depth
  bx_options.video.monitor = -1;			// preserve default NVRAM monitor
  bx_options.video.refresh = 2;				// 25 Hz update
  bx_options.video.x_win_offset = -1;
  bx_options.video.y_win_offset = -1;
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
	{ NULL , Error_Tag, NULL, 0, 0 }
};

void preset_opengl()
{
  bx_options.opengl.enabled = false;
  bx_options.opengl.filtered = false;
}

void postload_opengl()
{
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

static void set_ide(unsigned int number, char *dev_path, int cylinders, int heads, int spt, int byteswap, bool readonly, const char *model_name)
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
			 bx_options.aranymfs[i].halfSensitive ) {
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

	// ETH[1] - ETH[MAX_ETH] are empty by default
	for(int i=1; i<MAX_ETH; i++) {
		*ETH(i, type) = '\0';
		*ETH(i, tunnel) = '\0';
		*ETH(i, ip_host) = '\0';
		*ETH(i, ip_atari) = '\0';
		*ETH(i, netmask) = '\0';
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
	{ "Kernel", String_Tag, &LILO(kernel), sizeof(LILO(kernel)), 0},
	{ "Args", String_Tag, &LILO(args), sizeof(LILO(args)), 0},
	{ "Ramdisk", String_Tag, &LILO(ramdisk), sizeof(LILO(ramdisk)), 0},
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
	{ "Enabled", Bool_Tag, &MIDI(enabled), 0, 0},
	{ "Output", Path_Tag, &MIDI(output), sizeof(MIDI(output)), 0},
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

void postload_natfeat() {
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
#ifdef OS_darwin
#define HOTKEY_OPENGUI		SDLK_PRINT	// F13
#define	HOTKEY_FULLSCREEN	SDLK_NUMLOCK
#define	HOTKEY_SCREENSHOT	SDLK_F15
#else
#define HOTKEY_OPENGUI		SDLK_PAUSE
#define	HOTKEY_FULLSCREEN	SDLK_SCROLLOCK
#define	HOTKEY_SCREENSHOT	SDLK_PRINT
#endif

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
	bx_options.hotkeys.setup.sym = HOTKEY_OPENGUI;
	bx_options.hotkeys.setup.mod = KMOD_NONE;
	bx_options.hotkeys.quit.sym = HOTKEY_OPENGUI;
	bx_options.hotkeys.quit.mod = KMOD_LSHIFT;
	bx_options.hotkeys.reboot.sym = HOTKEY_OPENGUI;
	bx_options.hotkeys.reboot.mod = KMOD_LCTRL;
	bx_options.hotkeys.ungrab.sym = SDLK_ESCAPE;
	bx_options.hotkeys.ungrab.mod = (SDLMod)(KMOD_LSHIFT | KMOD_LCTRL | KMOD_LALT);
	bx_options.hotkeys.debug.sym = HOTKEY_OPENGUI;
	bx_options.hotkeys.debug.mod = KMOD_LALT;
	bx_options.hotkeys.screenshot.sym = HOTKEY_SCREENSHOT;
	bx_options.hotkeys.screenshot.mod = KMOD_NONE;
	bx_options.hotkeys.fullscreen.sym = HOTKEY_FULLSCREEN;
	bx_options.hotkeys.fullscreen.mod = KMOD_NONE;
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
  -P <X> <Y>                 set window position\n\
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
#endif
#ifndef FixedSizeFastRAM
  printf("  -F, --fastram SIZE         FastRAM size (in MB)\n");
#endif
#if (defined(DEBUGGER) || defined(GDBSTUB))
  printf("  -D, --debug                start debugger\n");
# ifdef GDBSTUB
  printf("  -p, --port NUMBER          port number for GDB (default: 1234)\n");
  printf("  -b, --base TEXT DATA BSS   base pointers\n");
#endif
#endif
  exit (status);
}

void preset_cfg() {
  preset_global();
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
  preset_natfeat();
  preset_nfvdi();
}

void postload_cfg() {
  postload_global();
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
  postload_natfeat();
  postload_nfvdi();
}

void presave_cfg() {
  presave_global();
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
  presave_natfeat();
  presave_nfvdi();
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
			infoprint("Native features:");
			infoprint(" CD-ROM driver   : %s", (NFCDROM_SUPPORT == 1) ? "enabled" : "disabled");
			infoprint(" OSMesa rendering: %s", (NFOSMESA_SUPPORT == 1) ? "enabled" : "disabled");
			infoprint(" JPEG decoder    : %s", (NFJPEG_SUPPORT == 1) ? "enabled" : "disabled");

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
#endif
#if (defined(DEBUGGER) || defined(GDBSTUB))
							 "D"  /* debugger */
# ifdef GDBSTUB
							 "p:" /* gdb port */
							 "b:" /* base */
# endif
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
#if (defined(DEBUGGER) || defined(GDBSTUB))
			case 'D':
				bx_options.startup.debugger = true;
				break;
# ifdef GDBSTUB
			case 'p':
				port_number = atoi(optarg); 
				break;
			case 'b':
				if ((optind + 2 ) > argc)
				{
					fprintf(stderr, "Not enough parameters for  --base option");
					usage(EXIT_FAILURE);
				} else {
					bx_options.gdbstub.text_base = atoi(optarg);
					bx_options.gdbstub.data_base = atoi(argv[optind]);
					bx_options.gdbstub.bss_base = atoi(argv[optind]);
				}
				break;
# endif
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
				bx_options.video.x_win_offset = atoi(optarg);
				bx_options.video.y_win_offset = atoi(argv[optind]);
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

			default:
				usage (EXIT_FAILURE);
		}
	}
	return optind;
}

void postload_PathTag( const char *filename, struct Config_Tag *ptr ) {
	char *path = (char *)ptr->buf;
	if ( !strlen(path) )
		return;

	char home[2048];
	const char *prefix = NULL;
	size_t prefixLen = 0;

	if ( path[0] == '~' ) {
		// replace with the home folder path
		prefix = Host::getHomeFolder(home, sizeof(home));
		prefixLen = strlen( prefix );
		path++;
	} else if (path[0] == '*') {
		prefix= Host::getDataFolder(home, sizeof(home));
		prefixLen = strlen( prefix );
		path++;
	} else if ( path[0] != '/' && path[0] != '\\' && path[1] != ':' ) {
		// find the last dirseparator
		const char *slash = &filename[strlen(filename)];
		while ( --slash >= filename &&
			*slash != '/' &&
			*slash != '\\' );
		// get the path part (in front of the slash)
		prefix = filename;
		prefixLen = slash - prefix + 1;
	}

	if ( prefixLen > 0 ) {
		if ( (size_t)ptr->buf_size >= prefixLen + strlen(path) ) {
			memmove( (char*)ptr->buf + prefixLen, path, strlen(path)+1 );	
			memmove( (char*)ptr->buf, prefix, prefixLen );
		} else {
			fprintf(stderr, "Error - config entry size is insufficient\n" );
		}
	}
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
		mkdir(buffer, 0755);
	}

	return addFilename(buffer, file, bufsize);
}

// build a complete path to system wide data file
char *getDataFilename(const char *file, char *buffer, unsigned int bufsize)
{
	Host::getDataFolder(buffer, bufsize);
	return addFilename(buffer, file, bufsize);
}

static int process_config(FILE *f, const char *filename, struct Config_Tag *conf, char *title, bool verbose)
{
	int status = input_config(filename, conf, title);
	if (status >= 0) {
		if (verbose)
			fprintf(f, "%s configuration: found %d valid directives.\n", title, status);
		struct Config_Tag *ptr;
		for ( ptr = conf; ptr->buf; ++ptr )
			if ( ptr->type == Path_Tag )
				postload_PathTag(filename, ptr);
	} else {
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
#ifdef SDL_GUI
		startupGUI = true;
#endif
		return false;
	}

	fprintf(f, "Using config file: '%s'\n", rcfile);

	process_config(f, rcfile, global_conf, "[GLOBAL]", true);
	process_config(f, rcfile, hotkeys_conf, "[HOTKEYS]", true);
	process_config(f, rcfile, startup_conf, "[STARTUP]", true);
	process_config(f, rcfile, jit_conf, "[JIT]", true);
	process_config(f, rcfile, video_conf, "[VIDEO]", true);
	process_config(f, rcfile, tos_conf, "[TOS]", true);
	process_config(f, rcfile, ide_swap ? diskd_configs : diskc_configs, "[IDE0]", true);
	process_config(f, rcfile, ide_swap ? diskc_configs : diskd_configs, "[IDE1]", true);

	process_config(f, rcfile, disk0_configs, "[PARTITION0]", true);
	process_config(f, rcfile, disk1_configs, "[PARTITION1]", true);
	process_config(f, rcfile, disk2_configs, "[PARTITION2]", true);
	process_config(f, rcfile, disk3_configs, "[PARTITION3]", true);
	process_config(f, rcfile, disk4_configs, "[PARTITION4]", true);
	process_config(f, rcfile, disk5_configs, "[PARTITION5]", true);
	process_config(f, rcfile, disk6_configs, "[PARTITION6]", true);
	process_config(f, rcfile, disk7_configs, "[PARTITION7]", true);

	process_config(f, rcfile, arafs_conf, "[HOSTFS]", true);
	process_config(f, rcfile, opengl_conf, "[OPENGL]", true);
	process_config(f, rcfile, eth0_conf, "[ETH0]", true);
	process_config(f, rcfile, eth1_conf, "[ETH1]", true);
	process_config(f, rcfile, eth2_conf, "[ETH2]", true);
	process_config(f, rcfile, eth3_conf, "[ETH3]", true);
	process_config(f, rcfile, lilo_conf, "[LILO]", true);
	process_config(f, rcfile, midi_conf, "[MIDI]", true);
	process_config(f, rcfile, nfcdroms_conf, "[CDROMS]", true);
	process_config(f, rcfile, autozoom_conf, "[AUTOZOOM]", true);
	process_config(f, rcfile, osmesa_conf, "[NFOSMESA]", true);
	process_config(f, rcfile, parallel_conf, "[PARALLEL]", true);
	process_config(f, rcfile, natfeat_conf, "[NATFEATS]", true);
	process_config(f, rcfile, nfvdi_conf, "[NFVDI]", true);

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
	update_config(fs, hotkeys_conf, "[HOTKEYS]");
	update_config(fs, startup_conf, "[STARTUP]");
	update_config(fs, jit_conf, "[JIT]");
	update_config(fs, video_conf, "[VIDEO]");
	update_config(fs, tos_conf, "[TOS]");
	update_config(fs, ide_swap ? diskd_configs : diskc_configs, "[IDE0]");
	update_config(fs, ide_swap ? diskc_configs : diskd_configs, "[IDE1]");

	update_config(fs, disk0_configs, "[PARTITION0]");
	if (strlen((char *)disk1_configs->buf))
		update_config(fs, disk1_configs, "[PARTITION1]");
	if (strlen((char *)disk2_configs->buf))
		update_config(fs, disk2_configs, "[PARTITION2]");
	if (strlen((char *)disk3_configs->buf))
		update_config(fs, disk3_configs, "[PARTITION3]");
	if (strlen((char *)disk4_configs->buf))
		update_config(fs, disk4_configs, "[PARTITION4]");
	if (strlen((char *)disk5_configs->buf))
		update_config(fs, disk5_configs, "[PARTITION5]");
	if (strlen((char *)disk6_configs->buf))
		update_config(fs, disk6_configs, "[PARTITION6]");
	if (strlen((char *)disk7_configs->buf))
		update_config(fs, disk7_configs, "[PARTITION7]");

	update_config(fs, arafs_conf, "[HOSTFS]");
	update_config(fs, opengl_conf, "[OPENGL]");

	update_config(fs, eth0_conf, "[ETH0]");
	if (strlen((char *)eth1_conf->buf))
		update_config(fs, eth1_conf, "[ETH1]");
	if (strlen((char *)eth2_conf->buf))
		update_config(fs, eth2_conf, "[ETH2]");
	if (strlen((char *)eth3_conf->buf))
		update_config(fs, eth3_conf, "[ETH3]");

	update_config(fs, lilo_conf, "[LILO]");
	update_config(fs, midi_conf, "[MIDI]");
	update_config(fs, nfcdroms_conf, "[CDROMS]");
	update_config(fs, autozoom_conf, "[AUTOZOOM]");
	update_config(fs, osmesa_conf, "[NFOSMESA]");
	update_config(fs, parallel_conf, "[PARALLEL]");
	update_config(fs, natfeat_conf, "[NATFEATS]");
	update_config(fs, nfvdi_conf, "[NFVDI]");

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
	getDataFilename(FREEMINT_FILENAME, bx_options.bootstrap_path, sizeof(bx_options.bootstrap_path));

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

/*
vim:ts=4:sw=4:
*/
