/* MJ 2001 */
#include "sysdeps.h"
#include "config.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

static struct option const long_options[] =
{
#ifndef FixedSizeFastRAM
  {"fastram", required_argument, 0, 'F'},
#endif
  {"floppy", required_argument, 0, 'a'},
  {"resolution", required_argument, 0, 'r'},
  {"debug", no_argument, 0, 'D'},
  {"fullscreen", no_argument, 0, 'f'},
  {"nomouse", no_argument, 0, 'N'},
  {"refresh", required_argument, 0, 'v'},
#ifdef DIRECT_TRUECOLOR
  {"direct_truecolor", no_argument, 0, 't'},
#endif
  {"monitor", required_argument, 0, 'm'},
  {"disk", required_argument, 0, 'd'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"config", required_argument, 0, 'c'},
  {"save", no_argument, 0, 's'},
  {NULL, 0, NULL, 0}
};

char *program_name;
char rom_path[512] = DATADIR "/ROM";
char emutos_path[512] = "";
uint32 FastRAMSize;

static char config_folder[512] = ARANYMHOME;
static char config_file[512] = "";	// empty by default - can be set by --config <fname>

static uint32 FastRAMSizeMB;
static bool saveConfigFile = false;

bx_options_t bx_options;

// configuration file 
/*************************************************************************/
struct Config_Tag global_conf[]={
	{ "FastRAM", Int_Tag, &FastRAMSizeMB},
	{ "Floppy", String_Tag, bx_options.floppy.path, sizeof(bx_options.floppy.path)},
	{ "TOS", String_Tag, rom_path, sizeof(rom_path)},
	{ "EmuTOS", String_Tag, emutos_path, sizeof(emutos_path)},
	{ "AutoGrabMouse", Bool_Tag, &bx_options.autoMouseGrab},
	{ NULL , Error_Tag, NULL }
};

void preset_global() {
  bx_options.autoMouseGrab = true;
#ifdef FixedSizeFastRAM
  FastRAMSize = FixedSizeFastRAM * 1024 * 1024;
#else
  FastRAMSize = 0;
#endif
}

void postload_global() {
#ifndef FixedSizeFastRAM
	FastRAMSize = FastRAMSizeMB * 1024 * 1024;
#endif
}

void presave_global() {
	FastRAMSizeMB = FastRAMSize / 1024 / 1024;
}

/*************************************************************************/
struct Config_Tag startup_conf[]={
	{ "GrabMouse", Bool_Tag, &bx_options.startup.grabMouseAllowed},
#ifdef DEBUGGER
	{ "Debugger", Bool_Tag, &bx_options.startup.debugger},
#endif
	{ NULL , Error_Tag, NULL }
};

void preset_startup() {
  bx_options.startup.debugger = false;
  bx_options.startup.grabMouseAllowed = true;
}

void postload_startup() {
}

void presave_startup() {
}

/*************************************************************************/
struct Config_Tag jit_conf[]={
	{ "JIT", Bool_Tag, &bx_options.jit.jit},
	{ "JITFPU", Bool_Tag, &bx_options.jit.jitfpu},
	{ "JITCacheSize", Int_Tag, &bx_options.jit.jitcachesize},
	{ "JITLazyFlush", Int_Tag, &bx_options.jit.jitlazyflush},
	{ NULL , Error_Tag, NULL }
};

void preset_jit() {
	bx_options.jit.jit = false;
	bx_options.jit.jitfpu = true;
	bx_options.jit.jitcachesize = 8192;
	bx_options.jit.jitlazyflush = 1;
}

void postload_jit() {
}

void presave_jit() {
}

/*************************************************************************/
struct Config_Tag tos_conf[]={
	{ "Cookie_MCH", HexLong_Tag, &bx_options.tos.cookie_mch},
	{ "Console", Bool_Tag, &bx_options.tos.console_redirect},
	{ NULL , Error_Tag, NULL }
};

void preset_tos() {
  bx_options.tos.console_redirect = false;
  bx_options.tos.cookie_mch = 0x00030000; // Falcon030
}

void postload_tos() {
}

void presave_tos() {
}

/*************************************************************************/
struct Config_Tag video_conf[]={
	{ "FullScreen", Bool_Tag, &bx_options.video.fullscreen},
	{ "BootColorDepth", Byte_Tag, &bx_options.video.boot_color_depth},
	{ "VidelRefresh", Byte_Tag, &bx_options.video.refresh},
	{ "VidelMonitor", Byte_Tag, &bx_options.video.monitor},
#ifdef DIRECT_TRUECOLOR
	{ "DirectTruecolor", Bool_Tag, &bx_options.video.direct_truecolor},
#endif
	{ NULL , Error_Tag, NULL }
};

