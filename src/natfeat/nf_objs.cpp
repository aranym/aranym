/*
	Native features

	ARAnyM (C) 2005 Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*--- Includes ---*/

#include "nf_objs.h"
#include "nf_basicset.h"

#include "xhdi.h"
#include "nfaudio.h"
#include "nfbootstrap.h"
#include "hostfs.h"
#include "ethernet.h"
#include "debugprintf.h"
#ifdef NFVDI_SUPPORT
# include "nfvdi.h"
# include "nfvdi_soft.h"
# ifdef ENABLE_OPENGL
#  include "nfvdi_opengl.h"
# endif
#endif
#ifdef NFCDROM_SUPPORT
# if !SDL_VERSION_ATLEAST(2, 0, 0)
#  include "nfcdrom_sdl.h"
# endif
# ifdef NFCDROM_LINUX_SUPPORT
#  include "nfcdrom_linux.h"
# endif
# ifdef NFCDROM_WIN32_SUPPORT
#  include "nfcdrom_win32.h"
# endif
#endif
#ifdef NFPCI_SUPPORT
# include "nfpci.h"
# ifdef NFPCI_LINUX_SUPPORT
#  include "nfpci_linux.h"
# endif
#endif
#ifdef NFOSMESA_SUPPORT
# include "nfosmesa.h"
#endif
#ifdef NFJPEG_SUPPORT
# include "nfjpeg.h"
#endif
#ifdef NFCLIPBRD_SUPPORT
# include "nfclipbrd.h"
#endif
#ifdef NFSCSI_SUPPORT
# include "nf_scsidrv.h"
#endif
#ifdef USBHOST_SUPPORT
# include "usbhost.h"
#endif
#ifdef NFEXEC_SUPPORT
# include "nf_hostexec.h"
#endif
/* add your NatFeat class definition here */

/*--- Defines ---*/

#define MAX_NATFEATS 32

/*--- Variables ---*/

NF_Base *nf_objects[MAX_NATFEATS];	/* The natfeats we can use */
unsigned int nf_objs_cnt;			/* Number of natfeats we can use */

/*--- Functions prototypes ---*/

static void NFAdd(NF_Base *new_nf);

/*--- Functions ---*/

void NFCreate(void)
{
	nf_objs_cnt=0;
	memset(nf_objects, 0, sizeof(nf_objects));

	/* NF basic set */
	NFAdd(new NF_Name);
	NFAdd(new NF_Version);
	NFAdd(new NF_Shutdown);
	NFAdd(new NF_StdErr);

	/* additional NF */
	NFAdd(new NF_Exit);
	NFAdd(new BootstrapNatFeat);
	NFAdd(new DebugPrintf);
	NFAdd(new XHDIDriver);
	NFAdd(new AUDIODriver);

#ifdef NFVDI_SUPPORT
# if 0 /*def ENABLE_OPENGL*/
	if ((strcmp("opengl", bx_options.natfeats.vdi_driver)==0) && bx_options.opengl.enabled)
		NFAdd(new OpenGLVdiDriver);
	else 
# endif
		NFAdd(new SoftVdiDriver);
#endif

#ifdef HOSTFS_SUPPORT
	NFAdd(new HostFs);
#endif

#ifdef ETHERNET_SUPPORT
	NFAdd(new ETHERNETDriver);
#endif

#ifdef NFCDROM_SUPPORT
# if defined NFCDROM_LINUX_SUPPORT
	if (strcmp("linux", bx_options.natfeats.cdrom_driver)==0)
		NFAdd(new CdromDriverLinux);
	else
# elif defined NFCDROM_WIN32_SUPPORT
	if (strcmp("win32", bx_options.natfeats.cdrom_driver)==0)
		NFAdd(new CdromDriverWin32);
	else
# endif
# if !SDL_VERSION_ATLEAST(2, 0, 0)
		NFAdd(new CdromDriverSdl);
# endif
	{;}
#endif

#ifdef NFPCI_SUPPORT
# ifdef NFPCI_LINUX_SUPPORT
	NFAdd(new PciDriverLinux);
# else
	NFAdd(new PciDriver);
# endif
#endif

#ifdef NFOSMESA_SUPPORT
	NFAdd(new OSMesaDriver);
#endif

#ifdef NFJPEG_SUPPORT
	NFAdd(new JpegDriver);
#endif

#ifdef NFCLIPBRD_SUPPORT
	NFAdd(new ClipbrdNatFeat);
#endif

#ifdef USBHOST_SUPPORT
	NFAdd(new USBHost);
#endif

#ifdef NFSCSI_SUPPORT
	NFAdd(new SCSIDriver);
#endif

#ifdef NFEXEC_SUPPORT
	NFAdd(new HostExec);
#endif
	/* add your NatFeat object declaration here */
}

static void NFAdd(NF_Base *new_nf)
{
	/* Add a natfeat to our array */
	if (nf_objs_cnt == MAX_NATFEATS) {
		fprintf(stderr, "No more available slots to add a Natfeat\n");
		return;
	}

	nf_objects[nf_objs_cnt++] = new_nf;
}

void NFDestroy(void)
{
	for(unsigned int i=0; i<nf_objs_cnt; i++) {
		if (nf_objects[i]) {
			delete nf_objects[i];
			nf_objects[i] = NULL;
		}
	}
}

void NFReset(void)
{
	for(unsigned int i=0; i<nf_objs_cnt; i++) {
		if (nf_objects[i]) {
			nf_objects[i]->reset();
		}
	}
}

NF_Base *NFGetDriver(const char *name)
{
	for (unsigned int i=0; i<nf_objs_cnt; i++) {
		if (strcasecmp(name, nf_objects[i]->name())==0) {
			return nf_objects[i];
		}
	}
	return NULL;
}
