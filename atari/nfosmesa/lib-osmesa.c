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

#include <GL/gl.h>

#include "lib-osmesa.h"
#include "lib-misc.h"
#include "../natfeat/natfeat.h"
#include "../nfpci/nfpci_cookie.h"
#include "nfosmesa_nfapi.h"

/*--- Defines ---*/

#ifndef C___NF
#define C___NF	0x5f5f4e46L
#endif

/*--- Local variables ---*/

unsigned long nfOSMesaId=0;
OSMesaContext cur_context=0;

/*--- OSMesa functions redirectors ---*/

static void InitNatfeat(void)
{
	unsigned long dummy;

	if (!cookie_present(C___NF, &dummy)) {
		Cconws("__NF cookie not present on this system\r\n");
		return;
	}

	nfOSMesaId=nfGetID(("OSMESA"));
	if (nfOSMesaId==0) {
		Cconws("NF OSMesa functions not present on this system\r\n");
	}
}

OSMesaContext OSMesaCreateContext( GLenum format, OSMesaContext sharelist )
{
	if (nfOSMesaId==0) {
		InitNatfeat();
		if (nfOSMesaId==0) {
			return NULL;
		}
	}

	cur_context=nfCall((NFOSMESA(NFOSMESA_OSMESACREATECONTEXT), format, sharelist));
	return cur_context;
}

OSMesaContext OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist)
{
	if (nfOSMesaId==0) {
		InitNatfeat();
		if (nfOSMesaId==0) {
			return NULL;
		}
	}

	cur_context=nfCall((NFOSMESA(NFOSMESA_OSMESACREATECONTEXTEXT), format, depthBits, stencilBits, accumBits, sharelist));
	return cur_context;
}

void OSMesaDestroyContext( OSMesaContext ctx )
{
	nfCall((NFOSMESA(NFOSMESA_OSMESADESTROYCONTEXT), ctx));
	freeglGetString();
}

GLboolean OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height )
{
	cur_context=ctx;
	return nfCall((NFOSMESA(NFOSMESA_OSMESAMAKECURRENT),ctx, buffer, type, width, height));
}

OSMesaContext OSMesaGetCurrentContext( void )
{
	return nfCall((NFOSMESA(NFOSMESA_OSMESAGETCURRENTCONTEXT)));
}

void OSMesaPixelStore( GLint pname, GLint value )
{
	nfCall((NFOSMESA(NFOSMESA_OSMESAPIXELSTORE),cur_context,pname, value));
}

void OSMesaGetIntegerv( GLint pname, GLint *value )
{
	nfCall((NFOSMESA(NFOSMESA_OSMESAGETINTEGERV),cur_context,pname, value));
}

GLboolean OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	return nfCall((NFOSMESA(NFOSMESA_OSMESAGETDEPTHBUFFER),c, width, height, bytesPerValue, buffer));
}

GLboolean OSMesaGetColorBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	return nfCall((NFOSMESA(NFOSMESA_OSMESAGETCOLORBUFFER),c, width, height, format, buffer));
}

void *OSMesaGetProcAddress( const char *funcName )
{
	return nfCall((NFOSMESA(NFOSMESA_OSMESAGETPROCADDRESS),funcName));
}
