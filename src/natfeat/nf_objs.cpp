#include "nf_objs.h"

#include "nf_basicset.h"
#include "xhdi.h"
#include "audio.h"
#include "hostfs.h"
#include "ece.h"
#include "fvdidrv.h"
#include "debugprintf.h"
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
ECE ECe;
#endif
/* add your NatFeat object declaration here */

pNatFeat nf_objects[] = {
	&nf_name, &nf_version, &nf_shutdown, &nf_stderr,	/* NF basic set */
	&dbgprintf,
	&fVDIDrv,
	&Xhdi,
	&Audio,
#ifdef HOSTFS_SUPPORT
	&hostFs,
#endif
#ifdef ETHERNET_SUPPORT
	&ECe
#endif
	/* add your NatFeat object here */

};

unsigned int nf_objs_cnt = sizeof(nf_objects) / sizeof(nf_objects[0]);
