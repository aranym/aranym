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

#include "lib-osmesa.h"
#include "lib-misc.h"
#include "../natfeat/nf_ops.h"
#include "nfosmesa_nfapi.h"

/*--- Defines ---*/

#define NFOSMESA_DEVICE	"u:\\dev\\nfosmesa"

/*--- Global variables ---*/

/* FIXME: should be per thread */
OSMesaContext cur_context = 0;

/*--- Local variables ---*/

static struct nf_ops *nfOps;
static unsigned long nfOSMesaId=0;

int (*HostCall_p)(int function_number, OSMesaContext ctx, void *first_param);

/*--- Local functions ---*/

static int HostCall_natfeats(int function_number, OSMesaContext ctx, void *first_param);

/*--- OSMesa functions redirectors ---*/


void err_init(const char *str)
{
	/*
	 * FIXME: figure out how to get at the apid of the
	 * the app that loaded us (LDG->id), and perform
	 * either Cconws() or form_alert()
	 */
	(void) Cconws(str);
	(void) Cconws("\r\n");
	/* if (__mint) */ Salert(str);
}


static void InitNatfeat(void)
{
	long ver;
	
	nfOps = nf_init();
	if (!nfOps) {
		err_init("__NF cookie not present on this system");
		return;
	}

	nfOSMesaId=nfOps->get_id("OSMESA");
	if (nfOSMesaId==0) {
		err_init("NF OSMesa functions not present on this system");
		return;
	}
	ver = nfOps->call(nfOSMesaId+GET_VERSION, 0l, 0l);
	if (ver < ARANFOSMESA_NFAPI_VERSION)
	{
		if (err_old_nfapi())
		{
			err_init("NF OSMesa functions in ARAnyM too old");
			nfOSMesaId = 0;
			return;
		}
	}
	if (ver > ARANFOSMESA_NFAPI_VERSION)
	{
		/*
		 * they should be backward compatible, but give a warning
		 */
	    (void) Cconws("Warning: NF OSMesa functions in ARAnyM newer than library\r\n");
	}
}

static int HostCall_natfeats(int function_number, OSMesaContext ctx, void *first_param)
{
	return nfOps->call(nfOSMesaId+function_number,ctx,first_param);
}

static int InstallHostCall(void)
{
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

OSMesaContext APIENTRY OSMesaCreateContext( GLenum format, OSMesaContext sharelist )
{
	if (HostCall_p==NULL) {
		if (!InstallHostCall()) {
			return NULL;
		}
	}

	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXT, cur_context, &format);
}

OSMesaContext APIENTRY OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist)
{
	if (HostCall_p==NULL) {
		if (!InstallHostCall()) {
			return NULL;
		}
	}

	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXTEXT, cur_context, &format);
}

void APIENTRY OSMesaDestroyContext( OSMesaContext ctx )
{
	(*HostCall_p)(NFOSMESA_OSMESADESTROYCONTEXT, cur_context, &ctx);
	freeglGetString();
	if (ctx == cur_context)
		cur_context = 0;
}

GLboolean APIENTRY OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height )
{
	GLboolean ret = (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAMAKECURRENT, cur_context, &ctx);
	if (ret)
		cur_context = ctx;
	return ret;
}

OSMesaContext APIENTRY OSMesaGetCurrentContext( void )
{
#if 0
	/*
	 * wrong; the host manages his current context for all processes using NFOSMesa;
	 * return local copy instead
	 */
	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESAGETCURRENTCONTEXT, cur_context, NULL);
#else
	return cur_context;
#endif
}

void APIENTRY OSMesaPixelStore( GLint pname, GLint value )
{
	(*HostCall_p)(NFOSMESA_OSMESAPIXELSTORE, cur_context, &pname);
}

void APIENTRY OSMesaGetIntegerv( GLint pname, GLint *value )
{
	(*HostCall_p)(NFOSMESA_OSMESAGETINTEGERV, cur_context, &pname);
}

GLboolean APIENTRY OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAGETDEPTHBUFFER, cur_context, &c);
}

GLboolean APIENTRY OSMesaGetColorBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAGETCOLORBUFFER, cur_context, &c);
}

OSMESAproc APIENTRY OSMesaGetProcAddress( const char *funcName )
{
	return (OSMESAproc)(*HostCall_p)(NFOSMESA_OSMESAGETPROCADDRESS, cur_context, &funcName);
}

void APIENTRY OSMesaColorClamp(GLboolean32 enable)
{
	(*HostCall_p)(NFOSMESA_OSMESACOLORCLAMP, cur_context, &enable);
}

void APIENTRY OSMesaPostprocess(OSMesaContext osmesa, const char *filter, GLuint enable_value)
{
	(*HostCall_p)(NFOSMESA_OSMESAPOSTPROCESS, cur_context, &osmesa);
}
