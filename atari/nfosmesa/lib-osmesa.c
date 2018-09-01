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

/*--- Local variables ---*/

static const struct nf_ops *nfOps;
static unsigned long nfOSMesaId=0;

static long do_nothing(unsigned long function_number, OSMesaContext ctx, void *first_param)
{
	(void) function_number;
	(void) ctx;
	(void) first_param;
	return 0;
}

long (*HostCall_p)(unsigned long function_number, OSMesaContext ctx, void *first_param) = do_nothing;
void (*HostCall64_p)(unsigned long function_number, OSMesaContext ctx, void *first_param, GLuint64 *retvalue);

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

static long HostCall_natfeats(unsigned long function_number, OSMesaContext ctx, void *first_param)
{
	return nfOps->call(nfOSMesaId+function_number,ctx,first_param);
}

static void HostCall_natfeats64(unsigned long function_number, OSMesaContext ctx, void *first_param, GLuint64 *retvalue)
{
	nfOps->call(nfOSMesaId+function_number, ctx, first_param, retvalue);
}

static int InstallHostCall(void)
{
	/* TOS maybe, try the cookie */
	if (nfOSMesaId==0) {
		InitNatfeat();
	}
	if (nfOSMesaId!=0) {
		HostCall_p = HostCall_natfeats;
		HostCall64_p = HostCall_natfeats64;
		return 1;
	}

	return 0;
}

OSMesaContext APIENTRY internal_OSMesaCreateContext(gl_private *private, GLenum format, OSMesaContext sharelist)
{
	if (!InstallHostCall()) {
		return NULL;
	}

	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXT, private->cur_context, &format);
}

OSMesaContext APIENTRY internal_OSMesaCreateContextExt(gl_private *private, GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist)
{
	if (!InstallHostCall()) {
		return NULL;
	}

	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXTEXT, private->cur_context, &format);
}

OSMesaContext APIENTRY internal_OSMesaCreateContextAttribs(gl_private *private, const GLint *attribList, OSMesaContext sharelist)
{
	if (!InstallHostCall()) {
		return NULL;
	}

	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXTATTRIBS, private->cur_context, &attribList);
}

void APIENTRY internal_OSMesaDestroyContext(gl_private *private, OSMesaContext ctx)
{
	(*HostCall_p)(NFOSMESA_OSMESADESTROYCONTEXT, private->cur_context, &ctx);
	freeglGetString(private, ctx);
	if (ctx == private->cur_context)
		private->cur_context = 0;
}

GLboolean APIENTRY internal_OSMesaMakeCurrent(gl_private *private, OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height)
{
	GLboolean ret = (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAMAKECURRENT, private->cur_context, &ctx);
	if (ret)
		private->cur_context = ctx;
	return ret;
}

OSMesaContext APIENTRY internal_OSMesaGetCurrentContext(gl_private *private)
{
#if 0
	/*
	 * wrong; the host manages his current context for all processes using NFOSMesa;
	 * return local copy instead
	 */
	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESAGETCURRENTCONTEXT, private->cur_context, NULL);
#else
	return private->cur_context;
#endif
}

void APIENTRY internal_OSMesaPixelStore(gl_private *private, GLint pname, GLint value)
{
	(*HostCall_p)(NFOSMESA_OSMESAPIXELSTORE, private->cur_context, &pname);
}

void APIENTRY internal_OSMesaGetIntegerv(gl_private *private, GLint pname, GLint *value)
{
	(*HostCall_p)(NFOSMESA_OSMESAGETINTEGERV, private->cur_context, &pname);
}

GLboolean APIENTRY internal_OSMesaGetDepthBuffer(gl_private *private, OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAGETDEPTHBUFFER, private->cur_context, &c);
}

GLboolean APIENTRY internal_OSMesaGetColorBuffer(gl_private *private, OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAGETCOLORBUFFER, private->cur_context, &c);
}

OSMESAproc APIENTRY internal_OSMesaGetProcAddress(gl_private *private, const char *funcName)
{
	long func = (*HostCall_p)(NFOSMESA_OSMESAGETPROCADDRESS, private->cur_context, &funcName);
	if (func == 0 || func > NFOSMESA_LAST)
		return 0;
	return (OSMESAproc)func;
}

void APIENTRY internal_OSMesaColorClamp(gl_private *private, GLboolean32 enable)
{
	(*HostCall_p)(NFOSMESA_OSMESACOLORCLAMP, private->cur_context, &enable);
}

void APIENTRY internal_OSMesaPostprocess(gl_private *private, OSMesaContext osmesa, const char *filter, GLuint enable_value)
{
	(*HostCall_p)(NFOSMESA_OSMESAPOSTPROCESS, private->cur_context, &osmesa);
}


void internal_glInit(gl_private *private)
{
	unsigned int i, j;
	
	if (!private)
		return;
	private->cur_context = 0;
	private->gl_exception = 0;
	for (i = 0; i < MAX_OSMESA_CONTEXTS; i++)
	{
		private->contexts[i].oldmesa_buffer = 0;
		for (j = 0; j < sizeof(private->contexts[i].gl_strings) / sizeof(private->contexts[i].gl_strings[0]); j++)
			private->contexts[i].gl_strings[j] = 0;
	}
	InstallHostCall();
}
