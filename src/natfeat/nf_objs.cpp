#include "nf_objs.h"

#include "nf_basicset.h"
#include "xhdi.h"
#include "audio.h"
#include "hostfs.h"
#include "ethernet.h"
#include "fvdidrv.h"
#include "debugprintf.h"
#ifdef NFCDROM_SUPPORT
# include "nfcdrom.h"
# ifdef NFCDROM_LINUX_SUPPORT
#  include "nfcdrom_linux.h"
# endif
# ifdef NFCDROM_SDL_SUPPORT
#  include "nfcdrom_sdl.h"
# endif
#endif
/* add your NatFeat class definition here */


/* NF basic set */
NF_Name nf_name;
NF_Version nf_version;
NF_Shutdown nf_shutdown;
NF_StdErr nf_stderr;

/* additional NF */
DebugPrintf dbgprintf;
XHDIDriver Xhdi;
AUDIODriver Audio;
FVDIDriver fVDIDrv;
#ifdef HOSTFS_SUPPORT
HostFs hostFs;
#endif
#ifdef ETHERNET_SUPPORT
ETHERNETDriver Ethernet;
#endif
#ifdef NFCDROM_SUPPORT
# ifdef NFCDROM_LINUX_SUPPORT
CdromDriverLinux CdRom;
# endif
# ifdef NFCDROM_SDL_SUPPORT
CdromDriverSdl CdRom;
# endif
#endif
/* add your NatFeat object declaration here */

pNatFeat nf_objects[] = {
	&nf_name, &nf_version, &nf_shutdown, &nf_stderr,	/* NF basic set */
	&fVDIDrv,
	&Xhdi,
	&Audio,
#ifdef HOSTFS_SUPPORT
	&hostFs,
#endif
#ifdef ETHERNET_SUPPORT
	&Ethernet,
#endif
#ifdef NFCDROM_SUPPORT
	&CdRom,
#endif
	/* add your NatFeat object below */

	/* */
	&dbgprintf
};

unsigned int nf_objs_cnt = sizeof(nf_objects) / sizeof(nf_objects[0]);
