/*
	OSMesa LDG loader

	Copyright (C) 2004	Patrice Mandin

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

/*--- Includes ---*/

#include <stdarg.h>
#include <stdlib.h>
#include <mint/cookie.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>

#include <GL/gl.h>

#include "lib-osmesa.h"
#include "lib-misc.h"
#include "../natfeat/nf_ops.h"
#include "nfosmesa_nfapi.h"

/*--- Defines ---*/

#define NFOSMESA_DEVICE	"u:\\dev\\nfosmesa"

/*--- Global variables ---*/

OSMesaContext cur_context=0;

/*--- Local variables ---*/

static struct nf_ops *nfOps;
static unsigned long nfOSMesaId=0;
static int dev_handle=-1;

int (*HostCall_p)(int function_number, OSMesaContext ctx, void *first_param);

/*--- Local functions ---*/

static int HostCall_natfeats(int function_number, OSMesaContext ctx, void *first_param);

/*--- OSMesa functions redirectors ---*/

static void InitNatfeat(void)
{
	nfOps = nf_init();
	if (!nfOps) {
		Cconws("__NF cookie not present on this system\r\n");
		return;
	}

	nfOSMesaId=nfOps->get_id("OSMESA");
	if (nfOSMesaId==0) {
		Cconws("NF OSMesa functions not present on this system\r\n");
	}
}

static int HostCall_natfeats(int function_number, OSMesaContext ctx, void *first_param)
{
	return nfOps->call(nfOSMesaId+function_number,ctx,first_param);
}

static int HostCall_device(int function_number, OSMesaContext ctx, void *first_param)
{
	unsigned long params[3];

	params[0] = (unsigned long) function_number;
	params[1] = (unsigned long) ctx;
	params[2] = (unsigned long) first_param;

	return Fcntl(dev_handle, params, NFOSMESA_IOCTL);
}

static int InstallHostCall(void)
{
	/* Try the MiNT device */
	if (dev_handle<0) {
		dev_handle = Fopen(NFOSMESA_DEVICE, 0);
		if (dev_handle>0) {
			HostCall_p = HostCall_device;
			return 1;
		}
	}

	/* TOS maybe, try the cookie */
	if (nfOSMesaId==0) {
		InitNatfeat();
		if (nfOSMesaId!=0) {
			HostCall_p = HostCall_natfeats;
			return 1;
		}
	}

	return 0;
}

OSMesaContext OSMesaCreateContext( GLenum format, OSMesaContext sharelist )
{
	if (HostCall_p==NULL) {
		if (!InstallHostCall()) {
			return NULL;
		}
	}

	cur_context=(OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXT, 0, &format);
	return cur_context;
}

OSMesaContext OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist)
{
	if (HostCall_p==NULL) {
		if (!InstallHostCall()) {
			return NULL;
		}
	}

	cur_context=(OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXTEXT, 0, &format);
	return cur_context;
}

void OSMesaDestroyContext( OSMesaContext ctx )
{
	(*HostCall_p)(NFOSMESA_OSMESADESTROYCONTEXT, 0, &ctx);
	freeglGetString();
	if (dev_handle>0) {
		Fclose(dev_handle);
		dev_handle=-1;
	}
}

GLboolean OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height )
{
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAMAKECURRENT,0, &ctx);
}

OSMesaContext OSMesaGetCurrentContext( void )
{
	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESAGETCURRENTCONTEXT, 0, NULL);
}

void OSMesaPixelStore( GLint pname, GLint value )
{
	(*HostCall_p)(NFOSMESA_OSMESAPIXELSTORE,0, &pname);
}

void OSMesaGetIntegerv( GLint pname, GLint *value )
{
	(*HostCall_p)(NFOSMESA_OSMESAGETINTEGERV,0, &pname);
}

GLboolean OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAGETDEPTHBUFFER,0, &c);
}

GLboolean OSMesaGetColorBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAGETCOLORBUFFER,0, &c);
}

void *OSMesaGetProcAddress( const char *funcName )
{
	return (void *)(*HostCall_p)(NFOSMESA_OSMESAGETPROCADDRESS,0, &funcName);
}
