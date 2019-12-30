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

OSMesaContext APIENTRY internal_OSMesaCreateContext(gl_private *priv, GLenum format, OSMesaContext sharelist)
{
	OSMesaContext c;
	
	if (!InstallHostCall()) {
		return NULL;
	}
	
	if (priv == NULL)
		return NULL;
	c = (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXT, priv->cur_context, &format);
	TRACE(("OSMesaCreateContext(priv=0x%x, format=0x%x, sharelist=%u) -> %u\n", (unsigned int)priv, format, CTX_TO_IDX(sharelist), CTX_TO_IDX(c)));
	return c;
}

OSMesaContext APIENTRY internal_OSMesaCreateContextExt(gl_private *priv, GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist)
{
	OSMesaContext c;

	if (!InstallHostCall()) {
		return NULL;
	}

	if (priv == NULL)
		return NULL;
	c = (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXTEXT, priv->cur_context, &format);
	TRACE(("OSMesaCreateContextExt(priv=0x%x, format=0x%x, depth=%d, stencil=%d, accum=%d, sharelist=%u) -> %u\n", (unsigned int)priv, format, depthBits, stencilBits, accumBits, CTX_TO_IDX(sharelist), CTX_TO_IDX(c)));
	return c;
}

OSMesaContext APIENTRY internal_OSMesaCreateContextAttribs(gl_private *priv, const GLint *attribList, OSMesaContext sharelist)
{
	OSMesaContext c;

	if (!InstallHostCall()) {
		return NULL;
	}

	if (priv == NULL)
		return NULL;
	c = (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESACREATECONTEXTATTRIBS, priv->cur_context, &attribList);
	TRACE(("OSMesaCreateContextAttribs(priv=0x%x, attrib=0x%x, sharelist=%u) -> %u\n", (unsigned int)priv, (unsigned int)attribList, CTX_TO_IDX(sharelist), CTX_TO_IDX(c)));
	return c;
}

void APIENTRY internal_OSMesaDestroyContext(gl_private *priv, OSMesaContext ctx)
{
	TRACE(("OSMesaDestroyContext(priv=0x%x, ctx=%u)\n", (unsigned int)priv, CTX_TO_IDX(ctx)));
	if (priv == NULL)
		return;
	(*HostCall_p)(NFOSMESA_OSMESADESTROYCONTEXT, priv->cur_context, &ctx);
	freeglGetString(priv, ctx);
	if (ctx == priv->cur_context)
		priv->cur_context = 0;
}

GLboolean APIENTRY internal_OSMesaMakeCurrent(gl_private *priv, OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height)
{
	GLboolean ret;
	TRACE(("OSMesaMakeCurrent(priv=0x%x, ctx=%u, buffer=0x%x, type=0x%x, width=%d, height=%d)\n", (unsigned int)priv, CTX_TO_IDX(ctx), (unsigned int)buffer, type, width, height));
	ret = (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAMAKECURRENT, priv->cur_context, &ctx);
	if (ret)
		priv->cur_context = ctx;
	return ret;
}

OSMesaContext APIENTRY internal_OSMesaGetCurrentContext(gl_private *priv)
{
#if 0
	/*
	 * wrong; the host manages his current context for all processes using NFOSMesa;
	 * return local copy instead
	 */
	return (OSMesaContext)(*HostCall_p)(NFOSMESA_OSMESAGETCURRENTCONTEXT, priv->cur_context, NULL);
#else
	return priv->cur_context;
#endif
}

void APIENTRY internal_OSMesaPixelStore(gl_private *priv, GLint pname, GLint value)
{
	TRACE(("OSMesaPixelStore(priv=0x%x, name=0x%x, value=%d)\n", (unsigned int)priv, pname, value));
	(*HostCall_p)(NFOSMESA_OSMESAPIXELSTORE, priv->cur_context, &pname);
}

void APIENTRY internal_OSMesaGetIntegerv(gl_private *priv, GLint pname, GLint *value)
{
	TRACE(("OSMesaGetIntegerv(priv=0x%x, name=0x%x)\n", (unsigned int)priv, pname));
	(*HostCall_p)(NFOSMESA_OSMESAGETINTEGERV, priv->cur_context, &pname);
}

GLboolean APIENTRY internal_OSMesaGetDepthBuffer(gl_private *priv, OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	TRACE(("OSMesaGetDepthBuffer(priv=0x%x, ctx=%u)\n", (unsigned int)priv, CTX_TO_IDX(c)));
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAGETDEPTHBUFFER, priv->cur_context, &c);
}

GLboolean APIENTRY internal_OSMesaGetColorBuffer(gl_private *priv, OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	TRACE(("OSMesaGetColorBuffer(priv=0x%x, ctx=%u)\n", (unsigned int)priv, CTX_TO_IDX(c)));
	return (GLboolean)(*HostCall_p)(NFOSMESA_OSMESAGETCOLORBUFFER, priv->cur_context, &c);
}

OSMESAproc APIENTRY internal_OSMesaGetProcAddress(gl_private *priv, const char *funcName)
{
	TRACE(("OSMesaGetProcAddress(%s)\n", funcName));
	long func = (*HostCall_p)(NFOSMESA_OSMESAGETPROCADDRESS, priv->cur_context, &funcName);
	if (func == 0 || func > NFOSMESA_LAST)
		return 0;
	return (OSMESAproc)func;
}

void APIENTRY internal_OSMesaColorClamp(gl_private *priv, GLboolean32 enable)
{
	TRACE(("OSMesaColorClamp(%d)\n", enable));
	(*HostCall_p)(NFOSMESA_OSMESACOLORCLAMP, priv->cur_context, &enable);
}

void APIENTRY internal_OSMesaPostprocess(gl_private *priv, OSMesaContext osmesa, const char *filter, GLuint enable_value)
{
	TRACE(("OSMesaPostProcess(ctx=%u, filter=%s, enable=%d)\n", CTX_TO_IDX(osmesa), filter, enable_value));
	(*HostCall_p)(NFOSMESA_OSMESAPOSTPROCESS, priv->cur_context, &osmesa);
}


void internal_glInit(gl_private *priv)
{
	unsigned int i, j;
	
	TRACE(("glInit(priv=0x%x)\n", (unsigned int)priv));
	if (!priv)
		return;
	priv->cur_context = 0;
	priv->gl_exception = 0;
	for (i = 0; i < MAX_OSMESA_CONTEXTS; i++)
	{
		priv->contexts[i].oldmesa_buffer = 0;
		for (j = 0; j < sizeof(priv->contexts[i].gl_strings) / sizeof(priv->contexts[i].gl_strings[0]); j++)
			priv->contexts[i].gl_strings[j] = 0;
	}
	InstallHostCall();
}


#if DBG_TRACE
void osmesa_trace(const char *format, ...)
{
	static long nf_stderr;
	va_list args;
	int arg1;
	int arg2;
	int arg3;
	int arg4;
	int arg5;

	if (nf_stderr == 0)
	{
		nfOps = nf_init();
		if (nfOps == 0)
			return;
		nf_stderr = nfOps->get_id("DEBUGPRINTF");
		if (nf_stderr == 0)
			return;
	}
	va_start(args, format);
	arg1 = va_arg(args, int);
	arg2 = va_arg(args, int);
	arg3 = va_arg(args, int);
	arg4 = va_arg(args, int);
	arg5 = va_arg(args, int);
	va_end(args);
	nfOps->call(nf_stderr | 0, format, arg1, arg2, arg3, arg4, arg5);
}
#endif
