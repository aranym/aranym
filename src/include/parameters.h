/* MJ 2001 */

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "sysdeps.h"
#include "version.h"

// External filesystem type
typedef struct {
	bool halfSensitive;
	char *rootPath;
} ExtDrive;

extern uint8 start_debug;		// starts debugger
extern bool fullscreen;		// boot in fullscreen
extern int8 boot_color_depth;		// boot color depth
extern int8 monitor;				// VGA or TV
extern bool direct_truecolor;	// patch TOS to enable direct true color
extern bool grab_mouse;
extern ExtDrive extdrives[ 'Z' - 'A' ];// External filesystem drives

extern char *program_name;
extern char *rom_path;

void usage(int status);
int decode_switches(int argc, char **argv);

#endif
