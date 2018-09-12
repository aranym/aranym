/*
 * parameters.h - parameter init/load/save code - header file
 *
 * Copyright (c) 2001-2008 ARAnyM developer team (see AUTHORS)
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

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "sysdeps.h"
#include "SDL_compat.h"

#ifndef ARANYMHOME
# define ARANYMHOME	".aranym"
#endif
#ifndef ARANYMCONFIG
# define ARANYMCONFIG	"config"
#endif
#ifndef ARANYMNVRAM
# define ARANYMNVRAM	"nvram"
#endif
#ifndef ARANYMKEYMAP
# define ARANYMKEYMAP	"keymap"
#endif
#ifndef DIRSEPARATOR
# define DIRSEPARATOR	"/"
#endif

#define BX_MAX_ATA_CHANNEL 1
#define MAX_ETH		4

enum geo_type {
	geoCylinders,
	geoHeads,
	geoSpt,
	geoByteswap
};

extern int get_geometry(const char *, enum geo_type geo);

// External filesystem type
#define HOSTFS_MAX_DRIVES 32

typedef struct {
	bool halfSensitive;
	char rootPath[512];
	char configPath[520];
} bx_aranymfs_drive_t;
typedef struct {
	char symlinks[20];
	bx_aranymfs_drive_t drive[HOSTFS_MAX_DRIVES];
} bx_aranymfs_options_t;

// Floppy device
typedef struct {
  char path[512];
} bx_floppy_options_t;

typedef enum {
      IDE_NONE, IDE_DISK, IDE_CDROM
} device_type_t;

// Generic ATA device
typedef struct {
  bool present;		// is converted to device_type_t below
  bool isCDROM;		// is converted to device_type_t below
  device_type_t type;
  char path[512];
  unsigned int cylinders;
  unsigned int heads;
  unsigned int spt;
  bool byteswap;
  bool readonly;
  bool status;		// inserted = true
  char model[41];
} bx_atadevice_options_t;

// SCSI device
typedef struct {
  bool present;
  bool byteswap;
  bool readonly;
  char path[512];
  char partID[3+1];	// 3 chars + EOS ( = '\0')
} bx_scsidevice_options_t;

// TOS options
typedef struct {
  bool redirect_CON;
  long cookie_mch;
  long cookie_akp;
  char			tos_path[512];
  char			emutos_path[512];
} bx_tos_options_t;

// Video output options
typedef struct {
  bool fullscreen;		// boot in fullscreen
  int8 boot_color_depth;		// boot color depth
  uint8 refresh;
  int8 monitor;				// VGA or TV
  char window_pos[10];	// window position coordinates in the form "123,456"
  bool single_blit_composing; // whether the screen is composed using a single BitBlit/rectangluar copy
  bool single_blit_refresh;   // whether the composed screen is updated/sent to screen using a single BitBlit/rectangluar copy
} bx_video_options_t;

// Startup options
typedef struct {
  bool grabMouseAllowed;
  bool debugger;
} bx_startup_options_t;

// JIT compiler options
typedef struct {
  bool jit;
  bool jitfpu;
  bool jitinline;
  bool jitdebug;
  uint32 jitcachesize;
  uint32 jitlazyflush;
  char jitblacklist[512];
} bx_jit_options_t;

// OpenGL options
typedef struct {
  bool enabled;
  bool filtered;
  char library[1024];
} bx_opengl_options_t;

// Ethernet options
typedef struct {
  char type[32];
  char tunnel[16];
  char ip_host[16];
  char ip_atari[16];
  char netmask[16];
  char mac_addr[18];
} bx_ethernet_options_t;

// Lilo options
typedef struct {
	char kernel[512];	/* /path/to/vmlinux[.gz] */
	char args[512];		/* command line arguments for kernel */
	char ramdisk[512];	/* /path/to/ramdisk[.gz] */
	bool load_to_fastram;	/* load kernel to Fast RAM */
} bx_lilo_options_t;

// Midi options
typedef struct {
	char type[32];		/* MIDI output type */
	char file[512];	/* /path/to/output */
	char sequencer[256];	/* /dev/sequencer */
	bool enabled;
} bx_midi_options_t;

// NfCdrom options
typedef struct {
	int32 physdevtohostdev;
} bx_nfcdrom_options_t;

// Misc CPU emulation options
typedef struct {
	bool	eps_enabled;	/* Exception per second limiter enabled ? */
	int32	eps_max;		/* Maximum tolerated eps before shutdown */
} bx_cpu_options_t;

// Autozoom options
typedef struct {
  bool enabled;		// Autozoom enabled
  bool integercoefs;	// Autozoom with integer coefficients
  bool fixedsize;	// Keep host screen size constant ?
  uint32 width;		// Wanted host screen size
  uint32 height;
} bx_autozoom_options_t;

