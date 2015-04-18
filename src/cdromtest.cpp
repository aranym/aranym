#include "sysdeps.h"
#include "toserror.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfcdrom_sdl.h"

#define DEBUG 1
#include "debug.h"
#undef DEBUG

#define RAM_SIZE 0x20000

unsigned char atari_mem[RAM_SIZE];
#define Atari2HostAddr(buffer) &atari_mem[buffer]

uintptr MEMBaseDiff;
uint32 FastRAMSize = 0;
memptr RAMBase = 0;
uint32 RAMSize = RAM_SIZE;
uae_u32 VideoRAMBase = 0;
uint8 *RAMBaseHost;
uint8 *VideoRAMBaseHost;
JMP_BUF excep_env;
struct regstruct regs;
bx_options_t bx_options;

#ifdef NFCDROM_LINUX_SUPPORT
# include "nfcdrom_linux.cpp"
# undef DEBUG
# undef NFCD_NAME
#endif
#ifdef NFCDROM_WIN32_SUPPORT
# include "nfcdrom_win32.cpp"
# undef DEBUG
# undef NFCD_NAME
#endif
#include "nfcdrom_sdl.cpp"
#undef DEBUG
#undef NFCD_NAME
#include "nfcdrom.cpp"
#undef DEBUG
#include "nf_base.cpp"
#if defined OS_mingw || defined OS_cygwin
#include "Unix/cygwin/win32_supp.cpp"
#endif


int ndebug::dbprintf(const char *s, ...)
{
	va_list a;
	int i;
	va_start(a, s);
	i = vfprintf(stderr, s, a);
	i += fprintf(stderr, "\n");
	va_end(a);
	return i;
}

int ndebug::pdbprintf(const char *s, ...)
{
	va_list a;
	int i;
	va_start(a, s);
	i = vfprintf(stderr, s, a);
	i += fprintf(stderr, "\n");
	va_end(a);
	return i;
}

void preset_nfcdroms() {
	for(int i=0; i < CD_MAX_DRIVES; i++) {
		bx_options.nfcdroms[i].physdevtohostdev = -1;
	}
}


char *safe_strncpy(char *dest, const char *src, size_t size)
{
	if (dest == NULL) return NULL;
	if (size > 0) {
		strncpy(dest, src != NULL ? src : "", size);
		dest[size-1] = '\0';
	}
	return dest;
}

static void minimum_init(void)
{
	RAMBaseHost = VideoRAMBaseHost = atari_mem;
	InitMEMBaseDiff(RAMBaseHost, RAMBase);
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	
	preset_nfcdroms();
	
	bx_options.nfcdroms[DriveFromLetter('X')].physdevtohostdev = 0;
}

extern "C" void breakpt(void) { }

static int32 params[6];
CdromDriver *driver;

static int32 nf_call(int32 cmd, ...)
{
	va_list args;
	va_start(args, cmd);
	params[0] = va_arg(args, int32);
	params[1] = va_arg(args, int32);
	params[2] = va_arg(args, int32);
	params[3] = va_arg(args, int32);
	params[4] = va_arg(args, int32);
	params[5] = va_arg(args, int32);
	va_end(args);
	return driver->dispatch(cmd);
}
uint32 nf_getparameter(int i) { return params[i]; }


#define bos_header_addr 0x10000
#define ext_status_addr 0x10100
#define toc_addr        0x11000
#define msf_addr        0x12000



static void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE) != 0)
	{
		fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);
}


static metados_bos_header_t *bos_header;
static const char *cdrom_driver = NULL;
static atari_cdrom_msf_t *atari_msf;

int main(int argc, char **argv)
{
	if (argc > 1)
		cdrom_driver = argv[1];
	
	minimum_init();

    TRY(prb) {

#if defined NFCDROM_LINUX_SUPPORT
	if (cdrom_driver == NULL || strcmp("linux", cdrom_driver) == 0)
	{
		sdl_init();
		driver = new CdromDriverLinux();
	} else
#elif defined NFCDROM_WIN32_SUPPORT
	if (cdrom_driver == NULL || strcmp("win32", cdrom_driver) == 0)
	{
		driver = new CdromDriverWin32();
	} else
#endif
	{
		if (cdrom_driver != NULL && strcmp(cdrom_driver, "sdl") != 0)
			fprintf(stderr, "cdrom driver %s not supported, using sdl instead\n", cdrom_driver);
		sdl_init();
		driver = new CdromDriverSdl();
	}
	if (!driver)
		return 1;
	
	bos_header = (metados_bos_header_t *)Atari2HostAddr(bos_header_addr);
	bos_header->phys_letter = SDL_SwapBE16('X');
	
	nf_call(NFCD_DRIVESMASK);

	nf_call(NFCD_OPEN, bos_header_addr, ext_status_addr);

	nf_call(NFCD_STATUS, bos_header_addr, ext_status_addr);
	
	nf_call(NFCD_GETTOC, bos_header_addr, (uint32)0, toc_addr);
	
	nf_call(NFCD_IOCTL, bos_header_addr, (uint32)ATARI_CDROMSUBCHNL, toc_addr);
	
	nf_call(NFCD_DISCINFO, bos_header_addr, toc_addr);

	memset(Atari2HostAddr(toc_addr), 1, 4);
	nf_call(NFCD_IOCTL, bos_header_addr, (uint32)ATARI_CDROMPLAYTRKIND, toc_addr);
	nf_call(NFCD_STATUS, bos_header_addr, ext_status_addr);
	sleep(10);

	atari_msf = (atari_cdrom_msf_t *)Atari2HostAddr(msf_addr);
	atari_msf->cdmsf_min0 = 0;
	atari_msf->cdmsf_sec0 = 2;
	atari_msf->cdmsf_frame0 = 0;
	atari_msf->cdmsf_min1 = 4;
	atari_msf->cdmsf_sec1 = 0;
	atari_msf->cdmsf_frame1 = 0;
	nf_call(NFCD_IOCTL, bos_header_addr, (uint32)ATARI_CDROMPLAYMSF, msf_addr);
	nf_call(NFCD_STATUS, bos_header_addr, ext_status_addr);
	sleep(10);
	
	nf_call(NFCD_CLOSE, bos_header_addr, ext_status_addr);

    }
    CATCH(prb) {
		fprintf(stderr, "got m68k exception %d: %s addr %08x\n", prb, regs.mmu_ssw & 0x100 ? "read" : "write", regs.mmu_fault_addr);
		return 1;
    }
	
	delete driver;
	
	return 0;
}
