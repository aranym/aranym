#include "nf_objs.h"

#include "nf_name.h"
#include "nf_version.h"
#include "nf_stderr.h"
#include "xhdi.h"
#include "hostfs.h"
#include "fvdidrv.h"
/* add your NatFeat class definition here */

NF_Name nf_name;
NF_Version nf_version;
NF_StdErr nf_stderr;
XHDIDriver Xhdi;
FVDIDriver fVDIDrv;
#ifdef ARANYM_HOSTFS
HostFs hostFs;
#endif
/* add your NatFeat object declaration here */

pNatFeat nf_objects[] = {
	&fVDIDrv,
	&Xhdi,

#ifdef ARANYM_HOSTFS
	&hostFs,
#endif
	/* add your NatFeat object here */

	&nf_stderr,
	&nf_version,
	&nf_name
};

unsigned int nf_objs_cnt = sizeof(nf_objects) / sizeof(nf_objects[0]);