void preset_video() {
  bx_options.video.fullscreen = false;		// Boot in Fullscreen
  bx_options.video.boot_color_depth = -1;	// Boot in color depth
  bx_options.video.monitor = -1;			// preserve default NVRAM monitor
  bx_options.video.refresh = 2;			// 25 Hz update
#ifdef DIRECT_TRUECOLOR
  bx_options.video.direct_truecolor = false;
#endif
}

void postload_video() {
	if (bx_options.video.refresh < 1 || bx_options.video.refresh > 200)
		bx_options.video.refresh = 2;	// default if input parameter is insane
#ifdef DIRECT_TRUECOLOR
	if (bx_options.video.direct_truecolor) {
		bx_options.video.fullscreen = true;
		bx_options.video.boot_color_depth = 16;
	}
#endif
}

void presave_video() {
}

/*************************************************************************/
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

  bx_disk_options_t *disk;
  switch (number) {
    case 0: disk = &bx_options.diskc; break;
    case 1: disk = &bx_options.diskd; break;
    default: disk = NULL;
  }

  if (disk != NULL) {
    disk->present = strlen(dev_path) > 0;
    disk->isCDROM = false;
    disk->byteswap = byteswap;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->spt = spt;
    strcpy(disk->path, dev_path);
  }

  if (cylinders && heads && spt)
    D(bug("IDE%d CHS geometry: %d/%d/%d %d", number, cylinders, heads, spt, byteswap));
}

void preset_ide() {
  set_ide(0, "", 0, 0, 0, false);
  set_ide(1, "", 0, 0, 0, false);

  bx_options.newHardDriveSupport = true;
}

void postload_ide() {
/* this is more or less a hack but it makes sense to put CDROM under IDEx config option */
	if (bx_options.diskd.isCDROM) {
		bx_options.cdromd.present = bx_options.diskd.present;
		bx_options.diskd.present = false;
		strcpy(bx_options.cdromd.path, bx_options.diskd.path);
		bx_options.cdromd.inserted = true;	// this is auto insert of a CD
	}
}

void presave_ide() {
}

/*************************************************************************/
struct Config_Tag arafs_conf[]={
	{ "A", String_Tag, &bx_options.aranymfs[0].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "B", String_Tag, &bx_options.aranymfs[1].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "C", String_Tag, &bx_options.aranymfs[2].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "D", String_Tag, &bx_options.aranymfs[3].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "E", String_Tag, &bx_options.aranymfs[4].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "F", String_Tag, &bx_options.aranymfs[5].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "G", String_Tag, &bx_options.aranymfs[6].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "H", String_Tag, &bx_options.aranymfs[7].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "I", String_Tag, &bx_options.aranymfs[8].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "J", String_Tag, &bx_options.aranymfs[9].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "K", String_Tag, &bx_options.aranymfs[10].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "L", String_Tag, &bx_options.aranymfs[11].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "M", String_Tag, &bx_options.aranymfs[12].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "N", String_Tag, &bx_options.aranymfs[13].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "O", String_Tag, &bx_options.aranymfs[14].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "P", String_Tag, &bx_options.aranymfs[15].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "Q", String_Tag, &bx_options.aranymfs[16].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "R", String_Tag, &bx_options.aranymfs[17].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "S", String_Tag, &bx_options.aranymfs[18].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "T", String_Tag, &bx_options.aranymfs[19].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "U", String_Tag, &bx_options.aranymfs[20].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "V", String_Tag, &bx_options.aranymfs[21].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "W", String_Tag, &bx_options.aranymfs[22].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "X", String_Tag, &bx_options.aranymfs[23].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "Y", String_Tag, &bx_options.aranymfs[24].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ "Z", String_Tag, &bx_options.aranymfs[25].rootPath, sizeof(bx_options.aranymfs[0].rootPath)},
	{ NULL , Error_Tag, NULL }
};

void preset_arafs() {
	for(int i=0; i < 'Z'-'A'+1; i++) {
		bx_options.aranymfs[i].rootPath[0] = '\0';
		bx_options.aranymfs[i].halfSensitive = true;
	}
}

