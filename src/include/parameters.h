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
  // bool inserted;
  // bool enforceRemount;
} bx_floppy_options_t;

// IDE device
typedef struct {
  bool present;
  bool isCDROM;
  bool byteswap;
  bool readonly;
  char path[512];
  unsigned int cylinders;
  unsigned int heads;
  unsigned int spt;
} bx_disk_options_t;

// CDROM device
typedef struct {
  bool present;
  char path[512];
  bool inserted;
} bx_cdrom_options_t;

typedef enum {
      IDE_NONE, IDE_DISK, IDE_CDROM
} device_type_t;

// Generic ATA device
typedef struct {
  bool present;
  device_type_t type;
  char path[512];
  unsigned int cylinders;
  unsigned int heads;
  unsigned int spt;
  bool byteswap;
  bool readonly;
  bool status;
  char model[41];
} bx_atadevice_options_t;

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
#ifdef DIRECT_TRUECOLOR
  bool direct_truecolor;	// patch TOS to enable direct true color
#endif /* DIRECT_TRUECOLOR */
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
} bx_jit_options_t;

// OpenGL options
typedef struct {
  bool enabled;
  uint32 width;
  uint32 height;
  uint32 bpp;
} bx_opengl_options_t;

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
  // bx_floppy_options_t floppyb;
  bx_disk_options_t	diskc;
  bx_disk_options_t	diskd;
  bx_cdrom_options_t	cdromd;
  bx_atadevice_options_t atadevice[BX_MAX_ATA_CHANNEL][2];
  bx_aranymfs_options_t	aranymfs[ 'Z'-'A'+1 ];
//  bx_cookies_options_t cookies;
  bx_video_options_t	video;
  bx_tos_options_t	tos;
  bx_startup_options_t	startup;
  bx_jit_options_t	jit;
  bx_opengl_options_t	opengl;
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
extern bool boot_emutos;

void usage(int);
extern bool check_cfg();
extern bool decode_switches(FILE *, int, char **);
extern int saveSettings(const char *);
extern char *getConfFilename(const char *file, char *buffer, unsigned int bufsize);

#endif
