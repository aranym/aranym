/* MJ 2001 */

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "sysdeps.h"
#include "version.h"
#include "cfgopts.h"

#ifdef HAVE_NEW_HEADERS
# include <cassert>
# include <cstdio>
#else
# include <assert.h>
# include <stdio.h>
#endif

#ifndef ARANYMHOME
# define ARANYMHOME		".aranym"
#endif
#define ARANYMCONFIG	"config"
#define ARANYMNVRAM		"nvram"
#define ARANYMKEYMAP	"keymap"
#ifndef DIRSEPARATOR
# define DIRSEPARATOR	"/"
#endif

#define BX_MAX_ATA_CHANNEL 1

enum geo_type {
	geoCylinders,
	geoHeads,
	geoSpt,
	geoByteswap
};

extern int get_geometry(char *, geo_type geo);

// External filesystem type
typedef struct {
	bool halfSensitive;
	char rootPath[512];
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
  char partID[4];
} bx_scsidevice_options_t;
 /* 
typedef struct {
  char *path;
  unsigned long address;
  } bx_rom_options_t;
*/

// TOS options
typedef struct {
  bool redirect_PRT;
  bool redirect_CON;
  long cookie_mch;
} bx_tos_options_t;

// Video output options
typedef struct {
  bool fullscreen;		// boot in fullscreen
  int8 boot_color_depth;		// boot color depth
  uint8 refresh;
  int8 monitor;				// VGA or TV
  bool autozoom;	// Autozoom enabled
  bool autozoomint;	// Autozoom with integer coefficients
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
  bool tunealign;
  bool tunenop;
  uint32 jitcachesize;
  uint32 jitlazyflush;
  char jitblacklist[512];
} bx_jit_options_t;

// OpenGL options
typedef struct {
  bool enabled;
  uint32 width;
  uint32 height;
  uint32 bpp;
  bool filtered;
} bx_opengl_options_t;

// Ethernet options
typedef struct {
  char ip_host[16];
  char ip_atari[16];
  char netmask[16];
} bx_ethernet_options_t;

// Lilo options
typedef struct {
	char kernel[512];	/* /path/to/vmlinux[.gz] */
	char args[512];		/* command line arguments for kernel */
	char ramdisk[512];	/* /path/to/ramdisk[.gz] */
} bx_lilo_options_t;

// Midi options
typedef struct {
	bool enabled;		/* MIDI output to file enabled ? */
	char output[512];	/* /path/to/output.txt */
} bx_midi_options_t;

// NfCdrom options
typedef struct {
	int32 physdevtohostdev;
} bx_nfcdrom_options_t;

// Misc CPU emulation options
#ifdef ENABLE_EPSLIMITER
typedef struct {
	bool	eps_enabled;	/* Exception per second limiter enabled ? */
	int32	eps_max;		/* Maximum tolerated eps before shutdown */
} bx_cpu_options_t;
#endif

/*
typedef struct {
  char      *path;
  bool   cmosImage;
  unsigned int time0;
  } bx_cmos_options;
*/

// Options 
typedef struct {
  bx_floppy_options_t	floppy;
  bx_atadevice_options_t atadevice[BX_MAX_ATA_CHANNEL][2];
  bx_scsidevice_options_t	disk0;
  bx_scsidevice_options_t	disk1;
  bx_scsidevice_options_t	disk2;
  bx_scsidevice_options_t	disk3;
  bx_scsidevice_options_t	disk4;
  bx_scsidevice_options_t	disk5;
  bx_scsidevice_options_t	disk6;
  bx_scsidevice_options_t	disk7;
  bx_aranymfs_options_t	aranymfs[ 'Z'-'A'+1 ];
//  bx_cookies_options_t cookies;
  bx_video_options_t	video;
  bx_tos_options_t	tos;
  bx_startup_options_t	startup;
  bx_jit_options_t	jit;
  bx_opengl_options_t	opengl;
  bx_ethernet_options_t ethernet;
  bx_lilo_options_t		lilo;
  bx_midi_options_t		midi;
  bx_nfcdrom_options_t	nfcdroms[ 'Z'-'A'+1 ];
#ifdef ENABLE_EPSLIMITER
  bx_cpu_options_t  cpu;
#endif
  char			tos_path[512];
  char			emutos_path[512];
  uint32		fastram;
  bool			autoMouseGrab;
  // char              bootdrive[2];
  // unsigned long     vga_update_interval;
  // unsigned long     keyboard_serial_delay;
  // unsigned long     floppy_command_delay;
  // unsigned long     ips;
  // bool           private_colormap;
  // bx_cmos_options_t	cmos;
  bool			newHardDriveSupport;
} bx_options_t;

extern bx_options_t bx_options;


extern uint32 FastRAMSize;	// Size of Fast-RAM

extern char *program_name;
extern char rom_path[512];
extern char emutos_path[512];
extern bool startupGUI;
extern bool boot_emutos;
extern bool boot_lilo;

void usage(int);
extern bool check_cfg();
extern bool decode_switches(FILE *, int, char **);
extern char *getConfFilename(const char *file, char *buffer, unsigned int bufsize);
extern char *getDataFilename(const char *file, char *buffer, unsigned int bufsize);
char *addFilename(char *buffer, const char *file, unsigned int bufsize);

// following functions implemented in parameters_[unix|linux|cygwin].cpp
char *getConfFolder(char *buffer, unsigned int bufsize);
char *getDataFolder(char *buffer, unsigned int bufsize);

extern const char *getConfigFile();
extern bool loadSettings(const char *);
extern bool saveSettings(const char *);

#endif
