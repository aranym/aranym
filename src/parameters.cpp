/* MJ 2001 */
#include "sysdeps.h"
#include "config.h"
#include "parameters.h"
#include "tools.h"		// for safe_strncpy()

#define BX_INSERTED	true	// copied from emu_bochs.h

#define DEBUG 0
#include "debug.h"

#ifdef HAVE_NEW_HEADERS
# include <cstdlib>
#else
# include <stdlib.h>
#endif

#ifndef USE_JIT
# define USE_JIT 0
#endif

#ifndef FULLMMU
# define FULLMMU 0
#endif

#ifndef DSP_EMULATION
# define DSP_EMULATION 0
#endif

#ifndef DSP_DISASM
# define DSP_DISASM 0
#endif

#ifndef ENABLE_OPENGL
# define ENABLE_OPENGL 0
#endif

#ifndef HOSTFS_SUPPORT
# define HOSTFS_SUPPORT 0
#endif

#ifndef EXTFS_SUPPORT
# define EXTFS_SUPPORT 0
#endif

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
  {"swap-ide", no_argument, 0, 'S'},
  {"emutos", no_argument, 0, 'e'},
  {"lilo", no_argument, 0, 'l'},
  {NULL, 0, NULL, 0}
};

#define TOS_FILENAME	"ROM"
#define EMUTOS_FILENAME	"etos512k.img"

char *program_name;		// set by main()
char rom_path[512];		// set by build_datafilenames()
char emutos_path[512];	// set by build_datafilenames()

bool boot_emutos = false;
bool boot_lilo = false;
bool ide_swap = false;
uint32 FastRAMSize;

static char config_file[512];

#if !defined(XIF_HOST_IP) && !defined(XIF_ATARI_IP) && !defined(XIF_NETMASK)
# define XIF_HOST_IP	"192.168.0.1"
# define XIF_ATARI_IP	"192.168.0.2"
# define XIF_NETMASK	"255.255.255.0"
#endif

static uint32 FastRAMSizeMB;
static bool saveConfigFile = false;

bx_options_t bx_options;

static bx_atadevice_options_t *diskc = &bx_options.atadevice[0][0];
static bx_atadevice_options_t *diskd = &bx_options.atadevice[0][1];


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
  strcpy(bx_options.floppy.path, "");
#ifdef FixedSizeFastRAM
  FastRAMSize = FixedSizeFastRAM * 1024 * 1024;
#else
  FastRAMSize = 0;
#endif
}

void postload_global() {
	bx_options.floppy.inserted = (strlen(bx_options.floppy.path) > 0);
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
	{ "TuneAlignment", Bool_Tag, &bx_options.jit.tunealign},
	{ "TuneNOPfill", Bool_Tag, &bx_options.jit.tunenop},
	{ "JITCacheSize", Int_Tag, &bx_options.jit.jitcachesize},
	{ "JITLazyFlush", Int_Tag, &bx_options.jit.jitlazyflush},
	{ NULL , Error_Tag, NULL }
};

void preset_jit() {
	bx_options.jit.jit = false;
	bx_options.jit.jitfpu = true;
	bx_options.jit.jitcachesize = 8192;
	bx_options.jit.jitlazyflush = 1;
	bx_options.jit.tunealign = true;
	bx_options.jit.tunenop = true;
}

void postload_jit() {
}

void presave_jit() {
}

/*************************************************************************/
struct Config_Tag tos_conf[]={
	{ "Cookie_MCH", HexLong_Tag, &bx_options.tos.cookie_mch},
	{ "RedirConsole", Bool_Tag, &bx_options.tos.redirect_CON},
	{ "RedirPrinter", Bool_Tag, &bx_options.tos.redirect_PRT},
	{ NULL , Error_Tag, NULL }
};

