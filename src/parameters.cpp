/* MJ 2001 */
#include "sysdeps.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>  // for toupper

#include "parameters.h"

#define DEBUG 1
#include "debug.h"

#define ARANYMRC	"/.aranymrc"

static struct option const long_options[] =
{
  {"ttram", required_argument, 0, 'T'},
  {"rom", required_argument, 0, 'R'},
  {"resolution", required_argument, 0, 'r'},
  {"debug", no_argument, 0, 'D'},
  {"fullscreen", no_argument, 0, 'f'},
  {"direct_truecolor", no_argument, 0, 't'},
  {"grab_mouse", no_argument, 0, 'g'},
  {"monitor", required_argument, 0, 'm'},
  {"disk", required_argument, 0, 'd'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {NULL, 0, NULL, 0}
};


char *program_name;
char *rom_path;

uint8 start_debug = 0;			// Start debugger
bool fullscreen = false;			// Boot in Fullscreen
int8 boot_color_depth = -1;	// Boot in color depth
int8 monitor = -1;				// VGA
extern uint32 TTRAMSize;		// TTRAM size
bool direct_truecolor = false;
bool grab_mouse = false;
ExtDrive extdrives[ 'Z' - 'A' ];// External filesystem drives

bx_options_t bx_options;

static void decode_ini_file(void);

void usage (int status) {
  printf ("ARAnyM\n");
  printf ("Usage: %s [OPTION]... [FILE]...\n", program_name);
  printf ("\
Options:
  -R, --rom NAME             ROM file NAME\n\
  -T, --ttram SIZE           TT-RAM size\n\
  -D, --debug                start debugger\n\
  -f, --fullscreen           start in fullscreen\n\
  -t, --direct_truecolor     patch TOS to enable direct true color, implies -f -r 16\n\
  -g, --grab_mouse           ARAnyM grabs mouse and keyboard control in X-Windows\n\
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
  if (cylinders == -1) ;
  if (heads == -1) ;
  if (spt == -1) ;
  if (byteswap == -1) ;

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
}

void preset_ide() {
  set_ide(0, "", 0, 0, 0, false);
  set_ide(1, "", 0, 0, 0, false);

  bx_options.newHardDriveSupport = 1;
  bx_options.cdromd.present = 0;
}

void preset_cfg() {
  preset_ide();
}

int decode_switches (int argc, char **argv) {
	int c;

        preset_cfg();
	decode_ini_file();
  
	while ((c = getopt_long (argc, argv,
							 "R:" /* ROM file */
							 "D"  /* debugger */
							 "T:" /* TT-RAM */
							 "f"  /* fullscreen */
							 "t"  /* direct truecolor */
							 "g"  /* grab mouse */
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

			case 'g':
				grab_mouse = true;
				break;

			case 'm':
				monitor = atoi(optarg);
				break;
	
			case 'R':
				rom_path = strdup(optarg);
				break;

			case 'r':
				boot_color_depth = atoi(optarg);
				break;

			case 'd':
				if ( strlen(optarg) < 4 )
					break;
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

					fprintf(stderr, "parameters: installing drive %c:%s:%d\n",
							driveNo + 'A',
							extdrives[ driveNo ].rootPath,
							extdrives[ driveNo ].halfSensitive);
				}
				break;

			case 'T':
				TTRAMSize = atoi(optarg);
				break;

			default:
				usage (EXIT_FAILURE);
		}
	}
	return optind;
}

static void decode_ini_file(void) {
  FILE *f;
  char c;
  char *s;
  char *rcfile;
  char *home;
  char *dsk;
  unsigned int number = 0;
  char *dev_path;
  int cylinders = -1, heads = -1, spt = -1, byteswap = 0;
  if ((home = getenv("HOME")) != NULL) {
    if ((rcfile = (char *)malloc((strlen(home) + strlen(ARANYMRC) + 1) * sizeof(char))) == NULL) {
      fprintf(stderr, "Not enough memory\n");
      exit(-1);
    }
    strcpy(rcfile, home);
    strcat(rcfile, ARANYMRC);
    if ((f = fopen(rcfile, "r")) != 0) {
      fprintf(stderr, ARANYMRC" found\n");
      for (;;) {
        switch(getc(f)) {
	  case ';':
            while ((c = getc(f)) != '\n' && (c != EOF))
              ;
          case '\n':
	    break;

          case 'd':
            start_debug = 1;
            while ((c = getc(f)) != '\n' && (c != EOF))
              ;
            break;

          case 'R':
            (void) getc(f);
            if ((rom_path = (char *)malloc(sizeof(char))) == NULL) {
	      fprintf(stderr, "Not enough memory\n");
	      exit(-1);
	    }
	    strcpy(rom_path, "");

            while ((c = getc(f)) != '\n' && (c != EOF)) {
              s = rom_path;
	      if ((rom_path = (char *)malloc((strlen(s)+2) * sizeof(char))) == NULL) {
	        fprintf(stderr, "Not enough memory\n");
		exit(-1);
	      }
	      strcpy(rom_path, s);
	      rom_path[strlen(s)] = c;
	      rom_path[strlen(s)+1] = '\0';
	      free((void *)s);
	    }
	    D(bug("ROM file: %s",rom_path));
            break;

          case 'i':
            if ((dsk = (char *)malloc(sizeof(char))) == NULL) {
	      fprintf(stderr, "Not enough memory\n");
	      exit(-1);
	    }
	    strcpy(dsk, "");

            if ((dev_path = (char *)malloc(1024 * sizeof(char))) == NULL) {
	      fprintf(stderr, "Not enough memory\n");
	      exit(-1);
	    }
	    strcpy(dev_path, "");


            while ((c = getc(f)) != '\n' && (c != EOF)) {
              s = dsk;
	      if ((dsk = (char *)malloc((strlen(s)+3) * sizeof(char))) == NULL) {
	        fprintf(stderr, "Not enough memory\n");
		exit(-1);
	      }
	      strcpy(dsk, s);
	      dsk[strlen(s)] = c;
	      dsk[strlen(s)+1] = '\0';
	      free((void *)s);
	    }
            dsk[strlen(dsk)+1] = '\0';
            dsk[strlen(dsk)] = '\n';

            if (sscanf(dsk, "%u %1023s %d %d %d %d", &number, dev_path, &cylinders, &heads, &spt, &byteswap) > 1) {
	      if (number > 1) {
                fprintf(stderr, "Unknown IDE number: %d in " ARANYMRC "\n", number);
	        exit(-1);
              }
	      set_ide(number, dev_path, cylinders, heads, spt, byteswap == 0 ? false : true);
              D(bug("IDE %d: %s",number, dev_path));
            } else {
              fprintf(stderr, "Unknown in " ARANYMRC ": ");
              fprintf(stderr, "i%s", dsk);
	      fprintf(stderr, "\n");
              exit(-1);
            }
	    
	    free((void *)dsk);
	    free((void *)dev_path);
            break;

          case EOF:
	    fclose(f);
	    return;

          default:
            fprintf(stderr, "Unknown in " ARANYMRC ": ");
            while ((c = getc(f)) != '\n' && (c != EOF))
              fprintf(stderr, "%c", c);
	    fprintf(stderr, "\n");
            exit(-1);
        }
      }
    }
  }
}
