/* MJ 2001 */
#include "sysdeps.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>	// for toupper

#include "parameters.h"

#define DEBUG 1
#include "debug.h"

#define ARANYMRC	"/.aranymrc"

static struct option const long_options[] =
{
  {"fastram", required_argument, 0, 'T'},
  {"rom", required_argument, 0, 'R'},
  {"resolution", required_argument, 0, 'r'},
  {"debug", no_argument, 0, 'D'},
  {"fullscreen", no_argument, 0, 'f'},
  {"direct_truecolor", no_argument, 0, 't'},
  {"monitor", required_argument, 0, 'm'},
  {"disk", required_argument, 0, 'd'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {NULL, 0, NULL, 0}
};

char *program_name;
char rom_path[512] = DATADIR "/ROM";

uint8 start_debug = 0;			// Start debugger
bool fullscreen = false;			// Boot in Fullscreen
int8 boot_color_depth = -1;	// Boot in color depth
int8 monitor = -1;				// VGA
extern uint32 FastRAMSize;		// FastRAM size
uint32 FastRAMSizeMB;
bool direct_truecolor = false;
ExtDrive extdrives[ 'Z' - 'A' ];// External filesystem drives

bx_options_t bx_options;

// configuration file 
struct Config_Tag global_conf[]={
	{ "TOS", String_Tag, rom_path, sizeof(rom_path)},
	{ "FastRAM", Int_Tag, &FastRAMSizeMB},
	{ "Cookie_MCH", HexLong_Tag, &bx_options.cookies._mch},
	{ "DebugOnStart", Bool_Tag, &start_debug},
	{ NULL , Error_Tag, NULL }
};

#define BX_DISK_CONFIG(a)	struct Config_Tag a ## _configs[] = {	\
	{ "Present", Bool_Tag, &bx_options. ## a ## .present},	\
	{ "IsCDROM", Bool_Tag, &bx_options. ## a ## .isCDROM},	\
	{ "ByteSwap", Bool_Tag, &bx_options. ## a ## .byteswap},	\
	{ "Path", String_Tag, bx_options. ## a ## .path, sizeof(bx_options. ## a ## .path)},	\
	{ "Cylinders", Int_Tag, &bx_options. ## a ## .cylinders},	\
	{ "Heads", Int_Tag, &bx_options. ## a ## .heads},	\
	{ "SectorsPerTrack", Int_Tag, &bx_options. ## a ## .spt},	\
	{ NULL , Error_Tag, NULL }	\
}	\

BX_DISK_CONFIG(diskc);
BX_DISK_CONFIG(diskd);

static void decode_ini_file(FILE *);

void usage (int status) {
  printf ("ARAnyM\n");
  printf ("Usage: %s [OPTION]... [FILE]...\n", program_name);
  printf ("\
Options:
  -R, --rom NAME             ROM file NAME\n\
  -F, --fastram SIZE         FastRAM size (in MB)\n\
  -D, --debug                start debugger\n\
  -f, --fullscreen           start in fullscreen\n\
  -t, --direct_truecolor     patch TOS to enable direct true color, implies -f -r 16\n\
  -r, --resolution <X>       boot in X color depth [1,2,4,8,16]\n\
  -m, --monitor <X>          attached monitor: 0 = VGA, 1 = TV\n\
  -d, --disk CHAR:ROOTPATH   filesystem assignment e.g. d:/atari/d_drive\n\
  -h, --help                 display this help and exit\n\
  -V, --version              output version information and exit\n\
");
  exit (status);
}

void set_ide(unsigned int number, char *dev_path, int cylinders, int heads, int spt, int byteswap) {
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

  switch (number) {
    case 0: bx_options.diskc.present = 1;
            bx_options.diskc.byteswap = byteswap;
            bx_options.diskc.cylinders = cylinders;
            bx_options.diskc.heads = heads;
            bx_options.diskc.spt = spt;
            strcpy(bx_options.diskc.path, dev_path);
	    break;

    case 1: bx_options.diskd.present = 1;
            bx_options.diskd.byteswap = byteswap;
            bx_options.diskd.cylinders = cylinders;
            bx_options.diskd.heads = heads;
            bx_options.diskd.spt = spt;
            strcpy(bx_options.diskd.path, dev_path);
	    break;
    }
    if (cylinders || heads || spt)
      D(bug("Geometry of IDE%d: %d/%d/%d %d", number, cylinders, heads, spt, byteswap));
}

void preset_ide() {
  set_ide(0, "", 0, 0, 0, false);
  set_ide(1, "", 0, 0, 0, false);

  bx_options.newHardDriveSupport = 1;
}

void preset_cfg() {
  preset_ide();
  bx_options.cookies._mch = 0x00003000; // Falcon030
}

/* this is more or less a hack but it makes sense to put CDROM under IDEx config option */
void update_cdrom1() {
	if (bx_options.diskd.isCDROM) {
		bx_options.cdromd.present = bx_options.diskd.present;
		bx_options.diskd.present = false;
		strcpy(bx_options.cdromd.path, bx_options.diskd.path);
		bx_options.cdromd.inserted = true;	// this is auto insert of a CD
	}
}

int decode_switches (FILE *f, int argc, char **argv) {
	preset_cfg();
	decode_ini_file(f);
	update_cdrom1();

#ifndef CONFGUI
	int c;
	while ((c = getopt_long (argc, argv,
							 "R:" /* ROM file */
							 "D"  /* debugger */
							 "F:" /* TT-RAM */
							 "f"  /* fullscreen */
							 "t"  /* direct truecolor */
							 "r:" /* resolution */
							 "m:" /* attached monitor */
							 "d:" /* filesystem assignment */
							 "h"  /* help */
							 "V"  /* version */,
							 long_options, (int *) 0)) != EOF) {
		switch (c) {
			case 'V':
				printf ("%s\n", VERSION_STRING);
				exit (0);

			case 'h':
				usage (0);
				exit(0);
	
			case 'D':
				start_debug = 1;
				break;
	
			case 'f':
				fullscreen = true;
				break;
	
			case 't':
				direct_truecolor = true;
				fullscreen = true;
				boot_color_depth = 16;
				break;

			case 'm':
				monitor = atoi(optarg);
				break;
	
			case 'R':
				if ((strlen(optarg)-1) > sizeof(rom_path))
					fprintf(stderr, "ROM path longer that %d chars.\n", sizeof(rom_path));
				strncpy(rom_path, optarg, sizeof(rom_path));
				rom_path[sizeof(rom_path)-1] = '\0';
				break;

			case 'r':
				boot_color_depth = atoi(optarg);
				break;

			case 'd':
				if ( strlen(optarg) < 4 ) {
					fprintf(stderr, "Not enough parameters for -d\n");
					break;
				}
				{
					char path[2048];

					// add the drive
					int8  driveNo = toupper(optarg[0]) - 'A';
					char* colonPos = strchr( optarg, ':' );
					if ( colonPos == NULL )
						break;

					colonPos++;
					char* colonPos2 = strchr( colonPos, ':' );

					extdrives[ driveNo ].halfSensitive = (colonPos2 == NULL); //halfSensitive if the third part is NOT set;

					// copy the path only
					if ( colonPos2 == NULL )
						colonPos2 = colonPos + strlen( colonPos );
					strncpy( path, colonPos, colonPos2 - colonPos );
					path[ colonPos2 - colonPos ] = '\0';

					extdrives[ driveNo ].rootPath = strdup( path );

					D(bug("parameters: installing drive %c:%s:%d",
							driveNo + 'A',
							extdrives[ driveNo ].rootPath,
							extdrives[ driveNo ].halfSensitive));
				}
				break;

			case 'F':
				FastRAMSizeMB = atoi(optarg);
				FastRAMSize = FastRAMSizeMB * 1024 * 1024;
				break;

			default:
				usage (EXIT_FAILURE);
		}
	}
	return optind;
#else /* CONFGUI */
	return 0;
#endif
}

static void process_config(FILE *f, const char *filename, struct Config_Tag *conf, char *title, bool verbose) {
	int status = input_config(filename, conf, title);
	if (verbose) {
		if (status >= 0)
			fprintf(f, "%s configuration: found %d valid directives.\n", title, status);
		else
			fprintf(f, "Error while reading/processing the '%s' config file.\n", filename);
	}
}

static void decode_ini_file(FILE *f) {
	char *home;
	char *rcfile;

	// compose ini file name
	if ((home = getenv("HOME")) == NULL)
		home = "";
	if ((rcfile = (char *)alloca((strlen(home) + strlen(ARANYMRC) + 1) * sizeof(char))) == NULL) {
	  fprintf(stderr, "Not enough memory\n");
	  exit(-1);
	}
	strcpy(rcfile, home);
	strcat(rcfile, ARANYMRC);
	fprintf(f, "Using config file: '%s'\n", rcfile);

	process_config(f, rcfile, global_conf, "[GLOBAL]", true);
	FastRAMSize = FastRAMSizeMB * 1024 * 1024;
	process_config(f, rcfile, diskc_configs, "[IDE0]", true);
	process_config(f, rcfile, diskd_configs, "[IDE1]", true);
}

int save_settings(const char *fs) {
	FastRAMSizeMB = FastRAMSize / 1024 / 1024;
	update_config(fs,global_conf,"[GLOBAL]");
	update_config(fs,diskc_configs,"[IDE0]");
	update_config(fs,diskd_configs,"[IDE1]");
	return 0;
}