void preset_tos() {
  bx_options.tos.redirect_CON = false;
  bx_options.tos.redirect_PRT = false;
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
	{ "AutoZoom", Bool_Tag, &bx_options.video.autozoom},
	{ "AutoZoomInteger", Bool_Tag, &bx_options.video.autozoomint},
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
  bx_options.video.autozoom = false;
  bx_options.video.autozoomint = false;
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
struct Config_Tag opengl_conf[]={
	{ "Enabled", Bool_Tag, &bx_options.opengl.enabled},
	{ "Width", Int_Tag, &bx_options.opengl.width},
	{ "Height", Int_Tag, &bx_options.opengl.height},
	{ "Bpp", Int_Tag, &bx_options.opengl.bpp},
	{ "Filtered", Bool_Tag, &bx_options.opengl.filtered},
	{ NULL , Error_Tag, NULL }
};

void preset_opengl() {
  bx_options.opengl.enabled = false;
  bx_options.opengl.width = 640;
  bx_options.opengl.height = 480;
  bx_options.opengl.bpp = 16;
  bx_options.opengl.filtered = false;
}

void postload_opengl() {
}

void presave_opengl() {
}

/*************************************************************************/
#define BX_DISK_CONFIG(Disk)	struct Config_Tag Disk ## _configs[] = {	\
	{ "Present", Bool_Tag, &Disk->present},	\
	{ "IsCDROM", Bool_Tag, &Disk->isCDROM},	\
	{ "ByteSwap", Bool_Tag, &Disk->byteswap},	\
	{ "ReadOnly", Bool_Tag, &Disk->readonly},	\
	{ "Path", String_Tag, Disk->path, sizeof(Disk->path)},	\
	{ "Cylinders", Int_Tag, &Disk->cylinders},	\
	{ "Heads", Int_Tag, &Disk->heads},	\
	{ "SectorsPerTrack", Int_Tag, &Disk->spt},	\
	{ "ModelName", String_Tag, Disk->model, sizeof(Disk->model)},	\
	{ NULL , Error_Tag, NULL }	\
}

BX_DISK_CONFIG(diskc);
BX_DISK_CONFIG(diskd);

void set_ide(unsigned int number, char *dev_path, int cylinders, int heads, int spt, int byteswap, bool readonly, const char *model_name) {
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

	bx_atadevice_options_t *disk;
	switch(number) {
		case 0: disk = diskc; break;
		case 1: disk = diskd; break;
		default: disk = NULL; break;
	}
  if (disk != NULL) {
    disk->present = strlen(dev_path) > 0;
    disk->isCDROM = false;
    disk->byteswap = byteswap;
    disk->readonly = readonly;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->spt = spt;
    strcpy(disk->path, dev_path);
	safe_strncpy(disk->model, model_name, sizeof(disk->model));
  }

  if (cylinders && heads && spt) {
    D(bug("IDE%d CHS geometry: %d/%d/%d %d", number, cylinders, heads, spt, byteswap));
  }

}

void preset_ide() {
  set_ide(0, "", 0, 0, 0, false, false, "Master");
  set_ide(1, "", 0, 0, 0, false, false, "Slave");

  bx_options.newHardDriveSupport = true;
}

void postload_ide() {
	int channel = 0;
	for(int device=0; device<2; device++) {
    	if (bx_options.atadevice[channel][device].present) {
    		bx_options.atadevice[channel][device].status = BX_INSERTED;
			bx_options.atadevice[channel][device].type = (bx_options.atadevice[channel][device].isCDROM) ? IDE_CDROM : IDE_DISK;
		}
		else {
			bx_options.atadevice[channel][device].type = IDE_NONE;
		}
	}
}

void presave_ide() {
}

/*************************************************************************/
#define DISK_CONFIG(Disk)	struct Config_Tag Disk ## _configs[] = {	\
	{ "Path", String_Tag, bx_options.Disk.path, sizeof(bx_options.Disk.path)},	\
	{ "Present", Bool_Tag, &bx_options.Disk.present},	\
	{ "PartID", String_Tag, bx_options.Disk.partID, sizeof(bx_options.Disk.partID)},	\
	{ "ByteSwap", Bool_Tag, &bx_options.Disk.byteswap},	\
	{ "ReadOnly", Bool_Tag, &bx_options.Disk.readonly},	\
	{ NULL , Error_Tag, NULL }	\
}

DISK_CONFIG(disk0);
DISK_CONFIG(disk1);
DISK_CONFIG(disk2);
DISK_CONFIG(disk3);
DISK_CONFIG(disk4);
DISK_CONFIG(disk5);
DISK_CONFIG(disk6);
DISK_CONFIG(disk7);

void preset_disk() {
}

void postload_disk() {
}