// NfOSMesa options
typedef struct {
	uint32 channel_size;    /* If using libOSMesa[16|32], size of channel */
	char libgl[1024];		/* Pathname to libGL */
	char libosmesa[1024];	/* Pathname to libOSMesa */
} bx_nfosmesa_options_t;

// NatFeats options
typedef struct {
	char cdrom_driver[256];	/* CD-ROM driver */
	char vdi_driver[256];	/* VDI driver */
	bool hostexec_enabled;
} bx_natfeat_options_t;

// NFvdi options
typedef struct {
	bool use_host_mouse_cursor; /* Use host mouse cursor */
} bx_nfvdi_options_t;

// Parallel port options
typedef struct {
	char type[256];
	char file[256];
	char parport[256];
	bool enabled;
} bx_parallel_options_t;

// Serial port options
typedef struct {
	char serport[256];
	bool enabled;
} bx_serial_options_t;

// Keyboard and mouse
typedef struct {
  bool wheel_eiffel;	// eiffel compatible scancodes for mouse wheel
  bool altgr;			// real AltGr like on Milan
} bx_ikbd_options_t;

// Hotkeys
#define HOTKEYS_STRING_SIZE		60
typedef struct {
	SDL_Keycode sym;
	SDL_Keymod mod;
} bx_hotkey;

typedef struct {
	bx_hotkey	setup;
	bx_hotkey	quit;
	bx_hotkey	warmreboot;
	bx_hotkey	coldreboot;
	bx_hotkey	debug;
	bx_hotkey	ungrab;
	bx_hotkey	fullscreen;
	bx_hotkey	screenshot;
	bx_hotkey	sound;
} bx_hotkeys_t;

// Audio
typedef struct {
	uint32 freq;
	uint32 chans;
	uint32 bits;
	uint32 samples;
	bool enabled;
} bx_audio_options_t;

// Joysticks
typedef struct {
	int32	ikbd0;
	int32	ikbd1;
	int32	joypada;
	char	joypada_mapping[256];
	int32	joypadb;
	char	joypadb_mapping[256];
} bx_joysticks_options_t;

#define DISKS	8

#define CD_MAX_DRIVES 32

// Options 
typedef struct {
  bx_floppy_options_t	floppy;
  bx_atadevice_options_t atadevice[BX_MAX_ATA_CHANNEL][2];
  bx_scsidevice_options_t	disks[DISKS];
  bx_aranymfs_options_t	aranymfs;
  bx_video_options_t	video;
  bx_tos_options_t	tos;
  bx_startup_options_t	startup;
  bx_jit_options_t	jit;
  bx_opengl_options_t	opengl;
  bx_ethernet_options_t ethernet[MAX_ETH];
  bx_lilo_options_t		lilo;
  bx_midi_options_t		midi;
  bx_ikbd_options_t		ikbd;
  bx_nfcdrom_options_t	nfcdroms[ CD_MAX_DRIVES ];
  bx_cpu_options_t  cpu;
  bx_autozoom_options_t	autozoom;
  bx_nfosmesa_options_t	osmesa;
  bx_parallel_options_t parallel;
  bx_serial_options_t serial;
  bx_natfeat_options_t natfeats;
  bx_nfvdi_options_t	nfvdi;
  bx_audio_options_t	audio;
  bx_joysticks_options_t	joysticks;
  char			bootstrap_path[512];
  char			bootstrap_args[512];
  char			bootdrive;
  uint32		fastram;
  unsigned long fixed_memory_offset; // FIXME: should be uintptr
  bool			gmtime;
  bx_hotkeys_t		hotkeys;
  bool			newHardDriveSupport;
  char			snapshot_dir[512];
} bx_options_t;

extern bx_options_t bx_options;


extern uint32 FastRAMSize;	// Size of Fast-RAM
#if FIXED_ADDRESSING
extern uintptr fixed_memory_offset;	// Virtual address of atari memory
#endif

extern char *program_name;
extern bool boot_emutos;
extern bool boot_lilo;
extern bool halt_on_reboot;

extern bool decode_switches(int, char **);
extern char *getConfFilename(const char *file, char *buffer, unsigned int bufsize);
extern char *getDataFilename(const char *file, char *buffer, unsigned int bufsize);
char *addFilename(char *buffer, const char *file, unsigned int bufsize);
bool setConfigValue(const char *section_name, const char *key, const char *value);
void listConfigValues(bool type);

char *keysymToString(char *buffer, const bx_hotkey *keysym);
bool stringToKeysym(bx_hotkey *keysym, const char *string);
char **split_pathlist(const char *pathlist);

extern const char *getConfigFile();
void setConfigFile(const char *filename);
extern bool loadSettings(const char *);
extern bool saveSettings(const char *);

extern const struct Config_Section *getConfigSections(void);

#endif
