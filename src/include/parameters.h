/* MJ 2001 */

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "sysdeps.h"
#include "version.h"
#include "cfgopts.h"

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
} bx_aranymfs_options;

typedef struct {
  char path[512];
  // bool inserted;
  // bool enforceRemount;
  } bx_floppy_options;

typedef struct {
  bool present;
  bool isCDROM;
  bool byteswap;
  char path[512];
  unsigned int cylinders;
  unsigned int heads;
  unsigned int spt;
  } bx_disk_options;


struct bx_cdrom_options
{
  bool present;
  char path[512];
  bool inserted;
};
 /* 
typedef struct {
  char *path;
  unsigned long address;
  } bx_rom_options;
*/
typedef struct {
  bool console_redirect;
  long cookie_mch;
  } bx_tos_options;
 
typedef struct {
  bool fullscreen;		// boot in fullscreen
  int8 boot_color_depth;		// boot color depth
  uint8 refresh;
  int8 monitor;				// VGA or TV
#ifdef DIRECT_TRUECOLOR
  bool direct_truecolor;	// patch TOS to enable direct true color
#endif /* DIRECT_TRUECOLOR */
  } bx_video_options;


typedef struct {
  bool grabMouseAllowed;
  bool debugger;
  } bx_startup_options;

/*
typedef struct {
  char      *path;
  bool   cmosImage;
  unsigned int time0;
  } bx_cmos_options;
*/

typedef struct {
  bx_floppy_options    floppy;
  // bx_floppy_options floppyb;
  bx_disk_options      diskc;
  bx_disk_options      diskd;
  bx_cdrom_options     cdromd;
  bx_aranymfs_options  aranymfs[ 'Z' - 'A' ];
//  bx_cookies_options cookies;
  bx_video_options	video;
  bx_tos_options	tos;
  bx_startup_options   startup;
  bool				autoMouseGrab;
  // char              bootdrive[2];
  // unsigned long     vga_update_interval;
  // unsigned long     keyboard_serial_delay;
  // unsigned long     floppy_command_delay;
  // unsigned long     ips;
  // bool           private_colormap;
  // bx_cmos_options   cmos;
  bool           newHardDriveSupport;
  } bx_options_t;

extern bx_options_t bx_options;


extern uint32 FastRAMSize;	// Size of Fast-RAM

extern char *program_name;
extern char rom_path[512];
extern char emutos_path[512];

void usage(int);
extern int decode_switches(FILE *, int, char **);
extern int save_settings(const char *);

#endif