void postload_arafs() {
	for(int i=0; i < 'Z'-'A'+1; i++) {
		int len = strlen(bx_options.aranymfs[i].rootPath);
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

void presave_arafs() {
	for(int i=0; i < 'Z'-'A'+1; i++) {
		if ( strlen(bx_options.aranymfs[i].rootPath) > 0 &&
			 bx_options.aranymfs[i].halfSensitive ) {
			// set the halfSensitive indicator
			// NOTE: I can't add more chars here due to the fixed char[] width
			strcat( bx_options.aranymfs[i].rootPath, ":" );
		}
	}
}

/*************************************************************************/
void usage (int status) {
  printf ("%s\n", VERSION_STRING);
  printf ("Usage: %s [OPTION]... [FILE]...\n", program_name);
  printf ("\
Options:\n\
  -a, --floppy NAME          floppy image file NAME\n\
  -N, --nomouse              don't grab mouse at startup\n\
  -f, --fullscreen           start in fullscreen\n\
  -v, --refresh <X>          VIDEL refresh rate in VBL (default 2)\n\
  -r, --resolution <X>       boot in X color depth [1,2,4,8,16]\n\
  -m, --monitor <X>          attached monitor: 0 = VGA, 1 = TV\n\
  -d, --disk CHAR:ROOTPATH   METADOS filesystem assignment e.g. d:/atari/d_drive\n\
  -c, --config FILE          read different configuration file\n\
  -s, --save                 save configuration file\n\
  -h, --help                 display this help and exit\n\
  -V, --version              output version information and exit\n\
");
#ifndef FixedSizeFastRAM
  printf("  -F, --fastram SIZE         FastRAM size (in MB)\n");
#endif
#ifdef DIRECT_TRUECOLOR
  printf("  -t, --direct_truecolor     patch TOS to enable direct true color, implies -f -r 16\n");
#endif
#ifdef DEBUGGER
  printf("  -D, --debug                start debugger\n");
#endif
  exit (status);
}

void preset_cfg() {
  preset_global();
  preset_ide();
  preset_arafs();
  preset_video();
  preset_tos();
  preset_startup();
  preset_jit();
}

void postload_cfg() {
  postload_global();
  postload_ide();
  postload_arafs();
  postload_video();
  postload_tos();
  postload_startup();
  postload_jit();
}

void presave_cfg() {
  presave_global();
  presave_ide();
  presave_arafs();
  presave_video();
  presave_tos();
  presave_startup();
  presave_jit();
}

void check_for_help_version_configfile(int argc, char **argv) {
	for (int c = 0; c < argc; c++) {
		if ((strcmp(argv[c], "-c") == 0) || (strcmp(argv[c], "--config") == 0)) {
			if ((c + 1) < argc) {
				strncpy(config_file, argv[c + 1], sizeof(config_file)-1);
				config_file[sizeof(config_file)-1] = '\0';
			} else {
				fprintf(stderr, "config switch requires one parameter\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[c], "-h") == 0) || (strcmp(argv[c], "--help") == 0)) {
			usage(0);
			exit(0);
		} else if ((strcmp(argv[c], "-V") == 0) || (strcmp(argv[c], "--version") == 0)) {
			printf ("%s\n", VERSION_STRING);
			exit (0);
		}
	}
}

int process_cmdline(int argc, char **argv)
{
	int c;
	while ((c = getopt_long (argc, argv,
							 "a:" /* floppy image file */
#ifdef DEBUGGER
							 "D"  /* debugger */
#endif
#ifndef FixedSizeFastRAM
							 "F:" /* FastRAM */
#endif
							 "N"  /* no mouse */
							 "f"  /* fullscreen */
							 "v:" /* VIDEL refresh */
							 "t"  /* direct truecolor */
							 "r:" /* resolution */
							 "m:" /* attached monitor */
							 "d:" /* filesystem assignment */
							 "c:" /* small hack*/
							 "s"  /* save config file */
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
	
#ifdef DEBUGGER
			case 'D':
				bx_options.startup.debugger = true;
				break;
#endif

			case 'c':
				break;
	
			case 'f':
				bx_options.video.fullscreen = true;
				break;

			case 'v':
				bx_options.video.refresh = atoi(optarg);
				break;
	
			case 'N':
				bx_options.startup.grabMouseAllowed = false;
				break;
	
#ifdef DIRECT_TRUECOLOR
			case 't':
				bx_options.video.direct_truecolor = true;
				break;
#endif

			case 'm':
				bx_options.video.monitor = atoi(optarg);
				break;
	
			case 'a':
				if ((strlen(optarg)-1) > sizeof(bx_options.floppy.path))
					fprintf(stderr, "Floppy image filename longer that %d chars.\n", sizeof(bx_options.floppy.path));
				strncpy(bx_options.floppy.path, optarg, sizeof(bx_options.floppy.path));
				bx_options.floppy.path[sizeof(bx_options.floppy.path)-1] = '\0';
				break;

			case 'r':
				bx_options.video.boot_color_depth = atoi(optarg);
				break;

			case 'd':
				if ( strlen(optarg) < 4 ) {
					fprintf(stderr, "Not enough parameters for -d\n");
					break;
				}
				{
					// set the drive
					int8  driveNo = toupper(optarg[0]) - 'A';
					char* colonPos = strchr( optarg, ':' );
					if ( colonPos == NULL )
						break;
					colonPos++;
					strncpy( bx_options.aranymfs[ driveNo ].rootPath, colonPos, sizeof(bx_options.aranymfs[ driveNo ].rootPath) - 1 );
				}
				break;

			case 's':
				saveConfigFile = true;
				break;

#ifndef FixedSizeFastRAM
			case 'F':
				FastRAMSizeMB = atoi(optarg);
				break;
#endif

			default:
				usage (EXIT_FAILURE);
		}
	}
	return optind;
}

char *getConfFilename(const char *file, char *buffer, unsigned int bufsize)
{
	unsigned int len = strlen(config_folder)+1 + strlen(file)+1;
	if (len < bufsize) {
		strcpy(buffer, config_folder);
		strcat(buffer, DIRSEPARATOR);
		strcat(buffer, file);
	}
	else
		strcpy(buffer, file);	// at least the filename

	return buffer;
}

void build_cfgfilename()
{
	char *home = getenv("HOME");
	if (home != NULL) {
		int homelen = strlen(home);
		if (homelen > 0) {
			unsigned int len = strlen(ARANYMHOME);
			if ((homelen+1 + len+1) < sizeof(config_folder)) {
				strcpy(config_folder, home);
				strcat(config_folder, DIRSEPARATOR);
				strcat(config_folder, ARANYMHOME);
			}
		}
	}

	// Does the folder exist?
	struct stat buf;
	if (stat(config_folder, &buf) == -1) {
		D(bug("Creating config folder '%s'", config_folder));
		mkdir(config_folder, 0755);
	}

	if (strlen(config_file) == 0)
		getConfFilename(ARANYMCONFIG, config_file, sizeof(config_file));
}

static int process_config(FILE *f, const char *filename, struct Config_Tag *conf, char *title, bool verbose)
{
	int status = input_config(filename, conf, title);
	if (verbose) {
		if (status >= 0)
			fprintf(f, "%s configuration: found %d valid directives.\n", title, status);
		else
			fprintf(f, "Error while reading/processing the '%s' config file.\n", filename);
	}
	return status;
}

static void decode_ini_file(FILE *f, const char *rcfile)
{
	// Does the config exist?
	struct stat buf;
	if (stat(config_file, &buf) == -1) {
		fprintf(f, "Config file '%s' not found.\nThe config file is created with default values. Edit it to suit your needs.\n", config_file);
		saveConfigFile = true;
		return;
	}

	fprintf(f, "Using config file: '%s'\n", rcfile);

	process_config(f, rcfile, global_conf, "[GLOBAL]", true);
	process_config(f, rcfile, startup_conf, "[STARTUP]", true);
#ifdef USE_JIT
	process_config(f, rcfile, jit_conf, "[JIT]", true);
#endif
	process_config(f, rcfile, video_conf, "[VIDEO]", true);
	process_config(f, rcfile, tos_conf, "[TOS]", true);
	process_config(f, rcfile, diskc_configs, "[IDE0]", true);
	process_config(f, rcfile, diskd_configs, "[IDE1]", true);
	process_config(f, rcfile, arafs_conf, "[ARANYMFS]", true);
}

int saveSettings(const char *fs)
{
	presave_cfg();

	if (update_config(fs, global_conf, "[GLOBAL]") < 0)
		fprintf(stderr, "Error while writting the '%s' config file.\n", fs);
	update_config(fs, startup_conf, "[STARTUP]");
#ifdef USE_JIT
	update_config(fs, jit_conf, "[JIT]");
#endif
	update_config(fs, video_conf, "[VIDEO]");
	update_config(fs, tos_conf, "[TOS]");
	update_config(fs, diskc_configs, "[IDE0]");
	update_config(fs, diskd_configs, "[IDE1]");
	update_config(fs, arafs_conf, "[ARANYMFS]");

	return 0;
}

int decode_switches(FILE *f, int argc, char **argv)
{
	build_cfgfilename();
	check_for_help_version_configfile(argc, argv);
	preset_cfg();
	decode_ini_file(f, config_file);
	process_cmdline(argc, argv);
	postload_cfg();

	if (saveConfigFile) {
		D(bug("Storing configuration to file '%s'", config_file));
		saveSettings(config_file);
	}

	return 0;
}
