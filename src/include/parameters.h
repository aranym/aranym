/* MJ 2001 */

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "sysdeps.h"
#include "version.h"

extern uint8 start_debug;		// starts debugger
extern uint8 fullscreen;		// boot in fullscreen
extern uint16 boot_color_depth;		// boot color depth
extern bool direct_truecolor;	// patch TOS to enable direct true color
extern uint8 monitor;				// VGA or TV

extern char *program_name;
extern char *rom_path;

void usage(int status);
int decode_switches(int argc, char **argv);

#endif