void presave_disk() {
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
#define ETH(x) bx_options.ethernet.x
struct Config_Tag ethernet_conf[]={
	{ "HostIP", String_Tag, &ETH(ip_host), sizeof(ETH(ip_host))},
	{ "AtariIP", String_Tag, &ETH(ip_atari), sizeof(ETH(ip_atari))},
	{ "Netmask", String_Tag, &ETH(netmask), sizeof(ETH(netmask))},
	{ NULL , Error_Tag, NULL }
};

void preset_ethernet() {
  safe_strncpy(ETH(ip_host), XIF_HOST_IP, sizeof(ETH(ip_host)));
  safe_strncpy(ETH(ip_atari ), XIF_ATARI_IP, sizeof(ETH(ip_atari)));
  safe_strncpy(ETH(netmask), XIF_NETMASK, sizeof(ETH(netmask)));
}

void postload_ethernet() {
}

void presave_ethernet() {
}

/*************************************************************************/
#define LILO(x) bx_options.lilo.x

struct Config_Tag lilo_conf[]={
	{ "Kernel", String_Tag, &LILO(kernel), sizeof(LILO(kernel))},
	{ "Args", String_Tag, &LILO(args), sizeof(LILO(args))},
	{ "Ramdisk", String_Tag, &LILO(ramdisk), sizeof(LILO(ramdisk))},
	{ NULL , Error_Tag, NULL }
};

void preset_lilo() {
  safe_strncpy(LILO(kernel), "linux.bin", sizeof(LILO(kernel)));
  safe_strncpy(LILO(args), "root=/dev/ram video=atafb:vga16", sizeof(LILO(args)));
  safe_strncpy(LILO(ramdisk), "root.bin", sizeof(LILO(ramdisk)));
}

void postload_lilo() {
}

void presave_lilo() {
}

/*************************************************************************/
#define MIDI(x) bx_options.midi.x

struct Config_Tag midi_conf[]={
	{ "Enabled", Bool_Tag, &MIDI(enabled)},
	{ "Output", String_Tag, &MIDI(output), sizeof(MIDI(output))},
	{ NULL , Error_Tag, NULL }
};

void preset_midi() {
  MIDI(enabled)=false;
  safe_strncpy(MIDI(output), "/tmp/aranym-midi.txt", sizeof(MIDI(output)));
}

void postload_midi() {
}

void presave_midi() {
}

/*************************************************************************/
void usage (int status) {
  printf ("%s\n", VERSION_STRING);
  printf ("Usage: %s [OPTION]... [FILE]...\n", program_name);
  printf ("\
Options:\n\
  -a, --floppy NAME          floppy image file NAME\n\
  -e, --emutos               boot EmuTOS\n\
  -l, --lilo                 boot a linux kernel\n\
  -N, --nomouse              don't grab mouse at startup\n\
  -f, --fullscreen           start in fullscreen\n\
  -v, --refresh <X>          VIDEL refresh rate in VBL (default 2)\n\
  -r, --resolution <X>       boot in X color depth [1,2,4,8,16]\n\
  -m, --monitor <X>          attached monitor: 0 = VGA, 1 = TV\n\
  -d, --disk CHAR:PATH[:]    HostFS mapping, e.g. d:/atari/d_drive\n\
  -c, --config FILE          read different configuration file\n\
  -s, --save                 save configuration file\n\
  -S, --swap-ide             swap IDE drives\n\
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
  preset_disk();
  preset_arafs();
  preset_video();
  preset_tos();
  preset_startup();
  preset_jit();
  preset_opengl();
  preset_ethernet();
  preset_lilo();
  preset_midi();
}

void postload_cfg() {
  postload_global();
  postload_ide();
  postload_disk();
  postload_arafs();
  postload_video();
  postload_tos();
  postload_startup();
  postload_jit();
  postload_opengl();
  postload_ethernet();
  postload_lilo();
  postload_midi();
}

void presave_cfg() {
  presave_global();
  presave_ide();
  presave_disk();
  presave_arafs();
  presave_video();
  presave_tos();
  presave_startup();
  presave_jit();
  presave_opengl();
  presave_ethernet();
  presave_lilo();
  presave_midi();
}

void early_cmdline_check(int argc, char **argv) {
	for (int c = 0; c < argc; c++) {
		char *p = argv[c];
		if (strcmp(p, "-S") == 0  || strcmp(p, "--swap-ide") == 0)
			ide_swap = true;

		else if ((strcmp(p, "-c") == 0) || (strcmp(p, "--config") == 0)) {
			if ((c + 1) < argc) {
				safe_strncpy(config_file, argv[c + 1], sizeof(config_file));
			} else {
				fprintf(stderr, "config switch requires one parameter\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(p, "-h") == 0) || (strcmp(p, "--help") == 0)) {
			usage(0);
			exit(0);
		} else if ((strcmp(p, "-V") == 0) || (strcmp(p, "--version") == 0)) {
			infoprint("%s\n", VERSION_STRING);
			infoprint("Capabilities:");
			infoprint("JIT compiler     : %s", (USE_JIT == 1) ? "enabled" : "disabled");
			infoprint("Full MMU         : %s", (FULLMMU == 1) ? "enabled" : "disabled");
			infoprint("DSP              : %s", (DSP_EMULATION == 1) ? "enabled" : "disabled");
			infoprint("DSP disassembler : %s", (DSP_DISASM == 1) ? "enabled" : "disabled");
			infoprint("OpenGL support   : %s", (ENABLE_OPENGL == 1) ? "enabled" : "disabled");
			infoprint("HOSTFS support   : %s", (HOSTFS_SUPPORT == 1) ? "enabled" : "disabled");
			infoprint("ARANYMFS support : %s", (EXTFS_SUPPORT == 1) ? "enabled (obsolete)" : "disabled");

			exit (0);
		}
	}
}

int process_cmdline(int argc, char **argv)
{
	int c;
	while ((c = getopt_long (argc, argv,
							 "a:" /* floppy image file */
							 "e"  /* boot emutos */
							 "l"  /* boot lilo */
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
							 "s"  /* save config file */

							 "c:" /* path to config file */
							 "S"  /* swap IDE drives */
							 "h"  /* help */
							 "V"  /* version */,
							 long_options, (int *) 0)) != EOF) {
		switch (c) {
			case 'V':
			case 'h':
			case 'c':
			case 'S':
				/* processed in early_cmdline_check already */
				break;

	
#ifdef DEBUGGER
			case 'D':
				bx_options.startup.debugger = true;
				break;
#endif

			case 'e':
				boot_emutos = true;
				break;
	
			case 'f':
				bx_options.video.fullscreen = true;
				break;

			case 'l':
				boot_lilo = true;
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
				safe_strncpy(bx_options.floppy.path, optarg, sizeof(bx_options.floppy.path));
				break;

			case 'r':
				bx_options.video.boot_color_depth = atoi(optarg);
				break;

			case 'd':
				if ( strlen(optarg) < 4 || optarg[1] != ':') {
					fprintf(stderr, "Not enough parameters for -d\n");
					break;
				}
				// set the drive
				{
					int8 i = toupper(optarg[0]) - 'A';
					if (i <= 0 || i>('Z'-'A')) {
						fprintf(stderr, "Drive out of [A-Z] range for -d\n");
						break;
					}

					safe_strncpy( bx_options.aranymfs[i].rootPath, optarg+2,
								sizeof(bx_options.aranymfs[i].rootPath) );
					// Note: tail colon processing (case sensitivity flag) is 
					// done later by calling postload_cfg.
					// Just make sure postload_cfg is called after this.
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

// append a filename to a path
char *addFilename(char *buffer, const char *file, unsigned int bufsize)
{
	int dirlen = strlen(buffer);
	if ((dirlen + 1 + strlen(file) + 1) < bufsize) {
		if (dirlen > 0) {
			char *ptrLast = buffer + dirlen - 1;
			if (*ptrLast != '/' && *ptrLast != '\\')
				strcat(buffer, DIRSEPARATOR);
		}
		strcat(buffer, file);
	}
	else {
		panicbug("addFilename(\"%s\") - buffer too small!", file);
		safe_strncpy(buffer, file, bufsize);	// at least the filename
	}

	return buffer;
}

// build a complete path to an user-specific file
char *getConfFilename(const char *file, char *buffer, unsigned int bufsize)
{
	getConfFolder(buffer, bufsize);

	// Does the folder exist?
	struct stat buf;
	if (stat(buffer, &buf) == -1) {
		D(bug("Creating config folder '%s'", buffer));
		mkdir(buffer, 0755);
	}

	return addFilename(buffer, file, bufsize);
}

// build a complete path to system wide data file
char *getDataFilename(const char *file, char *buffer, unsigned int bufsize)
{
	getDataFolder(buffer, bufsize);
	return addFilename(buffer, file, bufsize);
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
	if (stat(rcfile, &buf) == -1) {
		fprintf(f, "Config file '%s' not found.\nThe config file is created with default values. Edit it to suit your needs.\n", rcfile);
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
	process_config(f, rcfile, ide_swap ? diskd_configs : diskc_configs, "[IDE0]", true);
	process_config(f, rcfile, ide_swap ? diskc_configs : diskd_configs, "[IDE1]", true);

/*
	process_config(f, rcfile, disk0_configs, "[DISK0]", true);
	process_config(f, rcfile, disk1_configs, "[DISK1]", true);
	process_config(f, rcfile, disk2_configs, "[DISK2]", true);
	process_config(f, rcfile, disk3_configs, "[DISK3]", true);
	process_config(f, rcfile, disk4_configs, "[DISK4]", true);
	process_config(f, rcfile, disk5_configs, "[DISK5]", true);
	process_config(f, rcfile, disk6_configs, "[DISK6]", true);
	process_config(f, rcfile, disk7_configs, "[DISK7]", true);
*/
	if (process_config(f, rcfile, arafs_conf, "[HOSTFS]", true) == 0) {
		// fallback to obsolete [ARANYMFS]
		fprintf(stderr, "[HOSTFS] section in config file not found.\n"
						"falling back to obsolete [ARANYMFS]\n");
		process_config(f, rcfile, arafs_conf, "[ARANYMFS]", true);
	}
	process_config(f, rcfile, opengl_conf, "[OPENGL]", true);
	process_config(f, rcfile, ethernet_conf, "[ETH0]", true);
	process_config(f, rcfile, lilo_conf, "[LILO]", true);
	process_config(f, rcfile, midi_conf, "[MIDI]", true);
}

int saveSettings(const char *fs)
{
	presave_cfg();

	if (update_config(fs, global_conf, "[GLOBAL]") < 0)
		fprintf(stderr, "Error while writing the '%s' config file.\n", fs);
	update_config(fs, startup_conf, "[STARTUP]");
#ifdef USE_JIT
	update_config(fs, jit_conf, "[JIT]");
#endif
	update_config(fs, video_conf, "[VIDEO]");
	update_config(fs, tos_conf, "[TOS]");
	update_config(fs, ide_swap ? diskd_configs : diskc_configs, "[IDE0]");
	update_config(fs, ide_swap ? diskc_configs : diskd_configs, "[IDE1]");
/*
	update_config(fs, disk0_configs, "[DISK0]");
	update_config(fs, disk1_configs, "[DISK1]");
	update_config(fs, disk2_configs, "[DISK2]");
	update_config(fs, disk3_configs, "[DISK3]");
	update_config(fs, disk4_configs, "[DISK4]");
	update_config(fs, disk5_configs, "[DISK5]");
	update_config(fs, disk6_configs, "[DISK6]");
	update_config(fs, disk7_configs, "[DISK7]");
*/
	/// update_config(fs, arafs_conf, "[ARANYMFS]");
	update_config(fs, arafs_conf, "[HOSTFS]");
	update_config(fs, opengl_conf, "[OPENGL]");
	update_config(fs, ethernet_conf, "[ETH0]");
	update_config(fs, lilo_conf, "[LILO]");
	update_config(fs, midi_conf, "[MIDI]");

	return 0;
}

bool check_cfg()
{
#if REAL_ADDRESSING
# if defined(__i386__) && defined(OS_linux)
	if (FastRAMSizeMB > (128 - 16))
	{
		panicbug("Maximum Fast RAM size for real addressing on x86/Linux is 112 MB");
		panicbug("If you need bigger Fast RAM, you must recompile ARAnyM");
		panicbug("./configure --enable-addressing=direct");
		return false;
	}
# endif
#endif
	return true;
}

bool decode_switches(FILE *f, int argc, char **argv)
{
	getConfFilename(ARANYMCONFIG, config_file, sizeof(config_file));
	getDataFilename(TOS_FILENAME, rom_path, sizeof(rom_path));
	getDataFilename(EMUTOS_FILENAME, emutos_path, sizeof(emutos_path));

	early_cmdline_check(argc, argv);
	preset_cfg();
	decode_ini_file(f, config_file);
	process_cmdline(argc, argv);
	postload_cfg();

	if (saveConfigFile) {
		D(bug("Storing configuration to file '%s'", config_file));
		saveSettings(config_file);
	}

	return check_cfg();
}
