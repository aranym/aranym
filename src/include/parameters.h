/* MJ 2001 */

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "sysdeps.h"
#include "version.h"
#include "cfgopts.h"

extern void init_ide();

// External filesystem type
typedef struct {
	bool halfSensitive;
	char *rootPath;
} ExtDrive;

typedef struct {
  char path[512];
  unsigned type;
  unsigned initial_status;
  } bx_floppy_options;

typedef struct {
  bool present;
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
 
typedef struct {
  char *path;
  unsigned long address;
  } bx_rom_options;
 
typedef struct {
  char *path;
  } bx_vgarom_options;
 
typedef struct {
  size_t megs;
  } bx_mem_options;
 
typedef struct {
  char      *path;
  bool   cmosImage;
  unsigned int time0;
  } bx_cmos_options;
 
typedef struct {
  bx_floppy_options floppya;
  bx_floppy_options floppyb;
  bx_disk_options   diskc;
  bx_disk_options   diskd;
  bx_cdrom_options  cdromd;
  char              bootdrive[2];
  unsigned long     vga_update_interval;
  unsigned long     keyboard_serial_delay;
  unsigned long     floppy_command_delay;
  unsigned long     ips;
  bool           mouse_enabled;
  bool           private_colormap;
  bx_cmos_options   cmos;
  bool           newHardDriveSupport;
  } bx_options_t;

extern bx_options_t bx_options;

#define BX_DISK_CONFIG(a)	struct Config_Tag a ## _configs[] = {	\
	{ "Present", Boolean_Tag, &bx_options. ## a ## .present},	\
	{ "Path", String_Tag, &bx_options. ## a ## .path, sizeof(bx_options. ## a ## .path)},	\
	{ "Cylinders", Long_Tag, &bx_options. ## a ## .cylinders},	\
	{ "Heads", Long_Tag, &bx_options. ## a ## .heads},	\
	{ "SectorsPerTrack", Long_Tag, &bx_options. ## a ## .spt},	\
	{ "ByteSwap", Boolean_Tag, &bx_options. ## a ## .byteswap},	\
	{ NULL , Error_Tag, NULL }	\
}	\

extern uint8 start_debug;		// starts debugger
extern bool fullscreen;		// boot in fullscreen
extern int8 boot_color_depth;		// boot color depth
extern int8 monitor;				// VGA or TV
extern bool direct_truecolor;	// patch TOS to enable direct true color
extern bool grab_mouse;
extern ExtDrive extdrives[ 'Z' - 'A' ];// External filesystem drives

extern char *program_name;
extern char rom_path[512];

void usage(int status);
int decode_switches(int argc, char **argv);

#endif
