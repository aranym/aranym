/*
	NatFeat host OSMesa rendering

	ARAnyM (C) 2004,2005 Patrice Mandin

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

#define __STDC_FORMAT_MACROS

#ifdef OS_darwin
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "SDL_compat.h"
#include <SDL_loadso.h>
#include <SDL_endian.h>
#include <math.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfosmesa.h"
#include "../../atari/nfosmesa/nfosmesa_nfapi.h"
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifndef PRId64
#  define PRId64 "lld"
#endif
#ifndef PRIu64
#  define PRIu64 "llu"
#endif
#if SIZEOF_VOID_P >= 8
#  define PRI_IPTR PRIu64
#  define PRI_PTR "p"
#else
#  define PRI_IPTR "u"
#  define PRI_PTR "p"
#endif

#define DEBUG 0
#include "debug.h"
#include "verify.h"

/*--- Assumptions ---*/

/* these native types must match the Atari types */
verify(sizeof(GLshort) == 2);
verify(sizeof(GLushort) == 2);
verify(sizeof(GLint) == 4);
verify(sizeof(GLuint) == 4);
verify(sizeof(GLenum) == 4);
verify(sizeof(GLsizei) == 4);
verify(sizeof(GLfixed) == 4);
verify(sizeof(GLfloat) == ATARI_SIZEOF_FLOAT);
verify(sizeof(GLint64) == 8);
verify(sizeof(GLuint64) == 8);

/*--- Defines ---*/

#define TOS_ENOSYS -32

#define M(m,row,col)  m[col*4+row]

#ifndef GL_VIEWPORT_BOUNDS_RANGE
#define GL_VIEWPORT_BOUNDS_RANGE          0x825D
#endif
#ifdef GL_DEPTH_BOUNDS_EXT
#define GL_DEPTH_BOUNDS_EXT               0x8891
#endif
#ifndef GL_POINT_DISTANCE_ATTENUATION
#define GL_POINT_DISTANCE_ATTENUATION_ARB 0x8129
#endif
#ifndef GL_CURRENT_RASTER_SECONDARY_COLOR
#define GL_CURRENT_RASTER_SECONDARY_COLOR 0x845F
#endif
#ifndef GL_RGBA_SIGNED_COMPONENTS_EXT
#define GL_RGBA_SIGNED_COMPONENTS_EXT     0x8C3C
#endif
#ifndef GL_NUM_COMPRESSED_TEXTURE_FORMATS
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#endif
#ifndef GL_COMPRESSED_TEXTURE_FORMATS
#define GL_COMPRESSED_TEXTURE_FORMATS     0x86A3
#endif
#ifndef GL_TRANSPOSE_MODELVIEW_MATRIX
#define GL_TRANSPOSE_MODELVIEW_MATRIX     0x84E3
#endif
#ifndef GL_TRANSPOSE_PROJECTION_MATRIX
#define GL_TRANSPOSE_PROJECTION_MATRIX    0x84E4
#endif
#ifndef GL_TRANSPOSE_TEXTURE_MATRIX
#define GL_TRANSPOSE_TEXTURE_MATRIX       0x84E5
#endif
#ifndef GL_TRANSPOSE_COLOR_MATRIX
#define GL_TRANSPOSE_COLOR_MATRIX         0x84E6
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL                  0x84F9
#endif
#ifndef GL_RG
#define GL_RG                             0x8227
#endif
#ifndef GL_CONSTANT_COLOR0_NV
#define GL_CONSTANT_COLOR0_NV             0x852A
#endif
#ifndef GL_CONSTANT_COLOR1_NV
#define GL_CONSTANT_COLOR1_NV             0x852B
#endif
#ifndef GL_CULL_VERTEX_EYE_POSITION_EXT
#define GL_CULL_VERTEX_EYE_POSITION_EXT   0x81AB
#endif
#ifndef GL_CULL_VERTEX_OBJECT_POSITION_EXT
#define GL_CULL_VERTEX_OBJECT_POSITION_EXT 0x81AC
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER                   0x8892
#endif
#ifndef GL_ATOMIC_COUNTER_BUFFER
#define GL_ATOMIC_COUNTER_BUFFER          0x92C0
#endif
#ifndef GL_COPY_READ_BUFFER
#define GL_COPY_READ_BUFFER               0x8F36
#endif
#ifndef GL_COPY_WRITE_BUFFER
#define GL_COPY_WRITE_BUFFER              0x8F37
#endif
#ifndef GL_DISPATCH_INDIRECT_BUFFER
#define GL_DISPATCH_INDIRECT_BUFFER       0x90EE
#endif
#ifndef GL_DRAW_INDIRECT_BUFFER
#define GL_DRAW_INDIRECT_BUFFER           0x8F3F
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#endif
#ifndef GL_PIXEL_PACK_BUFFER
#define GL_PIXEL_PACK_BUFFER              0x88EB
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER            0x88EC
#endif
#ifndef GL_QUERY_BUFFER
#define GL_QUERY_BUFFER                   0x9192
#endif
#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#endif
#ifndef GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER                 0x8C2A
#endif
#ifndef GL_TRANSFORM_FEEDBACK_BUFFER
#define GL_TRANSFORM_FEEDBACK_BUFFER      0x8C8E
#endif
#ifndef GL_UNIFORM_BUFFER
#define GL_UNIFORM_BUFFER                 0x8A11
#endif
#ifndef GL_SECONDARY_COLOR_ARRAY
#define GL_SECONDARY_COLOR_ARRAY          0x845E
#endif
#ifndef GL_FIXED
#define GL_FIXED                          0x140C
#endif
#ifndef GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX
#define GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX 0x840A
#endif
#ifndef GL_TEXTURE_CLIPMAP_CENTER_SGIX
#define GL_TEXTURE_CLIPMAP_CENTER_SGIX    0x8171
#endif
#ifndef GL_TEXTURE_CLIPMAP_OFFSET_SGIX
#define GL_TEXTURE_CLIPMAP_OFFSET_SGIX    0x8173
#endif
#ifndef GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX
#define GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX 0x8174
#endif
#ifndef GL_POST_TEXTURE_FILTER_BIAS_SGIX
#define GL_POST_TEXTURE_FILTER_BIAS_SGIX  0x8179
#endif
#ifndef GL_POST_TEXTURE_FILTER_SCALE_SGIX
#define GL_POST_TEXTURE_FILTER_SCALE_SGIX 0x817A
#endif
#ifndef GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES 0x92C6
#endif
#ifndef GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS 0x92C5
#endif
#ifndef GL_NUM_COMPATIBLE_SUBROUTINES
#define GL_NUM_COMPATIBLE_SUBROUTINES     0x8E4A
#endif
#ifndef GL_COMPATIBLE_SUBROUTINES
#define GL_COMPATIBLE_SUBROUTINES         0x8E4B
#endif
#ifndef GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS  0x8A42
#endif
#ifndef GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES 0x8A43
#endif
#ifndef GL_ARRAY_OBJECT_BUFFER_ATI
#define GL_ARRAY_OBJECT_BUFFER_ATI        0x8766
#endif
#ifndef GL_OBJECT_BUFFER_SIZE_ATI
#define GL_OBJECT_BUFFER_SIZE_ATI         0x8764
#endif
#ifndef GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS
#define GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS 0x809C
#endif
#ifndef GL_COMBINER_MAPPING_NV
#define GL_COMBINER_MAPPING_NV            0x8543
#endif
#ifndef GL_COMBINER_COMPONENT_USAGE_NV
#define GL_COMBINER_COMPONENT_USAGE_NV    0x8544
#endif
#ifndef GL_COMBINER_INPUT_NV
#define GL_COMBINER_INPUT_NV              0x8542
#endif
#ifndef GL_FOG_FUNC_POINTS_SGIS
#define GL_FOG_FUNC_POINTS_SGIS           0x812B
#endif
#ifndef GL_MAX_FOG_FUNC_POINTS_SGIS
#define GL_MAX_FOG_FUNC_POINTS_SGIS       0x812C
#endif
#ifndef GL_CURRENT_RASTER_NORMAL_SGIX
#define GL_CURRENT_RASTER_NORMAL_SGIX     0x8406
#endif
#ifndef GL_EVAL_2D_NV
#define GL_EVAL_2D_NV                     0x86C0
#endif
#ifndef GL_EVAL_TRIANGULAR_2D_NV
#define GL_EVAL_TRIANGULAR_2D_NV          0x86C1
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB0_NV
#define GL_EVAL_VERTEX_ATTRIB0_NV         0x86C6
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB1_NV
#define GL_EVAL_VERTEX_ATTRIB1_NV         0x86C7
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB2_NV
#define GL_EVAL_VERTEX_ATTRIB2_NV         0x86C8
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB3_NV
#define GL_EVAL_VERTEX_ATTRIB3_NV         0x86C9
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB4_NV
#define GL_EVAL_VERTEX_ATTRIB4_NV         0x86CA
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB5_NV
#define GL_EVAL_VERTEX_ATTRIB5_NV         0x86CB
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB6_NV
#define GL_EVAL_VERTEX_ATTRIB6_NV         0x86CC
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB7_NV
#define GL_EVAL_VERTEX_ATTRIB7_NV         0x86CD
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB8_NV
#define GL_EVAL_VERTEX_ATTRIB8_NV         0x86CE
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB9_NV
#define GL_EVAL_VERTEX_ATTRIB9_NV         0x86CE
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB10_NV
#define GL_EVAL_VERTEX_ATTRIB10_NV        0x86D0
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB11_NV
#define GL_EVAL_VERTEX_ATTRIB11_NV        0x86D1
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB12_NV
#define GL_EVAL_VERTEX_ATTRIB12_NV        0x86D2
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB13_NV
#define GL_EVAL_VERTEX_ATTRIB13_NV        0x86D3
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB14_NV
#define GL_EVAL_VERTEX_ATTRIB14_NV        0x86D4
#endif
#ifndef GL_EVAL_VERTEX_ATTRIB15_NV
#define GL_EVAL_VERTEX_ATTRIB15_NV        0x86D5
#endif
#ifndef GL_UTF8_NV
#define GL_UTF8_NV                        0x909A
#endif
#ifndef GL_UTF16_NV
#define GL_UTF16_NV                       0x909B
#endif
#ifndef GL_TRANSLATE_X_NV
#define GL_TRANSLATE_X_NV                 0x908E
#endif
#ifndef GL_TRANSLATE_Y_NV
#define GL_TRANSLATE_Y_NV                 0x908F
#endif
#ifndef GL_TRANSLATE_2D_NV
#define GL_TRANSLATE_2D_NV                0x9090
#endif
#ifndef GL_TRANSLATE_3D_NV
#define GL_TRANSLATE_3D_NV                0x9091
#endif
#ifndef GL_PROJECTIVE_2D_NV
#define GL_PROJECTIVE_2D_NV               0x9093
#endif
#ifndef GL_AFFINE_2D_NV
#define GL_AFFINE_2D_NV                   0x9092
#endif
#ifndef GL_PROJECTIVE_2D_NV
#define GL_PROJECTIVE_2D_NV               0x9093
#endif
#ifndef GL_AFFINE_3D_NV
#define GL_AFFINE_3D_NV                   0x9094
#endif
#ifndef GL_PROJECTIVE_3D_NV
#define GL_PROJECTIVE_3D_NV               0x9095
#endif
#ifndef GL_TRANSPOSE_AFFINE_2D_NV
#define GL_TRANSPOSE_AFFINE_2D_NV         0x9096
#endif
#ifndef GL_TRANSPOSE_PROJECTIVE_2D_NV
#define GL_TRANSPOSE_PROJECTIVE_2D_NV     0x9097
#endif
#ifndef GL_TRANSPOSE_AFFINE_3D_NV
#define GL_TRANSPOSE_AFFINE_3D_NV         0x9098
#endif
#ifndef GL_TRANSPOSE_PROJECTIVE_3D_NV
#define GL_TRANSPOSE_PROJECTIVE_3D_NV     0x9099
#endif
#ifndef GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV
#define GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV 0x909C
#endif
#ifndef GL_PATH_COMMAND_COUNT_NV
#define GL_PATH_COMMAND_COUNT_NV          0x909D
#endif
#ifndef GL_PATH_COORD_COUNT_NV
#define GL_PATH_COORD_COUNT_NV            0x909E
#endif
#ifndef GL_PATH_DASH_ARRAY_COUNT_NV
#define GL_PATH_DASH_ARRAY_COUNT_NV       0x909F
#endif
#ifndef GL_PATH_COMPUTED_LENGTH_NV
#define GL_PATH_COMPUTED_LENGTH_NV        0x90A0
#endif
#ifndef GL_PATH_FILL_BOUNDING_BOX_NV
#define GL_PATH_FILL_BOUNDING_BOX_NV      0x90A1
#endif
#ifndef GL_PATH_STROKE_BOUNDING_BOX_NV
#define GL_PATH_STROKE_BOUNDING_BOX_NV    0x90A2
#endif
#ifndef GL_PATH_OBJECT_BOUNDING_BOX_NV
#define GL_PATH_OBJECT_BOUNDING_BOX_NV    0x908A
#endif
#ifndef GL_PATH_GEN_COEFF_NV
#define GL_PATH_GEN_COEFF_NV              0x90B1
#endif
#ifndef GL_COUNTER_TYPE_AMD
#define GL_COUNTER_TYPE_AMD               0x8BC0
#endif
#ifndef GL_COUNTER_RANGE_AMD
#define GL_COUNTER_RANGE_AMD              0x8BC1
#endif
#ifndef GL_UNSIGNED_INT64_AMD
#define GL_UNSIGNED_INT64_AMD             0x8BC2
#endif
#ifndef GL_PERCENTAGE_AMD
#define GL_PERCENTAGE_AMD                 0x8BC3
#endif
#ifndef GL_PERFMON_RESULT_AVAILABLE_AMD
#define GL_PERFMON_RESULT_AVAILABLE_AMD   0x8BC4
#endif
#ifndef GL_PERFMON_RESULT_SIZE_AMD
#define GL_PERFMON_RESULT_SIZE_AMD        0x8BC5
#endif
#define GL_PERFMON_RESULT_AMD             0x8BC6
#ifndef GL_PERFMON_RESULT_AMD
#endif
#ifndef GL_CURRENT_VERTEX_ATTRIB
#define GL_CURRENT_VERTEX_ATTRIB          0x8626
#endif
#ifndef GL_DISPATCH_INDIRECT_BUFFER
#define GL_DISPATCH_INDIRECT_BUFFER       0x90EE
#endif
#ifndef GL_DISPATCH_INDIRECT_BUFFER_BINDING
#define GL_DISPATCH_INDIRECT_BUFFER_BINDING 0x90EF
#endif
#ifndef GL_SHADER_BINARY_FORMATS
#define GL_SHADER_BINARY_FORMATS          0x8DF8
#endif
#ifndef GL_NUM_SHADER_BINARY_FORMATS
#define GL_NUM_SHADER_BINARY_FORMATS      0x8DF9
#endif

/*--- Types ---*/

typedef struct {
#define GL_PROC(type, gl, name, export, upper, params, first, ret) type (APIENTRY *gl ## name) params ;
#define GLU_PROC(type, gl, name, export, upper, params, first, ret)
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) type (APIENTRY *OSMesa ## name) params ;
#include "../../atari/nfosmesa/glfuncs.h"
} osmesa_funcs;

/*--- Variables ---*/

static osmesa_funcs fn;

/*--- Constructor/Destructor ---*/

OSMesaDriver::OSMesaDriver()
{
	D(bug("nfosmesa: OSMesaDriver()"));
	memset(contexts, 0, sizeof(contexts));
	num_contexts = 0;
	cur_context = 0;
	libgl_handle = libosmesa_handle = NULL;
}

OSMesaDriver::~OSMesaDriver()
{
	int i;

	D(bug("nfosmesa: ~OSMesaDriver()"));
	for (i=1;i<=MAX_OSMESA_CONTEXTS;i++) {
		if (contexts[i].ctx) {
			OSMesaDestroyContext(i);
			contexts[i].ctx=NULL;
		}
	}
	num_contexts=0;
	cur_context = 0;

	CloseLibrary();
}

/*--- Public functions ---*/

int32 OSMesaDriver::dispatch(uint32 fncode)
{
	int32 ret = 0;
	Uint32 *ctx_ptr;	/* Current parameter list */

#define Host2AtariIntPtr   Atari2HostIntPtr
#define Host2AtariShortPtr Atari2HostShortPtr
#define Host2AtariInt64Ptr Atari2HostInt64Ptr

	/* Read parameter on m68k stack */
#define getStackedParameter(n) SDL_SwapBE32(ctx_ptr[n])
#define getStackedParameter64(n) SDL_SwapBE64(*((Uint64 *)&ctx_ptr[n]))
#define getStackedFloat(n) Atari2HostFloat(getStackedParameter(n))
#define getStackedDouble(n) Atari2HostDouble(getStackedParameter(n), getStackedParameter(n + 1))
#define getStackedPointer(n) (ctx_ptr[n] ? Atari2HostAddr(getStackedParameter(n)) : NULL)

	/* undo the effects of Atari2HostAddr for pointer arguments when they specify a buffer offset */
#define Host2AtariAddr(a) ((void *)((uintptr_t)(a) - MEMBaseDiff))

	if (fncode != NFOSMESA_OSMESAPOSTPROCESS && fncode != GET_VERSION)
	{
		/*
		 * OSMesaPostprocess() cannot be called after OSMesaMakeCurrent().
		 * FIXME: this will fail if ARAnyM already has a current context
		 * that was created by a different MiNT process
		 */
		SelectContext(getParameter(0));
	}
	ctx_ptr = (Uint32 *)Atari2HostAddr(getParameter(1));

	switch(fncode) {
		case GET_VERSION:
    		ret = ARANFOSMESA_NFAPI_VERSION;
			break;
		case NFOSMESA_LENGLGETSTRING:
			ret = LenglGetString(getStackedParameter(0),getStackedParameter(1));
			break;
		case NFOSMESA_PUTGLGETSTRING:
			PutglGetString(getStackedParameter(0),getStackedParameter(1),(GLubyte *)getStackedPointer(2));
			break;
		case NFOSMESA_LENGLGETSTRINGI:
			ret = LenglGetStringi(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2));
			break;
		case NFOSMESA_PUTGLGETSTRINGI:
			PutglGetStringi(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2),(GLubyte *)getStackedPointer(3));
			break;

		case NFOSMESA_OSMESACREATECONTEXT:
			ret = OSMesaCreateContext(getStackedParameter(0),getStackedParameter(1));
			break;
		case NFOSMESA_OSMESACREATECONTEXTEXT:
			ret = OSMesaCreateContextExt(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2),getStackedParameter(3),getStackedParameter(4));
			break;
		case NFOSMESA_OSMESADESTROYCONTEXT:
			OSMesaDestroyContext(getStackedParameter(0));
			break;
		case NFOSMESA_OSMESAMAKECURRENT:
			ret = OSMesaMakeCurrent(getStackedParameter(0),getStackedParameter(1),getStackedParameter(2),getStackedParameter(3),getStackedParameter(4));
			break;
		case NFOSMESA_OSMESAGETCURRENTCONTEXT:
			ret = OSMesaGetCurrentContext();
			break;
		case NFOSMESA_OSMESAPIXELSTORE:
			OSMesaPixelStore(getStackedParameter(0),getStackedParameter(1));
			break;
		case NFOSMESA_OSMESAGETINTEGERV:
			OSMesaGetIntegerv(getStackedParameter(0), (GLint *)getStackedPointer(1));
			break;
		case NFOSMESA_OSMESAGETDEPTHBUFFER:
			ret = OSMesaGetDepthBuffer(getStackedParameter(0), (GLint *)getStackedPointer(1), (GLint *)getStackedPointer(2), (GLint *)getStackedPointer(3), (void **)getStackedPointer(4));
			break;
		case NFOSMESA_OSMESAGETCOLORBUFFER:
			ret = OSMesaGetColorBuffer(getStackedParameter(0), (GLint *)getStackedPointer(1), (GLint *)getStackedPointer(2), (GLint *)getStackedPointer(3), (void **)getStackedPointer(4));
			break;
		case NFOSMESA_OSMESAGETPROCADDRESS:
			/* FIXME: Native side do not need this */
			ret = (int32) (0 != OSMesaGetProcAddress((const char *)getStackedPointer(0)));
			break;
		case NFOSMESA_OSMESACOLORCLAMP:
			if (!fn.OSMesaColorClamp || GL_ISNOP(fn.OSMesaColorClamp))
				ret = TOS_ENOSYS;
			else
				OSMesaColorClamp(getStackedParameter(0));
			break;
		case NFOSMESA_OSMESAPOSTPROCESS:
			if (!fn.OSMesaPostprocess || GL_ISNOP(fn.OSMesaPostprocess))
				ret = TOS_ENOSYS;
			else
				OSMesaPostprocess(getStackedParameter(0), (const char *)getStackedPointer(1), getStackedParameter(2));
			break;

		/*
		 * maybe FIXME: functions below usually need a current context,
		 * which is not checked here.
		 * Also, if some dumb program tries to call any of these
		 * without ever creating a context, OpenLibrary() will not have been called yet,
		 * and crash ARAnyM because the fn ptrs have not been initialized,
		 * but that would require to call OpenLibrary for every function call.
		 */
		case NFOSMESA_GLULOOKATF:
			nfgluLookAtf(
				getStackedFloat(0) /* GLfloat eyeX */,
				getStackedFloat(1) /* GLfloat eyeY */,
				getStackedFloat(2) /* GLfloat eyeZ */,
				getStackedFloat(3) /* GLfloat centerX */,
				getStackedFloat(4) /* GLfloat centerY */,
				getStackedFloat(5) /* GLfloat centerZ */,
				getStackedFloat(6) /* GLfloat upX */,
				getStackedFloat(7) /* GLfloat upY */,
				getStackedFloat(8) /* GLfloat upZ */);
			break;
		
		case NFOSMESA_GLFRUSTUMF:
			nfglFrustumf(
				getStackedFloat(0) /* GLfloat left */,
				getStackedFloat(1) /* GLfloat right */,
				getStackedFloat(2) /* GLfloat bottom */,
				getStackedFloat(3) /* GLfloat top */,
				getStackedFloat(4) /* GLfloat near_val */,
				getStackedFloat(5) /* GLfloat far_val */);
			break;
		
		case NFOSMESA_GLORTHOF:
			nfglOrthof(
				getStackedFloat(0) /* GLfloat left */,
				getStackedFloat(1) /* GLfloat right */,
				getStackedFloat(2) /* GLfloat bottom */,
				getStackedFloat(3) /* GLfloat top */,
				getStackedFloat(4) /* GLfloat near_val */,
				getStackedFloat(5) /* GLfloat far_val */);
			break;
		
		case NFOSMESA_TINYGLSWAPBUFFER:
			nftinyglswapbuffer(getStackedParameter(0));
			break;
		
#include "nfosmesa/dispatch-gl.c"

		case NFOSMESA_ENOSYS:
		default:
			D(bug("nfosmesa: unimplemented function #%d", fncode));
			ret = TOS_ENOSYS;
			break;
	}
/*	D(bug("nfosmesa: function returning with 0x%08x", ret));*/
	return ret;
}

/*--- Protected functions ---*/

int OSMesaDriver::OpenLibrary(void)
{
#ifdef OS_darwin
	char exedir[MAXPATHLEN];
	char curdir[MAXPATHLEN];
	getcwd(curdir, MAXPATHLEN);
	CFURLRef url = CFBundleCopyExecutableURL(CFBundleGetMainBundle());
	CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
	CFURLGetFileSystemRepresentation(url2, false, (UInt8 *)exedir, MAXPATHLEN);
	CFRelease(url2);
	CFRelease(url);
#endif
	bool libgl_needed = false;
	
	D(bug("nfosmesa: OpenLibrary()"));

	/* Check if channel size is correct */
	switch(bx_options.osmesa.channel_size) {
		case 16:
		case 32:
		case 8:
		case 0:
			break;
		default:
			D(bug("nfosmesa: bogus Channel size: %d", bx_options.osmesa.channel_size));
			bx_options.osmesa.channel_size = 0;
			break;
	}

	/* Load libOSMesa */
	if (libosmesa_handle==NULL) {
		libosmesa_handle=SDL_LoadObject(bx_options.osmesa.libosmesa);
#ifdef OS_darwin
		/* If loading failed, try to load from executable directory */
		if (libosmesa_handle==NULL) {
			chdir(exedir);
			libosmesa_handle=SDL_LoadObject(bx_options.osmesa.libosmesa);
			chdir(curdir);
		}
#endif
		if (libosmesa_handle==NULL) {
			D(bug("nfosmesa: Can not load '%s' library", bx_options.osmesa.libosmesa));
			panicbug("nfosmesa: %s: %s", bx_options.osmesa.libosmesa, SDL_GetError());
			return -1;
		}
		InitPointersOSMesa(libosmesa_handle);
		InitPointersGL(libosmesa_handle);
		D(bug("nfosmesa: OpenLibrary(): libOSMesa loaded"));
		if (GL_ISNOP(fn.glBegin)) {
			libgl_needed = true;
			D(bug("nfosmesa: Channel size: %d -> libGL separated from libOSMesa", bx_options.osmesa.channel_size));
		} else {
			D(bug("nfosmesa: Channel size: %d -> libGL included in libOSMesa", bx_options.osmesa.channel_size));
		}
	}

	/* Load LibGL if needed */
	if ((libgl_handle==NULL) && libgl_needed) {
		libgl_handle=SDL_LoadObject(bx_options.osmesa.libgl);
#ifdef OS_darwin
		/* If loading failed, try to load from executable directory */
		if (libgl_handle==NULL) {
			chdir(exedir);
			libgl_handle=SDL_LoadObject(bx_options.osmesa.libgl);
			chdir(curdir);
		}
#endif
		if (libgl_handle==NULL) {
			D(bug("nfosmesa: Can not load '%s' library", bx_options.osmesa.libgl));
			panicbug("nfosmesa: %s: %s", bx_options.osmesa.libgl, SDL_GetError());
			return -1;
		}
		InitPointersGL(libgl_handle);
		D(bug("nfosmesa: OpenLibrary(): libGL loaded"));
	}

	return 0;
}

int OSMesaDriver::CloseLibrary(void)
{
	D(bug("nfosmesa: CloseLibrary()"));

	if (libosmesa_handle) {
		SDL_UnloadObject(libosmesa_handle);
		libosmesa_handle=NULL;
	}

/* nullify OSMesa functions */
#define GL_PROC(type, gl, name, export, upper, params, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) fn.OSMesa ## name = (type (APIENTRY *) params) 0;
#include "../../atari/nfosmesa/glfuncs.h"

	D(bug("nfosmesa: CloseLibrary(): libOSMesa unloaded"));

	if (libgl_handle) {
		SDL_UnloadObject(libgl_handle);
		libgl_handle=NULL;
	}
	
/* nullify GL functions */
#define GL_PROC(type, gl, name, export, upper, params, first, ret) fn.gl ## name = (type (APIENTRY *) params) 0;
#define GLU_PROC(type, gl, name, export, upper, params, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"

	D(bug("nfosmesa: CloseLibrary(): libGL unloaded"));

	return 0;
}

void *APIENTRY OSMesaDriver::glNop(void)
{
	return 0;
}

void OSMesaDriver::InitPointersGL(void *handle)
{
	D(bug("nfosmesa: InitPointersGL()"));

#define GL_PROC(type, gl, name, export, upper, params, first, ret) \
	fn.gl ## name = (type (APIENTRY *) params) SDL_LoadFunction(handle, "gl" #name); \
	if (fn.gl ## name == 0) fn.gl ## name = (type (APIENTRY *) params)glNop;
#define GLU_PROC(type, gl, name, export, upper, params, first, ret)
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret)
#include "../../atari/nfosmesa/glfuncs.h"
}

void OSMesaDriver::InitPointersOSMesa(void *handle)
{
	D(bug("nfosmesa: InitPointersOSMesa()"));

#define GL_PROC(type, gl, name, export, upper, params, first, ret) 
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret) \
	fn.OSMesa ## name = (type (APIENTRY *) params) SDL_LoadFunction(handle, "OSMesa" #name); \
	if (fn.OSMesa ## name == 0) fn.OSMesa ## name = (type (APIENTRY *) params)glNop;
#include "../../atari/nfosmesa/glfuncs.h"
}

bool OSMesaDriver::SelectContext(Uint32 ctx)
{
	void *draw_buffer;
	bool ret = true;
	
	if (ctx>MAX_OSMESA_CONTEXTS) {
		D(bug("nfosmesa: SelectContext: %d out of bounds",ctx));
		return false;
	}
	if (!fn.OSMesaMakeCurrent)
	{
		/* can happen if we did not load the library yet, e.g because no context was created yet */
		if (OpenLibrary() < 0 || !fn.OSMesaMakeCurrent)
			return false;
	}
	if (cur_context != ctx) {
		if (ctx != 0)
		{
			draw_buffer = contexts[ctx].dst_buffer;
			if (contexts[ctx].src_buffer) {
				draw_buffer = contexts[ctx].src_buffer;
			}
			ret = GL_TRUE == fn.OSMesaMakeCurrent(contexts[ctx].ctx, draw_buffer, contexts[ctx].type, contexts[ctx].width, contexts[ctx].height);
		} else
		{
			ret = GL_TRUE == fn.OSMesaMakeCurrent(NULL, NULL, 0, 0, 0);
		}
		if (ret)
		{
			D(bug("nfosmesa: SelectContext: %d is current",ctx));
		} else
		{
			D(bug("nfosmesa: SelectContext: %d failed",ctx));
		}
		cur_context = ctx;
	}
	return ret;
}

Uint32 OSMesaDriver::OSMesaCreateContext( GLenum format, Uint32 sharelist )
{
	D(bug("nfosmesa: OSMesaCreateContext(0x%x, 0x%x)", format, sharelist));
	return OSMesaCreateContextExt(format, 16, 8, (format == OSMESA_COLOR_INDEX) ? 0 : 16, sharelist);
}

Uint32 OSMesaDriver::OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, Uint32 sharelist)
{
	int i,j;
	OSMesaContext share_ctx;
	GLenum osmesa_format;

	D(bug("nfosmesa: OSMesaCreateContextExt(0x%x,%d,%d,%d,0x%08x)",format,depthBits,stencilBits,accumBits,sharelist));

	/* TODO: shared contexts */
	if (sharelist) {
		return 0;
	}

	if (num_contexts==0) {
		if (OpenLibrary()<0) {
			return 0;
		}
		D(bug("nfosmesa: Library loaded"));
	}

	/* Find a free context */
	j=0;
	for (i=1;i<=MAX_OSMESA_CONTEXTS;i++) {
		if (contexts[i].ctx==NULL) {
			j=i;
			break;
		}
	}

	/* Create our host OSMesa context */
	if (j==0) {
		D(bug("nfosmesa: No free context found"));
		return 0;
	}
	memset((void *)&(contexts[j]),0,sizeof(context_t));

	share_ctx = NULL;
	if (sharelist > 0 && sharelist <= MAX_OSMESA_CONTEXTS) {
		share_ctx = contexts[sharelist].ctx;
	}

	/* Select format */
	osmesa_format = format;
	contexts[j].conversion = SDL_FALSE;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	if (format == OSMESA_RGB_565) {
		osmesa_format = OSMESA_RGB_565;
		contexts[j].conversion = SDL_TRUE;
	}
#endif

	if (bx_options.osmesa.channel_size > 8 && (format!=OSMESA_COLOR_INDEX)) {
		/* We are using libOSMesa[16,32] */
		osmesa_format = OSMESA_ARGB;
		contexts[j].conversion = SDL_TRUE;
	}

	contexts[j].enabled_arrays=0;
	contexts[j].render_mode = GL_RENDER;
	contexts[j].error_code = GL_NO_ERROR;

	D(bug("nfosmesa: format:0x%x -> 0x%x, conversion: %s", osmesa_format, format, contexts[j].conversion ? "true" : "false"));
	D(bug("nfosmesa: depth=%d, stencil=%d, accum=%d", depthBits, stencilBits, accumBits));
	contexts[j].ctx=fn.OSMesaCreateContextExt(osmesa_format,depthBits,stencilBits,accumBits,share_ctx);
	if (contexts[j].ctx==NULL) {
		D(bug("nfosmesa: Can not create context"));
		return 0;
	}
	contexts[j].srcformat = osmesa_format;
	contexts[j].dstformat = format;
	contexts[j].src_buffer = contexts[j].dst_buffer = NULL;
	num_contexts++;
	return j;
}

void OSMesaDriver::OSMesaDestroyContext( Uint32 ctx )
{
	D(bug("nfosmesa: OSMesaDestroyContext(%u)", ctx));
	if (ctx>MAX_OSMESA_CONTEXTS || !contexts[ctx].ctx) {
		bug("nfosmesa: OSMesaDestroyContext(%u): invalid context", ctx);
		return;
	}
	
	fn.OSMesaDestroyContext(contexts[ctx].ctx);
	num_contexts--;
	if (contexts[ctx].src_buffer) {
		free(contexts[ctx].src_buffer);
		contexts[ctx].src_buffer = NULL;
	}
	if (contexts[ctx].feedback_buffer_host) {
		free(contexts[ctx].feedback_buffer_host);
		contexts[ctx].feedback_buffer_host = NULL;
	}
	contexts[ctx].feedback_buffer_type = 0;
	if (contexts[ctx].select_buffer_host) {
		free(contexts[ctx].select_buffer_host);
		contexts[ctx].select_buffer_host = NULL;
	}
	contexts[ctx].select_buffer_size = 0;
	contexts[ctx].ctx = NULL;
	if (ctx == cur_context)
		cur_context = 0;
/*
	if (num_contexts==0) {
		CloseLibrary();
	}
*/
}

GLboolean OSMesaDriver::OSMesaMakeCurrent( Uint32 ctx, memptr buffer, GLenum type, GLsizei width, GLsizei height )
{
	void *draw_buffer;
	GLboolean ret;
	
	D(bug("nfosmesa: OSMesaMakeCurrent(%u,$%08x,%d,%d,%d)",ctx,buffer,type,width,height));
	if (ctx>MAX_OSMESA_CONTEXTS) {
		return GL_FALSE;
	}
	
	if (ctx != 0 && (!fn.OSMesaMakeCurrent || !contexts[ctx].ctx)) {
		return GL_FALSE;
	}

	if (ctx != 0)
	{
		contexts[ctx].dst_buffer = draw_buffer = Atari2HostAddr(buffer);
		contexts[ctx].type = type;
		if (bx_options.osmesa.channel_size > 8) {
			if (contexts[ctx].src_buffer) {
				free(contexts[ctx].src_buffer);
			}
			contexts[ctx].src_buffer = draw_buffer = malloc(width * height * 4 * (bx_options.osmesa.channel_size>>3));
			D(bug("nfosmesa: Allocated shadow buffer for channel reduction"));
			switch(bx_options.osmesa.channel_size) {
				case 16:
					contexts[ctx].type = GL_UNSIGNED_SHORT;
					break;
				case 32:
					contexts[ctx].type = GL_FLOAT;
					break;
			}
		} else {
			contexts[ctx].type = GL_UNSIGNED_BYTE;
		}
		contexts[ctx].width = width;
		contexts[ctx].height = height;
		ret = fn.OSMesaMakeCurrent(contexts[ctx].ctx, draw_buffer, contexts[ctx].type, width, height);
	} else
	{
		if (fn.OSMesaMakeCurrent)
			ret = fn.OSMesaMakeCurrent(NULL, NULL, 0, 0, 0);
		else
			ret = GL_TRUE;
	}
	if (ret)
	{
		cur_context = ctx;
		D(bug("nfosmesa: MakeCurrent: %d is current",ctx));
	} else
	{
		D(bug("nfosmesa: MakeCurrent: %d failed",ctx));
	}
	return ret;
}

Uint32 OSMesaDriver::OSMesaGetCurrentContext( void )
{
	Uint32 ctx;
#if 0
	/*
	 * wrong; the host manages his current context for all processes using NFOSMesa;
	 * return interface parameter instead
	 */
	ctx = cur_context;
	D(bug("nfosmesa: OSMesaGetCurrentContext() -> %u", ctx));
#else
	ctx = getParameter(0);
#endif
	return ctx;
}

void OSMesaDriver::OSMesaPixelStore(GLint pname, GLint value )
{
	D(bug("nfosmesa: OSMesaPixelStore(0x%x, %d)", pname, value));
	if (SelectContext(cur_context))
		fn.OSMesaPixelStore(pname, value);
}

void OSMesaDriver::OSMesaGetIntegerv(GLint pname, GLint *value )
{
	GLint tmp = 0;

	D(bug("nfosmesa: OSMesaGetIntegerv(0x%x)", pname));
	if (SelectContext(cur_context))
		fn.OSMesaGetIntegerv(pname, &tmp);
	if (value)
		*value = SDL_SwapBE32(tmp);
}

GLboolean OSMesaDriver::OSMesaGetDepthBuffer(Uint32 c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )
{
	D(bug("nfosmesa: OSMesaGetDepthBuffer"));
	if (!SelectContext(c))
		return GL_FALSE;
	if (width)
		*width = 0;
	if (height)
		*height = 0;
	if (bytesPerValue)
		*bytesPerValue = 0;
	if (buffer)
		*buffer = NULL;	/* Can not return pointer in host memory */
	return GL_FALSE;
}

GLboolean OSMesaDriver::OSMesaGetColorBuffer(Uint32 c, GLint *width, GLint *height, GLint *format, void **buffer )
{
	D(bug("nfosmesa: OSMesaGetColorBuffer(%u)", c));
	if (!SelectContext(c))
		return GL_FALSE;
	if (width)
		*width = contexts[c].width;
	if (height)
		*height = contexts[c].height;
	if (format)
		*format = contexts[c].dstformat;
	if (buffer)
		*buffer = contexts[c].dst_buffer;
	return GL_TRUE;
}

void *OSMesaDriver::OSMesaGetProcAddress( const char *funcName )
{
	D(bug("nfosmesa: OSMesaGetProcAddress(%s)", funcName));
	OpenLibrary();
	return (void *)fn.OSMesaGetProcAddress(funcName);
}

void OSMesaDriver::OSMesaColorClamp(GLboolean enable)
{
	D(bug("nfosmesa: OSMesaColorClamp(%d)", enable));
	OpenLibrary();
	if (!SelectContext(cur_context))
		return;
	if (fn.OSMesaColorClamp)
		fn.OSMesaColorClamp(enable);
	else
		bug("nfosmesa: OSMesaColorClamp: no such function");
}

void OSMesaDriver::OSMesaPostprocess(Uint32 ctx, const char *filter, GLuint enable_value)
{
	D(bug("nfosmesa: OSMesaPostprocess(%u, %s, %d)", ctx, filter, enable_value));
	if (ctx>MAX_OSMESA_CONTEXTS || ctx == 0 || !contexts[ctx].ctx)
		return;
	/* no SelectContext() here; OSMesaPostprocess must be called without having a current context */
	if (fn.OSMesaPostprocess)
		fn.OSMesaPostprocess(contexts[ctx].ctx, filter, enable_value);
	else
		bug("nfosmesa: OSMesaPostprocess: no such function");
}

Uint32 OSMesaDriver::LenglGetString(Uint32 ctx, GLenum name)
{
	D(bug("nfosmesa: LenglGetString(%u, 0x%x)", ctx, name));
	OpenLibrary();
	SelectContext(ctx);
	const char *s = (const char *)fn.glGetString(name);
	if (s == NULL) return 0;
	return strlen(s);
}

void OSMesaDriver::PutglGetString(Uint32 ctx, GLenum name, GLubyte *buffer)
{
	D(bug("nfosmesa: PutglGetString(%u, 0x%x, %p)", ctx, name, buffer));
	OpenLibrary();
	SelectContext(ctx);
	const char *s = (const char *)fn.glGetString(name);
	if (buffer)
		strcpy((char *)buffer, s ? s : "");
}

Uint32 OSMesaDriver::LenglGetStringi(Uint32 ctx, GLenum name, GLuint index)
{
	D(bug("nfosmesa: LenglGetStringi(%u, 0x%x, %u)", ctx, name, index));
	OpenLibrary();
	SelectContext(ctx);
	const char *s = (const char *)fn.glGetStringi(name, index);
	if (s == NULL) return (Uint32)-1;
	return strlen(s);
}

void OSMesaDriver::PutglGetStringi(Uint32 ctx, GLenum name, GLuint index, GLubyte *buffer)
{
	D(bug("nfosmesa: PutglGetStringi(%u, 0x%x, %d, %p)", ctx, name, index, buffer));
	OpenLibrary();
	SelectContext(ctx);
	const char *s = (const char *)fn.glGetStringi(name, index);
	if (buffer)
		strcpy((char *)buffer, s ? s : "");
}

void OSMesaDriver::ConvertContext(Uint32 ctx)
{
	int x,y, srcpitch;

	if (contexts[ctx].conversion==SDL_FALSE) {
		return;
	}

	D(bug("nfosmesa: ConvertContext"));

	switch(contexts[ctx].srcformat) {
		case OSMESA_RGB_565:
			{
				Uint16 *srcline,*srccol,color;

				D(bug("nfosmesa: ConvertContext LE:565->BE:565, %dx%d",contexts[ctx].width,contexts[ctx].height));
				srcline = (Uint16 *)contexts[ctx].dst_buffer;
				srcpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol=srcline;
					for (x=0;x<contexts[ctx].width;x++) {
						color=SDL_SwapBE16(*srccol);
						*srccol++=color;
					}
					srcline += srcpitch;
				}
			}
			break;
		case OSMESA_ARGB:	/* 16 or 32 bits per channel */
			switch (bx_options.osmesa.channel_size) {
				case 16:
					ConvertContext16(ctx);
					break;
				case 32:
					ConvertContext32(ctx);
					break;
				default:
					D(bug("nfosmesa: ConvertContext: Unsupported channel size"));
					break;
			}
			break;
	}
}

void OSMesaDriver::ConvertContext16(Uint32 ctx)
{
	int x,y, r,g,b,a, srcpitch, dstpitch, color;
	Uint16 *srcline, *srccol;

	srcline = (Uint16 *) contexts[ctx].src_buffer;
	srcpitch = contexts[ctx].width * 4;

	switch(contexts[ctx].dstformat) {
		case OSMESA_RGB_565:
			{
				Uint16 *dstline, *dstcol;

				dstline = (Uint16 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						r = ((*srccol++)>>11) & 31;
						g = ((*srccol++)>>10) & 63;
						b = ((*srccol++)>>11) & 31;

						color = (r<<11)|(g<<5)|b;
						*dstcol++ = SDL_SwapBE16(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_RGB:
			{
				Uint8 *dstline, *dstcol;

				dstline = (Uint8 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width * 3;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						*dstcol++ = r;
						*dstcol++ = g;
						*dstcol++ = b;
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_BGR:
			{
				Uint8 *dstline, *dstcol;

				dstline = (Uint8 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width * 3;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						*dstcol++ = b;
						*dstcol++ = g;
						*dstcol++ = r;
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_BGRA:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						a = ((*srccol++)>>8) & 255;
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						color = (b<<24)|(g<<16)|(r<<8)|a;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_ARGB:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						a = ((*srccol++)>>8) & 255;
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						color = (a<<24)|(r<<16)|(g<<8)|b;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_RGBA:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						a = ((*srccol++)>>8) & 255;
						r = ((*srccol++)>>8) & 255;
						g = ((*srccol++)>>8) & 255;
						b = ((*srccol++)>>8) & 255;

						color = (r<<24)|(g<<16)|(b<<8)|a;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
	}
}

void OSMesaDriver::ConvertContext32(Uint32 ctx)
{
#define FLOAT_TO_INT(source, value, maximum) \
	{ \
		value = (int) (source * maximum ## .0); \
		if (value>maximum) value=maximum; \
		if (value<0) value=0; \
	}

	int x,y, r,g,b,a, srcpitch, dstpitch, color;
	float *srcline, *srccol;

	srcline = (float *) contexts[ctx].src_buffer;
	srcpitch = contexts[ctx].width * 4;

	switch(contexts[ctx].dstformat) {
		case OSMESA_RGB_565:
			{
				Uint16 *dstline, *dstcol;

				dstline = (Uint16 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						FLOAT_TO_INT(*srccol++, r, 31);
						FLOAT_TO_INT(*srccol++, g, 63);
						FLOAT_TO_INT(*srccol++, b, 31);

						color = (r<<11)|(g<<5)|b;
						*dstcol++ = SDL_SwapBE16(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_RGB:
			{
				Uint8 *dstline, *dstcol;

				dstline = (Uint8 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width * 3;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						*dstcol++ = r;
						*dstcol++ = g;
						*dstcol++ = b;
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_BGR:
			{
				Uint8 *dstline, *dstcol;

				dstline = (Uint8 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width * 3;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						srccol++; /* Skip alpha */
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						*dstcol++ = b;
						*dstcol++ = g;
						*dstcol++ = r;
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_BGRA:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						FLOAT_TO_INT(*srccol++, a, 255);
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						color = (b<<24)|(g<<16)|(r<<8)|a;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_ARGB:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						FLOAT_TO_INT(*srccol++, a, 255);
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						color = (a<<24)|(r<<16)|(g<<8)|b;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
		case OSMESA_RGBA:
			{
				Uint32 *dstline, *dstcol;

				dstline = (Uint32 *) contexts[ctx].dst_buffer;
				dstpitch = contexts[ctx].width;
				for (y=0;y<contexts[ctx].height;y++) {
					srccol = srcline;
					dstcol = dstline;
					for (x=0;x<contexts[ctx].width;x++) {
						FLOAT_TO_INT(*srccol++, a, 255);
						FLOAT_TO_INT(*srccol++, r, 255);
						FLOAT_TO_INT(*srccol++, g, 255);
						FLOAT_TO_INT(*srccol++, b, 255);

						color = (r<<24)|(g<<16)|(g<<8)|a;
						*dstcol++ = SDL_SwapBE32(color);
					}
					srcline += srcpitch;
					dstline += dstpitch;
				}
			}
			break;
	}
}


bool OSMesaDriver::pixelParams(GLenum format, GLenum type, GLsizei &size, GLsizei &count)
{
	switch (type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
	case GL_BITMAP:
	case GL_UTF8_NV:
	case 1:
		size = sizeof(GLubyte);
		break;
	case GL_UNSIGNED_SHORT:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_HALF_FLOAT:
	case GL_UTF16_NV:
	case 2:
		size = sizeof(GLushort);
		break;
	case GL_UNSIGNED_INT:
	case GL_INT:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	case GL_FIXED:
	case 4:
		size = sizeof(GLuint);
		break;
	case GL_FLOAT:
		size = sizeof(GLfloat);
		break;
	default:
		glSetError(GL_INVALID_ENUM);
		return false;
	}
	switch (format)
	{
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
	case GL_ALPHA:
	case GL_LUMINANCE:
	case GL_STENCIL_INDEX:
	case GL_DEPTH_COMPONENT:
	case GL_COLOR_INDEX:
	case 1:
		count = 1;
		break;
	case GL_LUMINANCE_ALPHA:
	case GL_DEPTH_STENCIL:
	case GL_RG:
	case 2:
		count = 2;
		break;
	case GL_RGB:
	case GL_BGR:
	case 3:
		count = 3;
		break;
	case GL_RGBA:
	case GL_BGRA:
	case 4:
		count = 4;
		break;
	default:
		glSetError(GL_INVALID_ENUM);
		return false;
	}
	return true;
}


void *OSMesaDriver::pixelBuffer(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei &size, GLsizei &count)
{
	if (!pixelParams(format, type, size, count))
		return NULL;
	
	/* FIXME: glPixelStore parameters are not taken into account */
	count *= width * height * depth;
	return malloc(size * count);
}


void *OSMesaDriver::convertPixels(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
	GLsizei size, count;
	GLubyte *ptr;
	void *result;
	
	if (pixels == NULL)
		return NULL;
	switch (type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
    case GL_2_BYTES:
    case GL_3_BYTES:
    case GL_4_BYTES:
	case GL_BITMAP:
	case GL_UTF8_NV:
	case 1:
		return (void *)pixels;
	}
	result = pixelBuffer(width, height, depth, format, type, size, count);
	if (result == NULL)
		return result;
	/* FIXME: glPixelStore parameters are not taken into account */
	ptr = (GLubyte *)result;
	if (type == GL_FLOAT)
		Atari2HostFloatArray(count, (const GLfloat *)pixels, (GLfloat *)ptr);
	else if (size == 2)
		Atari2HostShortArray(count, (const Uint16 *)pixels, (GLushort *)ptr);
	else /* if (size == 4) */
		Atari2HostIntArray(count, (const Uint32 *)pixels, (GLuint *)ptr);
	return result;
}

void *OSMesaDriver::convertArray(GLsizei count, GLenum type, const GLvoid *pixels)
{
	return convertPixels(count, 1, 1, 1, type, pixels);
}


void OSMesaDriver::setupClientArray(GLenum texunit, vertexarray_t &array, GLint size, GLenum type, GLsizei stride, GLsizei count, GLint ptrstride, const GLvoid *pointer)
{
	UNUSED(texunit); // FIXME
	array.size = size;
	array.type = type;
	array.count = count;
	array.ptrstride = ptrstride;
	GLsizei atari_defstride;
	GLsizei basesize;
	switch (type)
	{
	case GL_UNSIGNED_BYTE:
	case GL_BYTE:
	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
    case GL_2_BYTES:
    case GL_3_BYTES:
    case GL_4_BYTES:
	case GL_BITMAP:
	case 1:
		array.basesize = sizeof(Uint8);
		basesize = sizeof(GLubyte);
		break;
	case GL_SHORT:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_HALF_FLOAT:
	case 2:
		array.basesize = sizeof(Uint16);
		basesize = sizeof(GLshort);
		break;
	case GL_UNSIGNED_INT:
	case GL_INT:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
	case GL_FIXED:
	case 4:
		array.basesize = sizeof(Uint32);
		basesize = sizeof(GLint);
		break;
	case GL_FLOAT:
		array.basesize = ATARI_SIZEOF_FLOAT;
		basesize = sizeof(GLfloat);
		break;
	case GL_DOUBLE:
		array.basesize = ATARI_SIZEOF_DOUBLE;
		basesize = sizeof(GLdouble);
		break;
	default:
		glSetError(GL_INVALID_ENUM);
		return;
	}
	atari_defstride = size * array.basesize;
	if (stride == 0)
	{
		stride = atari_defstride;
	}
	array.atari_stride = stride;
	array.host_stride = size * basesize;
	array.defstride = atari_defstride;
	if (array.host_pointer != array.atari_pointer)
	{
		free(array.host_pointer);
		array.host_pointer = NULL;
	}
	array.atari_pointer = pointer;
	array.converted = 0;
	array.vendor = 0;
}


void OSMesaDriver::convertClientArrays(GLsizei count)
{
	if (contexts[cur_context].enabled_arrays & NFOSMESA_VERTEX_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].vertex;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glVertexPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == 2)
			fn.glVertexPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		else if (array.count > 0)
			fn.glVertexPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glVertexPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_NORMAL_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].normal;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glNormalPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == 2)
			fn.glNormalPointervINTEL(array.type, (const void **)array.host_pointer);
		else if (array.count > 0)
			fn.glNormalPointerEXT(array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glNormalPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_COLOR_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].color;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glColorPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == 2)
			fn.glColorPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		if (array.count > 0)
			fn.glColorPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glColorPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_INDEX_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].index;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glIndexPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.count > 0)
			fn.glIndexPointerEXT(array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glIndexPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_EDGEFLAG_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].edgeflag;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glEdgeFlagPointerListIBM(array.host_stride, (const GLboolean **)array.host_pointer, array.ptrstride);
		else if (array.count > 0)
			fn.glEdgeFlagPointerEXT(array.host_stride, array.count, (const GLboolean *)array.host_pointer);
		else
			fn.glEdgeFlagPointer(array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_TEXCOORD_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].texcoord;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glTexCoordPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.vendor == 2)
			fn.glTexCoordPointervINTEL(array.size, array.type, (const void **)array.host_pointer);
		else if (array.count > 0)
			fn.glTexCoordPointerEXT(array.size, array.type, array.host_stride, array.count, array.host_pointer);
		else
			fn.glTexCoordPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_FOGCOORD_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].fogcoord;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glFogCoordPointerListIBM(array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.count >= 0)
			fn.glFogCoordPointerEXT(array.type, array.host_stride, array.host_pointer);
		else
			fn.glFogCoordPointer(array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_2NDCOLOR_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].secondary_color;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glSecondaryColorPointerListIBM(array.size, array.type, array.host_stride, (const void **)array.host_pointer, array.ptrstride);
		else if (array.count >= 0)
			fn.glSecondaryColorPointerEXT(array.size, array.type, array.host_stride, array.host_pointer);
		else
			fn.glSecondaryColorPointer(array.size, array.type, array.host_stride, array.host_pointer);
	}
	if (contexts[cur_context].enabled_arrays & NFOSMESA_ELEMENT_ARRAY)
	{
		vertexarray_t &array = contexts[cur_context].element;
		convertClientArray(count, array);
		if (array.vendor == 1)
			fn.glElementPointerAPPLE(array.type, array.host_pointer);
		else if (array.vendor == 2)
			fn.glElementPointerATI(array.type, array.host_pointer);
	}
}

void OSMesaDriver::convertClientArray(GLsizei count, vertexarray_t &array)
{
	if (array.count > 0 && count > array.count)
	{
		D(bug("nfosmesa: convertClientArray: count %d might exceed array size %d", count, array.count));
	}
	if (array.host_pointer == NULL || count > array.converted)
	{
		if (array.basesize != 1 || array.atari_stride != array.defstride)
		{
			array.host_pointer = realloc(array.host_pointer, count * array.host_stride);
			GLsizei n = count - array.converted;
			for (GLsizei i = 0; i < n; i++)
			{
				const char *src = (const char *)array.atari_pointer + array.converted * array.atari_stride;
				char *dst = (char *)array.host_pointer + array.converted * array.host_stride;
				if (array.type == GL_FLOAT)
					Atari2HostFloatArray(array.size, (const GLfloat *)src, (GLfloat *)dst);
				else if (array.type == GL_DOUBLE)
					Atari2HostDoubleArray(array.size, (const GLdouble *)src, (GLdouble *)dst);
				else if (array.basesize == 1)
					memcpy(dst, src, array.size);
				else if (array.basesize == 2)
					Atari2HostShortArray(array.size, (const Uint16 *)src, (GLushort *)dst);
				else /* if (array.basesize == 4) */
					Atari2HostIntArray(array.size, (const Uint32 *)src, (GLuint *)dst);
				array.converted++;
			}
		} else
		{
			array.host_pointer = (void *)array.atari_pointer;
			array.converted = count;
		}
	}
}

void OSMesaDriver::nfglArrayElementHelper(GLint i)
{
	convertClientArrays(i + 1);
	fn.glArrayElement(i);
}

void OSMesaDriver::nfglInterleavedArraysHelper(GLenum format, GLsizei stride, const GLvoid *pointer)
{
	SDL_bool enable_texcoord, enable_normal, enable_color;
	int texcoord_size, color_size, vertex_size;
	const GLubyte *texcoord_ptr,*normal_ptr,*color_ptr,*vertex_ptr;
	GLenum color_type;
	int c, f;
	int defstride;
	
	f = ATARI_SIZEOF_FLOAT;
	c = f * ((4 * sizeof(GLubyte) + (f - 1)) / f);
	
	enable_texcoord=SDL_FALSE;
	enable_normal=SDL_FALSE;
	enable_color=SDL_FALSE;
	texcoord_size=color_size=vertex_size=0;
	texcoord_ptr=normal_ptr=color_ptr=vertex_ptr=(const GLubyte *)pointer;
	color_type = GL_FLOAT;
	switch(format) {
		case GL_V2F:
			vertex_size = 2;
			defstride = 2 * f;
			break;
		case GL_V3F:
			vertex_size = 3;
			defstride = 3 * f;
			break;
		case GL_C4UB_V2F:
			vertex_size = 2;
			color_size = 4;
			enable_color = SDL_TRUE;
			vertex_ptr += c;
			color_type = GL_UNSIGNED_BYTE;
			defstride = c + 2 * f;
			break;
		case GL_C4UB_V3F:
			vertex_size = 3;
			color_size = 4;
			enable_color = SDL_TRUE;
			vertex_ptr += c;
			color_type = GL_UNSIGNED_BYTE;
			defstride = c + 3 * f;
			break;
		case GL_C3F_V3F:
			vertex_size = 3;
			color_size = 3;
			enable_color = SDL_TRUE;
			vertex_ptr += 3 * f;
			defstride = 6 * f;
			break;
		case GL_N3F_V3F:
			vertex_size = 3;
			enable_normal = SDL_TRUE;
			vertex_ptr += 3 * f;
			defstride = 6 * f;
			break;
		case GL_C4F_N3F_V3F:
			vertex_size = 3;
			color_size = 4;
			enable_color = SDL_TRUE;
			enable_normal = SDL_TRUE;
			vertex_ptr += 7 * f;
			normal_ptr += 4 * f;
			defstride = 10 * f;
			break;
		case GL_T2F_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			enable_texcoord = SDL_TRUE;
			vertex_ptr += 2 * f;
			defstride = 5 * f;
			break;
		case GL_T4F_V4F:
			vertex_size = 4;
			texcoord_size = 4;
			enable_texcoord = SDL_TRUE;
			vertex_ptr += 4 * f;
			defstride = 8 * f;
			break;
		case GL_T2F_C4UB_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			color_size = 4;
			enable_texcoord = SDL_TRUE;
			enable_color = SDL_TRUE;
			vertex_ptr += c + 2 * f;
			color_ptr += 2 * f;
			color_type = GL_UNSIGNED_BYTE;
			defstride = c + 5 * f;
			break;
		case GL_T2F_C3F_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			color_size = 3;
			enable_texcoord = SDL_TRUE;
			enable_color = SDL_TRUE;
			vertex_ptr += 5 * f;
			color_ptr += 2 * f;
			defstride = 8 * f;
			break;
		case GL_T2F_N3F_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			enable_normal = SDL_TRUE;
			enable_texcoord = SDL_TRUE;
			vertex_ptr += 5 * f;
			normal_ptr += 2 * f;
			defstride = 8 * f;
			break;
		case GL_T2F_C4F_N3F_V3F:
			vertex_size = 3;
			texcoord_size = 2;
			color_size = 4;
			enable_normal = SDL_TRUE;
			enable_texcoord = SDL_TRUE;
			enable_color = SDL_TRUE;
			vertex_ptr += 9 * f;
			normal_ptr += 6 * f;
			color_ptr += 2 * f;
			defstride = 12 * f;
			break;
		case GL_T4F_C4F_N3F_V4F:
			vertex_size = 4;
			texcoord_size = 4;
			color_size = 4;
			enable_normal = SDL_TRUE;
			enable_texcoord = SDL_TRUE;
			enable_color = SDL_TRUE;
			vertex_ptr += 11 * f;
			normal_ptr += 8 * f;
			color_ptr += 4 * f;
			defstride = 15 * f;
			break;
		default:
			glSetError(GL_INVALID_ENUM);
			return;
	}

	if (stride==0)
	{
		stride = defstride;
	}

	/*
	 * FIXME:
	 * calling the dispatch functions here would trace
	 * calls to functions the client did not call directly
	 */
	nfglDisableClientState(GL_EDGE_FLAG_ARRAY);
	nfglDisableClientState(GL_INDEX_ARRAY);
	if(enable_normal) {
		nfglEnableClientState(GL_NORMAL_ARRAY);
		nfglNormalPointer(GL_FLOAT, stride, normal_ptr);
	} else {
		nfglDisableClientState(GL_NORMAL_ARRAY);
	}
	if(enable_color) {
		nfglEnableClientState(GL_COLOR_ARRAY);
		nfglColorPointer(color_size, color_type, stride, color_ptr);
	} else {
		nfglDisableClientState(GL_COLOR_ARRAY);
	}
	if(enable_texcoord) {
		nfglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		nfglTexCoordPointer(texcoord_size, GL_FLOAT, stride, texcoord_ptr);
	} else {
		nfglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	nfglEnableClientState(GL_VERTEX_ARRAY);
	nfglVertexPointer(vertex_size, GL_FLOAT, stride, vertex_ptr);
}

/*---
 * wrappers for functions that take float arguments, and sometimes only exist as
 * functions with double as arguments in GL
 ---*/

void OSMesaDriver::nfglFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	D(bug("nfosmesa: glFrustumf(%f, %f, %f, %f, %f, %f)", left, right, bottom, top, near_val, far_val));
	if (!GL_ISNOP(fn.glFrustumfOES))
		fn.glFrustumfOES(left, right, bottom, top, near_val, far_val);
	else
		fn.glFrustum(left, right, bottom, top, near_val, far_val);
}

void OSMesaDriver::nfglOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val)
{
	D(bug("nfosmesa: glOrthof(%f, %f, %f, %f, %f, %f)", left, right, bottom, top, near_val, far_val));
	if (!GL_ISNOP(fn.glOrthofOES))
		fn.glOrthofOES(left, right, bottom, top, near_val, far_val);
	else
		fn.glOrtho(left, right, bottom, top, near_val, far_val);
}

/* glClearDepthf already exists in GL */

/*---
 * GLU functions
 ---*/

void OSMesaDriver::nfgluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ )
{
	GLfloat m[16];
	GLfloat x[3], y[3], z[3];
	GLfloat mag;

	D(bug("nfosmesa: gluLookAtf(%f, %f, %f, %f, %f, %f, %f, %f, %f)", eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ));

	/* Make rotation matrix */

	/* Z vector */
	z[0] = eyeX - centerX;
	z[1] = eyeY - centerY;
	z[2] = eyeZ - centerZ;
	mag = sqrtf(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
	if (mag)
	{
		z[0] /= mag;
		z[1] /= mag;
		z[2] /= mag;
	}

	/* Y vector */
	y[0] = upX;
	y[1] = upY;
	y[2] = upZ;

	/* X vector = Y cross Z */
	x[0] = y[1] * z[2] - y[2] * z[1];
	x[1] = -y[0] * z[2] + y[2] * z[0];
	x[2] = y[0] * z[1] - y[1] * z[0];

	/* Recompute Y = Z cross X */
	y[0] = z[1] * x[2] - z[2] * x[1];
	y[1] = -z[0] * x[2] + z[2] * x[0];
	y[2] = z[0] * x[1] - z[1] * x[0];

	/* cross product gives area of parallelogram, which is < 1.0 for
	 * non-perpendicular unit-length vectors; so normalize x, y here
	 */

	mag = sqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	if (mag)
	{
		x[0] /= mag;
		x[1] /= mag;
		x[2] /= mag;
	}

	mag = sqrtf(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
	if (mag)
	{
		y[0] /= mag;
		y[1] /= mag;
		y[2] /= mag;
	}

	M(m, 0, 0) = x[0]; M(m, 0, 1) = x[1]; M(m, 0, 2) = x[2]; M(m, 0, 3) = 0.0; 
	M(m, 1, 0) = y[0]; M(m, 1, 1) = y[1]; M(m, 1, 2) = y[2]; M(m, 1, 3) = 0.0; 
	M(m, 2, 0) = z[0]; M(m, 2, 1) = z[1]; M(m, 2, 2) = z[2]; M(m, 2, 3) = 0.0;
	M(m, 3, 0) = 0.0;  M(m, 3, 1) = 0.0;  M(m, 3, 2) = 0.0;  M(m, 3, 3) = 1.0;

	fn.glMultMatrixf(m);

	/* Translate Eye to Origin */
	fn.glTranslatef(-eyeX, -eyeY, -eyeZ);
}

void OSMesaDriver::nfgluLookAt( GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ )
{
	D(bug("nfosmesa: gluLookAt(%f, %f, %f, %f, %f, %f, %f, %f, %f)", eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ));
	nfgluLookAtf(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
}

/*---
 * other functions needed by tiny_gl.ldg
 ---*/

void OSMesaDriver::nftinyglswapbuffer(memptr buffer)
{
	void *draw_buffer;
	Uint32 ctx = cur_context;
	
	D(bug("nfosmesa: swapbuffer($%08x)", buffer));

	if (ctx>MAX_OSMESA_CONTEXTS) {
		return;
	}
	
	if (ctx != 0 && (!fn.OSMesaMakeCurrent || !contexts[ctx].ctx)) {
		return;
	}

	if (ctx != 0)
	{
		contexts[ctx].dst_buffer = draw_buffer = Atari2HostAddr(buffer);
		if (bx_options.osmesa.channel_size > 8) {
			if (contexts[ctx].src_buffer) {
				free(contexts[ctx].src_buffer);
			}
			contexts[ctx].src_buffer = draw_buffer = malloc(contexts[ctx].width * contexts[ctx].height * 4 * (bx_options.osmesa.channel_size>>3));
			D(bug("nfosmesa: Allocated shadow buffer for channel reduction"));
		}
		fn.OSMesaMakeCurrent(contexts[ctx].ctx, draw_buffer, contexts[ctx].type, contexts[ctx].width, contexts[ctx].height);
	}
}


#define FN_GLGETERROR() \
	GLenum e = contexts[cur_context].error_code; \
	contexts[cur_context].error_code = GL_NO_ERROR; \
	if (e != GL_NO_ERROR) return e; \
	return fn.glGetError()

/*--- helper methods for object buffers ---*/

vertexarray_t *OSMesaDriver::gl_get_array(GLenum pname)
{
	switch (pname)
	{
		case GL_VERTEX_ARRAY: return &contexts[cur_context].vertex;
		case GL_NORMAL_ARRAY: return &contexts[cur_context].normal;
		case GL_COLOR_ARRAY: return &contexts[cur_context].color;
		case GL_INDEX_ARRAY: return &contexts[cur_context].index;
		case GL_EDGE_FLAG_ARRAY: return &contexts[cur_context].edgeflag;
		case GL_TEXTURE_COORD_ARRAY: return &contexts[cur_context].texcoord;
		case GL_FOG_COORD_ARRAY: return &contexts[cur_context].fogcoord;
		case GL_SECONDARY_COLOR_ARRAY: return &contexts[cur_context].secondary_color;
		case GL_ELEMENT_ARRAY_BUFFER: return &contexts[cur_context].element;
	}
	return NULL;
}

gl_buffer_t *OSMesaDriver::gl_get_buffer(GLuint name)
{
	/* TODO */
	UNUSED(name);
	return NULL;
}

gl_buffer_t *OSMesaDriver::gl_make_buffer(GLuint name, GLsizei size, const void *pointer)
{
	/* TODO */
	UNUSED(name);
	UNUSED(size);
	UNUSED(pointer);
	return NULL;
}

/*--- Functions that return a 64-bit value ---*/

/*
 * The NF interface currently only returns a single value in D0,
 * so the call has to pass an extra parameter, the location where to
 * store the result value
 */
#define FN_GLGETIMAGEHANDLEARB(texture, level, layered, layer, format) \
	GLuint64 *retaddr = (GLuint64 *)Atari2HostAddr(getParameter(2)); \
	GLuint64 ret = fn.glGetImageHandleARB(texture, level, layered, layer, format); \
	*retaddr = SDL_SwapBE64(ret); \
	return 0

#define FN_GLGETIMAGEHANDLENV(texture, level, layered, layer, format) \
	GLuint64 *retaddr = (GLuint64 *)Atari2HostAddr(getParameter(2)); \
	GLuint64 ret = fn.glGetImageHandleNV(texture, level, layered, layer, format); \
	*retaddr = SDL_SwapBE64(ret); \
	return 0

#define FN_GLGETTEXTUREHANDLEARB(texture) \
	GLuint64 *retaddr = (GLuint64 *)Atari2HostAddr(getParameter(2)); \
	GLuint64 ret = fn.glGetTextureHandleARB(texture); \
	*retaddr = SDL_SwapBE64(ret); \
	return 0

#define FN_GLGETTEXTUREHANDLENV(texture) \
	GLuint64 *retaddr = (GLuint64 *)Atari2HostAddr(getParameter(2)); \
	GLuint64 ret = fn.glGetTextureHandleNV(texture); \
	*retaddr = SDL_SwapBE64(ret); \
	return 0

#define FN_GLGETTEXTURESAMPLERHANDLEARB(texturem, sampler) \
	GLuint64 *retaddr = (GLuint64 *)Atari2HostAddr(getParameter(2)); \
	GLuint64 ret = fn.glGetTextureSamplerHandleARB(texture, sampler); \
	*retaddr = SDL_SwapBE64(ret); \
	return 0

#define FN_GLGETTEXTURESAMPLERHANDLENV(texturem, sampler) \
	GLuint64 *retaddr = (GLuint64 *)Atari2HostAddr(getParameter(2)); \
	GLuint64 ret = fn.glGetTextureSamplerHandleNV(texture, sampler); \
	*retaddr = SDL_SwapBE64(ret); \
	return 0

/*--- various helper functions ---*/

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
static GLsizei nfglPixelmapSize(GLenum pname)
{
	GLint size = 0;
	switch (pname)
	{
		case GL_PIXEL_MAP_I_TO_I: pname = GL_PIXEL_MAP_I_TO_I_SIZE; break;
		case GL_PIXEL_MAP_S_TO_S: pname = GL_PIXEL_MAP_S_TO_S_SIZE; break;
		case GL_PIXEL_MAP_I_TO_R: pname = GL_PIXEL_MAP_I_TO_R_SIZE; break;
		case GL_PIXEL_MAP_I_TO_G: pname = GL_PIXEL_MAP_I_TO_G_SIZE; break;
		case GL_PIXEL_MAP_I_TO_B: pname = GL_PIXEL_MAP_I_TO_B_SIZE; break;
		case GL_PIXEL_MAP_I_TO_A: pname = GL_PIXEL_MAP_I_TO_A_SIZE; break;
		case GL_PIXEL_MAP_R_TO_R: pname = GL_PIXEL_MAP_R_TO_R_SIZE; break;
		case GL_PIXEL_MAP_G_TO_G: pname = GL_PIXEL_MAP_G_TO_G_SIZE; break;
		case GL_PIXEL_MAP_B_TO_B: pname = GL_PIXEL_MAP_B_TO_B_SIZE; break;
		case GL_PIXEL_MAP_A_TO_A: pname = GL_PIXEL_MAP_A_TO_A_SIZE; break;
		default: return size;
	}
	fn.glGetIntegerv(pname, &size);
	return size;
}
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
static int nfglGetNumParams(GLenum pname)
{
	GLint count = 1;
	
	switch (pname)
	{
	case GL_ALIASED_LINE_WIDTH_RANGE:
	case GL_ALIASED_POINT_SIZE_RANGE:
	case GL_DEPTH_RANGE:
	case GL_DEPTH_BOUNDS_EXT:
	case GL_LINE_WIDTH_RANGE:
	case GL_MAP1_GRID_DOMAIN:
	case GL_MAP2_GRID_SEGMENTS:
	case GL_MAX_VIEWPORT_DIMS:
	case GL_POINT_SIZE_RANGE:
	case GL_POLYGON_MODE:
	case GL_VIEWPORT_BOUNDS_RANGE:
	/* case GL_SMOOTH_LINE_WIDTH_RANGE: same as GL_LINE_WIDTH_RANGE */
	/* case GL_SMOOTH_POINT_SIZE_RANGE: same as GL_POINT_SIZE_RANGE */
	case GL_TEXTURE_CLIPMAP_CENTER_SGIX:
	case GL_TEXTURE_CLIPMAP_OFFSET_SGIX:
	case GL_MAP1_TEXTURE_COORD_2:
	case GL_MAP2_TEXTURE_COORD_2:
	case GL_CURRENT_RASTER_NORMAL_SGIX:
		count = 2;
		break;
	case GL_CURRENT_NORMAL:
	case GL_POINT_DISTANCE_ATTENUATION:
	case GL_SPOT_DIRECTION:
	case GL_COLOR_INDEXES:
	case GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX:
	case GL_MAP1_NORMAL:
	case GL_MAP1_TEXTURE_COORD_3:
	case GL_MAP1_VERTEX_3:
	case GL_MAP2_NORMAL:
	case GL_MAP2_TEXTURE_COORD_3:
	case GL_MAP2_VERTEX_3:
	case GL_EVAL_VERTEX_ATTRIB2_NV: /* vertex normal */
		count = 3;
		break;
	case GL_ACCUM_CLEAR_VALUE:
	case GL_BLEND_COLOR:
	/* case GL_BLEND_COLOR_EXT: same as GL_BLEND_COLOR */
	case GL_COLOR_CLEAR_VALUE:
	case GL_COLOR_WRITEMASK:
	case GL_CURRENT_COLOR:
	case GL_CURRENT_RASTER_COLOR:
	case GL_CURRENT_RASTER_POSITION:
	case GL_CURRENT_RASTER_SECONDARY_COLOR:
	case GL_CURRENT_RASTER_TEXTURE_COORDS:
	case GL_CURRENT_SECONDARY_COLOR:
	case GL_CURRENT_TEXTURE_COORDS:
	case GL_FOG_COLOR:
	case GL_LIGHT_MODEL_AMBIENT:
	case GL_MAP2_GRID_DOMAIN:
	case GL_RGBA_SIGNED_COMPONENTS_EXT:
	case GL_SCISSOR_BOX:
	case GL_VIEWPORT:
	case GL_CONSTANT_COLOR0_NV:
	case GL_CONSTANT_COLOR1_NV:
	case GL_CULL_VERTEX_OBJECT_POSITION_EXT:
	case GL_CULL_VERTEX_EYE_POSITION_EXT:
	case GL_AMBIENT:
	case GL_DIFFUSE:
	case GL_SPECULAR:
	case GL_POSITION:
	case GL_EMISSION:
	case GL_AMBIENT_AND_DIFFUSE:
	case GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX:
	case GL_CONVOLUTION_FILTER_SCALE:
	case GL_CONVOLUTION_FILTER_BIAS:
	case GL_CONVOLUTION_BORDER_COLOR:
	case GL_TEXTURE_BORDER_COLOR:
	case GL_POST_TEXTURE_FILTER_BIAS_SGIX:
	case GL_POST_TEXTURE_FILTER_SCALE_SGIX:
	case GL_TEXTURE_ENV_COLOR:
	case GL_OBJECT_PLANE:
	case GL_EYE_PLANE:
	case GL_MAP1_COLOR_4:
	case GL_MAP1_TEXTURE_COORD_4:
	case GL_MAP1_VERTEX_4:
	case GL_MAP2_COLOR_4:
	case GL_MAP2_TEXTURE_COORD_4:
	case GL_MAP2_VERTEX_4:
	case GL_COLOR_TABLE_SCALE:
	case GL_COLOR_TABLE_BIAS:
	case GL_EVAL_VERTEX_ATTRIB0_NV: /* vertex position */
	case GL_EVAL_VERTEX_ATTRIB3_NV: /* primary color */
	case GL_EVAL_VERTEX_ATTRIB4_NV: /* secondary color */
	case GL_EVAL_VERTEX_ATTRIB8_NV: /* texture coord 0 */
	case GL_EVAL_VERTEX_ATTRIB9_NV: /* texture coord 1 */
	case GL_EVAL_VERTEX_ATTRIB10_NV: /* texture coord 2 */
	case GL_EVAL_VERTEX_ATTRIB11_NV: /* texture coord 3 */
	case GL_EVAL_VERTEX_ATTRIB12_NV: /* texture coord 4 */
	case GL_EVAL_VERTEX_ATTRIB13_NV: /* texture coord 5 */
	case GL_EVAL_VERTEX_ATTRIB14_NV: /* texture coord 6 */
	case GL_EVAL_VERTEX_ATTRIB15_NV: /* texture coord 7 */
	case GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV:
	case GL_PATH_OBJECT_BOUNDING_BOX_NV:
	case GL_PATH_FILL_BOUNDING_BOX_NV:
	case GL_PATH_STROKE_BOUNDING_BOX_NV:
	case GL_CURRENT_VERTEX_ATTRIB:
		count = 4;
		break;
	case GL_COLOR_MATRIX:
	case GL_MODELVIEW_MATRIX:
	case GL_PROJECTION_MATRIX:
	case GL_TEXTURE_MATRIX:
	case GL_TRANSPOSE_MODELVIEW_MATRIX:
	case GL_TRANSPOSE_PROJECTION_MATRIX:
	case GL_TRANSPOSE_TEXTURE_MATRIX:
	case GL_TRANSPOSE_COLOR_MATRIX:
	case GL_PATH_GEN_COEFF_NV:
		count = 16;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS:
		fn.glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count);
		break;
	case GL_SHADER_BINARY_FORMATS:
		fn.glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &count);
		break;
	case GL_EVAL_VERTEX_ATTRIB1_NV: /* vertex weight */
	case GL_EVAL_VERTEX_ATTRIB5_NV: /* fog coord */
		break;
	/* TODO: */
	case GL_COMBINER_INPUT_NV:
	case GL_COMBINER_COMPONENT_USAGE_NV:
	case GL_COMBINER_MAPPING_NV:
		break;
	}
	return count;
}
#endif

/*--- conversion macros used in generated code ---*/

#if NFOSMESA_NEED_INT_CONV

#define FN_GLARETEXTURESRESIDENT(n, textures, residences) \
	GLuint *tmp; \
	GLboolean result=GL_FALSE; \
	if(n<=0) { \
		return result; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(n, textures, tmp); \
		result = fn.glAreTexturesResident(n, tmp, residences); \
		free(tmp); \
	} \
	return result

#define FN_GLARETEXTURESRESIDENTEXT(n, textures, residences) \
	GLuint *tmp; \
	GLboolean result=GL_FALSE; \
	if(n<=0) { \
		return result; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(n, textures, tmp); \
		result = fn.glAreTexturesResidentEXT(n, tmp, residences); \
		free(tmp); \
	} \
	return result

#else

#define FN_GLARETEXTURESRESIDENT(n, textures, residences) return fn.glAreTexturesResident(n, textures, residences)
#define FN_GLARETEXTURESRESIDENTEXT(n, textures, residences) return fn.glAreTexturesResidentEXT(n, textures, residences)

#endif


#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLARRAYELEMENT(i) nfglArrayElementHelper(i)
#else
#define FN_GLARRAYELEMENT(i) fn.glArrayElement(i)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLBINORMAL3DVEXT(v) GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glBinormal3dvEXT(tmp)
#else
#define FN_GLBINORMAL3DVEXT(v) 	fn.glBinormal3dvEXT(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLBINORMAL3FVEXT(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glBinormal3fvEXT(tmp)
#else
#define FN_GLBINORMAL3FVEXT(v)	fn.glBinormal3fvEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINORMAL3IVEXT(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glBinormal3ivEXT(tmp)
#else
#define FN_GLBINORMAL3IVEXT(v)	fn.glBinormal3ivEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINORMAL3SVEXT(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glBinormal3svEXT(tmp)
#else
#define FN_GLBINORMAL3SVEXT(v)	fn.glBinormal3svEXT(v)
#endif

#define FN_GLCALLLISTS(n, type, lists) \
	void *tmp; \
	 \
	if(n<=0 || !lists) { \
		return; \
	} \
	tmp = convertArray(n, type, lists); \
	fn.glCallLists(n, type, tmp); \
	if (tmp != lists) free(tmp)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLIPPLANE(plane, equation)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, equation, tmp); \
	fn.glClipPlane(plane, tmp)
#else
#define FN_GLCLIPPLANE(plane, equation)	fn.glClipPlane(plane, equation)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLIPPLANEFOES(plane, equation)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, equation, tmp); \
	fn.glClipPlanefOES(plane, tmp)
#else
#define FN_GLCLIPPLANEFOES(plane, equation)	fn.glClipPlanefOES(plane, equation)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLIPPLANEXOES(plane, equation)	GLfixed tmp[4]; \
	Atari2HostIntPtr(4, equation, tmp); \
	fn.glClipPlanexOES(plane, tmp)
#else
#define FN_GLCLIPPLANEXOES(plane, equation)	fn.glClipPlanexOES(plane, equation)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLOR3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glColor3dv(tmp)
#else
#define FN_GLCOLOR3DV(v)	fn.glColor3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR3FVERTEX3FVSUN(c, v)	GLfloat tmp1[3], tmp2[3]; \
	Atari2HostFloatArray(3, c, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glColor3fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLCOLOR3FVERTEX3FVSUN(c, v)	fn.glColor3fVertex3fvSUN(c, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glColor3fv(tmp)
#else
#define FN_GLCOLOR3FV(v)	fn.glColor3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glColor3hvNV(tmp)
#else
#define FN_GLCOLOR3HVNV(v)	fn.glColor3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glColor3iv(tmp)
#else
#define FN_GLCOLOR3IV(v)	fn.glColor3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glColor3sv(tmp)
#else
#define FN_GLCOLOR3SV(v)	fn.glColor3sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3UIV(v)	GLuint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glColor3uiv(tmp)
#else
#define FN_GLCOLOR3UIV(v)	fn.glColor3uiv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3USV(v)	GLushort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glColor3usv(tmp)
#else
#define FN_GLCOLOR3USV(v)	fn.glColor3usv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR3XVOES(components)	GLint tmp[3]; \
	Atari2HostIntPtr(3, components, tmp); \
	fn.glColor3xvOES(tmp)
#else
#define FN_GLCOLOR3XVOES(components)	fn.glColor3xvOES(components)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLOR4DV(v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glColor4dv(tmp)
#else
#define FN_GLCOLOR4DV(v)	fn.glColor4dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4FNORMAL3FVERTEX3FVSUN(c, n, v)	GLfloat tmp1[4],tmp2[3],tmp3[3]; \
	Atari2HostFloatArray(4, c, tmp1); \
	Atari2HostFloatArray(3, n, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glColor4fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLCOLOR4FNORMAL3FVERTEX3FVSUN(c, n, v)	fn.glColor4fNormal3fVertex3fvSUN(c, n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4FV(v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glColor4fv(tmp)
#else
#define FN_GLCOLOR4FV(v)	fn.glColor4fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4HVNV(v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glColor4hvNV(tmp)
#else
#define FN_GLCOLOR4HVNV(v)	fn.glColor4hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4IV(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glColor4iv(tmp)
#else
#define FN_GLCOLOR4IV(v)	fn.glColor4iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4SV(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glColor4sv(tmp)
#else
#define FN_GLCOLOR4SV(v)	fn.glColor4sv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4UBVERTEX2FVSUN(c, v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glColor4ubVertex2fvSUN(c, tmp)
#else
#define FN_GLCOLOR4UBVERTEX2FVSUN(c, v)	fn.glColor4ubVertex2fvSUN(c, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLOR4UBVERTEX3FVSUN(c, v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glColor4ubVertex3fvSUN(c, tmp)
#else
#define FN_GLCOLOR4UBVERTEX3FVSUN(c, v)	fn.glColor4ubVertex3fvSUN(c, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4UIV(v)	GLuint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glColor4uiv(tmp)
#else
#define FN_GLCOLOR4UIV(v)	fn.glColor4uiv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4USV(v)	GLushort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glColor4usv(tmp)
#else
#define FN_GLCOLOR4USV(v)	fn.glColor4usv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLOR4XVOES(components)	GLfixed tmp[4]; \
	Atari2HostIntPtr(4, components, tmp); \
	fn.glColor4xvOES(tmp)
#else
#define FN_GLCOLOR4XVOES(components)	fn.glColor4xvOES(components)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORP3UIV(type, color)	GLuint tmp[3]; \
	Atari2HostIntPtr(3, color, tmp); \
	fn.glColorP3uiv(type, tmp)
#else
#define FN_GLCOLORP3UIV(type, color)	fn.glColorP3uiv(type, color)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORP4UIV(type, color)	GLuint tmp[4]; \
	Atari2HostIntPtr(4, color, tmp); \
	fn.glColorP4uiv(type, tmp)
#else
#define FN_GLCOLORP4UIV(type, color)	fn.glColorP4uiv(type, color)
#endif

#define FN_GLCOLORPOINTER(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].color, size, type, stride, -1, 0, pointer)

#define FN_GLCOLORPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].color, size, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].color.vendor = 1

#define FN_GLCOLORPOINTERVINTEL(size, type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].color, size, type, 0, -1, 0, pointer); \
	contexts[cur_context].color.vendor = 2

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORSUBTABLE(target, start, count, format, type, data) \
	void *tmp = convertPixels(count, 1, 1, format, type, data); \
	fn.glColorSubTable(target, start, count, format, type, tmp); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCOLORSUBTABLE(target, start, count, format, type, data)	fn.glColorSubTable(target, start, count, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLE(target, internalformat, width, format, type, table) \
	void *tmp = convertPixels(width, 1, 1, format, type, table); \
	fn.glColorTable(target, internalformat, width, format, type, tmp); \
	if (tmp != table) free(tmp)
#else
#define FN_GLCOLORTABLE(target, internalformat, width, format, type, table)	fn.glColorTable(target, internalformat, width, format, type, table)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETEXTURES(n, textures)	GLuint *tmp; \
	if(n<=0) { \
		return; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(n, textures, tmp); \
		fn.glDeleteTextures(n, tmp); \
		free(tmp); \
	} else { \
		glSetError(GL_OUT_OF_MEMORY); \
	}
#else
#define FN_GLDELETETEXTURES(n, textures)	fn.glDeleteTextures(n, textures)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETEXTURESEXT(n, textures)	GLuint *tmp; \
	if(n<=0) { \
		return; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(n, textures, tmp); \
		fn.glDeleteTexturesEXT(n, tmp); \
		free(tmp); \
	} else { \
		glSetError(GL_OUT_OF_MEMORY); \
	}
	
#else
#define FN_GLDELETETEXTURESEXT(n, textures)	fn.glDeleteTexturesEXT(n, textures)
#endif

#define FN_GLDISABLECLIENTSTATE(array) \
	switch(array) { \
		case GL_VERTEX_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_VERTEX_ARRAY; \
			break; \
		case GL_NORMAL_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_NORMAL_ARRAY; \
			break; \
		case GL_COLOR_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_COLOR_ARRAY; \
			break; \
		case GL_INDEX_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_INDEX_ARRAY; \
			break; \
		case GL_EDGE_FLAG_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_EDGEFLAG_ARRAY; \
			break; \
		case GL_TEXTURE_COORD_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_TEXCOORD_ARRAY; \
			break; \
		case GL_FOG_COORDINATE_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_FOGCOORD_ARRAY; \
			break; \
		case GL_SECONDARY_COLOR_ARRAY: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_2NDCOLOR_ARRAY; \
			break; \
		case GL_ELEMENT_ARRAY_APPLE: \
			contexts[cur_context].enabled_arrays &= ~NFOSMESA_ELEMENT_ARRAY; \
			break; \
	} \
	fn.glDisableClientState(array)

#define FN_GLDRAWARRAYS(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawArrays(mode, first, count)

#define FN_GLDRAWARRAYSINSTANCED(mode, first, count, instancecount) \
	convertClientArrays(first + count); \
	fn.glDrawArraysInstanced(mode, first, count, instancecount)

#define FN_GLDRAWELEMENTS(mode, count, type, indices) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElements(mode, count, type, tmp); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSBASEVERTEX(mode, count, type, indices, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsBaseVertex(mode, count, type, tmp, basevertex); \
	if (tmp != indices) free(tmp)


#define FN_GLDRAWRANGEELEMENTS(mode, start, end, count, type, indices) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElements(mode, start, end, count, type, indices); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWRANGEELEMENTSEXT(mode, start, end, count, type, indices) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElementsEXT(mode, start, end, count, type, indices); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWRANGEELEMENTSBASEVERTEX(mode, start, end, count, type, indices, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex); \
	if (tmp != indices) free(tmp)

#define FN_GLEDGEFLAGPOINTER(stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, -1, 0, pointer)

#define FN_GLEDGEFLAGPOINTERLISTIBM(stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, -1, ptrstride, pointer); \
	contexts[cur_context].edgeflag.vendor = 1

#define FN_GLENABLECLIENTSTATE(array) \
	switch(array) { \
		case GL_VERTEX_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_VERTEX_ARRAY; \
			break; \
		case GL_NORMAL_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_NORMAL_ARRAY; \
			break; \
		case GL_COLOR_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_COLOR_ARRAY; \
			break; \
		case GL_INDEX_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_INDEX_ARRAY; \
			break; \
		case GL_EDGE_FLAG_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_EDGEFLAG_ARRAY; \
			break; \
		case GL_TEXTURE_COORD_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_TEXCOORD_ARRAY; \
			break; \
		case GL_FOG_COORDINATE_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_FOGCOORD_ARRAY; \
			break; \
		case GL_SECONDARY_COLOR_ARRAY: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_2NDCOLOR_ARRAY; \
			break; \
		case GL_ELEMENT_ARRAY_APPLE: \
			contexts[cur_context].enabled_arrays |= NFOSMESA_ELEMENT_ARRAY; \
			break; \
	} \
	fn.glEnableClientState(array)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLEVALCOORD1DV(u)	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, u, tmp); \
	fn.glEvalCoord1dv(tmp)
#else
#define FN_GLEVALCOORD1DV(u)	fn.glEvalCoord1dv(u)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEVALCOORD1FV(u)	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, u, tmp); \
	fn.glEvalCoord1fv(tmp)
#else
#define FN_GLEVALCOORD1FV(u)	fn.glEvalCoord1fv(u)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLEVALCOORD2DV(u)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, u, tmp); \
	fn.glEvalCoord2dv(tmp)
#else
#define FN_GLEVALCOORD2DV(u)	fn.glEvalCoord2dv(u)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEVALCOORD2FV(u)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, u, tmp); \
	fn.glEvalCoord2fv(tmp)
#else
#define FN_GLEVALCOORD2FV(u)	fn.glEvalCoord2fv(u)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFINISH() \
	fn.glFinish(); \
	ConvertContext(cur_context)
#else
#define FN_GLFINISH() fn.glFinish()
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFLUSH()	fn.glFlush(); \
	ConvertContext(cur_context)
#else
#define FN_GLFLUSH()	fn.glFlush()
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGCOORDHVNV(fog)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, fog, tmp); \
	fn.glFogCoordhvNV(tmp)
#else
#define FN_GLFOGCOORDHVNV(fog)	fn.glFogCoordhvNV(fog)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGFV(pname, params) \
	int size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glFogfv(pname, tmp)
#else
#define FN_GLFOGFV(pname, params)	fn.glFogfv(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGIV(pname, params) \
	int size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glFogiv(pname, tmp)
#else
#define FN_GLFOGIV(pname, params)	fn.glFogiv(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFLOATV(pname, params) \
	int n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLfloat *tmp; \
		tmp = (GLfloat *)malloc(n * sizeof(*tmp)); \
		fn.glGetFloatv(pname, tmp); \
		Host2AtariFloatArray(n, tmp, params); \
		free(tmp); \
	} else { \
		GLfloat tmp[16]; \
		fn.glGetFloatv(pname, tmp); \
		Host2AtariFloatArray(n, tmp, params); \
	}
#else
#define FN_GLGETFLOATV(pname, params) \
	fn.glGetFloatv(pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETDOUBLEV(pname, params) \
	int n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLdouble *tmp; \
		tmp = (GLdouble *)malloc(n * sizeof(*tmp)); \
		fn.glGetDoublev(pname, tmp); \
		Host2AtariDoubleArray(n, tmp, params); \
		free(tmp); \
	} else { \
		GLdouble tmp[16]; \
		fn.glGetDoublev(pname, tmp); \
		Host2AtariDoubleArray(n, tmp, params); \
	}
#else
#define FN_GLGETDOUBLEV(pname, params)	fn.glGetDoublev(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERV(pname, params) \
	int n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLint *tmp; \
		tmp = (GLint *)malloc(n * sizeof(*tmp)); \
		fn.glGetIntegerv(pname, tmp); \
		Host2AtariIntPtr(n, tmp, params); \
		free(tmp); \
	} else { \
		GLint tmp[16]; \
		fn.glGetIntegerv(pname, tmp); \
		Host2AtariIntPtr(n, tmp, params); \
	}
#else
#define FN_GLGETINTEGERV(pname, params)	fn.glGetIntegerv(pname, params)
#endif

#define FN_GLGETPOINTERV(pname, data) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	gl_get_pointer(pname, texunit, data)

#define FN_GLINDEXPOINTER(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].index, 1, type, stride, -1, 0, pointer)

#define FN_GLINDEXPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].index, 1, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].index.vendor = 1

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLINDEXDV(c)	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, c, tmp); \
	fn.glIndexdv(tmp)
#else
#define FN_GLINDEXDV(c)	fn.glIndexdv(c)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLINDEXFV(c)	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, c, tmp); \
	fn.glIndexfv(tmp)
#else
#define FN_GLINDEXFV(c)	fn.glIndexfv(c)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINDEXIV(c)	GLint tmp[1]; \
	Atari2HostIntPtr(1, c, tmp); \
	fn.glIndexiv(tmp)
#else
#define FN_GLINDEXIV(c)	fn.glIndexiv(c)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINDEXSV(c)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, c, tmp); \
	fn.glIndexsv(tmp)
#else
#define FN_GLINDEXSV(c)	fn.glIndexsv(c)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLINTERLEAVEDARRAYS(format, stride, pointer)	nfglInterleavedArraysHelper(format, stride, pointer)
#else
#define FN_GLINTERLEAVEDARRAYS(format, stride, pointer)	fn.glInterleavedArrays(format, stride, pointer)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLIGHTMODELFV(pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params,tmp); \
	fn.glLightModelfv(pname, tmp)
#else
#define FN_GLLIGHTMODELFV(pname, params)	fn.glLightModelfv(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTMODELIV(pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glLightModeliv(pname, tmp)
#else
#define FN_GLLIGHTMODELIV(pname, params)	fn.glLightModeliv(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLIGHTFV(light, pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glLightfv(light, pname, tmp)
#else
#define FN_GLLIGHTFV(light, pname, params)	fn.glLightfv(light, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLIGHTIV(light, pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glLightiv(light, pname, tmp)
#else
#define FN_GLLIGHTIV(light, pname, params)	fn.glLightiv(light, pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLLOADMATRIXD(m)	GLdouble tmp[16]; \
	Atari2HostDoubleArray(16, m, tmp); \
	fn.glLoadMatrixd(tmp)
#else
#define FN_GLLOADMATRIXD(m)	fn.glLoadMatrixd(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLOADMATRIXF(m)	GLfloat tmp[16]; \
	Atari2HostFloatArray(16, m, tmp); \
	fn.glLoadMatrixf(tmp)
#else
#define FN_GLLOADMATRIXF(m)	fn.glLoadMatrixf(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLLOADTRANSPOSEMATRIXD(m)	GLdouble tmp[16]; \
	Atari2HostDoubleArray(16, m, tmp); \
	fn.glLoadTransposeMatrixd(tmp)
#else
#define FN_GLLOADTRANSPOSEMATRIXD(m)	fn.glLoadTransposeMatrixd(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLLOADTRANSPOSEMATRIXDARB(m)	GLdouble tmp[16]; \
	Atari2HostDoubleArray(16, m, tmp); \
	fn.glLoadTransposeMatrixdARB(tmp)
#else
#define FN_GLLOADTRANSPOSEMATRIXDARB(m)	fn.glLoadTransposeMatrixdARB(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLOADTRANSPOSEMATRIXF(m)	GLfloat tmp[16]; \
	Atari2HostFloatArray(16, m, tmp); \
	fn.glLoadTransposeMatrixf(m)
#else
#define FN_GLLOADTRANSPOSEMATRIXF(m)	fn.glLoadTransposeMatrixf(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLOADTRANSPOSEMATRIXFARB(m)	GLfloat tmp[16]; \
	Atari2HostFloatArray(16, m, tmp); \
	fn.glLoadTransposeMatrixfARB(m)
#else
#define FN_GLLOADTRANSPOSEMATRIXFARB(m)	fn.glLoadTransposeMatrixfARB(m)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
static GLint __glGetMap_Evalk(GLenum target)
{
	switch (target) {
		case GL_MAP1_INDEX:
		case GL_MAP1_TEXTURE_COORD_1:
		case GL_MAP2_INDEX:
		case GL_MAP2_TEXTURE_COORD_1:
			return 1;
		case GL_MAP1_TEXTURE_COORD_2:
		case GL_MAP2_TEXTURE_COORD_2:
			return 2;
		case GL_MAP1_VERTEX_3:
		case GL_MAP1_NORMAL:
		case GL_MAP1_TEXTURE_COORD_3:
		case GL_MAP2_VERTEX_3:
		case GL_MAP2_NORMAL:
		case GL_MAP2_TEXTURE_COORD_3:
			return 3;
		case GL_MAP1_VERTEX_4:
		case GL_MAP1_COLOR_4:
		case GL_MAP1_TEXTURE_COORD_4:
		case GL_MAP2_VERTEX_4:
		case GL_MAP2_COLOR_4:
		case GL_MAP2_TEXTURE_COORD_4:
		default:
			return 4;
	}
}
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAP1D(target, u1, u2, stride, order, points) \
	{ \
		GLdouble *tmp; \
		const GLubyte *ptr; \
		GLint i; \
		GLint size = __glGetMap_Evalk(target); \
		 \
		tmp=(GLdouble *)malloc(size*order*sizeof(GLdouble)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		ptr =(const GLubyte *)points; \
		for(i=0;i<order;i++) { \
			Atari2HostDoubleArray(size, (const GLdouble *)ptr, &tmp[i*size]); \
			ptr += stride * ATARI_SIZEOF_DOUBLE; \
		} \
		fn.glMap1d(target, u1, u2, size, order, tmp); \
		free(tmp); \
	}
#else
#define FN_GLMAP1D(target, u1, u2, stride, order, points)	fn.glMap1d(target, u1, u2, stride, order, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAP1F(target, u1, u2, stride, order, points) \
	{ \
		GLfloat *tmp; \
		const GLubyte *ptr; \
		GLint i; \
		GLint size = __glGetMap_Evalk(target); \
		 \
		tmp=(GLfloat *)malloc(size*order*sizeof(GLfloat)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		ptr =(const GLubyte *)points; \
		for(i=0;i<order;i++) { \
			Atari2HostFloatArray(size, (const GLfloat *)ptr, &tmp[i*size]); \
			ptr += stride * ATARI_SIZEOF_FLOAT; \
		} \
		fn.glMap1f(target, u1, u2, size, order, tmp); \
		free(tmp); \
	}
#else
#define FN_GLMAP1F(target, u1, u2, stride, order, points)	fn.glMap1f(target, u1, u2, stride, order, points)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAP2D(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	{ \
		GLdouble *tmp; \
		const GLubyte *ptr; \
		GLint i, j; \
		GLint size = __glGetMap_Evalk(target); \
		 \
		tmp=(GLdouble *)malloc(size*uorder*vorder*sizeof(GLdouble)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		ptr =(const GLubyte *)points; \
		for(i=0;i<uorder;i++) { \
			for(j=0;j<vorder;j++) { \
				Atari2HostDoubleArray(size, (const GLdouble *)&ptr[(i * ustride + j * vstride) * ATARI_SIZEOF_DOUBLE], &tmp[(i * vorder + j) * size]); \
			} \
		} \
		fn.glMap2d(target, \
			u1, u2, size*vorder, uorder, \
			v1, v2, size, vorder, tmp \
		); \
		free(tmp); \
	}
#else
#define FN_GLMAP2D(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points)	fn.glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAP2F(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
	{ \
		GLfloat *tmp; \
		const GLubyte *ptr; \
		GLint i,j; \
		GLint size = __glGetMap_Evalk(target); \
		 \
		tmp=(GLfloat *)malloc(size*uorder*vorder*sizeof(GLfloat)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		ptr =(const GLubyte *)points; \
		for(i=0;i<uorder;i++) { \
			for(j=0;j<vorder;j++) { \
				Atari2HostFloatArray(size, (const GLfloat *)&ptr[(i * ustride + j * vstride) * ATARI_SIZEOF_FLOAT], &tmp[(i * vorder + j) * size]); \
			} \
		} \
		fn.glMap2f(target, \
			u1, u2, size*vorder, uorder, \
			v1, v2, size, vorder, tmp \
		); \
		free(tmp); \
	}
#else
#define FN_GLMAP2F(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points)	fn.glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMAPFV(target, query, v) \
	GLint order[2] = { 0, 0 }; \
	GLint size; \
	GLfloat *tmp; \
	GLfloat tmpbuf[4]; \
	switch (target) { \
		case GL_MAP1_INDEX: \
		case GL_MAP1_TEXTURE_COORD_1: \
		case GL_MAP1_TEXTURE_COORD_2: \
		case GL_MAP1_VERTEX_3: \
		case GL_MAP1_NORMAL: \
		case GL_MAP1_TEXTURE_COORD_3: \
		case GL_MAP1_VERTEX_4: \
		case GL_MAP1_COLOR_4: \
		case GL_MAP1_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0) return; \
				size *= order[0]; \
				tmp = (GLfloat *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapfv(target, query, tmp); \
				Host2AtariFloatArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapfv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariFloatArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapfv(target, query, tmpbuf); \
				size = 1; \
				Host2AtariFloatArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
		case GL_MAP2_INDEX: \
		case GL_MAP2_TEXTURE_COORD_1: \
		case GL_MAP2_TEXTURE_COORD_2: \
		case GL_MAP2_VERTEX_3: \
		case GL_MAP2_NORMAL: \
		case GL_MAP2_TEXTURE_COORD_3: \
		case GL_MAP2_VERTEX_4: \
		case GL_MAP2_COLOR_4: \
		case GL_MAP2_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0 || order[1] <= 0) return; \
				size *= order[0] * order[1]; \
				tmp = (GLfloat *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapfv(target, query, tmp); \
				Host2AtariFloatArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapfv(target, query, tmpbuf); \
				size = 4; \
				Host2AtariFloatArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapfv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariFloatArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
	}
#else
#define FN_GLGETMAPFV(target, query, v) \
	fn.glGetMapfv(target, query, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETMAPDV(target, query, v) \
	GLint order[2] = { 0, 0 }; \
	GLint size; \
	GLdouble *tmp; \
	GLdouble tmpbuf[4]; \
	switch (target) { \
		case GL_MAP1_INDEX: \
		case GL_MAP1_TEXTURE_COORD_1: \
		case GL_MAP1_TEXTURE_COORD_2: \
		case GL_MAP1_VERTEX_3: \
		case GL_MAP1_NORMAL: \
		case GL_MAP1_TEXTURE_COORD_3: \
		case GL_MAP1_VERTEX_4: \
		case GL_MAP1_COLOR_4: \
		case GL_MAP1_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0) return; \
				size *= order[0]; \
				tmp = (GLdouble *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapdv(target, query, tmp); \
				Host2AtariDoubleArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapdv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariDoubleArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapdv(target, query, tmpbuf); \
				size = 1; \
				Host2AtariDoubleArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
		case GL_MAP2_INDEX: \
		case GL_MAP2_TEXTURE_COORD_1: \
		case GL_MAP2_TEXTURE_COORD_2: \
		case GL_MAP2_VERTEX_3: \
		case GL_MAP2_NORMAL: \
		case GL_MAP2_TEXTURE_COORD_3: \
		case GL_MAP2_VERTEX_4: \
		case GL_MAP2_COLOR_4: \
		case GL_MAP2_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0 || order[1] <= 0) return; \
				size *= order[0] * order[1]; \
				tmp = (GLdouble *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapdv(target, query, tmp); \
				Host2AtariDoubleArray(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapdv(target, query, tmpbuf); \
				size = 4; \
				Host2AtariDoubleArray(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapdv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariDoubleArray(size, tmpbuf, v); \
				break; \
			} \
			break; \
	}
#else
#define FN_GLGETMAPDV(target, query, v) \
	fn.glGetMapdv(target, query, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMAPIV(target, query, v) \
	GLint order[2] = { 0, 0 }; \
	GLint size; \
	GLint *tmp; \
	GLint tmpbuf[4]; \
	switch (target) { \
		case GL_MAP1_INDEX: \
		case GL_MAP1_TEXTURE_COORD_1: \
		case GL_MAP1_TEXTURE_COORD_2: \
		case GL_MAP1_VERTEX_3: \
		case GL_MAP1_NORMAL: \
		case GL_MAP1_TEXTURE_COORD_3: \
		case GL_MAP1_VERTEX_4: \
		case GL_MAP1_COLOR_4: \
		case GL_MAP1_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0) return; \
				size *= order[0]; \
				tmp = (GLint *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapiv(target, query, tmp); \
				Host2AtariIntPtr(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapiv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariIntPtr(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapiv(target, query, tmpbuf); \
				size = 1; \
				Host2AtariIntPtr(size, tmpbuf, v); \
				break; \
			} \
			break; \
		case GL_MAP2_INDEX: \
		case GL_MAP2_TEXTURE_COORD_1: \
		case GL_MAP2_TEXTURE_COORD_2: \
		case GL_MAP2_VERTEX_3: \
		case GL_MAP2_NORMAL: \
		case GL_MAP2_TEXTURE_COORD_3: \
		case GL_MAP2_VERTEX_4: \
		case GL_MAP2_COLOR_4: \
		case GL_MAP2_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0 || order[1] <= 0) return; \
				size *= order[0] * order[1]; \
				tmp = (GLint *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapiv(target, query, tmp); \
				Host2AtariIntPtr(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapiv(target, query, tmpbuf); \
				size = 4; \
				Host2AtariIntPtr(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapiv(target, query, tmpbuf); \
				size = 2; \
				Host2AtariIntPtr(size, tmpbuf, v); \
				break; \
			} \
			break; \
	}
#else
#define FN_GLGETMAPIV(target, query, v) \
	fn.glGetMapiv(target, query, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMAPXVOES(target, query, v) \
	GLint order[2] = { 0, 0 }; \
	GLint size; \
	GLfixed *tmp; \
	GLfixed tmpbuf[4]; \
	switch (target) { \
		case GL_MAP1_INDEX: \
		case GL_MAP1_TEXTURE_COORD_1: \
		case GL_MAP1_TEXTURE_COORD_2: \
		case GL_MAP1_VERTEX_3: \
		case GL_MAP1_NORMAL: \
		case GL_MAP1_TEXTURE_COORD_3: \
		case GL_MAP1_VERTEX_4: \
		case GL_MAP1_COLOR_4: \
		case GL_MAP1_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0) return; \
				size *= order[0]; \
				tmp = (GLfixed *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapxvOES(target, query, tmp); \
				Host2AtariIntPtr(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapxvOES(target, query, tmpbuf); \
				size = 2; \
				Host2AtariIntPtr(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapxvOES(target, query, tmpbuf); \
				size = 1; \
				Host2AtariIntPtr(size, tmpbuf, v); \
				break; \
			} \
			break; \
		case GL_MAP2_INDEX: \
		case GL_MAP2_TEXTURE_COORD_1: \
		case GL_MAP2_TEXTURE_COORD_2: \
		case GL_MAP2_VERTEX_3: \
		case GL_MAP2_NORMAL: \
		case GL_MAP2_TEXTURE_COORD_3: \
		case GL_MAP2_VERTEX_4: \
		case GL_MAP2_COLOR_4: \
		case GL_MAP2_TEXTURE_COORD_4: \
			switch (query) { \
			case GL_COEFF: \
				size = __glGetMap_Evalk(target); \
				fn.glGetMapiv(target, GL_ORDER, order); \
				if (order[0] <= 0 || order[1] <= 0) return; \
				size *= order[0] * order[1]; \
				tmp = (GLfixed *)malloc(size * sizeof(*tmp)); \
				if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
				fn.glGetMapxvOES(target, query, tmp); \
				Host2AtariIntPtr(size, tmp, v); \
				free(tmp); \
				break; \
			case GL_DOMAIN: \
				fn.glGetMapxvOES(target, query, tmpbuf); \
				size = 4; \
				Host2AtariIntPtr(size, tmpbuf, v); \
				break; \
			case GL_ORDER: \
				fn.glGetMapxvOES(target, query, tmpbuf); \
				size = 2; \
				Host2AtariIntPtr(size, tmpbuf, v); \
				break; \
			} \
			break; \
	}
#else
#define FN_GLGETMAPXVOES(target, query, v) \
	fn.glGetMapxvOES(target, query, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATERIALFV(face, pname, params) \
	GLfloat tmp[4]; \
	int size; \
	switch(pname) { \
		case GL_SHININESS: \
			size=1; \
			break; \
		case GL_COLOR_INDEXES: \
			size=3; \
			break; \
		default: \
			size=4; \
			break; \
	} \
	Atari2HostFloatArray(size, params,tmp); \
	fn.glMaterialfv(face, pname, tmp)
#else
#define FN_GLMATERIALFV(face, pname, params)	fn.glMaterialfv(face, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMATERIALIV(face, pname, params) \
	GLint tmp[4]; \
	int size; \
	switch(pname) { \
		case GL_SHININESS: \
			size=1; \
			break; \
		case GL_COLOR_INDEXES: \
			size=3; \
			break; \
		default: \
			size=4; \
			break; \
	} \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glMaterialiv(face, pname, tmp)
#else
#define FN_GLMATERIALIV(face, pname, params)	fn.glMaterialiv(face, pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTMATRIXD(m)	GLdouble tmp[16]; \
	Atari2HostDoubleArray(16, m, tmp); \
	fn.glMultMatrixd(tmp)
#else
#define FN_GLMULTMATRIXD(m)	fn.glMultMatrixd(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTMATRIXF(m)	GLfloat tmp[16]; \
	Atari2HostFloatArray(16, m, tmp); \
	fn.glMultMatrixf(tmp)
#else
#define FN_GLMULTMATRIXF(m)	fn.glMultMatrixf(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTTRANSPOSEMATRIXD(m)	GLdouble tmp[16]; \
	Atari2HostDoubleArray(16, m, tmp); \
	fn.glMultTransposeMatrixd(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXD(m)	fn.glMultTransposeMatrixd(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTTRANSPOSEMATRIXDARB(m)	GLdouble tmp[16]; \
	Atari2HostDoubleArray(16, m, tmp); \
	fn.glMultTransposeMatrixdARB(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXDARB(m)	fn.glMultTransposeMatrixdARB(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTTRANSPOSEMATRIXF(m)	GLfloat tmp[16]; \
	Atari2HostFloatArray(16, m, tmp); \
	fn.glMultTransposeMatrixf(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXF(m)	fn.glMultTransposeMatrixf(m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTTRANSPOSEMATRIXFARB(m)	GLfloat tmp[16]; \
	Atari2HostFloatArray(16, m, tmp); \
	fn.glMultTransposeMatrixfARB(tmp)
#else
#define FN_GLMULTTRANSPOSEMATRIXFARB(m)	fn.glMultTransposeMatrixfARB(m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD1DV(target, v)	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, v, tmp); \
	fn.glMultiTexCoord1dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1DV(target, v)	fn.glMultiTexCoord1dv(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD1DVARB(target, v)	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, v, tmp); \
	fn.glMultiTexCoord1dvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD1DVARB(target, v)	fn.glMultiTexCoord1dvARB(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD1FV(target, v)	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, v, tmp); \
	fn.glMultiTexCoord1fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1FV(target, v)	fn.glMultiTexCoord1fv(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD1FVARB(target, v)	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, v, tmp); \
	fn.glMultiTexCoord1fvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD1FVARB(target, v)	fn.glMultiTexCoord1fvARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1HVNV(target, v)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glMultiTexCoord1hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD1HVNV(target, v)	fn.glMultiTexCoord1hvNV(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1IV(target, v)	GLint tmp[1]; \
	Atari2HostIntPtr(1, v, tmp); \
	fn.glMultiTexCoord1iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1IV(target, v)	fn.glMultiTexCoord1iv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1IVARB(target, v)	GLint tmp[1]; \
	Atari2HostIntPtr(1, v, tmp); \
	fn.glMultiTexCoord1ivARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD1IVARB(target, v)	fn.glMultiTexCoord1ivARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1SV(target, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glMultiTexCoord1sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD1SV(target, v)	fn.glMultiTexCoord1sv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD1SVARB(target, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glMultiTexCoord1svARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD1SVARB(target, v)	fn.glMultiTexCoord1svARB(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD2DV(target, v)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glMultiTexCoord2dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2DV(target, v)	fn.glMultiTexCoord2dv(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD2DVARB(target, v)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glMultiTexCoord2dvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD2DVARB(target, v)	fn.glMultiTexCoord2dvARB(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD2FV(target, v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glMultiTexCoord2fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2FV(target, v)	fn.glMultiTexCoord2fv(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD2FVARB(target, v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glMultiTexCoord2fvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD2FVARB(target, v)	fn.glMultiTexCoord2fvARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2HVNV(target, v)	GLhalfNV tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glMultiTexCoord2hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD2HVNV(target, v)	fn.glMultiTexCoord2hvNV(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2IV(target, v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glMultiTexCoord2iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2IV(target, v)	fn.glMultiTexCoord2iv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2IVARB(target, v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glMultiTexCoord2ivARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD2IVARB(target, v)	fn.glMultiTexCoord2ivARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2SV(target, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glMultiTexCoord2sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD2SV(target, v)	fn.glMultiTexCoord2sv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD2SVARB(target, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glMultiTexCoord2svARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD2SVARB(target, v)	fn.glMultiTexCoord2svARB(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD3DV(target, v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glMultiTexCoord3dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3DV(target, v)	fn.glMultiTexCoord3dv(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD3DVARB(target, v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glMultiTexCoord3dvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD3DVARB(target, v)	fn.glMultiTexCoord3dvARB(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD3FV(target, v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glMultiTexCoord3fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3FV(target, v)	fn.glMultiTexCoord3fv(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD3FVARB(target, v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glMultiTexCoord3fvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD3FVARB(target, v)	fn.glMultiTexCoord3fvARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3HVNV(target, v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glMultiTexCoord3hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD3HVNV(target, v)	fn.glMultiTexCoord3hvNV(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3IV(target, v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glMultiTexCoord3iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3IV(target, v)	fn.glMultiTexCoord3iv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3IVARB(target, v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glMultiTexCoord3ivARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD3IVARB(target, v)	fn.glMultiTexCoord3ivARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3SV(target, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glMultiTexCoord3sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD3SV(target, v)	fn.glMultiTexCoord3sv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD3SVARB(target, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glMultiTexCoord3svARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD3SVARB(target, v)	fn.glMultiTexCoord3svARB(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD4DV(target, v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glMultiTexCoord4dv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4DV(target, v)	fn.glMultiTexCoord4dv(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXCOORD4DVARB(target, v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glMultiTexCoord4dvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD4DVARB(target, v)	fn.glMultiTexCoord4dvARB(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD4FV(target, v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glMultiTexCoord4fv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4FV(target, v)	fn.glMultiTexCoord4fv(target, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXCOORD4FVARB(target, v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glMultiTexCoord4fvARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD4FVARB(target, v)	fn.glMultiTexCoord4fvARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4HVNV(target, v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glMultiTexCoord4hvNV(target, tmp)
#else
#define FN_GLMULTITEXCOORD4HVNV(target, v)	fn.glMultiTexCoord4hvNV(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4IV(target, v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glMultiTexCoord4iv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4IV(target, v)	fn.glMultiTexCoord4iv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4IVARB(target, v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glMultiTexCoord4ivARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD4IVARB(target, v)	fn.glMultiTexCoord4ivARB(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4SV(target, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glMultiTexCoord4sv(target, tmp)
#else
#define FN_GLMULTITEXCOORD4SV(target, v)	fn.glMultiTexCoord4sv(target, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXCOORD4SVARB(target, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glMultiTexCoord4svARB(target, tmp)
#else
#define FN_GLMULTITEXCOORD4SVARB(target, v)	fn.glMultiTexCoord4svARB(target, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNORMAL3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glNormal3dv(tmp)
#else
#define FN_GLNORMAL3DV(v)	fn.glNormal3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMAL3FVERTEX3FVSUN(n, v)	GLfloat tmp1[3],tmp2[3]; \
	Atari2HostFloatArray(3, n, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glNormal3fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLNORMAL3FVERTEX3FVSUN(n, v)	fn.glNormal3fVertex3fvSUN(n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMAL3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glNormal3fv(tmp)
#else
#define FN_GLNORMAL3FV(v)	fn.glNormal3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glNormal3hvNV(tmp)
#else
#define FN_GLNORMAL3HVNV(v)	fn.glNormal3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glNormal3iv(tmp)
#else
#define FN_GLNORMAL3IV(v)	fn.glNormal3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMAL3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glNormal3sv(tmp)
#else
#define FN_GLNORMAL3SV(v)	fn.glNormal3sv(v)
#endif

#define FN_GLNORMALPOINTER(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].normal, 3, type, stride, -1, 0, pointer)

#define FN_GLNORMALPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].normal, 3, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].normal.vendor = 1

#define FN_GLNORMALPOINTERVINTEL(type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].normal, 3, type, 0, -1, 0, pointer); \
	contexts[cur_context].normal.vendor = 2

#define FN_GLFOGCOORDPOINTER(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].fogcoord, 1, type, stride, -1, 0, pointer)

#define FN_GLFOGCOORDPOINTEREXT(type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].fogcoord, 1, type, stride, 0, 0, pointer)

#define FN_GLFOGCOORDPOINTERLISTIBM(type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].fogcoord, 1, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].fogcoord.vendor = 1

#define FN_GLSECONDARYCOLORPOINTER(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].secondary_color, size, type, stride, -1, 0, pointer)

#define FN_GLSECONDARYCOLORPOINTEREXT(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].secondary_color, size, type, stride, 0, 0, pointer)

#define FN_GLSECONDARYCOLORPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].secondary_color, size, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].secondary_color.vendor = 1

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNORMALSTREAM3DVATI(stream, coords)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, coords, tmp); \
	fn.glNormalStream3dvATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3DVATI(stream, coords)	fn.glNormalStream3dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNORMALSTREAM3FVATI(stream, coords)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, coords, tmp); \
	fn.glNormalStream3fvATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3FVATI(stream, coords)	fn.glNormalStream3fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMALSTREAM3IVATI(stream, coords)	GLint tmp[3]; \
	Atari2HostIntPtr(3, coords, tmp); \
	fn.glNormalStream3ivATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3IVATI(stream, coords)	fn.glNormalStream3ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNORMALSTREAM3SVATI(stream, coords)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, coords, tmp); \
	fn.glNormalStream3svATI(stream, tmp)
#else
#define FN_GLNORMALSTREAM3SVATI(stream, coords)	fn.glNormalStream3svATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPIXELMAPFV(map, mapsize, values)	GLfloat *tmp; \
	tmp=(GLfloat *)malloc(mapsize*sizeof(GLfloat)); \
	if(tmp) { \
		Atari2HostFloatArray(mapsize, values, tmp); \
		fn.glPixelMapfv(map, mapsize, tmp); \
		free(tmp); \
	}
#else
#define FN_GLPIXELMAPFV(map, mapsize, values)	fn.glPixelMapfv(map, mapsize, values)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELMAPUIV(map, mapsize, values)	GLuint *tmp; \
	tmp=(GLuint *)malloc(mapsize*sizeof(GLuint)); \
	if(tmp) { \
		Atari2HostIntPtr(mapsize, values, tmp); \
		fn.glPixelMapuiv(map, mapsize, tmp); \
		free(tmp); \
	}
#else
#define FN_GLPIXELMAPUIV(map, mapsize, values)	fn.glPixelMapuiv(map, mapsize, values)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELMAPUSV(map, mapsize, values)	GLushort *tmp; \
	tmp=(GLushort *)malloc(mapsize*sizeof(GLushort)); \
	if(tmp) { \
		Atari2HostShortPtr(mapsize, values, tmp); \
		fn.glPixelMapusv(map, mapsize, tmp); \
		free(tmp); \
	}
#else
#define FN_GLPIXELMAPUSV(map, mapsize, values)	fn.glPixelMapusv(map, mapsize, values)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPRIORITIZETEXTURES(n, textures, priorities) \
	GLuint *tmp; \
	GLclampf *tmp2; \
	if(n<=0) { \
		return; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		tmp2=(GLclampf *)malloc(n*sizeof(GLclampf)); \
		if(tmp2) { \
			Atari2HostIntPtr(n, textures, tmp); \
			Atari2HostFloatArray(n, priorities, tmp2); \
			fn.glPrioritizeTextures(n, tmp, tmp2); \
			free(tmp2); \
		} \
		free(tmp); \
	}
#else
#define FN_GLPRIORITIZETEXTURES(n, textures, priorities)	fn.glPrioritizeTextures(n, textures, priorities)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPRIORITIZETEXTURESEXT(n, textures, priorities) \
	GLuint *tmp; \
	GLclampf *tmp2; \
	if(n<=0) { \
		return; \
	} \
	tmp=(GLuint *)malloc(n*sizeof(GLuint)); \
	if(tmp) { \
		tmp2=(GLclampf *)malloc(n*sizeof(GLclampf)); \
		if(tmp2) { \
			Atari2HostIntPtr(n, textures, tmp); \
			Atari2HostFloatArray(n, priorities, tmp2); \
			fn.glPrioritizeTexturesEXT(n, tmp, tmp2); \
			free(tmp2); \
		} \
		free(tmp); \
	}
#else
#define FN_GLPRIORITIZETEXTURESEXT(n, textures, priorities)	fn.glPrioritizeTexturesEXT(n, textures, priorities)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMNAMEDPARAMETER4DVNV(id, len, name, v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glProgramNamedParameter4dvNV(id, len, name, tmp)
#else
#define FN_GLPROGRAMNAMEDPARAMETER4DVNV(id, len, name, v)	fn.glProgramNamedParameter4dvNV(id, len, name, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMNAMEDPARAMETER4FVNV(id, len, name, v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glProgramNamedParameter4fvNV(id, len, name, tmp)
#else
#define FN_GLPROGRAMNAMEDPARAMETER4FVNV(id, len, name, v)	fn.glProgramNamedParameter4fvNV(id, len, name, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMPARAMETER4DVNV(target, index, v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glProgramParameter4dvNV(target, index, tmp)
#else
#define FN_GLPROGRAMPARAMETER4DVNV(target, index, v)	fn.glProgramParameter4dvNV(target, index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMPARAMETER4FVNV(target, index, v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glProgramParameter4fvNV(target, index, tmp)
#else
#define FN_GLPROGRAMPARAMETER4FVNV(target, index, v)	fn.glProgramParameter4fvNV(target, index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMPARAMETERS4DVNV(target, index, count, v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glProgramParameters4dvNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMPARAMETERS4DVNV(target, index, count, v)	fn.glProgramParameters4dvNV(target, index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMPARAMETERS4FVNV(target, index, count, v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glProgramParameters4fvNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMPARAMETERS4FVNV(target, index, count, v)	fn.glProgramParameters4fvNV(target, index, count, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS2DV(v)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glRasterPos2dv(tmp)
#else
#define FN_GLRASTERPOS2DV(v)	fn.glRasterPos2dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS2FV(v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glRasterPos2fv(tmp)
#else
#define FN_GLRASTERPOS2FV(v)	fn.glRasterPos2fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS2IV(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glRasterPos2iv(tmp)
#else
#define FN_GLRASTERPOS2IV(v)	fn.glRasterPos2iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS2SV(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glRasterPos2sv(tmp)
#else
#define FN_GLRASTERPOS2SV(v)	fn.glRasterPos2sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glRasterPos3dv(tmp)
#else
#define FN_GLRASTERPOS3DV(v)	fn.glRasterPos3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glRasterPos3fv(tmp)
#else
#define FN_GLRASTERPOS3FV(v)	fn.glRasterPos3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glRasterPos3iv(tmp)
#else
#define FN_GLRASTERPOS3IV(v)	fn.glRasterPos3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glRasterPos3sv(tmp)
#else
#define FN_GLRASTERPOS3SV(v)	fn.glRasterPos3sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRASTERPOS4DV(v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glRasterPos4dv(tmp)
#else
#define FN_GLRASTERPOS4DV(v)	fn.glRasterPos4dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRASTERPOS4FV(v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glRasterPos4fv(tmp)
#else
#define FN_GLRASTERPOS4FV(v)	fn.glRasterPos4fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS4IV(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glRasterPos4iv(tmp)
#else
#define FN_GLRASTERPOS4IV(v)	fn.glRasterPos4iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRASTERPOS4SV(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glRasterPos4sv(tmp)
#else
#define FN_GLRASTERPOS4SV(v)	fn.glRasterPos4sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLRECTDV(v1, v2)	GLdouble tmp1[4],tmp2[4]; \
	Atari2HostDoubleArray(4, v1, tmp1); \
	Atari2HostDoubleArray(4, v2, tmp2); \
	fn.glRectdv(tmp1, tmp2)
#else
#define FN_GLRECTDV(v1, v2)	fn.glRectdv(v1, v2)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLRECTFV(v1, v2)	GLfloat tmp1[4],tmp2[4]; \
	Atari2HostFloatArray(4, v1, tmp1); \
	Atari2HostFloatArray(4, v2, tmp2); \
	fn.glRectfv(tmp1, tmp2)
#else
#define FN_GLRECTFV(v1, v2)	fn.glRectfv(v1, v2)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRECTIV(v1, v2)	GLint tmp1[4],tmp2[4]; \
	Atari2HostIntPtr(4, v1, tmp1); \
	Atari2HostIntPtr(4, v2, tmp2); \
	fn.glRectiv(tmp1, tmp2)
#else
#define FN_GLRECTIV(v1, v2)	fn.glRectiv(v1, v2)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLRECTSV(v1, v2)	GLshort tmp1[4],tmp2[4]; \
	Atari2HostShortPtr(4, v1, tmp1); \
	Atari2HostShortPtr(4, v2, tmp2); \
	fn.glRectsv(tmp1, tmp2)
#else
#define FN_GLRECTSV(v1, v2)	fn.glRectsv(v1, v2)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLSECONDARYCOLOR3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glSecondaryColor3dv(tmp)
#else
#define FN_GLSECONDARYCOLOR3DV(v)	fn.glSecondaryColor3dv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLSECONDARYCOLOR3DVEXT(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glSecondaryColor3dvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3DVEXT(v)	fn.glSecondaryColor3dvEXT(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSECONDARYCOLOR3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glSecondaryColor3fv(tmp)
#else
#define FN_GLSECONDARYCOLOR3FV(v)	fn.glSecondaryColor3fv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSECONDARYCOLOR3FVEXT(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glSecondaryColor3fvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3FVEXT(v)	fn.glSecondaryColor3fvEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3hvNV(tmp)
#else
#define FN_GLSECONDARYCOLOR3HVNV(v)	fn.glSecondaryColor3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glSecondaryColor3iv(tmp)
#else
#define FN_GLSECONDARYCOLOR3IV(v)	fn.glSecondaryColor3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3IVEXT(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glSecondaryColor3ivEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3IVEXT(v)	fn.glSecondaryColor3ivEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3sv(tmp)
#else
#define FN_GLSECONDARYCOLOR3SV(v)	fn.glSecondaryColor3sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3SVEXT(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3svEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3SVEXT(v)	fn.glSecondaryColor3svEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3UIV(v)	GLuint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glSecondaryColor3uiv(tmp)
#else
#define FN_GLSECONDARYCOLOR3UIV(v)	fn.glSecondaryColor3uiv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3UIVEXT(v)	GLuint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glSecondaryColor3uivEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3UIVEXT(v)	fn.glSecondaryColor3uivEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3USV(v)	GLushort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3usv(tmp)
#else
#define FN_GLSECONDARYCOLOR3USV(v)	fn.glSecondaryColor3usv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSECONDARYCOLOR3USVEXT(v)	GLushort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glSecondaryColor3usvEXT(tmp)
#else
#define FN_GLSECONDARYCOLOR3USVEXT(v)	fn.glSecondaryColor3usvEXT(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTANGENT3DVEXT(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glTangent3dvEXT(tmp)
#else
#define FN_GLTANGENT3DVEXT(v)	fn.glTangent3dvEXT(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTANGENT3FVEXT(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glTangent3fvEXT(tmp)
#else
#define FN_GLTANGENT3FVEXT(v)	fn.glTangent3fvEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTANGENT3IVEXT(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glTangent3ivEXT(tmp)
#else
#define FN_GLTANGENT3IVEXT(v)	fn.glTangent3ivEXT(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTANGENT3SVEXT(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glTangent3svEXT(tmp)
#else
#define FN_GLTANGENT3SVEXT(v)	fn.glTangent3svEXT(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD1DV(v)	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, v, tmp); \
	fn.glTexCoord1dv(tmp)
#else
#define FN_GLTEXCOORD1DV(v)	fn.glTexCoord1dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD1FV(v)	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, v, tmp); \
	fn.glTexCoord1fv(tmp)
#else
#define FN_GLTEXCOORD1FV(v)	fn.glTexCoord1fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1HVNV(v)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glTexCoord1hvNV(tmp)
#else
#define FN_GLTEXCOORD1HVNV(v)	fn.glTexCoord1hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1IV(v)	GLint tmp[1]; \
	Atari2HostIntPtr(1, v, tmp); \
	fn.glTexCoord1iv(tmp)
#else
#define FN_GLTEXCOORD1IV(v)	fn.glTexCoord1iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD1SV(v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glTexCoord1sv(tmp)
#else
#define FN_GLTEXCOORD1SV(v)	fn.glTexCoord1sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD2DV(v)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glTexCoord2dv(tmp)
#else
#define FN_GLTEXCOORD2DV(v)	fn.glTexCoord2dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR3FVERTEX3FVSUN(tc, c, v) \
	GLfloat tmp1[2],tmp2[3],tmp3[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(3, c, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glTexCoord2fColor3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLTEXCOORD2FCOLOR3FVERTEX3FVSUN(tc, c, v)	fn.glTexCoord2fColor3fVertex3fvSUN(tc, c, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN(tc, c, n, v) \
	GLfloat tmp1[2],tmp2[4],tmp3[3],tmp4[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(4, c, tmp2); \
	Atari2HostFloatArray(3, n, tmp3); \
	Atari2HostFloatArray(3, v, tmp4); \
	fn.glTexCoord2fColor4fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3, tmp4)
#else
#define FN_GLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN(tc, c, n, v)	fn.glTexCoord2fColor4fNormal3fVertex3fvSUN(tc, c, n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FCOLOR4UBVERTEX3FVSUN(tc, c, v)	GLfloat tmp1[2],tmp2[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glTexCoord2fColor4ubVertex3fvSUN(tmp1, c, tmp2)
#else
#define FN_GLTEXCOORD2FCOLOR4UBVERTEX3FVSUN(tc, c, v)	fn.glTexCoord2fColor4ubVertex3fvSUN(tc, c, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FNORMAL3FVERTEX3FVSUN(tc, n, v)	GLfloat tmp1[2],tmp2[3],tmp3[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(3, n, tmp2); \
	Atari2HostFloatArray(3, v, tmp3); \
	fn.glTexCoord2fNormal3fVertex3fvSUN(tmp1, tmp2, tmp3)
#else
#define FN_GLTEXCOORD2FNORMAL3FVERTEX3FVSUN(tc, n, v)	fn.glTexCoord2fNormal3fVertex3fvSUN(tc, n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FVERTEX3FVSUN(tc, v)	GLfloat tmp1[2],tmp2[3]; \
	Atari2HostFloatArray(2, tc, tmp1); \
	Atari2HostFloatArray(3, v, tmp2); \
	fn.glTexCoord2fVertex3fvSUN(tmp1, tmp2)
#else
#define FN_GLTEXCOORD2FVERTEX3FVSUN(tc, v)	fn.glTexCoord2fVertex3fvSUN(tc, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD2FV(v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glTexCoord2fv(tmp)
#else
#define FN_GLTEXCOORD2FV(v)	fn.glTexCoord2fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2HVNV(v)	GLhalfNV tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glTexCoord2hvNV(tmp)
#else
#define FN_GLTEXCOORD2HVNV(v)	fn.glTexCoord2hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2IV(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glTexCoord2iv(tmp)
#else
#define FN_GLTEXCOORD2IV(v)	fn.glTexCoord2iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD2SV(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glTexCoord2sv(tmp)
#else
#define FN_GLTEXCOORD2SV(v)	fn.glTexCoord2sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glTexCoord3dv(tmp)
#else
#define FN_GLTEXCOORD3DV(v)	fn.glTexCoord3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glTexCoord3fv(tmp)
#else
#define FN_GLTEXCOORD3FV(v)	fn.glTexCoord3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3HVNV(v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glTexCoord3hvNV(tmp)
#else
#define FN_GLTEXCOORD3HVNV(v)	fn.glTexCoord3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glTexCoord3iv(tmp)
#else
#define FN_GLTEXCOORD3IV(v)	fn.glTexCoord3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glTexCoord3sv(tmp)
#else
#define FN_GLTEXCOORD3SV(v)	fn.glTexCoord3sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXCOORD4DV(v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glTexCoord4dv(tmp)
#else
#define FN_GLTEXCOORD4DV(v)	fn.glTexCoord4dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUN(tc, c, n, v) \
	GLfloat tmp1[4],tmp2[4],tmp3[3],tmp4[4]; \
	Atari2HostFloatArray(4, tc, tmp1); \
	Atari2HostFloatArray(4, c, tmp2); \
	Atari2HostFloatArray(3, n, tmp3); \
	Atari2HostFloatArray(4, v, tmp4); \
	fn.glTexCoord4fColor4fNormal3fVertex4fvSUN(tmp1, tmp2, tmp3, tmp4)
#else
#define FN_GLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUN(tc, c, n, v)	fn.glTexCoord4fColor4fNormal3fVertex4fvSUN(tc, c, n, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FVERTEX4FVSUN(tc, v) \
	GLfloat tmp1[4],tmp2[4]; \
	Atari2HostFloatArray(4, tc, tmp1); \
	Atari2HostFloatArray(4, v, tmp2); \
	fn.glTexCoord4fVertex4fvSUN(tmp1, tmp2)
#else
#define FN_GLTEXCOORD4FVERTEX4FVSUN(tc, v)	fn.glTexCoord4fVertex4fvSUN(tc, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXCOORD4FV(v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glTexCoord4fv(tmp)
#else
#define FN_GLTEXCOORD4FV(v)	fn.glTexCoord4fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4HVNV(v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glTexCoord4hvNV(tmp)
#else
#define FN_GLTEXCOORD4HVNV(v)	fn.glTexCoord4hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4IV(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glTexCoord4iv(tmp)
#else
#define FN_GLTEXCOORD4IV(v)	fn.glTexCoord4iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXCOORD4SV(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glTexCoord4sv(tmp)
#else
#define FN_GLTEXCOORD4SV(v)	fn.glTexCoord4sv(v)
#endif

#define FN_GLTEXCOORDPOINTER(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, stride, -1, 0, pointer)

#define FN_GLTEXCOORDPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].texcoord.vendor = 1

#define FN_GLTEXCOORDPOINTERVINTEL(size, type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, 0, -1, 0, pointer); \
	contexts[cur_context].texcoord.vendor = 2

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXENVFV(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glTexEnvfv(target, pname, tmp)
#else
#define FN_GLTEXENVFV(target, pname, params) \
	fn.glTexEnvfv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXENVIV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glTexEnviv(target, pname, tmp)
#else
#define FN_GLTEXENVIV(target, pname, params) \
	fn.glTexEnviv(target, pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLTEXGENDV(coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(size, 16)]; \
	Atari2HostDoubleArray(size, params, tmp); \
	fn.glTexGendv(coord, pname, tmp)
#else
#define FN_GLTEXGENDV(coord, pname, params) \
	fn.glTexGendv(coord, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXGENFV(coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glTexGenfv(coord, pname, tmp)
#else
#define FN_GLTEXGENFV(coord, pname, params) \
	fn.glTexGenfv(coord, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXGENIV(coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(4, params, tmp); \
	fn.glTexGeniv(coord, pname, tmp)
#else
#define FN_GLTEXGENIV(coord, pname, params)	\
	fn.glTexGeniv(coord, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXPARAMETERFV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glTexParameterfv(target, pname, tmp)
#else
#define FN_GLTEXPARAMETERFV(target, pname, params) \
	fn.glTexParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXPARAMETERIV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glTexParameteriv(target, pname, tmp)
#else
#define FN_GLTEXPARAMETERIV(target, pname, params) \
	fn.glTexParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM3FV(location, count, value) \
	GLint const size = 3 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform3fv(location, count, tmp)
#else
#define FN_GLUNIFORM3FV(location, count, value) \
	fn.glUniform3fv(location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX2DV(v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertex2dv(tmp)
#else
#define FN_GLVERTEX2DV(v) fn.glVertex2dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX2FV(v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertex2fv(tmp)
#else
#define FN_GLVERTEX2FV(v) fn.glVertex2fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2HVNV(v) \
	GLint const size = 2; \
	GLhalfNV tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertex2hvNV(tmp)
#else
#define FN_GLVERTEX2HVNV(v)	fn.glVertex2hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2IV(v) \
	GLint const size = 2; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertex2iv(tmp)
#else
#define FN_GLVERTEX2IV(v)	fn.glVertex2iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX2SV(v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertex2sv(tmp)
#else
#define FN_GLVERTEX2SV(v) fn.glVertex2sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX3DV(v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertex3dv(tmp)
#else
#define FN_GLVERTEX3DV(v)	fn.glVertex3dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX3FV(v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glVertex3fv(tmp)
#else
#define FN_GLVERTEX3FV(v)	fn.glVertex3fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3HVNV(v) \
	GLint const size = 4; \
	GLhalfNV tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertex3hvNV(tmp)
#else
#define FN_GLVERTEX3HVNV(v)	fn.glVertex3hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3IV(v) \
	GLint const size = 3; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertex3iv(tmp)
#else
#define FN_GLVERTEX3IV(v) fn.glVertex3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX3SV(v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertex3sv(tmp)
#else
#define FN_GLVERTEX3SV(v) fn.glVertex3sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEX4DV(v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertex4dv(tmp)
#else
#define FN_GLVERTEX4DV(v) fn.glVertex4dv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEX4FV(v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertex4fv(tmp)
#else
#define FN_GLVERTEX4FV(v) fn.glVertex4fv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4HVNV(v) \
	GLint const size = 4; \
	GLhalfNV tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertex4hvNV(tmp)
#else
#define FN_GLVERTEX4HVNV(v) fn.glVertex4hvNV(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4IV(v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertex4iv(tmp)
#else
#define FN_GLVERTEX4IV(v) fn.glVertex4iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEX4SV(v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertex4sv(tmp)
#else
#define FN_GLVERTEX4SV(v) fn.glVertex4sv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DVARB(index, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib1dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DVARB(index, v) fn.glVertexAttrib1dvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DVNV(index, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib1dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DVNV(index, v) fn.glVertexAttrib1dvNV(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FVARB(index, v) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib1fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FVARB(index, v) fn.glVertexAttrib1fvARB(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FVNV(index, v) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib1fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FVNV(index, v) fn.glVertexAttrib1fvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1HVNV(index, v) \
	GLint const size = 1; \
	GLhalfNV tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib1hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1HVNV(index, v) fn.glVertexAttrib1hvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SVARB(index, v) \
	GLint const size = 1; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib1svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SVARB(index, v) fn.glVertexAttrib1svARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SVNV(index, v) \
	GLint const size = 1; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib1svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SVNV(index, v) fn.glVertexAttrib1svNV(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DVARB(index, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib2dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DVARB(index, v) fn.glVertexAttrib2dvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DVNV(index, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib2dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DVNV(index, v) fn.glVertexAttrib2dvNV(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FVARB(index, v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib2fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FVARB(index, v) fn.glVertexAttrib2fvARB(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FVNV(index, v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib2fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FVNV(index, v) fn.glVertexAttrib2fvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2HVNV(index, v) \
	GLint const size = 2; \
	GLhalfNV tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib2hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2HVNV(index, v) fn.glVertexAttrib2hvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SVARB(index, v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib2svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SVARB(index, v) fn.glVertexAttrib2svARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SVNV(index, v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib2svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SVNV(index, v) fn.glVertexAttrib2svNV(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DVARB(index, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib3dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DVARB(index, v) fn.glVertexAttrib3dvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DVNV(index, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib3dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DVNV(index, v) fn.glVertexAttrib3dvNV(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FVARB(index, v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib3fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FVARB(index, v) fn.glVertexAttrib3fvARB(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FVNV(index, v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib3fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FVNV(index, v) fn.glVertexAttrib3fvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3HVNV(index, v) \
	GLint const size = 3; \
	GLhalfNV tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib3hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3HVNV(index, v) fn.glVertexAttrib3hvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SVARB(index, v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib3svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SVARB(index, v) fn.glVertexAttrib3svARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SVNV(index, v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib3svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SVNV(index, v) fn.glVertexAttrib3svNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NIVARB(index, v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertexAttrib4NivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NIVARB(index, v) fn.glVertexAttrib4NivARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NSV(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4Nsv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NSV(index, v) fn.glVertexAttrib4Nsv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NSVARB(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4NsvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NSVARB(index, v) fn.glVertexAttrib4NsvARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUIVARB(index, v) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertexAttrib4NuivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUIVARB(index, v) fn.glVertexAttrib4NuivARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUSV(index, v) \
	GLint const size = 4; \
	GLushort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4Nusv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUSV(index, v) fn.glVertexAttrib4Nusv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUSVARB(index, v) \
	GLint const size = 4; \
	GLushort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4NusvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUSVARB(index, v) fn.glVertexAttrib4NusvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DVARB(index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib4dvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DVARB(index, v) fn.glVertexAttrib4dvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DVNV(index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib4dvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DVNV(index, v) fn.glVertexAttrib4dvNV(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FVARB(index, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib4fvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FVARB(index, v) fn.glVertexAttrib4fvARB(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FVNV(index, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib4fvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FVNV(index, v) fn.glVertexAttrib4fvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4HVNV(index, v) \
	GLint const size = 4; \
	GLhalfNV tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4hvNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4HVNV(index, v) fn.glVertexAttrib4hvNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4IVARB(index, v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertexAttrib4ivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4IVARB(index, v) fn.glVertexAttrib4ivARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SV(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SV(index, v) fn.glVertexAttrib4sv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SVARB(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4svARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SVARB(index, v) fn.glVertexAttrib4svARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4SVNV(index, v) \
	GLint const size = 4; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4svNV(index, tmp)
#else
#define FN_GLVERTEXATTRIB4SVNV(index, v) fn.glVertexAttrib4svNV(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4UIVARB(index, v) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertexAttrib4uivARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UIVARB(index, v) fn.glVertexAttrib4uivARB(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4USVARB(index, v) \
	GLint const size = 4; \
	GLushort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4usvARB(index, tmp)
#else
#define FN_GLVERTEXATTRIB4USVARB(index, v) fn.glVertexAttrib4usvARB(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS1DVNV(index, count, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribs1dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1DVNV(index, count, v) fn.glVertexAttribs1dvNV(index, count, v)
#endif

/* YYY */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS1FVNV(index, count, v)	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, v, tmp); \
	fn.glVertexAttribs1fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1FVNV(index, count, v)	fn.glVertexAttribs1fvNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS1HVNV(index, n, v)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glVertexAttribs1hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS1HVNV(index, n, v)	fn.glVertexAttribs1hvNV(index, n, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS1SVNV(index, count, v)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, v, tmp); \
	fn.glVertexAttribs1svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS1SVNV(index, count, v)	fn.glVertexAttribs1svNV(index, count, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS2DVNV(index, count, v)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glVertexAttribs2dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2DVNV(index, count, v)	fn.glVertexAttribs2dvNV(index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS2FVNV(index, count, v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glVertexAttribs2fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2FVNV(index, count, v)	fn.glVertexAttribs2fvNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS2HVNV(index, n, v)	GLhalfNV tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertexAttribs2hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS2HVNV(index, n, v)	fn.glVertexAttribs2hvNV(index, n, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS2SVNV(index, count, v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glVertexAttribs2svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS2SVNV(index, count, v)	fn.glVertexAttribs2svNV(index, count, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS3DVNV(index, count, v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glVertexAttribs3dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3DVNV(index, count, v)	fn.glVertexAttribs3dvNV(index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS3FVNV(index, count, v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glVertexAttribs3fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3FVNV(index, count, v)	fn.glVertexAttribs3fvNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS3HVNV(index, n, v)	GLhalfNV tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertexAttribs3hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS3HVNV(index, n, v)	fn.glVertexAttribs3hvNV(index, n, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS3SVNV(index, count, v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glVertexAttribs3svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS3SVNV(index, count, v)	fn.glVertexAttribs3svNV(index, count, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBS4DVNV(index, count, v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glVertexAttribs4dvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4DVNV(index, count, v)	fn.glVertexAttribs4dvNV(index, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIBS4FVNV(index, count, v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glVertexAttribs4fvNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4FVNV(index, count, v)	fn.glVertexAttribs4fvNV(index, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS4HVNV(index, n, v)	GLhalfNV tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttribs4hvNV(index, n, tmp)
#else
#define FN_GLVERTEXATTRIBS4HVNV(index, n, v)	fn.glVertexAttribs4hvNV(index, n, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIBS4SVNV(index, count, v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glVertexAttribs4svNV(index, count, tmp)
#else
#define FN_GLVERTEXATTRIBS4SVNV(index, count, v)	fn.glVertexAttribs4svNV(index, count, v)
#endif

#define FN_GLVERTEXPOINTER(size, type, stride, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].vertex, size, type, stride, -1, 0, pointer)

#define FN_GLVERTEXPOINTERLISTIBM(size, type, stride, pointer, ptrstride) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].vertex, size, type, stride, -1, ptrstride, pointer); \
	contexts[cur_context].vertex.vendor = 1

#define FN_GLVERTEXPOINTERVINTEL(size, type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].vertex, size, type, 0, -1, 0, pointer); \
	contexts[cur_context].vertex.vendor = 2

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM1DVATI(stream, coords)	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, coords, tmp); \
	fn.glVertexStream1dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1DVATI(stream, coords)	fn.glVertexStream1dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM1FVATI(stream, coords)	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, coords, tmp); \
	fn.glVertexStream1fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1FVATI(stream, coords)	fn.glVertexStream1fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM1IVATI(stream, coords)	GLint tmp[1]; \
	Atari2HostIntPtr(1, coords, tmp); \
	fn.glVertexStream1ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1IVATI(stream, coords)	fn.glVertexStream1ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM1SVATI(stream, coords)	GLshort tmp[1]; \
	Atari2HostShortPtr(1, coords, tmp); \
	fn.glVertexStream1svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM1SVATI(stream, coords)	fn.glVertexStream1svATI(stream, coords)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM2DVATI(stream, coords)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, coords, tmp); \
	fn.glVertexStream2dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2DVATI(stream, coords)	fn.glVertexStream2dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM2FVATI(stream, coords)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, coords, tmp); \
	fn.glVertexStream2fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2FVATI(stream, coords)	fn.glVertexStream2fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM2IVATI(stream, coords)	GLint tmp[2]; \
	Atari2HostIntPtr(2, coords, tmp); \
	fn.glVertexStream2ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2IVATI(stream, coords)	fn.glVertexStream2ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM2SVATI(stream, coords)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, coords, tmp); \
	fn.glVertexStream2svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM2SVATI(stream, coords)	fn.glVertexStream2svATI(stream, coords)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM3DVATI(stream, coords)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, coords, tmp); \
	fn.glVertexStream3dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3DVATI(stream, coords)	fn.glVertexStream3dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM3FVATI(stream, coords)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, coords, tmp); \
	fn.glVertexStream3fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3FVATI(stream, coords)	fn.glVertexStream3fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM3IVATI(stream, coords)	GLint tmp[3]; \
	Atari2HostIntPtr(3, coords, tmp); \
	fn.glVertexStream3ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3IVATI(stream, coords)	fn.glVertexStream3ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM3SVATI(stream, coords)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, coords, tmp); \
	fn.glVertexStream3svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM3SVATI(stream, coords)	fn.glVertexStream3svATI(stream, coords)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXSTREAM4DVATI(stream, coords)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, coords, tmp); \
	fn.glVertexStream4dvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4DVATI(stream, coords)	fn.glVertexStream4dvATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXSTREAM4FVATI(stream, coords)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, coords, tmp); \
	fn.glVertexStream4fvATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4FVATI(stream, coords)	fn.glVertexStream4fvATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM4IVATI(stream, coords)	GLint tmp[4]; \
	Atari2HostIntPtr(4, coords, tmp); \
	fn.glVertexStream4ivATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4IVATI(stream, coords)	fn.glVertexStream4ivATI(stream, coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXSTREAM4SVATI(stream, coords)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, coords, tmp); \
	fn.glVertexStream4svATI(stream, tmp)
#else
#define FN_GLVERTEXSTREAM4SVATI(stream, coords)	fn.glVertexStream4svATI(stream, coords)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXWEIGHTFVEXT(weight)	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, weight, tmp); \
	fn.glVertexWeightfvEXT(tmp)
#else
#define FN_GLVERTEXWEIGHTFVEXT(weight)	fn.glVertexWeightfvEXT(weight)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXWEIGHTHVNV(weight)	GLhalfNV tmp[1]; \
	Atari2HostShortPtr(1, weight, tmp); \
	fn.glVertexWeighthvNV(tmp)
#else
#define FN_GLVERTEXWEIGHTHVNV(weight)	fn.glVertexWeighthvNV(weight)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DV(v)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glWindowPos2dv(tmp)
#else
#define FN_GLWINDOWPOS2DV(v)	fn.glWindowPos2dv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DVARB(v)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glWindowPos2dvARB(tmp)
#else
#define FN_GLWINDOWPOS2DVARB(v)	fn.glWindowPos2dvARB(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS2DVMESA(v)	GLdouble tmp[2]; \
	Atari2HostDoubleArray(2, v, tmp); \
	fn.glWindowPos2dvMESA(tmp)
#else
#define FN_GLWINDOWPOS2DVMESA(v)	fn.glWindowPos2dvMESA(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FV(v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glWindowPos2fv(tmp)
#else
#define FN_GLWINDOWPOS2FV(v)	fn.glWindowPos2fv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FVARB(v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glWindowPos2fvARB(tmp)
#else
#define FN_GLWINDOWPOS2FVARB(v)	fn.glWindowPos2fvARB(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS2FVMESA(v)	GLfloat tmp[2]; \
	Atari2HostFloatArray(2, v, tmp); \
	fn.glWindowPos2fvMESA(tmp)
#else
#define FN_GLWINDOWPOS2FVMESA(v)	fn.glWindowPos2fvMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IV(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glWindowPos2iv(tmp)
#else
#define FN_GLWINDOWPOS2IV(v)	fn.glWindowPos2iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IVARB(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glWindowPos2ivARB(tmp)
#else
#define FN_GLWINDOWPOS2IVARB(v)	fn.glWindowPos2ivARB(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2IVMESA(v)	GLint tmp[2]; \
	Atari2HostIntPtr(2, v, tmp); \
	fn.glWindowPos2ivMESA(tmp)
#else
#define FN_GLWINDOWPOS2IVMESA(v)	fn.glWindowPos2ivMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SV(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glWindowPos2sv(tmp)
#else
#define FN_GLWINDOWPOS2SV(v)	fn.glWindowPos2sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SVARB(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glWindowPos2svARB(tmp)
#else
#define FN_GLWINDOWPOS2SVARB(v)	fn.glWindowPos2svARB(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS2SVMESA(v)	GLshort tmp[2]; \
	Atari2HostShortPtr(2, v, tmp); \
	fn.glWindowPos2svMESA(tmp)
#else
#define FN_GLWINDOWPOS2SVMESA(v)	fn.glWindowPos2svMESA(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DV(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glWindowPos3dv(tmp)
#else
#define FN_GLWINDOWPOS3DV(v)	fn.glWindowPos3dv(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DVARB(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glWindowPos3dvARB(tmp)
#else
#define FN_GLWINDOWPOS3DVARB(v)	fn.glWindowPos3dvARB(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS3DVMESA(v)	GLdouble tmp[3]; \
	Atari2HostDoubleArray(3, v, tmp); \
	fn.glWindowPos3dvMESA(tmp)
#else
#define FN_GLWINDOWPOS3DVMESA(v)	fn.glWindowPos3dvMESA(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FV(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glWindowPos3fv(tmp)
#else
#define FN_GLWINDOWPOS3FV(v)	fn.glWindowPos3fv(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FVARB(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glWindowPos3fvARB(tmp)
#else
#define FN_GLWINDOWPOS3FVARB(v)	fn.glWindowPos3fvARB(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS3FVMESA(v)	GLfloat tmp[3]; \
	Atari2HostFloatArray(3, v, tmp); \
	fn.glWindowPos3fvMESA(tmp)
#else
#define FN_GLWINDOWPOS3FVMESA(v)	fn.glWindowPos3fvMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IV(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glWindowPos3iv(tmp)
#else
#define FN_GLWINDOWPOS3IV(v)	fn.glWindowPos3iv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IVARB(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glWindowPos3ivARB(tmp)
#else
#define FN_GLWINDOWPOS3IVARB(v)	fn.glWindowPos3ivARB(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3IVMESA(v)	GLint tmp[3]; \
	Atari2HostIntPtr(3, v, tmp); \
	fn.glWindowPos3ivMESA(tmp)
#else
#define FN_GLWINDOWPOS3IVMESA(v)	fn.glWindowPos3ivMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SV(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glWindowPos3sv(tmp)
#else
#define FN_GLWINDOWPOS3SV(v)	fn.glWindowPos3sv(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SVARB(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glWindowPos3svARB(tmp)
#else
#define FN_GLWINDOWPOS3SVARB(v)	fn.glWindowPos3svARB(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS3SVMESA(v)	GLshort tmp[3]; \
	Atari2HostShortPtr(3, v, tmp); \
	fn.glWindowPos3svMESA(tmp)
#else
#define FN_GLWINDOWPOS3SVMESA(v)	fn.glWindowPos3svMESA(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS4DVMESA(v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glWindowPos4dvMESA(tmp)
#else
#define FN_GLWINDOWPOS4DVMESA(v)	fn.glWindowPos4dvMESA(v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLWINDOWPOS4DVMESA(v)	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, v, tmp); \
	fn.glWindowPos4dvMESA(tmp)
#else
#define FN_GLWINDOWPOS4DVMESA(v)	fn.glWindowPos4dvMESA(v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWINDOWPOS4FVMESA(v)	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, v, tmp); \
	fn.glWindowPos4fvMESA(tmp)
#else
#define FN_GLWINDOWPOS4FVMESA(v)	fn.glWindowPos4fvMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS4IVMESA(v)	GLint tmp[4]; \
	Atari2HostIntPtr(4, v, tmp); \
	fn.glWindowPos4ivMESA(tmp)
#else
#define FN_GLWINDOWPOS4IVMESA(v)	fn.glWindowPos4ivMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLWINDOWPOS4SVMESA(v)	GLshort tmp[4]; \
	Atari2HostShortPtr(4, v, tmp); \
	fn.glWindowPos4svMESA(tmp)
#else
#define FN_GLWINDOWPOS4SVMESA(v)	fn.glWindowPos4svMESA(v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLAREPROGRAMSRESIDENTNV(n, programs, residences) \
	GLboolean res; \
	if (programs) { \
		GLuint *tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
		Atari2HostIntArray(n, programs, tmp); \
		res = fn.glAreProgramsResidentNV(n, tmp, residences); \
		free(tmp); \
	} else { \
		res = fn.glAreProgramsResidentNV(n, programs, residences); \
	} \
	return res
#else
#define FN_GLAREPROGRAMSRESIDENTNV(n, programs, residences)	return fn.glAreProgramsResidentNV(n, programs, residences)
#endif

void OSMesaDriver::gl_bind_buffer(GLenum target, GLuint buffer, GLuint first, GLuint count)
{
	fbo_buffer *fbo;
	
	switch (target) {
	case GL_ARRAY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.array;
		break;
	case GL_ATOMIC_COUNTER_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.atomic_counter;
		break;
	case GL_COPY_READ_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.copy_read;
		contexts[cur_context].buffer_bindings.copy_read.first = first;
		contexts[cur_context].buffer_bindings.copy_read.first = count;
		break;
	case GL_COPY_WRITE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.copy_write;
		break;
	case GL_DISPATCH_INDIRECT_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.dispatch_indirect;
		break;
	case GL_DRAW_INDIRECT_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.draw_indirect;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.element_array;
		break;
	case GL_PIXEL_PACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.pixel_pack;
		break;
	case GL_PIXEL_UNPACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.pixel_unpack;
		break;
	case GL_QUERY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.query;
		break;
	case GL_SHADER_STORAGE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.shader_storage;
		break;
	case GL_TEXTURE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.texture;
		break;
	case GL_TRANSFORM_FEEDBACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.transform_feedback;
		break;
	case GL_UNIFORM_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.uniform;
		break;
	default:
		return;
	}
	fbo->id = buffer;
	fbo->first = first;
	fbo->count = count;
}


void OSMesaDriver::gl_get_pointer(GLenum target, GLuint index, void **data)
{
	fbo_buffer *fbo;
	
	UNUSED(index); // FIXME
	switch (target) {
	case GL_ARRAY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.array;
		break;
	case GL_ATOMIC_COUNTER_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.atomic_counter;
		break;
	case GL_COPY_READ_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.copy_read;
		break;
	case GL_COPY_WRITE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.copy_write;
		break;
	case GL_DISPATCH_INDIRECT_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.dispatch_indirect;
		break;
	case GL_DRAW_INDIRECT_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.draw_indirect;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.element_array;
		break;
	case GL_PIXEL_PACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.pixel_pack;
		break;
	case GL_PIXEL_UNPACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.pixel_unpack;
		break;
	case GL_QUERY_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.query;
		break;
	case GL_SHADER_STORAGE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.shader_storage;
		break;
	case GL_TEXTURE_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.texture;
		break;
	case GL_TRANSFORM_FEEDBACK_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.transform_feedback;
		break;
	case GL_UNIFORM_BUFFER:
		fbo = &contexts[cur_context].buffer_bindings.uniform;
		break;
	case GL_SELECTION_BUFFER_POINTER:
		*data = (void *)contexts[cur_context].select_buffer_atari;
		return;
	case GL_FEEDBACK_BUFFER_POINTER:
		*data = (void *)contexts[cur_context].feedback_buffer_atari;
		return;
	default:
		glSetError(GL_INVALID_ENUM);
		return;
	}
	*data = fbo->atari_pointer; // not sure about this
}

#define FN_GLBINDBUFFER(target, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBuffer(target, buffer)

#define FN_GLBINDBUFFERBASE(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBase(target, index, buffer)

#define FN_GLBINDBUFFERBASEEXT(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBaseEXT(target, index, buffer)

#define FN_GLBINDBUFFERBASENV(target, index, buffer) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferBaseNV(target, index, buffer)

#define FN_GLBINDBUFFERRANGE(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRange(target, index, buffer, offset, size)

#define FN_GLBINDBUFFERRANGEEXT(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRangeEXT(target, index, buffer, offset, size)

#define FN_GLBINDBUFFERRANGENV(target, index, buffer, offset, size) \
	gl_bind_buffer(target, buffer, 0, 0); \
	fn.glBindBufferRangeNV(target, index, buffer, offset, size)

/* #define FN_GLULOOKAT(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ) */

/* glBinormalPointerEXT belongs to never completed EXT_coordinate_frame */
#define FN_GLBINORMALPOINTEREXT(type, stride, pointer) \
	fn.glBinormalPointerEXT(type, stride, pointer)

/* glTangentPointerEXT belongs to never completed EXT_coordinate_frame */
#define FN_GLTANGENTPOINTEREXT(type, stride, pointer) \
	fn.glTangentPointerEXT(type, stride, pointer)

/* nothing to do */
#define FN_GLBUFFERDATA(target, size, data, usage) \
	fn.glBufferData(target, size, data, usage)

/* nothing to do */
#define FN_GLBUFFERDATAARB(target, size, data, usage) \
	fn.glBufferDataARB(target, size, data, usage)

/* nothing to do */
#define FN_GLNAMEDBUFFERSTORAGE(buffer, size, data, flags) \
	fn.glNamedBufferStorage(buffer, size, data, flags)

/* nothing to do */
#define FN_GLBUFFERSUBDATA(target, offset, size, data) \
	fn.glBufferSubData(target, offset, size, data)

/* nothing to do */
#define FN_GLBUFFERSUBDATAARB(target, offset, size, data) \
	fn.glBufferSubDataARB(target, offset, size, data)

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARBUFFERDATA(target, internalformat, format, type, data) \
	void *tmp = convertPixels(1, 1, 1, format, type, data); \
	fn.glClearBufferData(target, internalformat, format, type, tmp); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCLEARBUFFERDATA(target, internalformat, format, type, data) \
	fn.glClearBufferData(target, internalformat, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARNAMEDBUFFERDATAEXT(buffer, internalformat, format, type, data) \
	void *tmp = convertPixels(1, 1, 1, format, type, data); \
	fn.glClearNamedBufferDataEXT(buffer, internalformat, format, type, data); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCLEARNAMEDBUFFERDATAEXT(buffer, internalformat, format, type, data) \
	fn.glClearNamedBufferDataEXT(buffer, internalformat, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARBUFFERSUBDATA(target, internalformat, offset, size, format, type, data) \
	void *tmp = convertPixels(1, 1, 1, format, type, data); \
	fn.glClearBufferSubData(target, internalformat, offset, size, format, type, tmp); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCLEARBUFFERSUBDATA(target, internalformat, offset, size, format, type, data) \
	fn.glClearBufferSubData(target, internalformat, offset, size, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCLEARNAMEDBUFFERSUBDATAEXT(buffer, internalformat, offset, size, format, type, data) \
	void *tmp = convertPixels(1, 1, 1, format, type, data); \
	fn.glClearNamedBufferSubDataEXT(buffer, internalformat, offset, size, format, type, tmp); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCLEARNAMEDBUFFERSUBDATAEXT(buffer, internalformat, offset, size, format, type, data) \
	fn.glClearNamedBufferSubDataEXT(buffer, internalformat, offset, size, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARBUFFERIV(buffer, drawbuffer, value) \
	if (value) { \
		GLint tmp[4]; \
		switch (buffer) { \
			case GL_COLOR: Atari2HostIntPtr(4, value, tmp); break; \
			case GL_STENCIL: \
			case GL_DEPTH: Atari2HostIntPtr(1, value, tmp); break; \
		} \
		fn.glClearBufferiv(buffer, drawbuffer, tmp); \
	} else { \
		fn.glClearBufferiv(buffer, drawbuffer, value); \
	}
#else
#define FN_GLCLEARBUFFERIV(buffer, drawbuffer, value) fn.glClearBufferiv(buffer, drawbuffer, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARBUFFERUIV(buffer, drawbuffer, value) \
	if (value) { \
		GLuint tmp[4]; \
		switch (buffer) { \
			case GL_COLOR: Atari2HostIntPtr(4, value, tmp); break; \
			case GL_STENCIL: \
			case GL_DEPTH: Atari2HostIntPtr(1, value, tmp); break; \
		} \
		fn.glClearBufferuiv(buffer, drawbuffer, tmp); \
	} else { \
		fn.glClearBufferuiv(buffer, drawbuffer, value); \
	}
#else
#define FN_GLCLEARBUFFERUIV(buffer, drawbuffer, value) fn.glClearBufferuiv(buffer, drawbuffer, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCLEARBUFFERFV(buffer, drawbuffer, value) \
	if (value) { \
		GLfloat tmp[4]; \
		switch (buffer) { \
			case GL_COLOR: Atari2HostFloatArray(4, value, tmp); break; \
			case GL_STENCIL: \
			case GL_DEPTH: Atari2HostFloatArray(1, value, tmp); break; \
		} \
		fn.glClearBufferfv(buffer, drawbuffer, tmp); \
	} else { \
		fn.glClearBufferfv(buffer, drawbuffer, value); \
	}
#else
#define FN_GLCLEARBUFFERFV(buffer, drawbuffer, value) fn.glClearBufferfv(buffer, drawbuffer, value)
#endif

/* FIXME for glTexImage*, glTexSubImage* etc:
If a non-zero named buffer object is bound to the
GL_PIXEL_UNPACK_BUFFER target (see glBindBuffer) while a texture image
is specified, data is treated as a byte offset into the buffer object's
data store.
*/

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE1D(target, level, internalformat, width, border, format, type, pixels) \
	void *tmp = convertPixels(width, 1, 1, format, type, pixels); \
	fn.glTexImage1D(target, level, internalformat, width, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLTEXIMAGE1D(target, level, internalformat, width, border, format, type, pixels) \
	fn.glTexImage1D(target, level, internalformat, width, border, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE2D(target, level, internalformat, width, height, border, format, type, pixels) \
	void *tmp = convertPixels(width, height, 1, format, type, pixels); \
	fn.glTexImage2D(target, level, internalformat, width, height, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLTEXIMAGE2D(target, level, internalformat, width, height, border, format, type, pixels) \
	fn.glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTEXIMAGE3D(target, level, internalformat, width, height, depth, border, format, type, pixels) \
	void *tmp = convertPixels(width, height, depth, format, type, pixels); \
	fn.glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLTEXIMAGE3D(target, level, internalformat, width, height, depth, border, format, type, pixels) fn.glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORSUBTABLEEXT(target, start, count, format, type, table) \
	void *tmp = convertPixels(count, 1, 1, format, type, table); \
	fn.glColorSubTableEXT(target, start, count, format, type, tmp); \
	if (tmp != table) free(tmp)
#else
#define FN_GLCOLORSUBTABLEEXT(target, start, count, format, type, table) fn.glColorSubTableEXT(target, start, count, format, type, table)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLEEXT(target, internalformat, width, format, type, table) \
	void *tmp = convertPixels(width, 1, 1, format, type, table); \
	fn.glColorTableEXT(target, internalformat, width, format, type, tmp); \
	if (tmp != table) free(tmp)
#else
#define FN_GLCOLORTABLEEXT(target, internalformat, width, format, type, table) fn.glColorTableEXT(target, internalformat, width, format, type, table)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLORTABLEPARAMETERFV(target, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		Atari2HostFloatArray(4, params, tmp); \
		fn.glColorTableParameterfv(target, pname, tmp); \
	} else { \
		fn.glColorTableParameterfv(target, pname, params); \
	}
#else
#define FN_GLCOLORTABLEPARAMETERFV(target, pname, params) fn.glColorTableParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOLORTABLEPARAMETERFVSGI(target, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		Atari2HostFloatArray(4, params, tmp); \
		fn.glColorTableParameterfvSGI(target, pname, tmp); \
	} else { \
		fn.glColorTableParameterfvSGI(target, pname, params); \
	}
#else
#define FN_GLCOLORTABLEPARAMETERFVSGI(target, pname, params) fn.glColorTableParameterfvSGI(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORTABLEPARAMETERIV(target, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		Atari2HostIntPtr(4, params, tmp); \
		fn.glColorTableParameteriv(target, pname, tmp); \
	} else { \
		fn.glColorTableParameteriv(target, pname, params); \
	}
#else
#define FN_GLCOLORTABLEPARAMETERIV(target, pname, params) fn.glColorTableParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOLORTABLEPARAMETERIVSGI(target, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		Atari2HostIntPtr(4, params, tmp); \
		fn.glColorTableParameterivSGI(target, pname, tmp); \
	} else { \
		fn.glColorTableParameterivSGI(target, pname, params); \
	}
#else
#define FN_GLCOLORTABLEPARAMETERIVSGI(target, pname, params) fn.glColorTableParameterivSGI(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCOLORTABLESGI(target, internalformat, width, format, type, table) \
	void *tmp = convertPixels(width, 1, 1, format, type, table); \
	fn.glColorTableSGI(target, internalformat, width, format, type, tmp); \
	if (tmp != table) free(tmp)
#else
#define FN_GLCOLORTABLESGI(target, internalformat, width, format, type, table)	fn.glColorTableSGI(target, internalformat, width, format, type, table)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOMBINERPARAMETERFVNV(pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostFloatArray(size, params, tmp); \
		fn.glCombinerParameterfvNV(pname, tmp); \
	} else { \
		fn.glCombinerParameterfvNV(pname, params); \
	}
#else
#define FN_GLCOMBINERPARAMETERFVNV(pname, params) fn.glCombinerParameterfvNV(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCOMBINERPARAMETERIVNV(pname, params) \
	if (params) { \
		GLint tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glCombinerParameterivNV(pname, tmp); \
	} else { \
		fn.glCombinerParameterivNV(pname, params); \
	}
#else
#define FN_GLCOMBINERPARAMETERIVNV(pname, params) fn.glCombinerParameterivNV(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOMBINERSTAGEPARAMETERFVNV(stage, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostFloatArray(size, params, tmp); \
		fn.glCombinerStageParameterfvNV(stage, pname, tmp); \
	} else { \
		fn.glCombinerStageParameterfvNV(stage, pname, params); \
	}
#else
#define FN_GLCOMBINERSTAGEPARAMETERFVNV(stage, pname, params) fn.glCombinerStageParameterfvNV(stage, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER1D(target, internalformat, width, format, type, image) \
	void *tmp = convertPixels(width, 1, 1, format, type, image); \
	fn.glConvolutionFilter1D(target, internalformat, width, format, type, tmp); \
	if (tmp != image) free(tmp)
#else
#define FN_GLCONVOLUTIONFILTER1D(target, internalformat, width, format, type, image) fn.glConvolutionFilter1D(target, internalformat, width, format, type, image)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER2D(target, internalformat, width, height, format, type, image) \
	void *tmp = convertPixels(width, height, 1, format, type, image); \
	fn.glConvolutionFilter2D(target, internalformat, width, height, format, type, tmp); \
	if (tmp != image) free(tmp)
#else
#define FN_GLCONVOLUTIONFILTER2D(target, internalformat, width, height, format, type, image) fn.glConvolutionFilter2D(target, internalformat, width, height, format, type, image)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER1DEXT(target, internalformat, width, format, type, image) \
	void *tmp = convertPixels(width, 1, 1, format, type, image); \
	fn.glConvolutionFilter1DEXT(target, internalformat, width, format, type, tmp); \
	if (tmp != image) free(tmp)
#else
#define FN_GLCONVOLUTIONFILTER1DEXT(target, internalformat, width, format, type, image) fn.glConvolutionFilter1DEXT(target, internalformat, width, format, type, image)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONFILTER2DEXT(target, internalformat, width, height, format, type, image) \
	void *tmp = convertPixels(width, height, 1, format, type, image); \
	fn.glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, tmp); \
	if (tmp != image) free(tmp)
#else
#define FN_GLCONVOLUTIONFILTER2DEXT(target, internalformat, width, height, format, type, image) fn.glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, image)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONPARAMETERFV(target, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostFloatArray(size, params, tmp); \
		fn.glConvolutionParameterfv(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameterfv(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERFV(target, pname, params) fn.glConvolutionParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCONVOLUTIONPARAMETERFVEXT(target, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostFloatArray(size, params, tmp); \
		fn.glConvolutionParameterfvEXT(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameterfvEXT(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERFVEXT(target, pname, params) fn.glConvolutionParameterfvEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERIV(target, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glConvolutionParameteriv(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameteriv(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERIV(target, pname, params) fn.glConvolutionParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERIVEXT(target, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glConvolutionParameterivEXT(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameterivEXT(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERIVEXT(target, pname, params) fn.glConvolutionParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCONVOLUTIONPARAMETERXVOES(target, pname, params) \
	if (params) { \
		GLfixed tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glConvolutionParameterxvOES(target, pname, tmp); \
	} else { \
		fn.glConvolutionParameterxvOES(target, pname, params); \
	}
#else
#define FN_GLCONVOLUTIONPARAMETERXVOES(target, pname, params) fn.glConvolutionParameterxvOES(target, pname, params)
#endif

#define FN_GLCREATESHADERPROGRAMV(type, count, strings) \
	GLchar **pstrings; \
	if (strings && count) { \
		pstrings = (GLchar **)malloc(count * sizeof(*pstrings)); \
		if (!pstrings) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
		const Uint32 *p = (const Uint32 *)strings; \
		for (GLsizei i = 0; i < count; i++) \
			pstrings[i] = p[i] ? (GLchar *)Atari2HostAddr(SDL_SwapBE32(p[i])) : NULL; \
	} else { \
		pstrings = NULL; \
	} \
	GLuint ret = fn.glCreateShaderProgramv(type, count, pstrings); \
	free(pstrings); \
	return ret

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLCULLPARAMETERDVEXT(pname, params) \
	if (params) { \
		GLdouble tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostDoubleArray(size, params, tmp); \
		fn.glCullParameterdvEXT(pname, tmp); \
	} else { \
		fn.glCullParameterdvEXT(pname, params); \
	}
#else
#define FN_GLCULLPARAMETERDVEXT(pname, params) fn.glCullParameterdvEXT(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCULLPARAMETERFVEXT(pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		Atari2HostFloatArray(size, params, tmp); \
		fn.glCullParameterfvEXT(pname, tmp); \
	} else { \
		fn.glCullParameterfvEXT(pname, params); \
	}
#else
#define FN_GLCULLPARAMETERFVEXT(pname, params) fn.glCullParameterfvEXT(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDEBUGMESSAGECONTROLARB(source, type, severity, count, ids, enabled) \
	GLuint *tmp; \
	if (ids && count) { \
		tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(count, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDebugMessageControlARB(source, type, severity, count, tmp, enabled); \
	free(tmp)
#else
#define FN_GLDEBUGMESSAGECONTROLARB(source, type, severity, count, ids, enabled) \
	fn.glDebugMessageControlARB(source, type, severity, count, ids, enabled)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDEBUGMESSAGEENABLEAMD(category, severity, count, ids, enabled) \
	GLuint *tmp; \
	if (ids && count) { \
		tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(count, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDebugMessageEnableAMD(category, severity, count, tmp, enabled); \
	free(tmp)
#else
#define FN_GLDEBUGMESSAGEENABLEAMD(category, severity, count, ids, enabled) \
	fn.glDebugMessageEnableAMD(category, severity, count, ids, enabled)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEBUFFERS(n, buffers) \
	GLuint *tmp; \
	if (buffers && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, buffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteBuffers(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEBUFFERS(n, buffers) \
	fn.glDeleteBuffers(n, buffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEBUFFERSARB(n, buffers) \
	GLuint *tmp; \
	if (buffers && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, buffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteBuffersARB(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEBUFFERSARB(n, buffers) \
	fn.glDeleteBuffersARB(n, buffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFENCESAPPLE(n, fences) \
	GLuint *tmp; \
	if (fences && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, fences, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteFencesAPPLE(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEFENCESAPPLE(n, fences) \
	fn.glDeleteFencesAPPLE(n, fences)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFENCESNV(n, fences) \
	GLuint *tmp; \
	if (fences && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, fences, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteFencesNV(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEFENCESNV(n, fences) \
	fn.glDeleteFencesNV(n, fences)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFRAMEBUFFERS(n, framebuffers) \
	GLuint *tmp; \
	if (framebuffers && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, framebuffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteFramebuffers(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEFRAMEBUFFERS(n, framebuffers) \
	fn.glDeleteFramebuffers(n, framebuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEFRAMEBUFFERSEXT(n, framebuffers) \
	GLuint *tmp; \
	if (framebuffers && n) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, framebuffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteFramebuffersEXT(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEFRAMEBUFFERSEXT(n, framebuffers) \
	fn.glDeleteFramebuffersEXT(n, framebuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETENAMESAMD(identifier, num, names) \
	GLuint *tmp; \
	if (num && names) { \
		tmp = (GLuint *)malloc(num * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(num, names, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteNamesAMD(identifier, num, tmp); \
	free(tmp)
#else
#define FN_GLDELETENAMESAMD(identifier, num, names) \
	fn.glDeleteNamesAMD(identifier, num, names)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDEFORMATIONMAP3DSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	GLdouble tmp[3]; \
	/* count(3) guessed from function name; has to be verified */ \
	Atari2HostDoubleArray(3, points, tmp); \
	fn.glDeformationMap3dSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, tmp)
#else
#define FN_GLDEFORMATIONMAP3DSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	fn.glDeformationMap3dSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLDEFORMATIONMAP3FSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	GLfloat tmp[3]; \
	/* count(3) guessed from function name; has to be verified */ \
	Atari2HostFloatArray(3, points, tmp); \
	fn.glDeformationMap3fSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, tmp)
#else
#define FN_GLDEFORMATIONMAP3FSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
	fn.glDeformationMap3fSGIX(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMSNV(n, programs) \
	GLuint *tmp; \
	if (n && programs) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, programs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteProgramsNV(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEPROGRAMSNV(n, programs) \
	fn.glDeleteProgramsNV(n, programs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEQUERIES(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteQueries(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEQUERIES(n, ids) \
	fn.glDeleteQueries(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEQUERIESARB(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteQueriesARB(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEQUERIESARB(n, ids) \
	fn.glDeleteQueriesARB(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETERENDERBUFFERS(n, renderbuffers) \
	GLuint *tmp; \
	if (n && renderbuffers) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, renderbuffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteRenderbuffers(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETERENDERBUFFERS(n, renderbuffers) \
	fn.glDeleteRenderbuffers(n, renderbuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETERENDERBUFFERSEXT(n, renderbuffers) \
	GLuint *tmp; \
	if (n && renderbuffers) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, renderbuffers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteRenderbuffersEXT(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETERENDERBUFFERSEXT(n, renderbuffers) \
	fn.glDeleteRenderbuffersEXT(n, renderbuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETESAMPLERS(n, samplers) \
	GLuint *tmp; \
	if (n && samplers) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, samplers, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteSamplers(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETESAMPLERS(n, samplers) \
	fn.glDeleteSamplers(n, samplers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETRANSFORMFEEDBACKS(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteTransformFeedbacks(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETETRANSFORMFEEDBACKS(n, ids) \
	fn.glDeleteTransformFeedbacks(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETETRANSFORMFEEDBACKSNV(n, ids) \
	GLuint *tmp; \
	if (n && ids) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteTransformFeedbacksNV(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETETRANSFORMFEEDBACKSNV(n, ids) \
	fn.glDeleteTransformFeedbacksNV(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEVERTEXARRAYS(n, arrays) \
	GLuint *tmp; \
	if (n && arrays) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, arrays, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteVertexArrays(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEVERTEXARRAYS(n, arrays) \
	fn.glDeleteVertexArrays(n, arrays)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEVERTEXARRAYSAPPLE(n, arrays) \
	GLuint *tmp; \
	if (n && arrays) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, arrays, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteVertexArraysAPPLE(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEVERTEXARRAYSAPPLE(n, arrays) \
	fn.glDeleteVertexArraysAPPLE(n, arrays)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLDETAILTEXFUNCSGIS(target, n, points) \
	GLfloat *tmp; \
	if (n && points) { \
		tmp = (GLfloat *)malloc(n * 2 * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostFloatArray(n * 2, points, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDetailTexFuncSGIS(target, n, tmp); \
	free(tmp)
#else
#define FN_GLDETAILTEXFUNCSGIS(target, n, points) \
	fn.glDetailTexFuncSGIS(target, n, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETDETAILTEXFUNCSGIS(target, points) \
	GLfloat *tmp; \
	GLint n = 0; \
	fn.glGetTexParameteriv(target, GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS, &n); \
	if (n && points) { \
		tmp = (GLfloat *)malloc(n * 2 * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		fn.glGetDetailTexFuncSGIS(target, tmp); \
		Host2AtariFloatArray(n * 2, tmp, points); \
		free(tmp); \
	}
#else
#define FN_GLGETDETAILTEXFUNCSGIS(target, points) \
	fn.glGetDetailTexFuncSGIS(target, points)
#endif

/*
If a buffer is bound to the GL_DRAW_INDIRECT_BUFFER binding at the time
of a call to glDrawArraysIndirect, indirect is interpreted as an
offset, in basic machine units, into that buffer and the parameter data
is read from the buffer rather than from client memory. 
*/

#define FN_GLDRAWARRAYSINDIRECT(mode, indirect) \
	/* \
	 * The parameters addressed by indirect are packed into a structure that takes the form (in C): \
	 * \
	 *    typedef  struct { \
	 *        uint  count; \
	 *        uint  primCount; \
	 *        uint  first; \
	 *        uint  baseInstance; \
	 *    } DrawArraysIndirectCommand; \
	 */ \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		indirect = Host2AtariAddr(indirect); \
		fn.glDrawArraysIndirect(mode, indirect); \
	} else if (indirect) { \
		GLuint tmp[4]; \
		Atari2HostIntPtr(4, (const GLuint *)indirect, tmp); \
		GLuint count = tmp[0]; \
		convertClientArrays(count); \
		fn.glDrawArraysInstancedBaseInstance(mode, tmp[2], count, tmp[1], tmp[3]); \
	}

/*
 * The parameters addressed by indirect are packed into a structure that takes the form (in C):
 *
 *    typedef  struct {
 *        uint  count;
 *        uint  primCount;
 *        uint  firstIndex;
 *        uint  baseVertex;
 *        uint  baseInstance;
 *    } DrawElementsIndirectCommand;
 */
#define FN_GLDRAWELEMENTSINDIRECT(mode, type, indirect) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		indirect = Host2AtariAddr(indirect); \
		fn.glDrawElementsIndirect(mode, type, indirect); \
	} else if (indirect) { \
		GLuint tmp[5]; \
		Atari2HostIntPtr(5, (const GLuint *)indirect, tmp); \
		GLuint count = tmp[0]; \
		convertClientArrays(count); \
		fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, (const void *)(uintptr_t)tmp[2], tmp[1], tmp[3], tmp[4]); \
	}

#define FN_GLDRAWELEMENTSINSTANCED(mode, count, type, indices, instancecount) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstanced(mode, count, type, tmp, instancecount); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDARB(mode, count, type, indices, instancecount) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedARB(mode, count, type, tmp, instancecount); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDEXT(mode, count, type, indices, instancecount) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedEXT(mode, count, type, tmp, instancecount); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDBASEVERTEX(mode, count, type, indices, instancecount, basevertex) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseVertex(mode, count, type, tmp, instancecount, basevertex); \
	if (tmp != indices) free(tmp)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERSARB(n, bufs) \
	GLenum *tmp; \
	if (n && bufs) { \
		tmp = (GLenum *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, bufs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDrawBuffersARB(n, tmp); \
	free(tmp)
#else
#define FN_GLDRAWBUFFERSARB(n, bufs) \
	fn.glDrawBuffersARB(n, bufs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERSATI(n, bufs) \
	GLenum *tmp; \
	if (n && bufs) { \
		tmp = (GLenum *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, bufs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDrawBuffersATI(n, tmp); \
	free(tmp)
#else
#define FN_GLDRAWBUFFERSATI(n, bufs) \
	fn.glDrawBuffersATI(n, bufs)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLDRAWPIXELS(width, height, format, type, pixels) \
	void *tmp = convertPixels(width, height, 1, format, type, pixels); \
	fn.glDrawPixels(width, height, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLDRAWPIXELS(width, height, format, type, pixels) \
	fn.glDrawPixels(width, height, format, type, pixels)
#endif

/* nothing to do */
#define FN_GLEDGEFLAGV(flag) \
	fn.glEdgeFlagv(flag)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLEVALCOORD1XVOES(coords) \
	GLfixed tmp[1]; \
	Atari2HostIntPtr(1, coords, tmp); \
	fn.glEvalCoord1xvOES(tmp)
#else
#define FN_GLEVALCOORD1XVOES(coords) \
	fn.glEvalCoord1xvOES(coords)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLEVALCOORD2XVOES(coords) \
	GLfixed tmp[2]; \
	Atari2HostIntPtr(2, coords, tmp); \
	fn.glEvalCoord2xvOES(tmp)
#else
#define FN_GLEVALCOORD2XVOES(coords) \
	fn.glEvalCoord2xvOES(coords)
#endif

#define FN_GLRENDERMODE(mode) \
	GLenum render_mode = contexts[cur_context].render_mode; \
	GLint ret = fn.glRenderMode(mode); \
	switch (mode) { \
	case GL_RENDER: \
	case GL_SELECT: \
	case GL_FEEDBACK: \
		contexts[cur_context].render_mode = mode; \
		break; \
	} \
	switch (render_mode) { \
	case GL_FEEDBACK: \
		if (ret > 0 && contexts[cur_context].feedback_buffer_host) { \
			switch (contexts[cur_context].feedback_buffer_type) { \
			case GL_FLOAT: \
				Host2AtariFloatArray(ret, (const GLfloat *)contexts[cur_context].feedback_buffer_host, (GLfloat *)contexts[cur_context].feedback_buffer_atari); \
				break; \
			case GL_FIXED: \
				Host2AtariIntPtr(ret, (const GLfixed *)contexts[cur_context].feedback_buffer_host, (Uint32 *)contexts[cur_context].feedback_buffer_atari); \
				break; \
			} \
		} \
		break; \
	case GL_SELECT: \
		if (ret > 0 && contexts[cur_context].select_buffer_host) { \
			Host2AtariIntPtr(contexts[cur_context].select_buffer_size, contexts[cur_context].select_buffer_host, contexts[cur_context].select_buffer_atari); \
		} \
		break; \
	} \
	return ret

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFEEDBACKBUFFER(size, type, buffer) \
	contexts[cur_context].feedback_buffer_atari = buffer; \
	free(contexts[cur_context].feedback_buffer_host); \
	contexts[cur_context].feedback_buffer_host = malloc(size * sizeof(GLfloat)); \
	if (!contexts[cur_context].feedback_buffer_host) { glSetError(GL_OUT_OF_MEMORY); return; } \
	contexts[cur_context].feedback_buffer_type = GL_FLOAT; \
	fn.glFeedbackBuffer(size, type, (GLfloat *)contexts[cur_context].feedback_buffer_host)
#else
#define FN_GLFEEDBACKBUFFER(size, type, buffer) \
	fn.glFeedbackBuffer(size, type, buffer)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFEEDBACKBUFFERXOES(size, type, buffer) \
	contexts[cur_context].feedback_buffer_atari = (void *)buffer; \
	free(contexts[cur_context].feedback_buffer_host); \
	contexts[cur_context].feedback_buffer_host = malloc(size * sizeof(GLfixed)); \
	if (!contexts[cur_context].feedback_buffer_host) { glSetError(GL_OUT_OF_MEMORY); return; } \
	contexts[cur_context].feedback_buffer_type = GL_FIXED; \
	fn.glFeedbackBufferxOES(size, type, (GLfixed *)contexts[cur_context].feedback_buffer_host)
#else
#define FN_GLFEEDBACKBUFFERXOES(size, type, buffer) \
	fn.glFeedbackBufferxOES(size, type, buffer)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSELECTBUFFER(size, buffer) \
	contexts[cur_context].select_buffer_atari = buffer; \
	free(contexts[cur_context].select_buffer_host); \
	contexts[cur_context].select_buffer_host = (GLuint *)calloc(size, sizeof(GLuint)); \
	if (!contexts[cur_context].select_buffer_host) { glSetError(GL_OUT_OF_MEMORY); return; } \
	contexts[cur_context].select_buffer_size = size; \
	fn.glSelectBuffer(size, contexts[cur_context].select_buffer_host)
#else
#define FN_GLSELECTBUFFER(size, buffer) \
	fn.glSelectBuffer(size, buffer)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLEXECUTEPROGRAMNV(target, id, params) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, params, tmp); \
	fn.glExecuteProgramNV(target, id, tmp)
#else
#define FN_GLEXECUTEPROGRAMNV(size, type, buffer) \
	fn.glExecuteProgramNV(target, id, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFINISHASYNCSGIX(markerp) \
	GLuint tmp[1]; \
	GLint ret = fn.glFinishAsyncSGIX(tmp); \
	if (ret) \
		Host2AtariIntPtr(1, tmp, markerp); \
	return ret
#else
#define FN_GLFINISHASYNCSGIX(markerp) \
	return fn.glFinishAsyncSGIX(markerp)
#endif

#define FN_GLFLUSHVERTEXARRAYRANGEAPPLE(length, pointer) \
	if (pointer == contexts[cur_context].vertex.atari_pointer) \
		pointer = contexts[cur_context].vertex.host_pointer; \
	fn.glFlushVertexArrayRangeAPPLE(length,	pointer)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFOGCOORDDV(coord) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, coord, tmp); \
	fn.glFogCoorddv(tmp)
#else
#define FN_GLFOGCOORDDV(coord) \
	fn.glFogCoorddv(coord)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLFOGCOORDDVEXT(coord) \
	GLdouble tmp[1]; \
	Atari2HostDoubleArray(1, coord, tmp); \
	fn.glFogCoorddvEXT(tmp)
#else
#define FN_GLFOGCOORDDVEXT(coord) \
	fn.glFogCoorddvEXT(coord)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGCOORDFV(coord) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, coord, tmp); \
	fn.glFogCoordfv(tmp)
#else
#define FN_GLFOGCOORDFV(coord) \
	fn.glFogCoordfv(coord)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGCOORDFVEXT(coord) \
	GLfloat tmp[1]; \
	Atari2HostFloatArray(1, coord, tmp); \
	fn.glFogCoordfvEXT(tmp)
#else
#define FN_GLFOGCOORDFVEXT(coord) \
	fn.glFogCoordfvEXT(coord)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFOGFUNCSGIS(n, points) \
	GLfloat *tmp; \
	if (n && points) { \
		tmp = (GLfloat *)malloc(n * 2 * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostFloatArray(n * 2, points, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glFogFuncSGIS(n, tmp); \
	free(tmp)
#else
#define FN_GLFOGFUNCSGIS(n, points) \
	fn.glFogFuncSGIS(n, points)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFOGXVOES(pname, param) \
	GLfixed tmp[1]; \
	Atari2HostIntPtr(1, param, tmp); \
	fn.glFogxvOES(pname, tmp)
#else
#define FN_GLFOGXVOES(pname, param) \
	fn.glFogxvOES(pname, param)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAGMENTLIGHTMODELFVSGIX(pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glFragmentLightModelfvSGIX(pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTMODELFVSGIX(pname, params) \
	fn.glFragmentLightModelfvSGIX(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAGMENTLIGHTFVSGIX(light, pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glFragmentLightfvSGIX(light, pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTFVSGIX(light, pname, params) \
	fn.glFragmentLightfvSGIX(light, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFRAGMENTLIGHTMODELIVSGIX(pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glFragmentLightModelivSGIX(pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTMODELIVSGIX(pname, params) \
	fn.glFragmentLightModelivSGIX(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFRAGMENTLIGHTIVSGIX(light, pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glFragmentLightivSGIX(light, pname, tmp)
#else
#define FN_GLFRAGMENTLIGHTIVSGIX(light, pname, params) \
	fn.glFragmentLightivSGIX(light, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLFRAGMENTMATERIALFVSGIX(face, pname, params) \
	GLfloat tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glFragmentMaterialfvSGIX(face, pname, tmp)
#else
#define FN_GLFRAGMENTMATERIALFVSGIX(face, pname, params) \
	fn.glFragmentMaterialfvSGIX(face, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFRAGMENTMATERIALIVSGIX(face, pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glFragmentMaterialivSGIX(face, pname, tmp)
#else
#define FN_GLFRAGMENTMATERIALIVSGIX(face, pname, params) \
	fn.glFragmentMaterialivSGIX(face, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENBUFFERS(n, buffers) \
	fn.glGenBuffers(n, buffers); \
	if (n && buffers) { \
		Atari2HostIntPtr(n, buffers, buffers); \
	}
#else
#define FN_GLGENBUFFERS(n, buffers) \
	fn.glGenBuffers(n, buffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENBUFFERSARB(n, buffers) \
	fn.glGenBuffersARB(n, buffers); \
	if (n && buffers) { \
		Atari2HostIntPtr(n, buffers, buffers); \
	}
#else
#define FN_GLGENBUFFERSARB(n, buffers) \
	fn.glGenBuffersARB(n, buffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENFENCESAPPLE(n, fences) \
	fn.glGenFencesAPPLE(n, fences); \
	if (n && fences) { \
		Atari2HostIntPtr(n, fences, fences); \
	}
#else
#define FN_GLGENFENCESAPPLE(n, fences) \
	fn.glGenFencesAPPLE(n, fences)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENFENCESNV(n, fences) \
	fn.glGenFencesNV(n, fences); \
	if (n && fences) { \
		Atari2HostIntPtr(n, fences, fences); \
	}
#else
#define FN_GLGENFENCESNV(n, fences) \
	fn.glGenFencesNV(n, fences)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENFRAMEBUFFERS(n, framebuffers) \
	fn.glGenFramebuffers(n, framebuffers); \
	if (n && framebuffers) { \
		Atari2HostIntPtr(n, framebuffers, framebuffers); \
	}
#else
#define FN_GLGENFRAMEBUFFERS(n, framebuffers) \
	fn.glGenFramebuffers(n, framebuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENFRAMEBUFFERSEXT(n, framebuffers) \
	fn.glGenFramebuffersEXT(n, framebuffers); \
	if (n && framebuffers) { \
		Atari2HostIntPtr(n, framebuffers, framebuffers); \
	}
#else
#define FN_GLGENFRAMEBUFFERSEXT(n, framebuffers) \
	fn.glGenFramebuffersEXT(n, framebuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENNAMESAMD(identifer, num, names) \
	fn.glGenNamesAMD(identifier, num, names); \
	if (num && names) { \
		Atari2HostIntPtr(num, names, names); \
	}
#else
#define FN_GLGENNAMESAMD(identifer, num, names) \
	fn.glGenNamesAMD(identifer, num, names)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPROGRAMSNV(n, programs) \
	fn.glGenProgramsNV(n, programs); \
	if (n && programs) { \
		Atari2HostIntPtr(n, programs, programs); \
	}
#else
#define FN_GLGENPROGRAMSNV(n, programs) \
	fn.glGenProgramsNV(n, programs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENQUERIES(n, ids) \
	fn.glGenQueries(n, ids); \
	if (n && ids) { \
		Atari2HostIntPtr(n, ids, ids); \
	}
#else
#define FN_GLGENQUERIES(n, ids) \
	fn.glGenQueries(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENQUERIESARB(n, ids) \
	fn.glGenQueriesARB(n, ids); \
	if (n && ids) { \
		Atari2HostIntPtr(n, ids, ids); \
	}
#else
#define FN_GLGENQUERIESARB(n, ids) \
	fn.glGenQueriesARB(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENRENDERBUFFERS(n, renderbuffers) \
	fn.glGenRenderbuffers(n, renderbuffers); \
	if (n && renderbuffers) { \
		Atari2HostIntPtr(n, renderbuffers, renderbuffers); \
	}
#else
#define FN_GLGENRENDERBUFFERS(n, renderbuffers) \
	fn.glGenRenderbuffers(n, renderbuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENRENDERBUFFERSEXT(n, renderbuffers) \
	fn.glGenRenderbuffersEXT(n, renderbuffers); \
	if (n && renderbuffers) { \
		Atari2HostIntPtr(n, renderbuffers, renderbuffers); \
	}
#else
#define FN_GLGENRENDERBUFFERSEXT(n, renderbuffers) \
	fn.glGenRenderbuffersEXT(n, renderbuffers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENSAMPLERS(count, samplers) \
	fn.glGenSamplers(count, samplers); \
	if (count && samplers) { \
		Atari2HostIntPtr(count, samplers, samplers); \
	}
#else
#define FN_GLGENSAMPLERS(count, samplers) \
	fn.glGenSamplers(count, samplers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTEXTURES(n, textures) \
	fn.glGenTextures(n, textures); \
	if (n && textures) { \
		Host2AtariIntPtr(n, textures, textures); \
	}
#else
#define FN_GLGENTEXTURES(n, textures)	fn.glGenTextures(n, textures)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTEXTURESEXT(n, textures) \
	fn.glGenTexturesEXT(n, textures); \
	if (n && textures) { \
		Host2AtariIntPtr(n, textures, textures); \
	}
#else
#define FN_GLGENTEXTURESEXT(n, textures) \
	fn.glGenTexturesEXT(n, textures)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTRANSFORMFEEDBACKS(n, ids) \
	fn.glGenTransformFeedbacks(n, ids); \
	if (n && ids) { \
		Host2AtariIntPtr(n, ids, ids); \
	}
#else
#define FN_GLGENTRANSFORMFEEDBACKS(n, ids) \
	fn.glGenTransformFeedbacks(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENTRANSFORMFEEDBACKSNV(n, ids) \
	fn.glGenTransformFeedbacksNV(n, ids); \
	if (n && ids) { \
		Host2AtariIntPtr(n, ids, ids); \
	}
#else
#define FN_GLGENTRANSFORMFEEDBACKSNV(n, ids) \
	fn.glGenTransformFeedbacksNV(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENVERTEXARRAYS(n, arrays) \
	fn.glGenVertexArrays(n, arrays); \
	if (n && arrays) { \
		Host2AtariIntPtr(n, arrays, arrays); \
	}
#else
#define FN_GLGENVERTEXARRAYS(n, arrays) \
	fn.glGenVertexArrays(n, arrays)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENVERTEXARRAYSAPPLE(n, arrays) \
	fn.glGenVertexArraysAPPLE(n, arrays); \
	if (n && arrays) { \
		Host2AtariIntPtr(n, arrays, arrays); \
	}
#else
#define FN_GLGENVERTEXARRAYSAPPLE(n, arrays) \
	fn.glGenVertexArraysAPPLE(n, arrays)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEATTRIBARB(program, index, bufSize, length, size, type, name) \
	GLsizei l; \
	GLint s; \
	GLenum t; \
	fn.glGetActiveAttribARB(program, index, bufSize, &l, &s, &t, name); \
	if (length) *length = SDL_SwapBE32(l); \
	if (size) *size = SDL_SwapBE32(s); \
	if (type) *type = SDL_SwapBE32(t)
#else
#define FN_GLGETACTIVEATTRIBARB(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveAttribARB(program, index, bufSize, length, size, type, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVESUBROUTINENAME(program, shadertype, index, bufSize, length, name) \
	GLsizei l; \
	fn.glGetActiveSubroutineName(program, shadertype, index, bufSize, &l, name); \
	if (length) *length = SDL_SwapBE32(l)
#else
#define FN_GLGETACTIVESUBROUTINENAME(program, shadertype, index, bufSize, length, name) \
	fn.glGetActiveSubroutineName(program, shadertype, index, bufSize, length, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVESUBROUTINEUNIFORMNAME(program, shadertype, index, bufSize, length, name) \
	GLsizei l; \
	fn.glGetActiveSubroutineUniformName(program, shadertype, index, bufSize, &l, name); \
	if (length) *length = SDL_SwapBE32(l)
#else
#define FN_GLGETACTIVESUBROUTINEUNIFORMNAME(program, shadertype, index, bufSize, length, name) \
	fn.glGetActiveSubroutineUniformName(program, shadertype, index, bufSize, length, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVESUBROUTINEUNIFORMIV(program, shadertype, index, pname, values) \
	GLint size = 1; \
	fn.glGetActiveSubroutineUniformiv(program, shadertype, index, pname, values); \
	switch (pname) { \
		case GL_COMPATIBLE_SUBROUTINES: fn.glGetActiveSubroutineUniformiv(program, shadertype, index, GL_NUM_COMPATIBLE_SUBROUTINES, &size); break; \
	} \
	if (values) \
		Host2AtariIntPtr(size, values, values)
#else
#define FN_GLGETACTIVESUBROUTINEUNIFORMIV(program, shadertype, index, pname, values) \
	fn.glGetActiveSubroutineUniformiv(program, shadertype, index, pname, values)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMARB(program, index, bufSize, length, size, type, name) \
	GLsizei l; \
	GLint s; \
	GLenum t; \
	fn.glGetActiveUniformARB(program, index, bufSize, &l, &s, &t, name); \
	if (length) *length = SDL_SwapBE32(l); \
	if (size) *size = SDL_SwapBE32(s); \
	if (type) *type = SDL_SwapBE32(t)
#else
#define FN_GLGETACTIVEUNIFORMARB(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveUniformARB(program, index, bufSize, length, size, type, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMBLOCKNAME(program, index, bufSize, length, name) \
	GLsizei l; \
	fn.glGetActiveUniformBlockName(program, index, bufSize, &l, name); \
	if (length) *length = SDL_SwapBE32(l)
#else
#define FN_GLGETACTIVEUNIFORMBLOCKNAME(program, index, bufSize, length, name) \
	fn.glGetActiveUniformBlockName(program, index, bufSize, length, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMBLOCKIV(program, uniformBlockIndex, pname, params) \
	GLint size = 1; \
	fn.glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params); \
	switch (pname) { \
		case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES: fn.glGetActiveUniformBlockiv(program, uniformBlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &size); break; \
	} \
	if (params) \
		Host2AtariIntPtr(size, params, params)
#else
#define FN_GLGETACTIVEUNIFORMBLOCKIV(program, uniformBlockIndex, pname, params) \
	fn.glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMNAME(program, index, bufSize, length, name) \
	GLsizei l; \
	fn.glGetActiveUniformName(program, index, bufSize, &l, name); \
	if (length) *length = SDL_SwapBE32(l)
#else
#define FN_GLGETACTIVEUNIFORMNAME(program, index, bufSize, length, name) \
	fn.glGetActiveUniformName(program, index, bufSize, length, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORMSIV(program, uniformCount, uniformIndices, pname, params) \
	GLuint *tmp; \
	if (uniformCount && uniformIndices) { \
		tmp = (GLuint *)malloc(uniformCount * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(uniformCount, uniformIndices, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glGetActiveUniformsiv(program, uniformCount, tmp, pname, params); \
	free(tmp); \
	if (params) \
		Host2AtariIntPtr(uniformCount, params, params)
#else
#define FN_GLGETACTIVEUNIFORMSIV(program, uniformCount, uniformIndices, pname, params) \
	fn.glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEVARYINGNV(program, index, bufSize, length, size, type, name) \
	GLsizei l; \
	GLsizei s; \
	GLenum t; \
	fn.glGetActiveVaryingNV(program, index, bufSize, &l, &s, &t, name); \
	if (length) *length = SDL_SwapBE32(l); \
	if (size) *size = SDL_SwapBE32(s); \
	if (type) *type = SDL_SwapBE32(t)
#else
#define FN_GLGETACTIVEVARYINGNV(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveVaryingNV(program, index, bufSize, length, size, type, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETATTACHEDOBJECTSARB(containerObj, maxCount, count, obj) \
	GLsizei size = 0; \
	GLhandleARB *tmp = (GLhandleARB *)malloc(maxCount * sizeof(*tmp)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	fn.glGetAttachedObjectsARB(containerObj, maxCount, &size, tmp); \
	if (count) Host2AtariIntPtr(1, &size, count); \
	Host2AtariIntPtr(size, tmp, (Uint32 *)obj); \
	free(tmp)
#else
#define FN_GLGETATTACHEDOBJECTSARB(containerObj, maxCount, count, obj) \
	fn.glGetAttachedObjectsARB(containerObj, maxCount, count, obj)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETBUFFERPARAMETERI64V(target, pname, params) \
	GLint64 tmp[4]; \
	int size = nfglGetNumParams(pname); \
	fn.glGetBufferParameteri64v(target, pname, tmp); \
	Host2AtariInt64Ptr(size, tmp, params)
#else
#define FN_GLGETBUFFERPARAMETERI64V(target, pname, params) \
	fn.glGetBufferParameteri64v(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETBUFFERPARAMETERIV(target, pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	fn.glGetBufferParameteriv(target, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETBUFFERPARAMETERIV(target, pname, params) \
	fn.glGetBufferParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETBUFFERPARAMETERIVARB(target, pname, params) \
	GLint tmp[4]; \
	int size = nfglGetNumParams(pname); \
	fn.glGetBufferParameterivARB(target, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETBUFFERPARAMETERIVARB(target, pname, params) \
	fn.glGetBufferParameterivARB(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETBUFFERPARAMETERUI64VNV(target, pname, params) \
	GLuint64 tmp[4]; \
	int size = nfglGetNumParams(pname); \
	fn.glGetBufferParameterui64vNV(target, pname, tmp); \
	Host2AtariInt64Ptr(size, tmp, params)
#else
#define FN_GLGETBUFFERPARAMETERUI64VNV(target, pname, params) \
	fn.glGetBufferParameterui64vNV(target, pname, params)
#endif

#define FN_GLGETBUFFERPOINTERV(target, pname, params) \
	void *tmp = NULL; \
	fn.glGetBufferPointerv(target, pname, &tmp); \
	/* TODO */ \
	*params = NULL

#define FN_GLGETBUFFERPOINTERVARB(target, pname, params) \
	void *tmp = NULL; \
	fn.glGetBufferPointervARB(target, pname, &tmp); \
	/* TODO */ \
	*params = NULL

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETCLIPPLANE(plane, equation) \
	GLdouble tmp[4]; \
	fn.glGetClipPlane(plane, tmp); \
	Host2AtariDoubleArray(4, tmp, equation)
#else
#define FN_GLGETCLIPPLANE(plane, equation) \
	fn.glGetClipPlane(plane, equation)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCLIPPLANEFOES(plane, equation) \
	GLfloat tmp[4]; \
	fn.glGetClipPlanefOES(plane, tmp); \
	Host2AtariFloatArray(4, tmp, equation)
#else
#define FN_GLGETCLIPPLANEFOES(plane, equation) \
	fn.glGetClipPlanefOES(plane, equation)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCLIPPLANEXOES(plane, equation) \
	GLfixed tmp[4]; \
	fn.glGetClipPlanexOES(plane, tmp); \
	Host2AtariIntPtr(4, tmp, equation)
#else
#define FN_GLGETCLIPPLANEXOES(plane, equation) \
	fn.glGetClipPlanexOES(plane, equation)
#endif

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while a histogram table is requested, table
 * is treated as a byte offset into the buffer object's data store.
 */
#define FN_GLGETCOLORTABLE(target, format, type, table) \
	GLsizei size, count; \
	GLint width = 0; \
	void *result = NULL; \
	const void *src; \
	void *dst; \
	 \
	switch (type) \
	{ \
	case GL_UNSIGNED_BYTE: \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE_3_3_2: \
	case GL_UNSIGNED_BYTE_2_3_3_REV: \
    case GL_2_BYTES: \
    case GL_3_BYTES: \
    case GL_4_BYTES: \
	case GL_BITMAP: \
	case 1: \
		fn.glGetColorTable(target, format, type, table); \
		return; \
	} \
	fn.glGetColorTableParameteriv(target, GL_COLOR_TABLE_WIDTH, &width); \
	if (width == 0) return; \
	/* FIXME: glPixelStore parameters are not taken into account */ \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetColorTable(target, format, type, table); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)table; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)table; \
	} else { \
		result = pixelBuffer(width, 1, 1, format, type, size, count); \
		if (result == NULL) return; \
		fn.glGetColorTable(target, format, type, result); \
		src = result; \
		dst = table; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst); \
	free(result)

#define FN_GLGETCOLORTABLEEXT(target, format, type, table) \
	GLsizei size, count; \
	GLint width = 0; \
	void *result = NULL; \
	const void *src; \
	void *dst; \
	 \
	switch (type) \
	{ \
	case GL_UNSIGNED_BYTE: \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE_3_3_2: \
	case GL_UNSIGNED_BYTE_2_3_3_REV: \
    case GL_2_BYTES: \
    case GL_3_BYTES: \
    case GL_4_BYTES: \
	case GL_BITMAP: \
	case 1: \
		fn.glGetColorTable(target, format, type, table); \
		return; \
	} \
	fn.glGetColorTableParameteriv(target, GL_COLOR_TABLE_WIDTH, &width); \
	if (width == 0) return; \
	/* FIXME: glPixelStore parameters are not taken into account */ \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetColorTableEXT(target, format, type, table); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)table; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)table; \
	} else { \
		result = pixelBuffer(width, 1, 1, format, type, size, count); \
		if (result == NULL) return; \
		fn.glGetColorTableEXT(target, format, type, result); \
		src = result; \
		dst = table; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst); \
	free(result)

#define FN_GLGETCOLORTABLESGI(target, format, type, table) \
	GLsizei size, count; \
	GLint width = 0; \
	void *result = NULL; \
	const void *src; \
	void *dst; \
	 \
	switch (type) \
	{ \
	case GL_UNSIGNED_BYTE: \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE_3_3_2: \
	case GL_UNSIGNED_BYTE_2_3_3_REV: \
    case GL_2_BYTES: \
    case GL_3_BYTES: \
    case GL_4_BYTES: \
	case GL_BITMAP: \
	case 1: \
		fn.glGetColorTable(target, format, type, table); \
		return; \
	} \
	fn.glGetColorTableParameteriv(target, GL_COLOR_TABLE_WIDTH, &width); \
	if (width == 0) return; \
	/* FIXME: glPixelStore parameters are not taken into account */ \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetColorTableSGI(target, format, type, table); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)table; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)table; \
	} else { \
		result = pixelBuffer(width, 1, 1, format, type, size, count); \
		if (result == NULL) return; \
		fn.glGetColorTableSGI(target, format, type, result); \
		src = result; \
		dst = table; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst); \
	free(result)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOLORTABLEPARAMETERFV(target, pname, params) \
	GLfloat tmp[4]; \
	fn.glGetColorTableParameterfv(target, pname, tmp); \
	Host2AtariFloatArray(1, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERFV(target, pname, params) \
	fn.glGetColorTableParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOLORTABLEPARAMETERFVEXT(target, pname, params) \
	GLfloat tmp[4]; \
	fn.glGetColorTableParameterfvEXT(target, pname, tmp); \
	Host2AtariFloatArray(1, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERFVEXT(target, pname, params) \
	fn.glGetColorTableParameterfvEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOLORTABLEPARAMETERFVSGI(target, pname, params) \
	GLfloat tmp[4]; \
	fn.glGetColorTableParameterfvSGI(target, pname, tmp); \
	Host2AtariFloatArray(1, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERFVSGI(target, pname, params) \
	fn.glGetColorTableParameterfvSGI(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCOLORTABLEPARAMETERIV(target, pname, params) \
	GLint tmp[4]; \
	fn.glGetColorTableParameteriv(target, pname, tmp); \
	Host2AtariIntPtr(1, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERIV(target, pname, params) \
	fn.glGetColorTableParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCOLORTABLEPARAMETERIVEXT(target, pname, params) \
	GLint tmp[4]; \
	fn.glGetColorTableParameterivEXT(target, pname, tmp); \
	Host2AtariIntPtr(1, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERIVEXT(target, pname, params) \
	fn.glGetColorTableParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCOLORTABLEPARAMETERIVSGI(target, pname, params) \
	GLint tmp[4]; \
	fn.glGetColorTableParameterivSGI(target, pname, tmp); \
	Host2AtariIntPtr(1, tmp, params)
#else
#define FN_GLGETCOLORTABLEPARAMETERIVSGI(target, pname, params) \
	fn.glGetColorTableParameterivSGI(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOMBINERINPUTPARAMETERFVNV(stage, portion, variable, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		fn.glGetCombinerInputParameterfvNV(stage, portion, variable, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	} else { \
		fn.glGetCombinerInputParameterfvNV(stage, portion, variable, pname, params); \
	}
#else
#define FN_GLGETCOMBINERINPUTPARAMETERFVNV(stage, portion, variable, pname, params) \
	fn.glGetCombinerInputParameterfvNV(stage, portion, variable, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCOMBINERINPUTPARAMETERIVNV(stage, portion, variable, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		int size = nfglGetNumParams(pname); \
		fn.glGetCombinerInputParameterivNV(stage, portion, variable, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	} else { \
		fn.glGetCombinerInputParameterivNV(stage, portion, variable, pname, params); \
	}
#else
#define FN_GLGETCOMBINERINPUTPARAMETERIVNV(stage, portion, variable, pname, params) \
	fn.glGetCombinerInputParameterivNV(stage, portion, variable, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOMBINEROUTPUTPARAMETERFVNV(stage, portion, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		fn.glGetCombinerOutputParameterfvNV(stage, portion, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	} else { \
		fn.glGetCombinerOutputParameterfvNV(stage, portion, pname, params); \
	}
#else
#define FN_GLGETCOMBINEROUTPUTPARAMETERFVNV(stage, portion, pname, params) \
	fn.glGetCombinerOutputParameterfvNV(stage, portion, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCOMBINEROUTPUTPARAMETERIVNV(stage, portion, pname, params) \
	if (params) { \
		GLint tmp[4]; \
		int size = nfglGetNumParams(pname); \
		fn.glGetCombinerOutputParameterivNV(stage, portion, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	} else { \
		fn.glGetCombinerOutputParameterivNV(stage, portion, pname, params); \
	}
#else
#define FN_GLGETCOMBINEROUTPUTPARAMETERIVNV(stage, portion, pname, params) \
	fn.glGetCombinerOutputParameterivNV(stage, portion, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCOMBINERSTAGEPARAMETERFVNV(stage, pname, params) \
	if (params) { \
		GLfloat tmp[4]; \
		int size = nfglGetNumParams(pname); \
		fn.glGetCombinerStageParameterfvNV(stage, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	} else { \
		fn.glGetCombinerStageParameterfvNV(stage, pname, params); \
	}
#else
#define FN_GLGETCOMBINERSTAGEPARAMETERFVNV(stage, pname, params) fn.glGetCombinerStageParameterfvNV(stage, pname, params)
#endif

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while a convolution filter is requested,
 * image is treated as a byte offset into the buffer object's data store.
 */
#define FN_GLGETCONVOLUTIONFILTER(target, format, type, image) \
	GLsizei size, count; \
	GLint width = 0; \
	GLint height = 0; \
	void *result = NULL; \
	const void *src; \
	void *dst; \
	 \
	switch (type) \
	{ \
	case GL_UNSIGNED_BYTE: \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE_3_3_2: \
	case GL_UNSIGNED_BYTE_2_3_3_REV: \
    case GL_2_BYTES: \
    case GL_3_BYTES: \
    case GL_4_BYTES: \
	case GL_BITMAP: \
	case 1: \
		fn.glGetConvolutionFilter(target, format, type, image); \
		return; \
	} \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_WIDTH, &width); \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_HEIGHT, &height); \
	if (width == 0 || height == 0) return; \
	/* FIXME: glPixelStore parameters are not taken into account */ \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetConvolutionFilter(target, format, type, image); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)image; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)image; \
	} else { \
		result = pixelBuffer(width, height, 1, format, type, size, count); \
		if (result == NULL) return; \
		fn.glGetConvolutionFilter(target, format, type, result); \
		src = result; \
		dst = image; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst); \
	free(result)

#define FN_GLGETCONVOLUTIONFILTEREXT(target, format, type, image) \
	GLsizei size, count; \
	GLint width = 0; \
	GLint height = 0; \
	void *result = NULL; \
	const void *src; \
	void *dst; \
	 \
	switch (type) \
	{ \
	case GL_UNSIGNED_BYTE: \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE_3_3_2: \
	case GL_UNSIGNED_BYTE_2_3_3_REV: \
    case GL_2_BYTES: \
    case GL_3_BYTES: \
    case GL_4_BYTES: \
	case GL_BITMAP: \
	case 1: \
		fn.glGetConvolutionFilterEXT(target, format, type, image); \
		return; \
	} \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_WIDTH, &width); \
	fn.glGetConvolutionParameteriv(target, GL_CONVOLUTION_HEIGHT, &height); \
	if (width == 0 || height == 0) return; \
	/* FIXME: glPixelStore parameters are not taken into account */ \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetConvolutionFilterEXT(target, format, type, image); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)image; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)image; \
	} else { \
		result = pixelBuffer(width, height, 1, format, type, size, count); \
		if (result == NULL) return; \
		fn.glGetConvolutionFilterEXT(target, format, type, result); \
		src = result; \
		dst = image; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst); \
	free(result)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERFV(target, pname, params) \
	int size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameterfv(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERFV(target, pname, params) \
	fn.glGetConvolutionParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERFVEXT(target, pname, params) \
	int size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameterfvEXT(target, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERFVEXT(target, pname, params) \
	fn.glGetConvolutionParameterfvEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERIV(target, pname, params) \
	int size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameteriv(target, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERIV(target, pname, params) \
	fn.glGetConvolutionParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERIVEXT(target, pname, params) \
	int size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameterivEXT(target, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERIVEXT(target, pname, params) \
	fn.glGetConvolutionParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETCONVOLUTIONPARAMETERXVOES(target, pname, params) \
	int size = nfglGetNumParams(pname); \
	GLfixed tmp[MAX(size, 16)]; \
	fn.glGetConvolutionParameterxvOES(target, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETCONVOLUTIONPARAMETERXVOES(target, pname, params) \
	fn.glGetConvolutionParameterxvOES(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETDEBUGLOGMESA(obj, logType, shaderType, maxLength, length, debugLog) \
	GLsizei l; \
	fn.glGetDebugLogMESA(obj, logType, shaderType, maxLength, &l, debugLog); \
	if (length) Host2AtariIntPtr(1, &l, length)
#else
#define FN_GLGETDEBUGLOGMESA(obj, logType, shaderType, maxLength, length, debugLog) \
	fn.glGetDebugLogMESA(obj, logType, shaderType, maxLength, length, debugLog)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETDEBUGMESSAGELOGAMD(count, bufsize, categories, severities, ids, lengths, message) \
	GLenum *pcategories; \
	GLuint *pids; \
	GLenum *pseverities; \
	GLsizei *plengths; \
	if (categories ) { \
		pcategories = (GLenum *)malloc(count * sizeof(*pcategories)); \
		if (pcategories == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pcategories = NULL; \
	} \
	if (ids) { \
		pids = (GLuint *)malloc(count * sizeof(*pids)); \
		if (pids == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pids = NULL; \
	} \
	if (severities) { \
		pseverities = (GLenum *)malloc(count * sizeof(*pseverities)); \
		if (pseverities == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pseverities = NULL; \
	} \
	if (lengths) { \
		plengths = (GLsizei *)malloc(count * sizeof(*plengths)); \
		if (plengths == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		plengths = NULL; \
	} \
	count = fn.glGetDebugMessageLogAMD(count, bufsize, pcategories, pseverities, pids, plengths, message); \
	if (pcategories) { Host2AtariIntPtr(count, pcategories, categories); free(pcategories); } \
	if (pids) { Host2AtariIntPtr(count, pids, ids); free(pids); } \
	if (pseverities) { Host2AtariIntPtr(count, pseverities, severities); free(pseverities); } \
	if (plengths) { Host2AtariIntPtr(count, plengths, lengths); free(plengths); } \
	return count
#else
#define FN_GLGETDEBUGMESSAGELOGAMD(count, bufSize, categories, severities, ids, lengths, message) \
	return fn.glGetDebugMessageLogAMD(count, bufSize, categories, severities, ids, lengths, message)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETDEBUGMESSAGELOGARB(count, bufSize, sources, types, ids, severities, lengths, messageLog) \
	GLenum *psources; \
	GLenum *ptypes; \
	GLuint *pids; \
	GLenum *pseverities; \
	GLsizei *plengths; \
	if (sources ) { \
		psources = (GLenum *)malloc(count * sizeof(*psources)); \
		if (psources == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		psources = NULL; \
	} \
	if (types) { \
		ptypes = (GLenum *)malloc(count * sizeof(*ptypes)); \
		if (ptypes == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		ptypes = NULL; \
	} \
	if (ids) { \
		pids = (GLuint *)malloc(count * sizeof(*pids)); \
		if (pids == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pids = NULL; \
	} \
	if (severities) { \
		pseverities = (GLenum *)malloc(count * sizeof(*pseverities)); \
		if (pseverities == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pseverities = NULL; \
	} \
	if (lengths) { \
		plengths = (GLsizei *)malloc(count * sizeof(*plengths)); \
		if (plengths == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		plengths = NULL; \
	} \
	count = fn.glGetDebugMessageLogARB(count, bufSize, psources, ptypes, pids, pseverities, plengths, messageLog); \
	if (psources) { Host2AtariIntPtr(count, psources, sources); free(psources); } \
	if (ptypes) { Host2AtariIntPtr(count, ptypes, types); free(ptypes); } \
	if (pids) { Host2AtariIntPtr(count, pids, ids); free(pids); } \
	if (pseverities) { Host2AtariIntPtr(count, pseverities, severities); free(pseverities); } \
	if (plengths) { Host2AtariIntPtr(count, plengths, lengths); free(plengths); } \
	return count
#else
#define FN_GLGETDEBUGMESSAGELOGARB(count, bufSize, sources, types, ids, severities, lengths, messageLog) \
	return fn.glGetDebugMessageLogARB(count, bufSize, sources, types, ids, severities, lengths, messageLog)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFENCEIVNV(fence, pname, params) \
	int size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetFenceivNV(fence, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETFENCEIVNV(fence, pname, params) \
	fn.glGetFenceivNV(fence, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFINALCOMBINERINPUTPARAMETERFVNV(variable, pname, params) \
	if (params) { \
		int size = nfglGetNumParams(pname); \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetFinalCombinerInputParameterfvNV(variable, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	} else { \
		fn.glGetFinalCombinerInputParameterfvNV(variable, pname, params); \
	}
#else
#define FN_GLGETFINALCOMBINERINPUTPARAMETERFVNV(variable, pname, params) \
	fn.glGetFinalCombinerInputParameterfvNV(variable, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFINALCOMBINERINPUTPARAMETERIVNV(variable, pname, params) \
	if (params) { \
		int size = nfglGetNumParams(pname); \
		GLint tmp[size]; \
		fn.glGetFinalCombinerInputParameterivNV(variable, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	} else { \
		fn.glGetFinalCombinerInputParameterivNV(variable, pname, params); \
	}
#else
#define FN_GLGETFINALCOMBINERINPUTPARAMETERIVNV(variable, pname, params) \
	fn.glGetFinalCombinerInputParameterivNV(variable, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFIXEDVOES(pname, params) \
	if (params) { \
		int n = nfglGetNumParams(pname); \
		GLfixed tmp[MAX(n, 16)]; \
		fn.glGetFixedvOES(pname, tmp); \
		Host2AtariIntPtr(n, tmp, params); \
	} else { \
		fn.glGetFixedvOES(pname, params); \
	}
#else
#define FN_GLGETFIXEDVOES(pname, params) \
	fn.glGetFixedvOES(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFOGFUNCSGIS(points) \
	GLint size = 0; \
	fn.glGetIntegerv(GL_FOG_FUNC_POINTS_SGIS, &size); \
	if (size && points) { \
		GLfloat tmp[size * 2]; \
		fn.glGetFogFuncSGIS(tmp); \
		Host2AtariFloatArray(size * 2, tmp, points); \
	}
#else
#define FN_GLGETFOGFUNCSGIS(points) \
	fn.glGetFogFuncSGIS(points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFRAGMENTLIGHTFVSGIX(light, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[size]; \
		fn.glGetFragmentLightfvSGIX(light, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETFRAGMENTLIGHTFVSGIX(size, tmp, params) \
	fn.glGetFragmentLightfvSGIX(size, tmp, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAGMENTLIGHTIVSGIX(light, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[size]; \
		fn.glGetFragmentLightivSGIX(light, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETFRAGMENTLIGHTIVSGIX(size, tmp, params) \
	fn.glGetFragmentLightivSGIX(size, tmp, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFRAGMENTMATERIALFVSGIX(face, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[size]; \
		fn.glGetFragmentMaterialfvSGIX(face, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETFRAGMENTMATERIALFVSGIX(face, tmp, params) \
	fn.glGetFragmentMaterialfvSGIX(face, tmp, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAGMENTMATERIALIVSGIX(face, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[size]; \
		fn.glGetFragmentMaterialivSGIX(face, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETFRAGMENTMATERIALIVSGIX(face, tmp, params) \
	fn.glGetFragmentMaterialivSGIX(face, tmp, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAMEBUFFERATTACHMENTPARAMETERIV(target, attachment, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetFramebufferAttachmentParameteriv(target, attachment, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETFRAMEBUFFERATTACHMENTPARAMETERIV(target, attachment, pname, params) \
	fn.glGetFramebufferAttachmentParameteriv(target, attachment, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXT(target, attachment, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXT(target, attachment, pname, params) \
	fn.glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, params)
#endif

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while a histogram table is requested, table
 * is treated as a byte offset into the buffer object's data store.
 */
#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETHISTOGRAM(target, reset, format, type, values) \
	GLsizei size, count; \
	GLint width = 0; \
	void *result = NULL; \
	const void *src; \
	void *dst; \
	 \
	switch (type) \
	{ \
	case GL_UNSIGNED_BYTE: \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE_3_3_2: \
	case GL_UNSIGNED_BYTE_2_3_3_REV: \
    case GL_2_BYTES: \
    case GL_3_BYTES: \
    case GL_4_BYTES: \
	case GL_BITMAP: \
	case 1: \
		fn.glGetHistogram(target, reset, format, type, values); \
		return; \
	} \
	fn.glGetHistogramParameteriv(target, GL_HISTOGRAM_WIDTH, &width); \
	if (width == 0) return; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetHistogram(target, reset, format, type, values); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)values; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)values; \
	} else { \
		result = pixelBuffer(width, 1, 1, format, type, size, count); \
		if (result == NULL) return; \
		fn.glGetHistogram(target, reset, format, type, result); \
		src = result; \
		dst = values; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst); \
	free(result)
#else
#define FN_GLGETHISTOGRAM(target, reset, format, type, values) \
	fn.glGetHistogram(target, reset, format, type, values)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETHISTOGRAMEXT(target, reset, format, type, values) \
	GLsizei size, count; \
	GLint width = 0; \
	void *result = NULL; \
	const void *src; \
	void *dst; \
	 \
	switch (type) \
	{ \
	case GL_UNSIGNED_BYTE: \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE_3_3_2: \
	case GL_UNSIGNED_BYTE_2_3_3_REV: \
    case GL_2_BYTES: \
    case GL_3_BYTES: \
    case GL_4_BYTES: \
	case GL_BITMAP: \
	case 1: \
		fn.glGetHistogramEXT(target, reset, format, type, values); \
		return; \
	} \
	fn.glGetHistogramParameteriv(target, GL_HISTOGRAM_WIDTH, &width); \
	if (width == 0) return; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetHistogramEXT(target, reset, format, type, values); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)values; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)values; \
	} else { \
		result = pixelBuffer(width, 1, 1, format, type, size, count); \
		if (result == NULL) return; \
		fn.glGetHistogramEXT(target, reset, format, type, result); \
		src = result; \
		dst = values; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst); \
	free(result)
#else
#define FN_GLGETHISTOGRAMEXT(target, reset, format, type, values) \
	fn.glGetHistogramEXT(target, reset, format, type, values)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETHISTOGRAMPARAMETERFV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetHistogramParameterfv(target, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETHISTOGRAMPARAMETERFV(target, pname, params) \
	fn.glGetHistogramParameterfv(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETHISTOGRAMPARAMETERFVEXT(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetHistogramParameterfvEXT(target, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETHISTOGRAMPARAMETERFVEXT(target, pname, params) \
	fn.glGetHistogramParameterfvEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETHISTOGRAMPARAMETERIV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetHistogramParameteriv(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETHISTOGRAMPARAMETERIV(target, pname, params) \
	fn.glGetHistogramParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETHISTOGRAMPARAMETERIVEXT(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetHistogramParameterivEXT(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETHISTOGRAMPARAMETERIVEXT(target, pname, params) \
	fn.glGetHistogramParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETHISTOGRAMPARAMETERXVOES(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfixed tmp[MAX(size, 16)]; \
		fn.glGetHistogramParameterxvOES(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETHISTOGRAMPARAMETERXVOES(target, pname, params) \
	fn.glGetHistogramParameterxvOES(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETIMAGETRANSFORMPARAMETERFVHP(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetImageTransformParameterfvHP(target, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETIMAGETRANSFORMPARAMETERFVHP(target, pname, params) \
	fn.glGetImageTransformParameterfvHP(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETIMAGETRANSFORMPARAMETERIVHP(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetImageTransformParameterivHP(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETIMAGETRANSFORMPARAMETERIVHP(target, pname, params) \
	fn.glGetImageTransformParameterivHP(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGER64I_V(target, index, data) \
	GLint size = nfglGetNumParams(target); \
	if (data) { \
		GLint64 tmp[MAX(size, 16)]; \
		fn.glGetInteger64i_v(target, index, tmp); \
		Host2AtariInt64Ptr(size, tmp, data); \
	}
#else
#define FN_GLGETINTEGER64I_V(target, index, data) \
	fn.glGetInteger64i_v(target, index, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGER64V(pname, data) \
	GLint size = nfglGetNumParams(pname); \
	if (data) { \
		GLint64 tmp[MAX(size, 16)]; \
		fn.glGetInteger64v(pname, tmp); \
		Host2AtariInt64Ptr(size, tmp, data); \
	}
#else
#define FN_GLGETINTEGER64V(pname, data) \
	fn.glGetInteger64v(pname, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERI_V(target, index, data) \
	GLint size = nfglGetNumParams(target); \
	if (data) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetIntegeri_v(target, index, tmp); \
		Host2AtariIntPtr(size, tmp, data); \
	}
#else
#define FN_GLGETINTEGERI_V(target, index, data) \
	fn.glGetIntegeri_v(target, index, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERI_V(target, index, data) \
	GLint size = nfglGetNumParams(target); \
	if (data) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetIntegeri_v(target, index, tmp); \
		Host2AtariIntPtr(size, tmp, data); \
	}
#else
#define FN_GLGETINTEGERI_V(target, index, data) \
	fn.glGetIntegeri_v(target, index, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERUI64I_VNV(value, index, data) \
	GLint size = nfglGetNumParams(value); \
	if (data) { \
		GLuint64EXT tmp[MAX(size, 16)]; \
		fn.glGetIntegerui64i_vNV(value, index, tmp); \
		Host2AtariInt64Ptr(size, tmp, data); \
	}
#else
#define FN_GLGETINTEGERUI64I_VNV(value, index, data) \
	fn.glGetIntegerui64i_vNV(value, index, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERUI64VNV(value, data) \
	GLint size = nfglGetNumParams(value); \
	if (data) { \
		GLuint64EXT tmp[MAX(size, 16)]; \
		fn.glGetIntegerui64vNV(value, tmp); \
		Host2AtariInt64Ptr(size, tmp, data); \
	}
#else
#define FN_GLGETINTEGERUI64VNV(value, data) \
	fn.glGetIntegerui64vNV(value, data)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETINVARIANTFLOATVEXT(id, value, data) \
	GLint size = 1; \
	if (data) { \
		GLfloat tmp[size]; \
		fn.glGetInvariantFloatvEXT(id, value, tmp); \
		Host2AtariFloatArray(size, tmp, data); \
	}
#else
#define FN_GLGETINVARIANTFLOATVEXT(id, value, data) \
	fn.glGetInvariantFloatvEXT(id, value, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINVARIANTINTEGERVEXT(id, value, data) \
	GLint size = 1; \
	if (data) { \
		GLint tmp[size]; \
		fn.glGetInvariantIntegervEXT(id, value, tmp); \
		Host2AtariIntPtr(size, tmp, data); \
	}
#else
#define FN_GLGETINVARIANTINTEGERVEXT(id, value, data) \
	fn.glGetInvariantIntegervEXT(id, value, data)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETLIGHTFV(light, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetLightfv(light, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETLIGHTFV(light, pname, params) \
	fn.glGetLightfv(light, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLIGHTIV(light, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetLightiv(light, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETLIGHTIV(light, pname, params) \
	fn.glGetLightiv(light, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLIGHTXOES(light, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfixed tmp[MAX(size, 16)]; \
		fn.glGetLightxOES(light, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETLIGHTXOES(light, pname, params) \
	fn.glGetLightxOES(light, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETLISTPARAMETERFVSGIX(list, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLfloat tmp[size]; \
		fn.glGetListParameterfvSGIX(list, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETLISTPARAMETERFVSGIX(list, pname, params) \
	fn.glGetListParameterfvSGIX(list, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLLISTPARAMETERFVSGIX(list, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLfloat tmp[size]; \
		Atari2HostFloatArray(size, params, tmp); \
		fn.glListParameterfvSGIX(list, pname, tmp); \
	}
#else
#define FN_GLLISTPARAMETERFVSGIX(list, pname, params) \
	fn.glListParameterfvSGIX(list, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLISTPARAMETERIVSGIX(list, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[size]; \
		fn.glGetListParameterivSGIX(list, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETLISTPARAMETERIVSGIX(list, pname, params) \
	fn.glGetListParameterivSGIX(list, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLLISTPARAMETERIVSGIX(list, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[size]; \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glListParameterivSGIX(list, pname, tmp); \
	}
#else
#define FN_GLLISTPARAMETERIVSGIX(list, pname, params) \
	fn.glListParameterivSGIX(list, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETLOCALCONSTANTFLOATVEXT(id, value, data) \
	GLint size = 1; \
	if (data) { \
		GLfloat tmp[size]; \
		fn.glGetLocalConstantFloatvEXT(id, value, tmp); \
		Host2AtariFloatArray(size, tmp, data); \
	}
#else
#define FN_GLGETLOCALCONSTANTFLOATVEXT(id, value, data) \
	fn.glGetLocalConstantFloatvEXT(id, value, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETLOCALCONSTANTINTEGERVEXT(id, value, data) \
	GLint size = 1; \
	if (data) { \
		GLint tmp[size]; \
		fn.glGetLocalConstantIntegervEXT(id, value, tmp); \
		Host2AtariIntPtr(size, tmp, data); \
	}
#else
#define FN_GLGETLOCALCONSTANTINTEGERVEXT(id, value, data) \
	fn.glGetLocalConstantIntegervEXT(id, value, data)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_evaluators
 *
 * Status: Discontinued.
 *
 * NVIDIA no longer supports this extension in driver updates after
 * November 2002.  Instead, use conventional OpenGL evaluators or
 * tessellate surfaces within your application.
 */
/*
 * note that, unlike glMap2(), ustride and vstride here are in terms
 * of bytes, not floats
 */
#if NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMAPCONTROLPOINTSNV(target, index, type, ustride, vstride, uorder, vorder, packed, points) \
	GLsizei size = 4; \
	GLsizei count; \
	GLsizei i, j; \
	const char *src; \
	void *tmp, *dst; \
	GLsizei ustride_host, vstride_host; \
	switch (target) { \
		case GL_EVAL_2D_NV: \
			count = uorder * vorder; \
			break; \
		case GL_EVAL_TRIANGULAR_2D_NV: \
			if (uorder != vorder) { glSetError(GL_INVALID_OPERATION); return; } \
			count = uorder * (uorder + 1) / 2; \
			break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	switch (type) { \
		case GL_FLOAT: \
			vstride_host = size * sizeof(GLfloat); \
			ustride_host = vorder * vstride_host; \
			break; \
		case GL_DOUBLE: \
			vstride_host = size * sizeof(GLdouble); \
			ustride_host = vorder * vstride_host; \
			break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	if (index > 15) { glSetError(GL_INVALID_VALUE); return; } \
	if (ustride < 0 || vstride < 0) { glSetError(GL_INVALID_VALUE); return; } \
	tmp = malloc(count * ustride_host); \
	if (tmp == NULL) glSetError(GL_OUT_OF_MEMORY); return; \
	for (i = 0; i < uorder; i++) { \
		for (j = 0; j < vorder; j++) { \
			if (!packed) { \
				src = (const char *)points + ustride * i + vstride * j; \
			} else if (target == GL_EVAL_2D_NV) { \
				src = (const char *)points + ustride * (j * uorder + i); \
			} else { \
				src = (const char *)points + ustride * (j * uorder + i - j * (j - 1) / 2); \
			} \
			dst = (char *)tmp + ustride_host * i + vstride_host * j; \
			if (type == GL_FLOAT) \
				Atari2HostFloatArray(size, (const GLfloat *)src, (GLfloat *)dst); \
			else \
				Atari2HostDoubleArray(size, (const GLdouble *)src, (GLdouble *)dst); \
		} \
	} \
	fn.glMapControlPointsNV(target, index, type, ustride_host, vstride_host, uorder, vorder, GL_TRUE, tmp); \
	free(tmp)
	
#define FN_GLGETMAPCONTROLPOINTSNV(target, index, type, ustride, vstride, packed, points) \
	GLsizei size = 4; \
	GLsizei count; \
	GLint uorder = 0, vorder = 0; \
	GLsizei i, j; \
	GLsizei ustride_host, vstride_host; \
	const char *src; \
	void *tmp, *dst; \
	fn.glGetMapAttribParameterivNV(target, index, GL_MAP_ATTRIB_U_ORDER_NV, &uorder); \
	fn.glGetMapAttribParameterivNV(target, index, GL_MAP_ATTRIB_V_ORDER_NV, &vorder); \
	if (uorder <= 0 || vorder <= 0) return; \
	switch (target) { \
		case GL_EVAL_2D_NV: \
			count = uorder * vorder; \
			break; \
		case GL_EVAL_TRIANGULAR_2D_NV: \
			if (uorder != vorder) { glSetError(GL_INVALID_OPERATION); return; } \
			count = uorder * (uorder + 1) / 2; \
			break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	switch (type) { \
		case GL_FLOAT: \
			vstride_host = size * sizeof(GLfloat); \
			ustride_host = vorder * vstride_host; \
			break; \
		case GL_DOUBLE: \
			vstride_host = size * sizeof(GLdouble); \
			ustride_host = vorder * vstride_host; \
			break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	if (index > 15) { glSetError(GL_INVALID_VALUE); return; } \
	if (ustride < 0 || vstride < 0) { glSetError(GL_INVALID_VALUE); return; } \
	tmp = malloc(count * ustride_host); \
	if (tmp == NULL) glSetError(GL_OUT_OF_MEMORY); return; \
	fn.glGetMapControlPointsNV(target, index, type, ustride_host, vstride_host, GL_TRUE, tmp); \
	for (i = 0; i < uorder; i++) { \
		for (j = 0; j < vorder; j++) { \
			if (!packed) { \
				dst = (char *)points + ustride * i + vstride * j; \
			} else if (target == GL_EVAL_2D_NV) { \
				dst = (char *)points + ustride * (j * uorder + i); \
			} else { \
				dst = (char *)points + ustride * (j * uorder + i - j * (j - 1) / 2); \
			} \
			src = (const char *)tmp + ustride_host * i + vstride_host * j; \
			if (type == GL_FLOAT) \
				Host2AtariFloatArray(size, (const GLfloat *)src, (GLfloat *)dst); \
			else \
				Host2AtariDoubleArray(size, (const GLdouble *)src, (GLdouble *)dst); \
		} \
	} \
	free(tmp)

#else
#define FN_GLMAPCONTROLPOINTSNV(target, index, type, ustride, vstride, uorder, vorder, packed, points) \
	fn.glMapControlPointsNV(target, index, type, ustride, vstride, uorder, vorder, packed, points)
#define FN_GLGETMAPCONTROLPOINTSNV(target, index, type, ustride, vstride, packed, points) \
	fn.glGetMapControlPointsNV(target, index, type, ustride, vstride, packed, points)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMAPPARAMETERFVNV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		Atari2HostFloatArray(size, params, tmp); \
		fn.glMapParameterfvNV(target, pname, tmp); \
	}
#define FN_GLGETMAPPARAMETERFVNV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMapParameterfvNV(target, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#define FN_GLMAPATTRIBPARAMETERFVNV(target, index, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		Atari2HostFloatArray(size, params, tmp); \
		fn.glMapAttribParameterfvNV(target, index, pname, tmp); \
	}
#define FN_GLGETMAPATTRIBPARAMETERFVNV(target, index, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMapAttribParameterfvNV(target, index, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLMAPPARAMETERFVNV(target, pname, params) \
	fn.glMapParameterfvNV(target, pname, params)
#define FN_GLGETMAPPARAMETERFVNV(target, pname, params) \
	fn.glGetMapParameterfvNV(target, pname, params)
#define FN_GLMAPATTRIBPARAMETERFVNV(target, index, pname, params) \
	fn.glMapAttribParameterfvNV(target, index, pname, params)
#define FN_GLGETMAPATTRIBPARAMETERFVNV(target, index, pname, params) \
	fn.glGetMapAttribParameterfvNV(target, index, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMAPPARAMETERIVNV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glMapParameterivNV(target, pname, tmp); \
	}
#define FN_GLGETMAPPARAMETERIVNV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetMapParameterivNV(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#define FN_GLMAPATTRIBPARAMETERIVNV(target, index, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		Atari2HostIntPtr(size, params, tmp); \
		fn.glMapAttribParameterivNV(target, index, pname, tmp); \
	}
#define FN_GLGETMAPATTRIBPARAMETERIVNV(target, index, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetMapAttribParameterivNV(target, index, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLMAPPARAMETERIVNV(target, pname, params) \
	fn.glMapParameterivNV(target, pname, params)
#define FN_GLGETMAPPARAMETERIVNV(target, pname, params) \
	fn.glGetMapParameterivNV(target, pname, params)
#define FN_GLMAPATTRIBPARAMETERIVNV(target, index, pname, params) \
	fn.glMapAttribParameterivNV(target, index, pname, params)
#define FN_GLGETMAPATTRIBPARAMETERIVNV(target, index, pname, params) \
	fn.glGetMapAttribParameterivNV(target, index, pname, params)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_INTEL_performance_query
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLCREATEPERFQUERYINTEL(queryId, queryHandle) \
	GLuint tmp[1]; \
	fn.glCreatePerfQueryINTEL(queryId, tmp); \
	Host2AtariIntPtr(1, tmp, queryHandle)
#else
#define FN_GLCREATEPERFQUERYINTEL(queryId, queryHandle) \
	fn.glCreatePerfQueryINTEL(queryId, queryHandle)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFIRSTPERFQUERYIDINTEL(queryId) \
	if (queryId) { \
		int size = 1; \
		GLuint tmp[size]; \
		fn.glGetFirstPerfQueryIdINTEL(tmp); \
		Host2AtariIntPtr(size, tmp, queryId); \
	} else { \
		fn.glGetFirstPerfQueryIdINTEL(queryId); \
	}
#else
#define FN_GLGETFIRSTPERFQUERYIDINTEL(queryId) \
	fn.glGetFirstPerfQueryIdINTEL(queryId)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNEXTPERFQUERYIDINTEL(queryId, nextQueryId) \
	GLuint tmp[1]; \
	fn.glGetNextPerfQueryIdINTEL(queryId, tmp); \
	Host2AtariIntPtr(1, tmp, nextQueryId)
#else
#define FN_GLGETNEXTPERFQUERYIDINTEL(queryId, nextQueryId) \
	fn.glGetNextPerfQueryIdINTEL(queryId, nextQueryId)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFCOUNTERINFOINTEL(queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue) \
	GLint const size = 1;\
	GLuint offset[size]; \
	GLuint datasize[size]; \
	GLuint countertype[size]; \
	GLuint datatype[size]; \
	GLuint64 countermax[size]; \
	fn.glGetPerfCounterInfoINTEL(queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, offset, datasize, countertype, datatype, countermax); \
	if (counterOffset) Host2AtariIntPtr(size, offset, counterOffset); \
	if (counterDataSize) Host2AtariIntPtr(size, datasize, counterDataSize); \
	if (counterTypeEnum) Host2AtariIntPtr(size, countertype, counterTypeEnum); \
	if (counterDataTypeEnum) Host2AtariIntPtr(size, datatype, counterDataTypeEnum); \
	if (rawCounterMaxValue) Host2AtariInt64Ptr(size, countermax, rawCounterMaxValue)
#else
#define FN_GLGETPERFCOUNTERINFOINTEL(queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue) \
	fn.glGetPerfCounterInfoINTEL(queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFQUERYDATAINTEL(queryHandle, flags, dataSize, data, bytesWritten) \
	GLuint bytes = 0;\
	fn.glGetPerfQueryDataINTEL(queryHandle, flags, dataSize, data, &bytes); \
	if (bytesWritten) Host2AtariIntPtr(1, &bytes, bytesWritten); \
	if (bytes && data) Host2AtariIntPtr(bytes / sizeof(GLuint), (const GLuint *)data, (Uint32 *) data)
#else
#define FN_GLGETPERFQUERYDATAINTEL(queryHandle, flags, dataSize, data, bytesWritten) \
	fn.glGetPerfQueryDataINTEL(queryHandle, flags, dataSize, data, bytesWritten)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFQUERYIDBYNAMEINTEL(queryName, queryId) \
	GLint const size = 1;\
	GLuint tmp[size]; \
	fn.glGetPerfQueryIdByNameINTEL(queryName, tmp); \
	if (queryId) Host2AtariIntPtr(size, tmp, queryId)
#else
#define FN_GLGETPERFQUERYIDBYNAMEINTEL(queryName, queryId) \
	fn.glGetPerfQueryIdByNameINTEL(queryName, queryId)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFQUERYINFOINTEL(queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask) \
	GLint const size = 1;\
	GLuint datasize[size]; \
	GLuint counters[size]; \
	GLuint instances[size]; \
	GLuint mask[size]; \
	fn.glGetPerfQueryInfoINTEL(queryId, queryNameLength, queryName, datasize, counters, instances, mask); \
	if (dataSize) Host2AtariIntPtr(size, datasize, dataSize); \
	if (noCounters) Host2AtariIntPtr(size, counters, noCounters); \
	if (noInstances) Host2AtariIntPtr(size, instances, noInstances); \
	if (capsMask) Host2AtariIntPtr(size, mask, capsMask)
#else
#define FN_GLGETPERFQUERYINFOINTEL(queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask) \
	fn.glGetPerfQueryInfoINTEL(queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_direct_state_access
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFLOATINDEXEDVEXT(pname, index, params) \
	if (params) { \
		int n = nfglGetNumParams(pname); \
		GLfloat tmp[MAX(n, 16)]; \
		fn.glGetFloatIndexedvEXT(pname, index, tmp); \
		Host2AtariFloatArray(n, tmp, params); \
	} else { \
		fn.glGetFloatIndexedvEXT(pname, index, params); \
	}
#else
#define FN_GLGETFLOATINDEXEDVEXT(pname, index, params) \
	fn.glGetFloatIndexedvEXT(pname, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETDOUBLEINDEXEDVEXT(pname, index, params) \
	int n; \
	n = nfglGetNumParams(pname); \
	if (n > 16) { \
		GLdouble *tmp; \
		tmp = (GLdouble *)malloc(n * sizeof(*tmp)); \
		fn.glGetDoubleIndexedvEXT(pname, index, tmp); \
		Host2AtariDoubleArray(n, tmp, params); \
		free(tmp); \
	} else { \
		GLdouble tmp[16]; \
		fn.glGetDoubleIndexedvEXT(pname, index, tmp); \
		Host2AtariDoubleArray(n, tmp, params); \
	}
#else
#define FN_GLGETDOUBLEINDEXEDVEXT(pname, index, params) \
	fn.glGetDoubleIndexedvEXT(pname, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTEGERINDEXEDVEXT(target, index, data) \
	GLint size = nfglGetNumParams(target); \
	if (data) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetIntegerIndexedvEXT(target, index, tmp); \
		Host2AtariIntPtr(size, tmp, data); \
	}
#else
#define FN_GLGETINTEGERINDEXEDVEXT(target, index, data) \
	fn.glGetIntegerIndexedvEXT(target, index, data)
#endif

/* nothing to do */
#define FN_GLGETCOMPRESSEDMULTITEXIMAGEEXT(texunit, target, lod, img) \
	fn.glGetCompressedMultiTexImageEXT(texunit, target, lod, img)

/* nothing to do */
#define FN_GLGETCOMPRESSEDTEXIMAGE(target, level, img) \
	fn.glGetCompressedTexImage(target, level, img)

/* nothing to do */
#define FN_GLGETCOMPRESSEDTEXIMAGEARB(target, level, img) \
	fn.glGetCompressedTexImageARB(target, level, img)

/* nothing to do */
#define FN_GLGETCOMPRESSEDTEXTUREIMAGEEXT(texture, target, lod, img) \
	fn.glGetCompressedTextureImageEXT(texture, target, lod, img)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXIMAGE1DEXT(texunit, target, level, internalformat, width, border, imageSize, bits) \
	fn.glCompressedMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXIMAGE2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, bits) \
	fn.glCompressedMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXIMAGE3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, bits) \
	fn.glCompressedMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE1DEXT(texunit, target, level, xoffset, width, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDMULTITEXSUBIMAGE3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	fn.glCompressedMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE1D(target, level, internalformat, width, border, imageSize, data) \
	fn.glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE2D(target, level, internalformat, width, height, border, imageSize, data) \
	fn.glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE3D(target, level, internalformat, width, height, depth, border, imageSize, data) \
	fn.glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE1DARB(target, level, internalformat, width, border, imageSize, data) \
	fn.glCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE2DARB(target, level, internalformat, width, height, border, imageSize, data) \
	fn.glCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXIMAGE3DARB(target, level, internalformat, width, height, depth, border, imageSize, data) \
	fn.glCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE1D(target, level, xoffset, width, format, imageSize, data) \
	fn.glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE2D(target, level, xoffset, yoffset, width, height, format, imageSize, data) \
	fn.glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
	fn.glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE1DARB(target, level, xoffset, width, format, imageSize, data) \
	fn.glCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data) \
	fn.glCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXSUBIMAGE3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
	fn.glCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTUREIMAGE1DEXT(texture, target, level, internalformat, width, border, imageSize, bits) \
	fn.glCompressedTextureImage1DEXT(texture, target, level, internalformat, width, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTUREIMAGE2DEXT(texture, target, level, internalformat, width, height, border, imageSize, bits) \
	fn.glCompressedTextureImage2DEXT(texture, target, level, internalformat, width, height, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTUREIMAGE3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, bits) \
	fn.glCompressedTextureImage3DEXT(texture, target, level, internalformat, width, height, depth, border, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE1DEXT(texture, target, level, xoffset, width, format, imageSize, bits) \
	fn.glCompressedTextureSubImage1DEXT(texture, target, level, xoffset, width, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits) \
	fn.glCompressedTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits)

/* nothing to do */
#define FN_GLCOMPRESSEDTEXTURESUBIMAGE3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits) \
	fn.glCompressedTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits)

/* nothing to do */
#define FN_GLNAMEDBUFFERDATAEXT(buffer, size, data, usage) \
	fn.glNamedBufferDataEXT(buffer, size, data, usage)

/* nothing to do */
#define FN_GLNAMEDBUFFERSUBDATAEXT(buffer, offset, size, data) \
	fn.glNamedBufferSubDataEXT(buffer, offset, size, data)

#define FN_GLGETNAMEDBUFFERPOINTERVEXT(buffer, pname, params) \
	void *tmp = NULL; \
	fn.glGetNamedBufferPointervEXT(buffer, pname, &tmp); \
	/* TODO */ \
	*params = NULL

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDBUFFERPARAMETERIVEXT(buffer, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetNamedBufferParameterivEXT(buffer, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETNAMEDBUFFERPARAMETERIVEXT(buffer, pname, params) \
	fn.glGetNamedBufferParameterivEXT(buffer, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDBUFFERPARAMETERUI64VNV(buffer, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLuint64EXT tmp[MAX(size, 16)]; \
		fn.glGetNamedBufferParameterui64vNV(buffer, pname, tmp); \
		Host2AtariInt64Ptr(size, tmp, params); \
	}
#else
#define FN_GLGETNAMEDBUFFERPARAMETERUI64VNV(buffer, pname, params) \
	fn.glGetNamedBufferParameterui64vNV(buffer, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXT(framebuffer, attachment, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetNamedFramebufferAttachmentParameterivEXT(framebuffer, attachment, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXT(framebuffer, attachment, pname, params) \
	fn.glGetNamedFramebufferAttachmentParameterivEXT(framebuffer, attachment, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDFRAMEBUFFERPARAMETERIVEXT(framebuffer, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetNamedFramebufferParameterivEXT(framebuffer, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETNAMEDFRAMEBUFFERPARAMETERIVEXT(framebuffer, pname, params) \
	fn.glGetNamedFramebufferParameterivEXT(framebuffer, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDRENDERBUFFERPARAMETERIVEXT(renderbuffer, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetNamedRenderbufferParameterivEXT(renderbuffer, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETNAMEDRENDERBUFFERPARAMETERIVEXT(renderbuffer, pname, params) \
	fn.glGetNamedRenderbufferParameterivEXT(renderbuffer, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFLOATI_VEXT(pname, index, params) \
	if (params) { \
		int n = nfglGetNumParams(pname); \
		GLfloat tmp[n]; \
		fn.glGetFloati_vEXT(pname, index, tmp); \
		Host2AtariFloatArray(n, tmp, params); \
	} else { \
		fn.glGetFloati_vEXT(pname, index, params); \
	}
#else
#define FN_GLGETFLOATI_VEXT(pname, index, params) \
	fn.glGetFloati_vEXT(pname, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETDOUBLEI_VEXT(pname, index, params) \
	int n = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(n, 16)]; \
	fn.glGetDoublei_vEXT(pname, index, tmp); \
	Host2AtariDoubleArray(n, tmp, params)
#else
#define FN_GLGETDOUBLEI_VEXT(pname, index, params) \
	fn.glGetDoublei_vEXT(pname, index, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTITEXENVFVEXT(texunit, target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMultiTexEnvfvEXT(texunit, target, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETMULTITEXENVFVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexEnvfvEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXENVIVEXT(texunit, target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetMultiTexEnvivEXT(texunit, target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETMULTITEXENVIVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexEnvivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETMULTITEXGENDVEXT(texunit, coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLdouble tmp[MAX(size, 16)]; \
		fn.glGetMultiTexGendvEXT(texunit, coord, pname, tmp); \
		Host2AtariDoubleArray(size, tmp, params); \
	}
#else
#define FN_GLGETMULTITEXGENDVEXT(texunit, coord, pname, params) \
	fn.glGetMultiTexGendvEXT(texunit, coord, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTITEXGENFVEXT(texunit, coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMultiTexGenfvEXT(texunit, coord, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETMULTITEXGENFVEXT(texunit, coord, pname, params) \
	fn.glGetMultiTexGenfvEXT(texunit, coord, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXGENIVEXT(texunit, coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetMultiTexGenivEXT(texunit, coord, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETMULTITEXGENIVEXT(texunit, coord, pname, params) \
	fn.glGetMultiTexGenivEXT(texunit, coord, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETMULTITEXIMAGEEXT(texunit, target, level, format, type, pixels) \
	GLsizei size, count; \
	GLint width = 0, height = 1, depth = 1; \
	void *result = NULL; \
	const void *src; \
	void *dst; \
	 \
	switch (type) \
	{ \
	case GL_UNSIGNED_BYTE: \
	case GL_BYTE: \
	case GL_UNSIGNED_BYTE_3_3_2: \
	case GL_UNSIGNED_BYTE_2_3_3_REV: \
    case GL_2_BYTES: \
    case GL_3_BYTES: \
    case GL_4_BYTES: \
	case GL_BITMAP: \
	case 1: \
		fn.glGetMultiTexImageEXT(texunit, target, level, format, type, pixels); \
		return; \
	} \
	switch (target) { \
	case GL_TEXTURE_1D: \
	case GL_PROXY_TEXTURE_1D: \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_WIDTH, &width); \
		break; \
	case GL_TEXTURE_2D: \
	case GL_PROXY_TEXTURE_2D: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: \
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: \
	case GL_PROXY_TEXTURE_CUBE_MAP: \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_HEIGHT, &height); \
		break; \
	case GL_TEXTURE_3D: \
	case GL_PROXY_TEXTURE_3D: \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_WIDTH, &width); \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_HEIGHT, &height); \
		fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, GL_TEXTURE_DEPTH, &depth); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	if (width == 0 || height == 0 || depth == 0) return; \
	/* FIXME: glPixelStore parameters are not taken into account */ \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetMultiTexImageEXT(texunit, target, level, format, type, pixels); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)pixels; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)pixels; \
	} else { \
		result = pixelBuffer(width, height, depth, format, type, size, count); \
		if (result == NULL) return; \
		fn.glGetMultiTexImageEXT(texunit, target, level, format, type, result); \
		src = result; \
		dst = pixels; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst); \
	free(result)
#else
#define FN_GLGETMULTITEXIMAGEEXT(texunit, target, level, format, type, pixels) \
	fn.glGetMultiTexImageEXT(texunit, target, level, format, type, pixels)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTITEXLEVELPARAMETERFVEXT(texunit, target, level, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMultiTexLevelParameterfvEXT(texunit, target, level, pname, tmp); \
	if (params) Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXLEVELPARAMETERFVEXT(texunit, target, level, pname, params) \
	fn.glGetMultiTexLevelParameterfvEXT(texunit, target, level, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXLEVELPARAMETERIVEXT(texunit, target, level, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETMULTITEXLEVELPARAMETERIVEXT(texunit, target, level, pname, params) \
	fn.glGetMultiTexLevelParameterivEXT(texunit, target, level, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXPARAMETERIIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexParameterIivEXT(texunit, target, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETMULTITEXPARAMETERIIVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexParameterIivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXPARAMETERIUIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLuint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexParameterIuivEXT(texunit, target, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETMULTITEXPARAMETERIUIVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexParameterIuivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTITEXPARAMETERFVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetMultiTexParameterfvEXT(texunit, target, pname, tmp); \
	if (params) Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETMULTITEXPARAMETERFVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexParameterfvEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTITEXPARAMETERIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetMultiTexParameterivEXT(texunit, target, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETMULTITEXPARAMETERIVEXT(texunit, target, pname, params) \
	fn.glGetMultiTexParameterivEXT(texunit, target, pname, params)
#endif

#define FN_GLMULTITEXCOORDPOINTEREXT(texunit, size, type, stride, pointer) \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, stride, -1, 0, pointer)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXENVFVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glMultiTexEnvfvEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXENVFVEXT(texunit, target, pname, params) \
	fn.glMultiTexEnvfvEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXENVIVEXT(texunit, target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glMultiTexEnvivEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXENVIVEXT(texunit, target, pname, params) \
	fn.glMultiTexEnvivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMULTITEXGENDVEXT(texunit, coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(size, 16)]; \
	Atari2HostDoubleArray(size, params, tmp); \
	fn.glMultiTexGendvEXT(texunit, coord, pname, tmp)
#else
#define FN_GLMULTITEXGENDVEXT(texunit, coord, pname, params) \
	fn.glMultiTexGendvEXT(texunit, coord, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXGENFVEXT(texunit, coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glMultiTexGenfvEXT(texunit, coord, pname, tmp)
#else
#define FN_GLMULTITEXGENFVEXT(texunit, coord, pname, params) \
	fn.glMultiTexGenfvEXT(texunit, coord, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXGENIVEXT(texunit, coord, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glMultiTexGenivEXT(texunit, coord, pname, tmp)
#else
#define FN_GLMULTITEXGENIVEXT(texunit, coord, pname, params) \
	fn.glMultiTexGenivEXT(texunit, coord, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXPARAMETERFVEXT(texunit, target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glMultiTexParameterfvEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXPARAMETERFVEXT(texunit, target, pname, params) \
	fn.glMultiTexParameterfvEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXPARAMETERIVEXT(texunit, target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glMultiTexParameterivEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXPARAMETERIVEXT(texunit, target, pname, params) \
	fn.glMultiTexParameterivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXPARAMETERIIVEXT(texunit, target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glMultiTexParameterIivEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXPARAMETERIIVEXT(texunit, target, pname, params) \
	fn.glMultiTexParameterIivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLMULTITEXPARAMETERIUIVEXT(texunit, target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glMultiTexParameterIuivEXT(texunit, target, pname, tmp)
#else
#define FN_GLMULTITEXPARAMETERIUIVEXT(texunit, target, pname, params) \
	fn.glMultiTexParameterIuivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXTUREPARAMETERIIVEXT(texunit, target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glTextureParameterIivEXT(texunit, target, pname, tmp)
#else
#define FN_GLTEXTUREPARAMETERIIVEXT(texunit, target, pname, params) \
	fn.glTextureParameterIivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLTEXTUREPARAMETERIUIVEXT(texunit, target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	GLuint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glTextureParameterIuivEXT(texunit, target, pname, tmp)
#else
#define FN_GLTEXTUREPARAMETERIUIVEXT(texunit, target, pname, params) \
	fn.glTextureParameterIuivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTUREPARAMETERIIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterIivEXT(texunit, target, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERIIVEXT(texunit, target, pname, params) \
	fn.glGetTextureParameterIivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETTEXTUREPARAMETERIUIVEXT(texunit, target, pname, params) \
	GLint const size = 1; \
	GLuint tmp[MAX(size, 16)]; \
	fn.glGetTextureParameterIuivEXT(texunit, target, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETTEXTUREPARAMETERIUIVEXT(texunit, target, pname, params) \
	fn.glGetTextureParameterIuivEXT(texunit, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXIMAGE1DEXT(texunit, target, level, internalformat, width, border, format, type, pixels) \
	void *tmp = convertPixels(width, 1, 1, format, type, pixels); \
	fn.glMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLMULTITEXIMAGE1DEXT(texunit, target, level, internalformat, width, border, format, type, pixels) \
	fn.glMultiTexImage1DEXT(texunit, target, level, internalformat, width, border, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXIMAGE2DEXT(texunit, target, level, internalformat, width, height, border, format, type, pixels) \
	void *tmp = convertPixels(width, height, 1, format, type, pixels); \
	fn.glMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLMULTITEXIMAGE2DEXT(texunit, target, level, internalformat, width, height, border, format, type, pixels) \
	fn.glMultiTexImage2DEXT(texunit, target, level, internalformat, width, height, border, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXIMAGE3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, pixels) \
	void *tmp = convertPixels(width, height, 1, format, type, pixels); \
	fn.glMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLMULTITEXIMAGE3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, pixels) \
	fn.glMultiTexImage3DEXT(texunit, target, level, internalformat, width, height, depth, border, format, type, pixels)
#endif

#define FN_GLGETPOINTERINDEXEDVEXT(target, index, data) \
	gl_get_pointer(target, index, data)

#define FN_GLGETPOINTERI_VEXT(target, index, data) \
	gl_get_pointer(target, index, data)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMATRIXLOADTRANSPOSEDEXT(mode, m) \
	GLint const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMatrixLoadTransposedEXT(mode, tmp)
#else
#define FN_GLMATRIXLOADTRANSPOSEDEXT(mode, m) \
	fn.glMatrixLoadTransposedEXT(mode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOADTRANSPOSEFEXT(mode, m) \
	GLint const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoadTransposefEXT(mode, tmp)
#else
#define FN_GLMATRIXLOADTRANSPOSEFEXT(mode, m) \
	fn.glMatrixLoadTransposefEXT(mode, m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMATRIXMULTTRANSPOSEDEXT(mode, m) \
	GLint const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMatrixMultTransposedEXT(mode, tmp)
#else
#define FN_GLMATRIXMULTTRANSPOSEDEXT(mode, m) \
	fn.glMatrixMultTransposedEXT(mode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULTTRANSPOSEFEXT(mode, m) \
	GLint const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMultTransposefEXT(mode, tmp)
#else
#define FN_GLMATRIXMULTTRANSPOSEFEXT(mode, m) \
	fn.glMatrixMultTransposefEXT(mode, m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMATRIXLOADDEXT(mode, m) \
	GLint const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMatrixLoaddEXT(mode, tmp)
#else
#define FN_GLMATRIXLOADDEXT(mode, m) \
	fn.glMatrixLoaddEXT(mode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOADFEXT(mode, m) \
	GLint const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoadfEXT(mode, tmp)
#else
#define FN_GLMATRIXLOADFEXT(mode, m) \
	fn.glMatrixLoadfEXT(mode, m)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLMATRIXMULTDEXT(mode, m) \
	GLint const size = 16; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, m, tmp); \
	fn.glMatrixMultdEXT(mode, tmp)
#else
#define FN_GLMATRIXMULTDEXT(mode, m) \
	fn.glMatrixMultdEXT(mode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULTFEXT(mode, m) \
	GLint const size = 16; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMultfEXT(mode, tmp)
#else
#define FN_GLMATRIXMULTFEXT(mode, m) \
	fn.glMatrixMultfEXT(mode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM1FVEXT(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform1fvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1FVEXT(program, location, count, value) \
	fn.glProgramUniform1fvEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM2FVEXT(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform2fvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2FVEXT(program, location, count, value) \
	fn.glProgramUniform2fvEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM3FVEXT(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform3fvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3FVEXT(program, location, count, value) \
	fn.glProgramUniform3fvEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM4FVEXT(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform4fvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4FVEXT(program, location, count, value) \
	fn.glProgramUniform4fvEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1IVEXT(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform1ivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1IVEXT(program, location, count, value) \
	fn.glProgramUniform1ivEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2IVEXT(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform2ivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2IVEXT(program, location, count, value) \
	fn.glProgramUniform2ivEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3IVEXT(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform3ivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3IVEXT(program, location, count, value) \
	fn.glProgramUniform3ivEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4IVEXT(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform4ivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4IVEXT(program, location, count, value) \
	fn.glProgramUniform4ivEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1UIVEXT(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform1uivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1UIVEXT(program, location, count, value) \
	fn.glProgramUniform1uivEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2UIVEXT(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform2uivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2UIVEXT(program, location, count, value) \
	fn.glProgramUniform2uivEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3UIVEXT(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform3uivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3UIVEXT(program, location, count, value) \
	fn.glProgramUniform3uivEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4UIVEXT(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform4uivEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4UIVEXT(program, location, count, value) \
	fn.glProgramUniform4uivEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM1DVEXT(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform1dvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1DVEXT(program, location, count, value) \
	fn.glProgramUniform1dvEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM2DVEXT(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform2dvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2DVEXT(program, location, count, value) \
	fn.glProgramUniform2dvEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM3DVEXT(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform3dvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3DVEXT(program, location, count, value) \
	fn.glProgramUniform3dvEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM4DVEXT(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform4dvEXT(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4DVEXT(program, location, count, value) \
	fn.glProgramUniform4dvEXT(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2FVEXT(program, location, count, transpose, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3FVEXT(program, location, count, transpose, value) \
	GLint const size = 9 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4FVEXT(program, location, count, transpose, value) \
	GLint const size = 16 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X3FVEXT(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x3fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X3FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x3fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X2FVEXT(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x2fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X2FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x2fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X4FVEXT(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x4fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X4FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x4fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X2FVEXT(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x2fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X2FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x2fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X4FVEXT(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x4fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X4FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x4fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X3FVEXT(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x3fvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X3FVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x3fvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2DVEXT(program, location, count, transpose, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3DVEXT(program, location, count, transpose, value) \
	GLint const size = 9 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4DVEXT(program, location, count, transpose, value) \
	GLint const size = 16 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X3DVEXT(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x3dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X3DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x3dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X2DVEXT(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x2dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X2DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x2dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X4DVEXT(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x4dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X4DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x4dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X2DVEXT(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x2dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X2DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x2dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X4DVEXT(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x4dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X4DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x4dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X3DVEXT(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x3dvEXT(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X3DVEXT(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x3dvEXT(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERS4FVEXT(program, target, index, count, params) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glNamedProgramLocalParameters4fvEXT(program, target, index, count, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERS4FVEXT(program, target, index, count, params) \
	fn.glNamedProgramLocalParameters4fvEXT(program, target, index, count, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERSI4IVEXT(program, target, index, count, params) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glNamedProgramLocalParametersI4ivEXT(program, target, index, count, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERSI4IVEXT(program, target, index, count, params) \
	fn.glNamedProgramLocalParametersI4ivEXT(program, target, index, count, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXT(program, target, index, count, params) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glNamedProgramLocalParametersI4uivEXT(program, target, index, count, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXT(program, target, index, count, params) \
	fn.glNamedProgramLocalParametersI4uivEXT(program, target, index, count, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERI4IVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glNamedProgramLocalParameterI4ivEXT(program, target, index, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERI4IVEXT(program, target, index, params) \
	fn.glNamedProgramLocalParameterI4ivEXT(program, target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETERI4UIVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glNamedProgramLocalParameterI4uivEXT(program, target, index, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETERI4UIVEXT(program, target, index, params) \
	fn.glNamedProgramLocalParameterI4uivEXT(program, target, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETER4DVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, params, tmp); \
	fn.glNamedProgramLocalParameter4dvEXT(program, target, index, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETER4DVEXT(program, target, index, params) \
	fn.glNamedProgramLocalParameter4dvEXT(program, target, index, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLNAMEDPROGRAMLOCALPARAMETER4FVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, params, tmp); \
	fn.glNamedProgramLocalParameter4fvEXT(program, target, index, tmp)
#else
#define FN_GLNAMEDPROGRAMLOCALPARAMETER4FVEXT(program, target, index, params) \
	fn.glNamedProgramLocalParameter4fvEXT(program, target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERIIVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLint tmp[size]; \
	fn.glGetNamedProgramLocalParameterIivEXT(program, target, index, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERIIVEXT(program, target, index, params) \
	fn.glGetNamedProgramLocalParameterIivEXT(program, target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	fn.glGetNamedProgramLocalParameterIuivEXT(program, target, index, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXT(program, target, index, params) \
	fn.glGetNamedProgramLocalParameterIuivEXT(program, target, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERDVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	fn.glGetNamedProgramLocalParameterdvEXT(program, target, index, tmp); \
	if (params) Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERDVEXT(program, target, index, params) \
	fn.glGetNamedProgramLocalParameterdvEXT(program, target, index, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERFVEXT(program, target, index, params) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	fn.glGetNamedProgramLocalParameterfvEXT(program, target, index, tmp); \
	if (params) Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMLOCALPARAMETERFVEXT(program, target, index, params) \
	fn.glGetNamedProgramLocalParameterfvEXT(program, target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDPROGRAMIVEXT(program, target, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetNamedProgramivEXT(program, target, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETNAMEDPROGRAMIVEXT(program, target, pname, params) \
	fn.glGetNamedProgramivEXT(program, target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXARRAYINTEGERVEXT(vaobj, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVertexArrayIntegervEXT(vaobj, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETVERTEXARRAYINTEGERVEXT(vaobj, pname, params) \
	fn.glGetVertexArrayIntegervEXT(vaobj, pname, params)
#endif

// FIXME: need to track vertex arrays
#define FN_GLGETVERTEXARRAYPOINTERVEXT(vaobj, pname, params) \
	UNUSED(vaobj); \
	UNUSED(pname); \
	UNUSED(params); \
	glSetError(GL_INVALID_OPERATION)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXARRAYINTEGERI_VEXT(vaobj, index, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVertexArrayIntegeri_vEXT(vaobj, index, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETVERTEXARRAYINTEGERI_VEXT(vaobj, index, pname, params) \
	fn.glGetVertexArrayIntegeri_vEXT(vaobj, index, pname, params)
#endif

// FIXME: need to track vertex arrays
#define FN_GLGETVERTEXARRAYPOINTERI_VEXT(vaobj, index, pname, params) \
	UNUSED(vaobj); \
	UNUSED(index); \
	UNUSED(pname); \
	UNUSED(params); \
	glSetError(GL_INVALID_OPERATION)

/* pname can only be PROGRAM_NAME_ASCII_ARB */
#define FN_GLGETNAMEDPROGRAMSTRINGEXT(program, target, pname, string) \
	fn.glGetNamedProgramStringEXT(program, target, pname, string)

/* format can only be PROGRAM_STRING_ARB */
#define FN_GLNAMEDPROGRAMSTRINGEXT(program, target, format, len, string) \
	fn.glNamedProgramStringEXT(program, target, format, len, string)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLFRAMEBUFFERDRAWBUFFERSEXT(framebuffer, n, bufs) \
	GLenum *tmp; \
	if (n && bufs) { \
		tmp = (GLenum *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, bufs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glFramebufferDrawBuffersEXT(framebuffer, n, tmp); \
	free(tmp)
#else
#define FN_GLFRAMEBUFFERDRAWBUFFERSEXT(framebuffer, n, bufs) \
	fn.glFramebufferDrawBuffersEXT(framebuffer, n, bufs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAMEBUFFERPARAMETERIVEXT(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetFramebufferParameterivEXT(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETFRAMEBUFFERPARAMETERIVEXT(target, pname, params) \
	fn.glGetFramebufferParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXSUBIMAGE1DEXT(texunit, target, level, xoffset, width, format, type, pixels) \
	void *tmp = convertPixels(width, 1, 1, format, type, pixels); \
	fn.glMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLMULTITEXSUBIMAGE1DEXT(texunit, target, level, xoffset, width, format, type, pixels) \
	fn.glMultiTexSubImage1DEXT(texunit, target, level, xoffset, width, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXSUBIMAGE2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, pixels) \
	void *tmp = convertPixels(width, height, 1, format, type, pixels); \
	fn.glMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLMULTITEXSUBIMAGE2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, pixels) \
	fn.glMultiTexSubImage2DEXT(texunit, target, level, xoffset, yoffset, width, height, format, type, pixels)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMULTITEXSUBIMAGE3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	void *tmp = convertPixels(width, height, depth, format, type, pixels); \
	fn.glMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp); \
	if (tmp != pixels) free(tmp)
#else
#define FN_GLMULTITEXSUBIMAGE3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
	fn.glMultiTexSubImage3DEXT(texunit, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_AMD_gpu_shader_int64
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1I64VNV(program, location, count, value) \
	GLint const size = 1 * count; \
	GLint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glProgramUniform1i64vNV(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1I64VNV(program, location, count, value) \
	fn.glProgramUniform1i64vNV(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2I64VNV(program, location, count, value) \
	GLint const size = 2 * count; \
	GLint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glProgramUniform2i64vNV(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2I64VNV(program, location, count, value) \
	fn.glProgramUniform2i64vNV(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3I64VNV(program, location, count, value) \
	GLint const size = 3 * count; \
	GLint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glProgramUniform3i64vNV(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3I64VNV(program, location, count, value) \
	fn.glProgramUniform3i64vNV(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4I64VNV(program, location, count, value) \
	GLint const size = 4 * count; \
	GLint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glProgramUniform4i64vNV(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4I64VNV(program, location, count, value) \
	fn.glProgramUniform4i64vNV(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1UI64VNV(program, location, count, value) \
	GLint const size = 1 * count; \
	GLuint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glProgramUniform1ui64vNV(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1UI64VNV(program, location, count, value) \
	fn.glProgramUniform1ui64vNV(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2UI64VNV(program, location, count, value) \
	GLint const size = 2 * count; \
	GLuint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glProgramUniform2ui64vNV(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2UI64VNV(program, location, count, value) \
	fn.glProgramUniform2ui64vNV(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3UI64VNV(program, location, count, value) \
	GLint const size = 3 * count; \
	GLuint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glProgramUniform3ui64vNV(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3UI64VNV(program, location, count, value) \
	fn.glProgramUniform3ui64vNV(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4UI64VNV(program, location, count, value) \
	GLint const size = 4 * count; \
	GLuint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glProgramUniform4ui64vNV(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4UI64VNV(program, location, count, value) \
	fn.glProgramUniform4ui64vNV(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1I64VNV(location, count, value) \
	GLint const size = 1 * count; \
	GLint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glUniform1i64vNV(location, count, tmp)
#else
#define FN_GLUNIFORM1I64VNV(location, count, value) \
	fn.glUniform1i64vNV(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2I64VNV(location, count, value) \
	GLint const size = 2 * count; \
	GLint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glUniform2i64vNV(location, count, tmp)
#else
#define FN_GLUNIFORM2I64VNV(location, count, value) \
	fn.glUniform2i64vNV(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3I64VNV(location, count, value) \
	GLint const size = 3 * count; \
	GLint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glUniform3i64vNV(location, count, tmp)
#else
#define FN_GLUNIFORM3I64VNV(location, count, value) \
	fn.glUniform3i64vNV(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4I64VNV(location, count, value) \
	GLint const size = 4 * count; \
	GLint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glUniform4i64vNV(location, count, tmp)
#else
#define FN_GLUNIFORM4I64VNV(location, count, value) \
	fn.glUniform4i64vNV(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1UI64VNV(location, count, value) \
	GLint const size = 1 * count; \
	GLuint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glUniform1ui64vNV(location, count, tmp)
#else
#define FN_GLUNIFORM1UI64VNV(location, count, value) \
	fn.glUniform1ui64vNV(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2UI64VNV(location, count, value) \
	GLint const size = 2 * count; \
	GLuint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glUniform2ui64vNV(location, count, tmp)
#else
#define FN_GLUNIFORM2UI64VNV(location, count, value) \
	fn.glUniform2ui64vNV(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3UI64VNV(location, count, value) \
	GLint const size = 1 * count; \
	GLuint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glUniform3ui64vNV(location, count, tmp)
#else
#define FN_GLUNIFORM3UI64VNV(location, count, value) \
	fn.glUniform3ui64vNV(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4UI64VNV(location, count, value) \
	GLint const size = 1 * count; \
	GLuint64EXT tmp[size]; \
	Atari2HostInt64Ptr(size, value, tmp); \
	fn.glUniform4ui64vNV(location, count, tmp)
#else
#define FN_GLUNIFORM4UI64VNV(location, count, value) \
	fn.glUniform4ui64vNV(location, count, value)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_vertex_array_object
 */
#define FN_GLDELETEOBJECTBUFFERATI(buffer) nfglFreeObjectBufferATI(buffer)

#define FN_GLNEWOBJECTBUFFERATI(size, pointer, usage) \
	GLuint name = fn.glNewObjectBufferATI(size, pointer, usage); \
	if (!name) return name; \
	gl_buffer_t *buf = gl_make_buffer(name, size, pointer); \
	if (buf) buf->usage = usage; \
	return name

#define FN_GLFREEOBJECTBUFFERATI(buffer) \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (buf) { \
		free(buf->atari_buffer); \
		buf->atari_buffer = NULL; \
		free(buf->host_buffer); \
		buf->host_buffer = NULL; \
		buf->size = 0; \
	}

#define FN_GLUPDATEOBJECTBUFFERATI(buffer, offset, size, pointer, preserve) \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (!buf) buf = gl_make_buffer(buffer, offset + size, pointer); \
	if (!buf) return; \
	memcpy((char *)buf->atari_buffer + offset, pointer, size); \
	convertClientArrays(0); \
	fn.glUpdateObjectBufferATI(buffer, offset, size, buf->host_buffer, preserve)
	
#define FN_GLGETARRAYOBJECTFVATI(array, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetArrayObjectfvATI(array, pname, tmp); \
	if (params) Host2AtariFloatArray(size, tmp, params)

#define FN_GLGETARRAYOBJECTIVATI(array, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetArrayObjectivATI(array, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)

#define FN_GLGETOBJECTBUFFERFVATI(buffer, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetObjectBufferfvATI(buffer, pname, tmp); \
	if (params) Host2AtariFloatArray(size, tmp, params)

#define FN_GLGETOBJECTBUFFERIVATI(buffer, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetObjectBufferivATI(buffer, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)

#define FN_GLGETVARIANTARRAYOBJECTFVATI(id, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetVariantArrayObjectfvATI(id, pname, tmp); \
	if (params) Host2AtariFloatArray(size, tmp, params)

#define FN_GLGETVARIANTARRAYOBJECTIVATI(id, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetVariantArrayObjectivATI(id, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)

#define FN_GLARRAYOBJECTATI(array, size, type, stride, buffer, offset) \
	vertexarray_t *arr = gl_get_array(array); \
	if (!arr) return; \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (!buf) return; \
	arr->buffer_offset = offset; \
	setupClientArray(0, *arr, size, type, stride, -1, 0, buf->atari_buffer + offset)

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_shader_objects
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINFOLOGARB(obj, maxLength, length, infoLog) \
	GLint const size = 1; \
	GLsizei tmp[size]; \
	fn.glGetInfoLogARB(obj, maxLength, tmp, infoLog); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETINFOLOGARB(obj, maxLength, length, infoLog) \
	fn.glGetInfoLogARB(obj, maxLength, length, infoLog)
#endif

#define FN_GLSHADERSOURCEARB(shaderObj, count, strings, length) \
	GLcharARB **pstrings; \
	GLint *plength = NULL; \
	if (strings && count) { \
		pstrings = (GLcharARB **)malloc(count * sizeof(*pstrings)); \
		if (!pstrings) { glSetError(GL_OUT_OF_MEMORY); return; } \
		if (length) { \
			plength = (GLint *)malloc(count * sizeof(*plength)); \
			if (!plength) { glSetError(GL_OUT_OF_MEMORY); return; } \
		} \
		const Uint32 *p = (const Uint32 *)strings; \
		for (GLsizei i = 0; i < count; i++) \
			pstrings[i] = p[i] ? (GLcharARB *)Atari2HostAddr(SDL_SwapBE32(p[i])) : NULL; \
		if (length) Atari2HostIntPtr(count, length, plength); \
	} else { \
		pstrings = NULL; \
	} \
	fn.glShaderSourceARB(shaderObj, count, (const GLcharARB **)pstrings, plength); \
	free(plength); \
	free(pstrings)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM1FVARB(location, count, value) \
	GLint const size = 1 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform1fvARB(location, count, tmp)
#else
#define FN_GLUNIFORM1FVARB(location, count, value) \
	fn.glUniform1fvARB(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1IVARB(location, count, value) \
	GLint const size = 1 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glUniform1ivARB(location, count, tmp)
#else
#define FN_GLUNIFORM1IVARB(location, count, value) \
	fn.glUniform1ivARB(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM2FVARB(location, count, value) \
	GLint const size = 2 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform2fvARB(location, count, tmp)
#else
#define FN_GLUNIFORM2FVARB(location, count, value) \
	fn.glUniform2fvARB(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2IVARB(location, count, value) \
	GLint const size = 2 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glUniform2ivARB(location, count, tmp)
#else
#define FN_GLUNIFORM2IVARB(location, count, value) \
	fn.glUniform2ivARB(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM3FVARB(location, count, value) \
	GLint const size = 3 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform3fvARB(location, count, tmp)
#else
#define FN_GLUNIFORM3FVARB(location, count, value) \
	fn.glUniform3fvARB(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3IVARB(location, count, value) \
	GLint const size = 3 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glUniform3ivARB(location, count, tmp)
#else
#define FN_GLUNIFORM3IVARB(location, count, value) \
	fn.glUniform3ivARB(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM4FVARB(location, count, value) \
	GLint const size = 4 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform4fvARB(location, count, tmp)
#else
#define FN_GLUNIFORM4FVARB(location, count, value) \
	fn.glUniform4fvARB(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4IVARB(location, count, value) \
	GLint const size = 4 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glUniform4ivARB(location, count, tmp)
#else
#define FN_GLUNIFORM4IVARB(location, count, value) \
	fn.glUniform4ivARB(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX2FVARB(location, count, transpose, value) \
	GLint const size = 4 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix2fvARB(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX2FVARB(location, count, transpose, value) \
	fn.glUniformMatrix2fvARB(location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX3FVARB(location, count, transpose, value) \
	GLint const size = 9 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix3fvARB(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX3FVARB(location, count, transpose, value) \
	fn.glUniformMatrix3fvARB(location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX4FVARB(location, count, transpose, value) \
	GLint const size = 16 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix4fvARB(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX4FVARB(location, count, transpose, value) \
	fn.glUniformMatrix4fvARB(location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETOBJECTPARAMETERFVARB(obj, pname, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetObjectParameterfvARB(obj, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETOBJECTPARAMETERFVARB(obj, pname, params) \
	fn.glGetObjectParameterfvARB(obj, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOBJECTPARAMETERIVARB(obj, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetObjectParameterivARB(obj, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETOBJECTPARAMETERIVARB(obj, pname, params) \
	fn.glGetObjectParameterivARB(obj, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETUNIFORMFVARB(program, location, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetUniformfvARB(program, location, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETUNIFORMFVARB(program, location, params) \
	fn.glGetUniformfvARB(program, location, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMIVARB(program, location, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetUniformivARB(program, location, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETUNIFORMIVARB(program, location, params) \
	fn.glGetUniformivARB(program, location, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERSOURCEARB(program, maxLength, length, source) \
	GLint const size = 1; \
	GLsizei tmp[size]; \
	fn.glGetShaderSourceARB(program, maxLength, tmp, source); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETSHADERSOURCEARB(program, maxLength, length, source) \
	fn.glGetShaderSourceARB(program, maxLength, length, source)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_shading_language_include
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDSTRINGIVARB(namelen, name, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetNamedStringivARB(namelen, name, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETNAMEDSTRINGIVARB(namelen, name, pname, params) \
	fn.glGetNamedStringivARB(namelen, name, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETNAMEDSTRINGARB(namelen, name, bufSize, stringlen, string) \
	GLint const size = 1;\
	GLint tmp[size]; \
	fn.glGetNamedStringARB(namelen, name, bufSize, tmp, string); \
	if (stringlen) Host2AtariIntPtr(size, tmp, stringlen)
#else
#define FN_GLGETNAMEDSTRINGARB(namelen, name, bufSize, stringlen, string) \
	fn.glGetNamedStringARB(namelen, name, bufSize, stringlen, string)
#endif

#define FN_GLCOMPILESHADERINCLUDEARB(shader, count, path, length) \
	GLchar **ppath; \
	GLint *plength; \
	if (path) { \
		ppath = (GLchar **)malloc(count * sizeof(*ppath)); \
		if (!ppath) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *p = (const Uint32 *)path; \
		/* FIXME: the pathnames here are meaningless to the host */ \
		for (GLsizei i = 0; i < count; i++) \
			ppath[i] = p[i] ? (GLchar *)Atari2HostAddr(SDL_SwapBE32(p[i])) : NULL; \
	} else { \
		ppath = NULL; \
	} \
	if (length) { \
		plength = (GLint *)malloc(count * sizeof(*plength)); \
		if (!plength) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(count, length, plength); \
	} else { \
		plength = NULL; \
	} \
	fn.glCompileShaderIncludeARB(shader, count, ppath, plength); \
	free(plength); \
	free(ppath)

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_element_array
 */

#define FN_GLELEMENTPOINTERAPPLE(type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].element, 1, type, 0, -1, 0, pointer); \
	contexts[cur_context].element.vendor = 1

#define FN_GLDRAWELEMENTARRAYAPPLE(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawElementArrayAPPLE(mode, first, count)

#define FN_GLDRAWRANGEELEMENTARRAYAPPLE(mode, start, end, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawRangeElementArrayAPPLE(mode, start, end, first, count)

#define FN_GLMULTIDRAWELEMENTARRAYAPPLE(mode, first, count, primcount) \
	GLsizei const size = primcount; \
	GLint firstbuf[size]; \
	GLsizei countbuf[size]; \
	if (size <= 0) { glSetError(GL_INVALID_VALUE); return; } \
	Atari2HostIntPtr(size, first, firstbuf); \
	Atari2HostIntPtr(size, count, countbuf); \
	for (GLsizei i = 0; i < size; i++) \
		convertClientArrays(firstbuf[i] + countbuf[i]); \
	fn.glMultiDrawElementArrayAPPLE(mode, firstbuf, countbuf, primcount)

#define FN_GLMULTIDRAWRANGEELEMENTARRAYAPPLE(mode, start, end, first, count, primcount) \
	GLsizei const size = primcount; \
	GLint firstbuf[size]; \
	GLsizei countbuf[size]; \
	if (size <= 0) { glSetError(GL_INVALID_VALUE); return; } \
	Atari2HostIntPtr(size, first, firstbuf); \
	Atari2HostIntPtr(size, count, countbuf); \
	for (GLsizei i = 0; i < size; i++) \
		convertClientArrays(firstbuf[i] + countbuf[i]); \
	fn.glMultiDrawRangeElementArrayAPPLE(mode, start, end, firstbuf, countbuf, primcount)

/* -------------------------------------------------------------------------- */

/*
 * GL_ATI_element_array
 */

#define FN_GLELEMENTPOINTERATI(type, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].element, 1, type, 0, -1, 0, pointer); \
	contexts[cur_context].element.vendor = 2

#define FN_GLDRAWELEMENTARRAYATI(mode, count) \
	convertClientArrays(count); \
	fn.glDrawElementArrayATI(mode, count)

#define FN_GLDRAWRANGEELEMENTARRAYATI(mode, start, end, count) \
	convertClientArrays(count); \
	fn.glDrawRangeElementArrayATI(mode, start, end, count)

/* -------------------------------------------------------------------------- */

/*
 * GL_APPLE_object_purgeable
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOBJECTPARAMETERIVAPPLE(objectType, name, pname, params) \
	GLint const size = 1; \
	GLint tmp[1]; \
	fn.glGetObjectParameterivAPPLE(objectType, name, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETOBJECTPARAMETERIVAPPLE(objectType, name, pname, params) \
	fn.glGetObjectParameterivAPPLE(objectType, name, pname, params)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_occlusion_query
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENOCCLUSIONQUERIESNV(n, ids) \
	fn.glGenOcclusionQueriesNV(n, ids); \
	if (n > 0 && ids) { \
		Host2AtariIntPtr(n, ids, ids); \
	}
#else
#define FN_GLGENOCCLUSIONQUERIESNV(n, ids) \
	fn.glGenOcclusionQueriesNV(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEOCCLUSIONQUERIESNV(n, ids) \
	GLsizei const size = n; \
	GLuint tmp[size]; \
	if (size <= 0) { glSetError(GL_INVALID_VALUE); return; } \
	Atari2HostIntPtr(size, ids, tmp); \
	fn.glDeleteOcclusionQueriesNV(n, tmp)
#else
#define FN_GLDELETEOCCLUSIONQUERIESNV(n, ids) \
	fn.glDeleteOcclusionQueriesNV(n, ids)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOCCLUSIONQUERYIVNV(id, pname, params) \
	GLsizei const size = 1; \
	GLint tmp[size]; \
	fn.glGetOcclusionQueryivNV(id, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETOCCLUSIONQUERYIVNV(id, pname, params) \
	fn.glGetOcclusionQueryivNV(id, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOCCLUSIONQUERYUIVNV(id, pname, params) \
	GLsizei const size = 1; \
	GLuint tmp[size]; \
	fn.glGetOcclusionQueryuivNV(id, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETOCCLUSIONQUERYUIVNV(id, pname, params) \
	fn.glGetOcclusionQueryuivNV(id, pname, params)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_path_rendering
 */

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	void *tmp = convertArray(numPaths, pathNameType, paths); \
	GLfloat *vals; \
	if (transformValues && numPaths) { \
		vals = (GLfloat *)malloc(numPaths * sizeof(*vals)); \
		if (!vals) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostFloatArray(numPaths, transformValues, vals); \
	} else { \
		vals = NULL; \
	} \
	fn.glCoverFillPathInstancedNV(numPaths, pathNameType, tmp, pathBase, coverMode, transformType, vals); \
	free(vals); \
	if (tmp != paths) free(tmp)
#else
#define FN_GLCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	fn.glCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	void *tmp = convertArray(numPaths, pathNameType, paths); \
	GLfloat *vals; \
	if (transformValues && numPaths) { \
		vals = (GLfloat *)malloc(numPaths * sizeof(*vals)); \
		if (!vals) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostFloatArray(numPaths, transformValues, vals); \
	} else { \
		vals = NULL; \
	} \
	fn.glCoverStrokePathInstancedNV(numPaths, pathNameType, tmp, pathBase, coverMode, transformType, vals); \
	free(vals); \
	if (tmp != paths) free(tmp)
#else
#define FN_GLCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues) \
	fn.glCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHCOMMANDSNV(path, numCommands, commands, numCoords, coordType, coords) \
	void *tmp = convertArray(numCoords, coordType, coords); \
	fn.glPathCommandsNV(path, numCommands, commands, numCoords, coordType, tmp); \
	if (tmp != coords) free(tmp)
#else
#define FN_GLPATHCOMMANDSNV(path, numCommands, commands, numCoords, coordType, coords) \
	fn.glPathCommandsNV(path, numCommands, commands, numCoords, coordType, coords)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHCOORDSNV(path, numCoords, coordType, coords) \
	void *tmp = convertArray(numCoords, coordType, coords); \
	fn.glPathCoordsNV(path, numCoords, coordType, tmp); \
	if (tmp != coords) free(tmp)
#else
#define FN_GLPATHCOORDSNV(path, numCoords, coordType, coords) \
	fn.glPathCoordsNV(path, numCoords, coordType, coords)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHSUBCOMMANDSNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords) \
	void *tmp = convertArray(numCoords, coordType, coords); \
	fn.glPathSubCommandsNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, tmp); \
	if (tmp != coords) free(tmp)
#else
#define FN_GLPATHSUBCOMMANDSNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords) \
	fn.glPathSubCommandsNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHSUBCOORDSNV(path, coordStart, numCoords, coordType, coords) \
	void *tmp = convertArray(numCoords, coordType, coords); \
	fn.glPathSubCoordsNV(path, coordStart, numCoords, coordType, tmp); \
	if (tmp != coords) free(tmp)
#else
#define FN_GLPATHSUBCOORDSNV(path, coordStart, numCoords, coordType, coords) \
	fn.glPathSubCoordsNV(path, coordStart, numCoords, coordType, coords)
#endif

/* nothing to do; pathString is ascii string */
#define FN_GLPATHSTRINGNV(path, format, length, pathString) \
	fn.glPathStringNV(path, format, length, pathString)
	
#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHGLYPHSNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale) \
	void *tmp = convertArray(numGlyphs, type, charcodes); \
	/* fontName is ascii string */ \
	fn.glPathGlyphsNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, tmp, handleMissingGlyphs, pathParameterTemplate, emScale); \
	if (tmp != charcodes) free(tmp)
#else
#define FN_GLPATHGLYPHSNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale) \
	fn.glPathGlyphsNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale)
#endif

/* nothing to do; fontName is ascii string */
#define FN_GLPATHGLYPHRANGENV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale) \
	fn.glPathGlyphRangeNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale)

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLWEIGHTPATHSNV(resultPath, numPaths, paths, weights) \
	GLsizei const size = numPaths; \
	GLuint pathbuf[size]; \
	GLfloat weightbuf[size]; \
	if (size <= 0) { glSetError(GL_INVALID_VALUE); return; } \
	Atari2HostIntPtr(size, paths, pathbuf); \
	Atari2HostFloatArray(size, weights, weightbuf); \
	fn.glWeightPathsNV(resultPath, numPaths, pathbuf, weightbuf)
#else
#define FN_GLWEIGHTPATHSNV(resultPath, numPaths, paths, weights) \
	fn.glWeightPathsNV(resultPath, numPaths, paths, weights)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLTRANSFORMPATHNV(resultPath, srcPath, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glTransformPathNV(resultPath, srcPath, transformType, tmp)
#else
#define FN_GLTRANSFORMPATHNV(resultPath, srcPath, transformType, transformValues) \
	fn.glTransformPathNV(resultPath, srcPath, transformType, transformValues)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHPARAMETERFVNV(path, pname, value) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glPathParameterfvNV(path, pname, tmp)
#else
#define FN_GLPATHPARAMETERFVNV(path, pname, value) \
	fn.glPathParameterfvNV(path, pname, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPATHPARAMETERIVNV(path, pname, value) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glPathParameterivNV(path, pname, tmp)
#else
#define FN_GLPATHPARAMETERIVNV(path, pname, value) \
	fn.glPathParameterivNV(path, pname, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHPARAMETERFVNV(path, pname, value) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetPathParameterfvNV(path, pname, tmp); \
	Host2AtariFloatArray(size, tmp, value)
#else
#define FN_GLGETPATHPARAMETERFVNV(path, pname, value) \
	fn.glGetPathParameterfvNV(path, pname, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPATHPARAMETERIVNV(path, pname, value) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetPathParameterivNV(path, pname, tmp); \
	Host2AtariIntPtr(size, tmp, value)
#else
#define FN_GLGETPATHPARAMETERIVNV(path, pname, value) \
	fn.glGetPathParameterivNV(path, pname, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHDASHARRAYNV(path, dashCount, dashArray) \
	GLsizei const size = dashCount; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, dashArray, tmp); \
	fn.glPathDashArrayNV(path, dashCount, tmp)
#else
#define FN_GLPATHDASHARRAYNV(path, dashCount, dashArray) \
	fn.glPathDashArrayNV(path, dashCount, dashArray)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHDASHARRAYNV(path, dashArray) \
	GLint size = 0; \
	fn.glGetPathParameterivNV(path, GL_PATH_DASH_ARRAY_COUNT_NV, &size); \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetPathDashArrayNV(path, tmp); \
	Host2AtariFloatArray(size, tmp, dashArray)
#else
#define FN_GLGETPATHDASHARRAYNV(path, dashArray) \
	fn.glGetPathDashArrayNV(path, dashArray)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHCOORDSNV(path, coords) \
	GLint size = 0; \
	fn.glGetPathParameterivNV(path, GL_PATH_COORD_COUNT_NV, &size); \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetPathCoordsNV(path, tmp); \
	Host2AtariFloatArray(size, tmp, coords)
#else
#define FN_GLGETPATHCOORDSNV(path, coords) \
	fn.glGetPathCoordsNV(path, coords)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSTENCILFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	void *ppaths = convertArray(numPaths, pathNameType, paths); \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glStencilFillPathInstancedNV(numPaths, pathNameType, ppaths, pathBase, fillMode, mask, transformType, tmp); \
	if (ppaths != paths) free(ppaths)
#else
#define FN_GLSTENCILFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues) \
	fn.glStencilFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSTENCILSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	void *ppaths = convertArray(numPaths, pathNameType, paths); \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glStencilStrokePathInstancedNV(numPaths, pathNameType, ppaths, pathBase, reference, mask, transformType, tmp); \
	if (ppaths != paths) free(ppaths)
#else
#define FN_GLSTENCILSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues) \
	fn.glStencilStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHMETRICSNV(metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics) \
	GLsizei size = stride; \
	if (size < 0) { glSetError(GL_INVALID_VALUE); return; } \
	void *ppaths = convertArray(numPaths, pathNameType, paths); \
	if (size == 0) { \
		GLbitfield mask = metricQueryMask; \
		while (mask) \
		{ \
			if (mask & 1) size += sizeof(GLfloat); \
			mask >>= 1; \
		} \
	} else { \
		size = (stride / ATARI_SIZEOF_FLOAT) * sizeof(GLfloat); \
	} \
	GLfloat *tmp = (GLfloat *)calloc(numPaths, size); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	fn.glGetPathMetricsNV(metricQueryMask, numPaths, pathNameType, ppaths, pathBase, size, tmp); \
	Host2AtariFloatArray((size / sizeof(GLfloat)) * numPaths, tmp, metrics); \
	free(tmp); \
	if (ppaths != paths) free(ppaths)
#else
#define FN_GLGETPATHMETRICSNV(metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics) \
	fn.glGetPathMetricsNV(metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHMETRICRANGENV(metricQueryMask, firstPathName, numPaths, stride, metrics) \
	GLsizei size = stride; \
	if (size < 0) { glSetError(GL_INVALID_VALUE); return; } \
	if (size == 0) { \
		GLbitfield mask = metricQueryMask; \
		while (mask) \
		{ \
			if (mask & 1) size += sizeof(GLfloat); \
			mask >>= 1; \
		} \
	} else { \
		size = (stride / ATARI_SIZEOF_FLOAT) * sizeof(GLfloat); \
	} \
	GLfloat *tmp = (GLfloat *)calloc(numPaths, size); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	fn.glGetPathMetricRangeNV(metricQueryMask, firstPathName, numPaths, size, tmp); \
	Host2AtariFloatArray((size / sizeof(GLfloat)) * numPaths, tmp, metrics); \
	free(tmp)
#else
#define FN_GLGETPATHMETRICRANGENV(metricQueryMask, firstPathName, numPaths, stride, metrics) \
	fn.glGetPathMetricRangeNV(metricQueryMask, firstPathName, numPaths, stride, metrics)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHSPACINGNV(pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing) \
	GLsizei size; \
	if (numPaths <= 1) { glSetError(GL_INVALID_VALUE); return; } \
	switch (transformType) { \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	size *= numPaths - 1; \
	void *ppaths = convertArray(numPaths, pathNameType, paths); \
	GLfloat *tmp = (GLfloat *)calloc(size, sizeof(GLfloat)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	fn.glGetPathSpacingNV(pathListMode, numPaths, pathNameType, ppaths, pathBase, advanceScale, kerningScale, transformType, tmp); \
	Host2AtariFloatArray(size, tmp, returnedSpacing); \
	if (ppaths != paths) free(ppaths)
#else
#define FN_GLGETPATHSPACINGNV(pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing) \
	fn.glGetPathSpacingNV(pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPOINTALONGPATHNV(path, startSegment, numSegments, distance, x, y, tangentX, tangentY) \
	GLsizei const size = 1; \
	GLfloat xret, yret, tangentXret, tangentYret; \
	GLboolean ret = fn.glPointAlongPathNV(path, startSegment, numSegments, distance, &xret, &yret, &tangentXret, &tangentYret); \
	if (x) Host2AtariFloatArray(size, &xret, x); \
	if (y) Host2AtariFloatArray(size, &yret, y); \
	if (tangentX) Host2AtariFloatArray(size, &tangentXret, tangentX); \
	if (tangentY) Host2AtariFloatArray(size, &tangentYret, tangentY); \
	return ret
#else
#define FN_GLPOINTALONGPATHNV(path, startSegment, numSegments, distance, x, y, tangentX, tangentY) \
	return fn.glPointAlongPathNV(path, startSegment, numSegments, distance, x, y, tangentX, tangentY)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOAD3X2NV(matrixMode, m) \
	GLsizei const size = 6; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoad3x2fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXLOAD3X2NV(matrixMode, m) \
	fn.glMatrixLoad3x2fNV(matrixMode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOAD3X3NV(matrixMode, m) \
	GLsizei const size = 9; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoad3x3fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXLOAD3X3NV(matrixMode, m) \
	fn.glMatrixLoad3x3fNV(matrixMode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXLOADTRANSPOSE3X3NV(matrixMode, m) \
	GLsizei const size = 9; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixLoadTranspose3x3fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXLOADTRANSPOSE3X3NV(matrixMode, m) \
	fn.glMatrixLoadTranspose3x3fNV(matrixMode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULT3X2NV(matrixMode, m) \
	GLsizei const size = 6; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMult3x2fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXMULT3X2NV(matrixMode, m) \
	fn.glMatrixMult3x2fNV(matrixMode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULT3X3NV(matrixMode, m) \
	GLsizei const size = 9; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMult3x3fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXMult3X3NV(matrixMode, m) \
	fn.glMatrixMult3x3fNV(matrixMode, m)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLMATRIXMULTTRANSPOSE3X3NV(matrixMode, m) \
	GLsizei const size = 9; \
	Atari2HostFloatArray(size, m, tmp); \
	fn.glMatrixMultTranspose3x3fNV(matrixMode, tmp)
#else
#define FN_GLMATRIXMULTTRANSPOSE3X3NV(matrixMode, m) \
	fn.glMatrixMultTranspose3x3fNV(matrixMode, m)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSTENCILTHENCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	void *ppaths = convertArray(numPaths, pathNameType, paths); \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glStencilThenCoverFillPathInstancedNV(numPaths, pathNameType, ppaths, pathBase, fillMode, mask, coverMode, transformType, tmp); \
	if (ppaths != paths) free(ppaths)
#else
#define FN_GLSTENCILTHENCOVERFILLPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues) \
	fn.glStencilThenCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLSTENCILTHENCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues) \
	GLsizei size; \
	GLfloat tmp[16]; \
	switch (transformType) { \
		case GL_NONE: size = 0; break; \
		case GL_TRANSLATE_X_NV: size = 1; break; \
		case GL_TRANSLATE_Y_NV: size = 1; break; \
		case GL_TRANSLATE_2D_NV: size = 2; break; \
		case GL_TRANSLATE_3D_NV: size = 3; break; \
		case GL_AFFINE_2D_NV: size = 6; break; \
		case GL_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_AFFINE_2D_NV: size = 6; break; \
		case GL_TRANSPOSE_PROJECTIVE_2D_NV: size = 6; break; \
		case GL_AFFINE_3D_NV: size = 12; break; \
		case GL_PROJECTIVE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_AFFINE_3D_NV: size = 12; break; \
		case GL_TRANSPOSE_PROJECTIVE_3D_NV: size = 12; break; \
		default: glSetError(GL_INVALID_ENUM); return; \
	} \
	void *ppaths = convertArray(numPaths, pathNameType, paths); \
	Atari2HostFloatArray(size, transformValues, tmp); \
	fn.glStencilThenCoverStrokePathInstancedNV(numPaths, pathNameType, ppaths, pathBase, reference, mask, coverMode, transformType, tmp); \
	if (ppaths != paths) free(ppaths)
#else
#define FN_GLSTENCILTHENCOVERSTROKEPATHINSTANCEDNV(numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues) \
	fn.glStencilThenCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPATHGLYPHINDEXRANGENV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount) \
	GLsizei const size = 2; \
	GLuint tmp[size]; \
	/* fontName is ascii string */ \
	fn.glPathGlyphIndexRangeNV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, tmp); \
	if (baseAndCount) Host2AtariIntPtr(size, tmp, baseAndCount)
#else
#define FN_GLPATHGLYPHINDEXRANGENV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount) \
	fn.glPathGlyphIndexRangeNV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount)
#endif

/* nothing to do; fontName is ascii string */
#define FN_GLPATHGLYPHINDEXARRAYNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, pathParameterTemplate, emScale) \
	fn.glPathGlyphIndexArrayNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, pathParameterTemplate, emScale)

/* to be checked */
#define FN_glPathMemoryGlyphIndexArrayNV(firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale) \
	fn.glPathMemoryGlyphIndexArrayNV(firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMPATHFRAGMENTINPUTGENNV(program, location, genMode, components, coeffs) \
	GLsizei const size = components; \
	GLfloat tmp[size]; \
	if (coeffs) { \
		Atari2HostFloatArray(size, coeffs, tmp); \
		coeffs = tmp; \
	} \
	fn.glProgramPathFragmentInputGenNV(program, location, genMode, components, coeffs)
#else
#define FN_GLPROGRAMPATHFRAGMENTINPUTGENNV(program, location, genMode, components, coeffs) \
	fn.glProgramPathFragmentInputGenNV(program, location, genMode, components, coeffs)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHCOLORGENNV(color, genMode, colorFormat, coeffs) \
	GLsizei basesize, count; \
	if (!pixelParams(GL_FLOAT, colorFormat, basesize, count)) return; \
	GLint const size = nfglGetNumParams(genMode) * count; \
	GLfloat tmp[size]; \
	if (coeffs) { \
		Atari2HostFloatArray(size, coeffs, tmp); \
		coeffs = tmp; \
	} \
	fn.glPathColorGenNV(color, genMode, colorFormat, coeffs)
#else
#define FN_GLPATHCOLORGENNV(color, genMode, colorFormat, coeffs) \
	fn.glPathColorGenNV(color, genMode, colorFormat, coeffs)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHCOLORGENFVNV(color, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetPathColorGenfvNV(color, pname, tmp); \
	if (params) Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETPATHCOLORGENFVNV(color, pname, params) \
	fn.glGetPathColorGenfvNV(color, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPATHCOLORGENIVNV(color, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetPathColorGenivNV(color, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPATHCOLORGENIVNV(color, pname, params) \
	fn.glGetPathColorGenivNV(color, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPATHTEXGENNV(texCoordSet, genMode, components, coeffs) \
	GLint const size = components; \
	GLfloat tmp[size]; \
	if (coeffs) { \
		Atari2HostFloatArray(size, coeffs, tmp); \
		coeffs = tmp; \
	} \
	fn.glPathTexGenNV(texCoordSet, genMode, components, coeffs)
#else
#define FN_GLPATHTEXGENNV(texCoordSet, genMode, components, coeffs) \
	fn.glPathTexGenNV(texCoordSet, genMode, components, coeffs)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPATHTEXGENFVNV(texCoordSet, pname, value) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetPathTexGenfvNV(texCoordSet, pname, tmp); \
	if (value) Host2AtariFloatArray(size, tmp, value)
#else
#define FN_GLGETPATHTEXGENFVNV(texCoordSet, pname, value) \
	fn.glGetPathTexGenfvNV(texCoordSet, pname, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPATHTEXGENIVNV(texCoordSet, pname, value) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetPathTexGenivNV(texCoordSet, pname, tmp); \
	if (value) Host2AtariIntPtr(size, tmp, value)
#else
#define FN_GLGETPATHTEXGENIVNV(texCoordSet, pname, value) \
	fn.glGetPathTexGenivNV(texCoordSet, pname, value)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_AMD_performance_monitor
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPERFMONITORSAMD(n, monitors) \
	fn.glGenPerfMonitorsAMD(n, monitors); \
	if (n && monitors) { \
		Host2AtariIntPtr(n, monitors, monitors); \
	}
#else
#define FN_GLGENPERFMONITORSAMD(n, monitors) \
	fn.glGenPerfMonitorsAMD(n, monitors)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPERFMONITORSAMD(n, monitors) \
	GLuint *tmp; \
	if (n && monitors) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, monitors, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeletePerfMonitorsAMD(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEPERFMONITORSAMD(n, monitors) \
	fn.glDeletePerfMonitorsAMD(n, monitors)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORGROUPSAMD(numGroups, groupsSize, groups) \
	GLsizei const size = groupsSize; \
	GLuint tmp[size]; \
	GLuint *pgroups; \
	GLint num = 0; \
	if (size > 0 && groups) { \
		pgroups = tmp; \
	} else { \
		pgroups = NULL; \
	} \
	fn.glGetPerfMonitorGroupsAMD(&num, groupsSize, pgroups); \
	if (numGroups) Host2AtariIntPtr(1, &num, numGroups); \
	if (num > 0 && pgroups) Host2AtariIntPtr(num, pgroups, groups)
#else
#define FN_GLGETPERFMONITORGROUPSAMD(numGroups, groupsSize, groups) \
	fn.glGetPerfMonitorGroupsAMD(numGroups, groupsSize, groups)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORCOUNTERSAMD(group, numCounters, maxActiveCounters, counterSize, counters) \
	GLsizei const size = counterSize; \
	GLuint tmp[size]; \
	GLuint *pcounters; \
	GLint num = 0; \
	GLint max = 0; \
	if (size > 0 && counters) { \
		pcounters = tmp; \
	} else { \
		pcounters = NULL; \
	} \
	fn.glGetPerfMonitorCountersAMD(group, &num, &max, counterSize, pcounters); \
	if (numCounters) Host2AtariIntPtr(1, &num, numCounters); \
	if (maxActiveCounters) Host2AtariIntPtr(1, &max, maxActiveCounters); \
	if (num > 0 && pcounters) Host2AtariIntPtr(num, pcounters, counters)
#else
#define FN_GLGETPERFMONITORCOUNTERSAMD(group, numCounters, maxActiveCounters, counterSize, counters) \
	fn.glGetPerfMonitorCountersAMD(group, numCounters, maxActiveCounters, counterSize, counters)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORGROUPSTRINGAMD(group, bufSize, length, groupString) \
	GLsizei const size = 1; \
	GLsizei tmp[size]; \
	fn.glGetPerfMonitorGroupStringAMD(group, bufSize, tmp, groupString); \
	if (length) Host2AtariIntPtr(1, tmp, length)
#else
#define FN_GLGETPERFMONITORGROUPSTRINGAMD(group, bufSize, length, groupString) \
	fn.glGetPerfMonitorGroupStringAMD(group, bufSize, length, groupString)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORCOUNTERSTRINGAMD(group, counter, bufSize, length, counterString) \
	GLsizei const size = 1; \
	GLsizei tmp[size]; \
	fn.glGetPerfMonitorCounterStringAMD(group, counter, bufSize, tmp, counterString); \
	if (length) Host2AtariIntPtr(1, tmp, length)
#else
#define FN_GLGETPERFMONITORCOUNTERSTRINGAMD(group, counter, bufSize, length, counterString) \
	fn.glGetPerfMonitorCounterStringAMD(group, counter, bufSize, length, counterString)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORCOUNTERINFOAMD(group, counter, pname, data) \
	switch (pname) { \
	case GL_COUNTER_TYPE_AMD: \
		{ \
			GLint const size = 1; \
			GLenum tmp[size]; \
			fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, tmp); \
			if (data) Host2AtariIntPtr(size, tmp, (Uint32 *)data); \
		} \
		break; \
	case GL_COUNTER_RANGE_AMD: \
		{ \
			GLenum type = 0; \
			GLint const size = 2; \
			fn.glGetPerfMonitorCounterInfoAMD(group, counter, GL_COUNTER_TYPE_AMD, &type); \
			switch (type) { \
			case GL_UNSIGNED_INT: \
				{ \
					GLuint tmp[size]; \
					fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, tmp); \
					if (data) Host2AtariIntPtr(size, tmp, (Uint32 *)data); \
				} \
				break; \
			case GL_UNSIGNED_INT64_AMD: \
				{ \
					GLuint64 tmp[size]; \
					fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, tmp); \
					if (data) Host2AtariInt64Ptr(size, tmp, (Uint64 *)data); \
				} \
				break; \
			case GL_FLOAT: \
			case GL_PERCENTAGE_AMD: \
				{ \
					GLfloat tmp[size]; \
					fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, tmp); \
					if (data) Host2AtariFloatArray(size, tmp, (GLfloat *)data); \
				} \
				break; \
			default: \
				glSetError(GL_INVALID_OPERATION); \
				break; \
			} \
		} \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		break; \
	}
#else
#define FN_GLGETPERFMONITORCOUNTERINFOAMD(group, counter, pname, data) \
	fn.glGetPerfMonitorCounterInfoAMD(group, counter, pname, data)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSELECTPERFMONITORCOUNTERSAMD(monitor, enable, group, numCounters, counterList) \
	GLsizei const size = numCounters; \
	GLuint tmp[size]; \
	if (size <= 0 || !counterList) return; \
	Atari2HostIntPtr(size, counterList, tmp); \
	fn.glSelectPerfMonitorCountersAMD(monitor, enable, group, numCounters, tmp)
#else
#define FN_GLSELECTPERFMONITORCOUNTERSAMD(monitor, enable, group, numCounters, counterList) \
	fn.glSelectPerfMonitorCountersAMD(monitor, enable, group, numCounters, counterList)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPERFMONITORCOUNTERDATAAMD(monitor, pname, dataSize, data, bytesWritten) \
	GLint written = 0; \
	switch (pname) { \
	case GL_PERFMON_RESULT_AVAILABLE_AMD: \
	case GL_PERFMON_RESULT_SIZE_AMD: \
		{ \
			GLint const size = 1; \
			GLuint tmp[size]; \
			fn.glGetPerfMonitorCounterDataAMD(monitor, pname, dataSize, tmp, &written); \
			if (data) Host2AtariIntPtr(size, tmp, data); \
		} \
		break; \
	case GL_PERFMON_RESULT_AMD: \
		{ \
			GLint const size = dataSize / sizeof(GLuint); \
			if (size <= 0) return; \
			GLuint tmp[size]; \
			fn.glGetPerfMonitorCounterDataAMD(monitor, pname, dataSize, tmp, &written); \
			if (written > 0 && data) Host2AtariIntPtr(written / sizeof(GLuint), tmp, data); \
		} \
		break; \
	} \
	if (bytesWritten) Host2AtariIntPtr(1, &written, bytesWritten)
#else
#define FN_GLGETPERFMONITORCOUNTERDATAAMD(monitor, pname, dataSize, data, bytesWritten) \
	fn.glGetPerfMonitorCounterDataAMD(monitor, pname, dataSize, data, bytesWritten)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_OES_fixed_point
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELMAPXV(map, size, values) \
	if (size <= 0) return; \
	GLfixed tmp[size]; \
	fn.glGetPixelMapxv(map, size, tmp); \
	if (values) Host2AtariIntPtr(size, tmp, values)
#else
#define FN_GLGETPIXELMAPXV(map, size, values) \
	fn.glGetPixelMapxv(map, size, values)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_SGIS_pixel_texture
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPIXELTEXGENPARAMETERFVSGIS(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetPixelTexGenParameterfvSGIS(pname, params); \
	if (params) Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETPIXELTEXGENPARAMETERFVSGIS(pname, params) \
	fn.glGetPixelTexGenParameterfvSGIS(pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPIXELTEXGENPARAMETERFVSGIS(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	if (params) { Atari2HostFloatArray(size, params, tmp); params = tmp; } \
	fn.glPixelTexGenParameterfvSGIS(pname, params)
#else
#define FN_GLPIXELTEXGENPARAMETERFVSGIS(pname, params) \
	fn.glPixelTexGenParameterfvSGIS(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELTEXGENPARAMETERIVSGIS(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetPixelTexGenParameterivSGIS(pname, params); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPIXELTEXGENPARAMETERIVSGIS(pname, params) \
	fn.glGetPixelTexGenParameterivSGIS(pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELTEXGENPARAMETERIVSGIS(pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	if (params) { Atari2HostIntPtr(size, params, tmp); params = tmp; } \
	fn.glPixelTexGenParameterivSGIS(pname, params)
#else
#define FN_GLPIXELTEXGENPARAMETERIVSGIS(pname, params) \
	fn.glPixelTexGenParameterivSGIS(pname, params)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_pixel_transform
 */
#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPIXELTRANSFORMPARAMETERFVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	fn.glGetPixelTransformParameterfvEXT(target, pname, tmp); \
	if (params) Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETPIXELTRANSFORMPARAMETERFVEXT(target, pname, params) \
	fn.glGetPixelTransformParameterfvEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetPixelTransformParameterivEXT(target, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	fn.glGetPixelTransformParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPIXELTRANSFORMPARAMETERFVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[MAX(size, 16)]; \
	if (params) { Atari2HostFloatArray(size, params, tmp); params = tmp; } \
	fn.glPixelTransformParameterfvEXT(target, pname, params)
#else
#define FN_GLPIXELTRANSFORMPARAMETERFVEXT(target, pname, params) \
	fn.glPixelTransformParameterfvEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	fn.glGetPixelTransformParameterivEXT(target, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	fn.glGetPixelTransformParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[MAX(size, 16)]; \
	if (params) { Atari2HostIntPtr(size, params, tmp); params = tmp; } \
	fn.glPixelTransformParameterivEXT(target, pname, params)
#else
#define FN_GLPIXELTRANSFORMPARAMETERIVEXT(target, pname, params) \
	fn.glPixelTransformParameterivEXT(target, pname, params)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_EXT_vertex_array
 */
#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLARRAYELEMENTEXT(i) nfglArrayElementHelper(i)
#else
#define FN_GLARRAYELEMENTEXT(i) fn.glArrayElement(i)
#endif

#define FN_GLCOLORPOINTEREXT(size, type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].color, size, type, stride, count, 0, pointer)

#define FN_GLDRAWARRAYSEXT(mode, first, count) \
	convertClientArrays(first + count); \
	fn.glDrawArraysEXT(mode, first, count)

#define FN_GLEDGEFLAGPOINTEREXT(stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].edgeflag, 1, GL_UNSIGNED_BYTE, stride, count, 0, pointer)

#define FN_GLGETPOINTERVEXT(pname, data) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	gl_get_pointer(pname, texunit, data)

#define FN_GLINDEXPOINTEREXT(type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].index, 1, type, stride, count, 0, pointer)

#define FN_GLNORMALPOINTEREXT(type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].normal, 3, type, stride, count, 0, pointer)

#define FN_GLTEXCOORDPOINTEREXT(size, type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].texcoord, size, type, stride, count, 0, pointer)

#define FN_GLVERTEXPOINTEREXT(size, type, stride, count, pointer) \
	GLint texunit = 0; \
	fn.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &texunit); \
	setupClientArray(texunit, contexts[cur_context].vertex, size, type, stride, count, 0, pointer)

/* -------------------------------------------------------------------------- */

/*
 * GL_NV_gpu_program4
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMLOCALPARAMETERI4IVNV(target, index, params) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glProgramLocalParameterI4ivNV(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERI4IVNV(target, index, params) \
	fn.glProgramLocalParameterI4ivNV(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMLOCALPARAMETERSI4IVNV(target, index, count, params) \
	GLint const size = 4 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glProgramLocalParametersI4ivNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERSI4IVNV(target, index, count, params) \
	fn.glProgramLocalParametersI4ivNV(target, index, count, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMLOCALPARAMETERI4UIVNV(target, index, params) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glProgramLocalParameterI4uivNV(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERI4UIVNV(target, index, params) \
	fn.glProgramLocalParameterI4uivNV(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMLOCALPARAMETERSI4UIVNV(target, index, count, params) \
	GLint const size = 4 * count; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glProgramLocalParametersI4uivNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETERSI4UIVNV(target, index, count, params) \
	fn.glProgramLocalParametersI4uivNV(target, index, count, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMENVPARAMETERI4IVNV(target, index, params) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glProgramEnvParameterI4ivNV(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERI4IVNV(target, index, params) \
	fn.glProgramEnvParameterI4ivNV(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMENVPARAMETERSI4IVNV(target, index, count, params) \
	GLint const size = 4 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glProgramEnvParametersI4ivNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERSI4IVNV(target, index, count, params) \
	fn.glProgramEnvParametersI4ivNV(target, index, count, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMENVPARAMETERI4UIVNV(target, index, params) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glProgramEnvParameterI4uivNV(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERI4UIVNV(target, index, params) \
	fn.glProgramEnvParameterI4uivNV(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMENVPARAMETERSI4UIVNV(target, index, count, params) \
	GLint const size = 4 * count; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, params, tmp); \
	fn.glProgramEnvParametersI4uivNV(target, index, count, tmp)
#else
#define FN_GLPROGRAMENVPARAMETERSI4UIVNV(target, index, count, params) \
	fn.glProgramEnvParametersI4uivNV(target, index, count, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMLOCALPARAMETERIIVNV(target, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramLocalParameterIivNV(target, index, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPROGRAMLOCALPARAMETERIIVNV(target, index, params) \
	fn.glGetProgramLocalParameterIivNV(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMLOCALPARAMETERIUIVNV(target, index, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetProgramLocalParameterIuivNV(target, index, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPROGRAMLOCALPARAMETERIUIVNV(target, index, params) \
	fn.glGetProgramLocalParameterIuivNV(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMENVPARAMETERIIVNV(target, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramEnvParameterIivNV(target, index, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPROGRAMENVPARAMETERIIVNV(target, index, params) \
	fn.glGetProgramEnvParameterIivNV(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMENVPARAMETERIUIVNV(target, index, params) \
	GLint const size = 1; \
	GLuint tmp[size]; \
	fn.glGetProgramEnvParameterIuivNV(target, index, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPROGRAMENVPARAMETERIUIVNV(target, index, params) \
	fn.glGetProgramEnvParameterIuivNV(target, index, params)
#endif

/* -------------------------------------------------------------------------- */

/*
 * GL_ARB_fragment_program
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMSARB(n, programs) \
	GLuint *tmp; \
	if (n && programs) { \
		tmp = (GLuint *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, programs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDeleteProgramsARB(n, tmp); \
	free(tmp)
#else
#define FN_GLDELETEPROGRAMSARB(n, programs) \
	fn.glDeleteProgramsARB(n, programs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPROGRAMSARB(n, programs) \
	fn.glGenProgramsARB(n, programs); \
	if (n && programs) { \
		Atari2HostIntPtr(n, programs, programs); \
	}
#else
#define FN_GLGENPROGRAMSARB(n, programs) \
	fn.glGenProgramsARB(n, programs)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMENVPARAMETER4FVARB(target, index, params) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, params, tmp); \
	fn.glProgramEnvParameter4fvARB(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETER4FVARB(target, index, params) \
	fn.glProgramEnvParameter4fvARB(target, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMENVPARAMETER4DVARB(target, index, params) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, params, tmp); \
	fn.glProgramEnvParameter4dvARB(target, index, tmp)
#else
#define FN_GLPROGRAMENVPARAMETER4DVARB(target, index, params) \
	fn.glProgramEnvParameter4dvARB(target, index, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMLOCALPARAMETER4FVARB(target, index, params) \
	GLfloat tmp[4]; \
	Atari2HostFloatArray(4, params, tmp); \
	fn.glProgramLocalParameter4fvARB(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETER4FVARB(target, index, params) \
	fn.glProgramLocalParameter4fvARB(target, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMLOCALPARAMETER4DVARB(target, index, params) \
	GLdouble tmp[4]; \
	Atari2HostDoubleArray(4, params, tmp); \
	fn.glProgramLocalParameter4dvARB(target, index, tmp)
#else
#define FN_GLPROGRAMLOCALPARAMETER4DVARB(target, index, params) \
	fn.glProgramLocalParameter4dvARB(target, index, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPROGRAMENVPARAMETERFVARB(target, index, params) \
	GLfloat tmp[4]; \
	fn.glGetProgramEnvParameterfvARB(target, index, tmp); \
	Host2AtariFloatArray(4, tmp, params)
#else
#define FN_GLGETPROGRAMENVPARAMETERFVARB(target, index, params) \
	fn.glGetProgramEnvParameterfvARB(target, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETPROGRAMENVPARAMETERDVARB(target, index, params) \
	GLdouble tmp[4]; \
	fn.glGetProgramEnvParameterdvARB(target, index, tmp); \
	Host2AtariDoubleArray(4, tmp, params)
#else
#define FN_GLGETPROGRAMENVPARAMETERDVARB(target, index, params) \
	fn.glGetProgramEnvParameterdvARB(target, index, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPROGRAMLOCALPARAMETERFVARB(target, index, params) \
	GLfloat tmp[4]; \
	fn.glGetProgramLocalParameterfvARB(target, index, tmp); \
	Host2AtariFloatArray(4, tmp, params)
#else
#define FN_GLGETPROGRAMLOCALPARAMETERFVARB(target, index, params) \
	fn.glGetProgramLocalParameterfvARB(target, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETPROGRAMLOCALPARAMETERDVARB(target, index, params) \
	GLdouble tmp[4]; \
	fn.glGetProgramLocalParameterdvARB(target, index, tmp); \
	Host2AtariDoubleArray(4, tmp, params)
#else
#define FN_GLGETPROGRAMLOCALPARAMETERDVARB(target, index, params) \
	fn.glGetProgramLocalParameterdvARB(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMIVARB(target, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramivARB(target, index, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPROGRAMIVARB(target, index, params) \
	fn.glGetProgramivARB(target, index, params)
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 2.0
 */
#if NFOSMESA_NEED_INT_CONV
#define FN_GLDRAWBUFFERS(n, bufs) \
	GLenum *tmp; \
	if (n && bufs) { \
		tmp = (GLenum *)malloc(n * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(n, bufs, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDrawBuffers(n, tmp); \
	free(tmp)
#else
#define FN_GLDRAWBUFFERS(n, bufs) \
	fn.glDrawBuffers(n, bufs)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEATTRIB(program, index, bufSize, length, size, type, name) \
	GLsizei l; \
	GLint s; \
	GLenum t; \
	fn.glGetActiveAttrib(program, index, bufSize, &l, &s, &t, name); \
	if (length) *length = SDL_SwapBE32(l); \
	if (size) *size = SDL_SwapBE32(s); \
	if (type) *type = SDL_SwapBE32(t)
#else
#define FN_GLGETACTIVEATTRIB(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveAttrib(program, index, bufSize, length, size, type, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEUNIFORM(program, index, bufSize, length, size, type, name) \
	GLsizei l; \
	GLint s; \
	GLenum t; \
	fn.glGetActiveUniform(program, index, bufSize, &l, &s, &t, name); \
	if (length) *length = SDL_SwapBE32(l); \
	if (size) *size = SDL_SwapBE32(s); \
	if (type) *type = SDL_SwapBE32(t)
#else
#define FN_GLGETACTIVEUNIFORM(program, index, bufSize, length, size, type, name) \
	fn.glGetActiveUniform(program, index, bufSize, length, size, type, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETATTACHEDSHADERS(program, maxCount, count, obj) \
	GLsizei size = 0; \
	GLuint *tmp = (GLuint *)malloc(maxCount * sizeof(*tmp)); \
	if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
	fn.glGetAttachedShaders(program, maxCount, &size, tmp); \
	if (count) Host2AtariIntPtr(1, &size, count); \
	Host2AtariIntPtr(size, tmp, obj); \
	free(tmp)
#else
#define FN_GLGETATTACHEDSHADERS(program, maxCount, count, obj) \
	fn.glGetAttachedShaders(program, maxCount, count, obj)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM1FV(location, count, value)	\
	GLint const size = 1 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform1fv(location, count, tmp)
#else
#define FN_GLUNIFORM1FV(location, count, value) \
	fn.glUniform1fv(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM2FV(location, count, value) \
	GLint const size = 2 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform2fv(location, count, tmp)
#else
#define FN_GLUNIFORM2FV(location, count, value) \
	fn.glUniform2fv(location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORM4FV(location, count, value) \
	GLint const size = 4 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniform4fv(location, count, tmp)
#else
#define FN_GLUNIFORM4FV(location, count, value) \
	fn.glUniform4fv(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM1IV(location, count, value) \
	GLint const size = 1 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glUniform1iv(location, count, tmp)
#else
#define FN_GLUNIFORM1IV(location, count, value) \
	fn.glUniform1iv(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM2IV(location, count, value) \
	GLint const size = 2 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glUniform2iv(location, count, tmp)
#else
#define FN_GLUNIFORM2IV(location, count, value) \
	fn.glUniform2iv(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM3IV(location, count, value) \
	GLint const size = 3 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glUniform3iv(location, count, tmp)
#else
#define FN_GLUNIFORM3IV(location, count, value) \
	fn.glUniform3iv(location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLUNIFORM4IV(location, count, value) \
	GLint const size = 4 * count; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glUniform4iv(location, count, tmp)
#else
#define FN_GLUNIFORM4IV(location, count, value) \
	fn.glUniform4iv(location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB1DV(index, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib1dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1DV(index, v)	fn.glVertexAttrib1dv(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB1FV(index, v) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib1fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1FV(index, v) fn.glVertexAttrib1fv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB1SV(index, v) \
	GLint const size = 1; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib1sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB1SV(index, v) fn.glVertexAttrib1sv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB2DV(index, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib2dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2DV(index, v)	fn.glVertexAttrib2dv(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB2FV(index, v) \
	GLint const size = 2; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib2fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2FV(index, v) fn.glVertexAttrib2fv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB2SV(index, v) \
	GLint const size = 2; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib2sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB2SV(index, v) fn.glVertexAttrib2sv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB3DV(index, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib3dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3DV(index, v) fn.glVertexAttrib3dv(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB3FV(index, v) \
	GLint const size = 3; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib3fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3FV(index, v) fn.glVertexAttrib3fv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB3SV(index, v) \
	GLint const size = 3; \
	GLshort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib3sv(index, tmp)
#else
#define FN_GLVERTEXATTRIB3SV(index, v) fn.glVertexAttrib3sv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NIV(index, v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertexAttrib4Niv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NIV(index, v) fn.glVertexAttrib4Niv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4NUIV(index, v) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertexAttrib4Nuiv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4NUIV(index, v) fn.glVertexAttrib4Nuiv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIB4DV(index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttrib4dv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4DV(index, v) fn.glVertexAttrib4dv(index, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVERTEXATTRIB4FV(index, v) \
	GLint const size = 4; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, v, tmp); \
	fn.glVertexAttrib4fv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4FV(index, v) fn.glVertexAttrib4fv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4IV(index, v) \
	GLint const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertexAttrib4iv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4IV(index, v) fn.glVertexAttrib4iv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4UIV(index, v) \
	GLint const size = 4; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glVertexAttrib4uiv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4UIV(index, v) fn.glVertexAttrib4uiv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLVERTEXATTRIB4USV(index, v) \
	GLint const size = 4; \
	GLushort tmp[size]; \
	Atari2HostShortPtr(size, v, tmp); \
	fn.glVertexAttrib4usv(index, tmp)
#else
#define FN_GLVERTEXATTRIB4USV(index, v) fn.glVertexAttrib4usv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMIV(target, index, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramiv(target, index, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPROGRAMIV(target, index, params) \
	fn.glGetProgramiv(target, index, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMINFOLOG(program, bufSize, length, infoLog) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetProgramInfoLog(program, bufSize, tmp, infoLog); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETPROGRAMINFOLOG(program, bufSize, length, infoLog) \
	fn.glGetProgramInfoLog(program, bufSize, length, infoLog)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERIV(shader, pname, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetShaderiv(shader, pname, params); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETSHADERIV(shader, pname, params) \
	fn.glGetShaderiv(shader, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERINFOLOG(program, bufSize, length, infoLog) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetShaderInfoLog(program, bufSize, tmp, infoLog); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETSHADERINFOLOG(program, bufSize, length, infoLog) \
	fn.glGetShaderInfoLog(program, bufSize, length, infoLog)
#endif

#define FN_GLSHADERSOURCE(shader, count, strings, length) \
	GLchar **pstrings; \
	GLint *plength = NULL; \
	if (strings && count > 0) { \
		pstrings = (GLchar **)malloc(count * sizeof(*pstrings)); \
		if (!pstrings) { glSetError(GL_OUT_OF_MEMORY); return; } \
		if (length) { \
			plength = (GLint *)malloc(count * sizeof(*plength)); \
			if (!plength) { glSetError(GL_OUT_OF_MEMORY); return; } \
		} \
		const Uint32 *p = (const Uint32 *)strings; \
		for (GLsizei i = 0; i < count; i++) \
			pstrings[i] = p[i] ? (GLchar *)Atari2HostAddr(SDL_SwapBE32(p[i])) : NULL; \
		if (length) Atari2HostIntPtr(count, length, plength); \
	} else { \
		pstrings = NULL; \
	} \
	fn.glShaderSource(shader, count, (const GLchar **)pstrings, plength); \
	free(plength); \
	free(pstrings)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERSOURCE(program, maxLength, length, source) \
	GLint const size = 1; \
	GLsizei tmp[size]; \
	fn.glGetShaderSource(program, maxLength, tmp, source); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETSHADERSOURCE(program, maxLength, length, source) \
	fn.glGetShaderSource(program, maxLength, length, source)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETUNIFORMFV(program, location, params) \
	GLint const size = 1; \
	GLfloat tmp[size]; \
	fn.glGetUniformfv(program, location, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETUNIFORMFV(program, location, params) \
	fn.glGetUniformfv(program, location, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETUNIFORMIV(program, location, params) \
	GLint const size = 1; \
	GLint tmp[size]; \
	fn.glGetUniformiv(program, location, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETUNIFORMIV(program, location, params) \
	fn.glGetUniformiv(program, location, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETVERTEXATTRIBDV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[size]; \
	fn.glGetVertexAttribdv(index, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBDV(index, pname, params) \
	fn.glGetVertexAttribdv(index, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETVERTEXATTRIBFV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLfloat tmp[size]; \
	fn.glGetVertexAttribfv(index, pname, tmp); \
	Host2AtariFloatArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBFV(index, pname, params) \
	fn.glGetVertexAttribfv(index, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETVERTEXATTRIBIV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetVertexAttribiv(index, pname, tmp); \
	Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBIV(index, pname, params) \
	fn.glGetVertexAttribiv(index, pname, params)
#endif

/* TODO */
#define FN_GLGETVERTEXATTRIBPOINTERV(index, pname, pointer) \
	fn.glGetVertexAttribPointerv(index, pname, pointer)

/* TODO */
#define FN_GLVERTEXATTRIBPOINTER(index, size, type, normalized, stride, pointer) \
	fn.glVertexAttribPointer(index, size, type, normalized, stride, pointer)

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX2FV(location, count, transpose, value) \
	GLint const size = 4 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix2fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX2FV(location, count, transpose, value) \
	fn.glUniformMatrix2fv(location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX3FV(location, count, transpose, value) \
	GLint const size = 9 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix3fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX3FV(location, count, transpose, value) \
	fn.glUniformMatrix3fv(location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLUNIFORMMATRIX4FV(location, count, transpose, value) \
	GLint const size = 16 * count; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glUniformMatrix4fv(location, count, transpose, tmp)
#else
#define FN_GLUNIFORMMATRIX4FV(location, count, transpose, value) \
	fn.glUniformMatrix4fv(location, count, transpose, value)
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 4.1
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSHADERBINARY(count, shaders, binaryformat, binary, length) \
	if (count <= 0 || !shaders) return; \
	GLuint tmp[count]; \
	Atari2HostIntPtr(count, shaders, tmp); \
	fn.glShaderBinary(count, tmp, binaryformat, binary, length)
#else
#define FN_GLSHADERBINARY(count, shaders, binaryformat, binary, length) \
	fn.glShaderBinary(count, shaders, binaryformat, binary, length)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETSHADERPRECISIONFORMAT(shadertype, precisiontype, range, precision) \
	GLint const size = 1; \
	GLint r = 0, p = 0; \
	fn.glGetShaderPrecisionFormat(shadertype, precisiontype, &r, &p); \
	if (range) Host2AtariIntPtr(size, &r, range); \
	if (precision) Host2AtariIntPtr(size, &p, precision)
#else
#define FN_GLGETSHADERPRECISIONFORMAT(shadertype, precisiontype, range, precision) \
	fn.glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMBINARY(program, bufSize, length, binaryFormat, binary) \
	GLint const size = 1; \
	GLsizei l = 0; \
	GLenum format = 0; \
	fn.glGetProgramBinary(program, bufSize, &l, &format, binary); \
	if (length) Host2AtariIntPtr(size, &l, length); \
	if (binaryFormat) Host2AtariIntPtr(size, &format, binaryFormat)
#else
#define FN_GLGETPROGRAMBINARY(program, bufSize, length, binaryFormat, binary) \
	fn.glGetProgramBinary(program, bufSize, length, binaryFormat, binary)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMBINARY(program, binaryformat, binary, length) \
	fn.glProgramBinary(program, binaryformat, binary, length)
#else
#define FN_GLPROGRAMBINARY(program, binaryformat, binary, length) \
	fn.glProgramBinary(program, binaryformat, binary, length)
#endif

#define FN_glCreateShaderProgramv(type, count, strings) \
	GLchar **pstrings; \
	if (strings && count > 0) { \
		pstrings = (GLchar **)malloc(count * sizeof(*pstrings)); \
		if (!pstrings) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *p = (const Uint32 *)strings; \
		for (GLsizei i = 0; i < count; i++) \
			pstrings[i] = p[i] ? (GLchar *)Atari2HostAddr(SDL_SwapBE32(p[i])) : NULL; \
	} else { \
		pstrings = NULL; \
	} \
	GLuint program = fn.glCreateShaderProgramv(type, count, pstrings); \
	free(pstrings); \
	return program

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDELETEPROGRAMPIPELINES(n, pipelines) \
	GLsizei const size = n; \
	if (size <= 0 || !pipelines) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, pipelines, tmp); \
	fn.glDeleteProgramPipelines(size, tmp)
#else
#define FN_GLDELETEPROGRAMPIPELINES(n, pipelines) \
	fn.glDeleteProgramPipelines(n, pipelines)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGENPROGRAMPIPELINES(n, pipelines) \
	GLsizei const size = n; \
	if (size <= 0 || !pipelines) return; \
	GLuint tmp[size]; \
	fn.glGenProgramPipelines(size, tmp); \
	Host2AtariIntPtr(size, tmp, pipelines)
#else
#define FN_GLGENPROGRAMPIPELINES(n, pipelines) \
	fn.glGenProgramPipelines(n, pipelines)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMPIPELINEIV(pipeline, pname, params) \
	GLsizei const size = nfglGetNumParams(pname); \
	GLint tmp[size]; \
	fn.glGetProgramPipelineiv(pipeline, pname, tmp); \
	if (params) Host2AtariIntPtr(size, tmp, params)
#else
#define FN_GLGETPROGRAMPIPELINEIV(pipeline, pname, params) \
	fn.glGetProgramPipelineiv(pipeline, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1IV(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform1iv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1IV(program, location, count, value) \
	fn.glProgramUniform1iv(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM1FV(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform1fv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1FV(program, location, count, value) \
	fn.glProgramUniform1fv(program, location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM1DV(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform1dv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1DV(program, location, count, value) \
	fn.glProgramUniform1dv(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM1UIV(program, location, count, value) \
	GLint const size = 1 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform1uiv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM1UIV(program, location, count, value) \
	fn.glProgramUniform1uiv(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2IV(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform2iv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2IV(program, location, count, value) \
	fn.glProgramUniform2iv(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM2FV(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform2fv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2FV(program, location, count, value) \
	fn.glProgramUniform2fv(program, location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM2DV(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform2dv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2DV(program, location, count, value) \
	fn.glProgramUniform2dv(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM2UIV(program, location, count, value) \
	GLint const size = 2 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform2uiv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM2UIV(program, location, count, value) \
	fn.glProgramUniform2uiv(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3IV(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform3iv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3IV(program, location, count, value) \
	fn.glProgramUniform3iv(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM3FV(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform3fv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3FV(program, location, count, value) \
	fn.glProgramUniform3fv(program, location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM3DV(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform3dv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3DV(program, location, count, value) \
	fn.glProgramUniform3dv(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM3UIV(program, location, count, value) \
	GLint const size = 3 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform3uiv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM3UIV(program, location, count, value) \
	fn.glProgramUniform3uiv(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4IV(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform4iv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4IV(program, location, count, value) \
	fn.glProgramUniform4iv(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORM4FV(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniform4fv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4FV(program, location, count, value) \
	fn.glProgramUniform4fv(program, location, count, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORM4DV(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniform4dv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4DV(program, location, count, value) \
	fn.glProgramUniform4dv(program, location, count, value)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLPROGRAMUNIFORM4UIV(program, location, count, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLuint tmp[size]; \
	Atari2HostIntPtr(size, value, tmp); \
	fn.glProgramUniform4uiv(program, location, count, tmp)
#else
#define FN_GLPROGRAMUNIFORM4UIV(program, location, count, value) \
	fn.glProgramUniform4uiv(program, location, count, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2FV(program, location, count, transpose, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3FV(program, location, count, transpose, value) \
	GLint const size = 9 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4FV(program, location, count, transpose, value) \
	GLint const size = 16 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2DV(program, location, count, transpose, value) \
	GLint const size = 4 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3DV(program, location, count, transpose, value) \
	GLint const size = 9 * count; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4DV(program, location, count, transpose, value) \
	GLint const size = 16 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X3FV(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x3fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X3FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x3fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X2FV(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x2fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X2FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x2fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X4FV(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x4fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X4FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x4fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X2FV(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x2fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X2FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x2fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X4FV(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x4fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X4FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x4fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X3FV(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	Atari2HostFloatArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x3fv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X3FV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x3fv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X3DV(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x3dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X3DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x3dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X2DV(program, location, count, transpose, value) \
	GLint const size = 6 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x2dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X2DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x2dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX2X4DV(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix2x4dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX2X4DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix2x4dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X2DV(program, location, count, transpose, value) \
	GLint const size = 8 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x2dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X2DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x2dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX3X4DV(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix3x4dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX3X4DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix3x4dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLPROGRAMUNIFORMMATRIX4X3DV(program, location, count, transpose, value) \
	GLint const size = 12 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, value, tmp); \
	fn.glProgramUniformMatrix4x3dv(program, location, count, transpose, tmp)
#else
#define FN_GLPROGRAMUNIFORMMATRIX4X3DV(program, location, count, transpose, value) \
	fn.glProgramUniformMatrix4x3dv(program, location, count, transpose, value)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETFLOATI_V(pname, index, params) \
	if (params) { \
		int n = nfglGetNumParams(pname); \
		GLfloat tmp[MAX(n, 16)]; \
		fn.glGetFloati_v(pname, index, tmp); \
		Host2AtariFloatArray(n, tmp, params); \
	} else { \
		fn.glGetFloati_v(pname, index, params); \
	}
#else
#define FN_GLGETFLOATI_V(pname, index, params) \
	fn.glGetFloati_v(pname, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETDOUBLEI_V(pname, index, params) \
	int n = nfglGetNumParams(pname); \
	GLdouble tmp[MAX(n, 16)]; \
	fn.glGetDoublei_v(pname, index, tmp); \
	Host2AtariDoubleArray(n, tmp, params)
#else
#define FN_GLGETDOUBLEI_V(pname, index, params) \
	fn.glGetDoublei_v(pname, index, params)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL1DV(index, v) \
	GLint const size = 1; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL1dv(index, tmp)
#else
#define FN_GLVERTEXATTRIBL1DV(index, v) \
	fn.glVertexAttribL1dv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL2DV(index, v) \
	GLint const size = 2; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL2dv(index, tmp)
#else
#define FN_GLVERTEXATTRIBL2DV(index, v) \
	fn.glVertexAttribL2dv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL3DV(index, v) \
	GLint const size = 3; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL3dv(index, tmp)
#else
#define FN_GLVERTEXATTRIBL3DV(index, v) \
	fn.glVertexAttribL3dv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLVERTEXATTRIBL4DV(index, v) \
	GLint const size = 4; \
	GLdouble tmp[size]; \
	Atari2HostDoubleArray(size, v, tmp); \
	fn.glVertexAttribL4dv(index, tmp)
#else
#define FN_GLVERTEXATTRIBL4DV(index, v) \
	fn.glVertexAttribL4dv(index, v)
#endif

/* TODO */
#define FN_GLVERTEXATTRIBLPOINTER(index, size, type, stride, pointer) \
	fn.glVertexAttribLPointer(index, size, type, stride, pointer)

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETVERTEXATTRIBLDV(index, pname, params) \
	GLint const size = nfglGetNumParams(pname); \
	GLdouble tmp[size]; \
	fn.glGetVertexAttribLdv(index, pname, tmp); \
	Host2AtariDoubleArray(size, tmp, params)
#else
#define FN_GLGETVERTEXATTRIBLDV(index, pname, params) \
	fn.glGetVertexAttribLdv(index, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMPIPELINEINFOLOG(pipeline, bufSize, length, infoLog) \
	GLsizei const size = 1; \
	GLsizei tmp[size] = { 0 }; \
	fn.glGetProgramPipelineInfoLog(pipeline, bufSize, tmp, infoLog); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETPROGRAMPIPELINEINFOLOG(pipeline, bufSize, length, infoLog) \
	fn.glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVIEWPORTARRAYV(first, count, v) \
	GLsizei const size = 4 * count; \
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	if (v) { Atari2HostFloatArray(size, v, tmp); v = tmp; } \
	fn.glViewportArrayv(first, count, v)
#else
#define FN_GLVIEWPORTARRAYV(first, count, v) \
	fn.glViewportArrayv(first, count, v)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLVIEWPORTINDEXEDFV(index, v) \
	GLsizei const size = 4; \
	GLfloat tmp[size]; \
	if (v) { Atari2HostFloatArray(size, v, tmp); v = tmp; } \
	fn.glViewportIndexedfv(index, v)
#else
#define FN_GLVIEWPORTINDEXEDFV(index, v) \
	fn.glViewportIndexedfv(index, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSCISSORARRAYV(first, count, v) \
	GLsizei const size = 4 * count; \
	if (size <= 0) return; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glScissorArrayv(first, count, tmp)
#else
#define FN_GLSCISSORARRAYV(first, count, v) \
	fn.glScissorArrayv(first, count, v)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLSCISSORINDEXEDV(index, v) \
	GLsizei const size = 4; \
	GLint tmp[size]; \
	Atari2HostIntPtr(size, v, tmp); \
	fn.glScissorIndexedv(index, tmp)
#else
#define FN_GLSCISSORINDEXEDV(index, v) \
	fn.glScissorIndexedv(index, v)
#endif

#if NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLDEPTHRANGEARRAYV(first, count, v) \
	GLsizei const size = 2 * count; \
	if (size <= 0) return; \
	GLdouble tmp[size]; \
	if (v) { Atari2HostDoubleArray(size, v, tmp); v = tmp; } \
	fn.glDepthRangeArrayv(first, count, v)
#else
#define FN_GLDEPTHRANGEARRAYV(first, count, v) \
	fn.glDepthRangeArrayv(first, count, v)
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 4.2
 */

#define FN_GLDRAWELEMENTSINSTANCEDBASEINSTANCE(mode, count, type, indices, instancecount, baseinstance) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseInstance(mode, count, type, tmp, instancecount, baseinstance); \
	if (tmp != indices) free(tmp)

#define FN_GLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE(mode, count, type, indices, instancecount, basevertex, baseinstance) \
	void *tmp; \
	convertClientArrays(count); \
	switch(type) { \
	case GL_UNSIGNED_BYTE: \
	case GL_UNSIGNED_SHORT: \
	case GL_UNSIGNED_INT: \
		tmp = convertArray(count, type, indices); \
		break; \
	default: \
		glSetError(GL_INVALID_ENUM); \
		return; \
	} \
	fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, tmp, instancecount, basevertex, baseinstance); \
	if (tmp != indices) free(tmp)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTERNALFORMATIV(target, internalformat, pname, bufSize, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[size]; \
		fn.glGetInternalformativ(target, internalformat, pname, bufSize, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETINTERNALFORMATIV(target, internalformat, pname, bufSize, params) \
	fn.glGetInternalformativ(target, internalformat, pname, bufSize, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETACTIVEATOMICCOUNTERBUFFERIV(program, bufferIndex, pname, params) \
	fn.glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, params); \
	GLint n; \
	if (pname == GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES) { \
		n = 0; \
		fn.glGetActiveAtomicCounterBufferiv(program, bufferIndex, GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS, &n); \
	} else { \
		n = 1; \
	} \
	if (n && params) { \
		Host2AtariIntPtr(n, params, params); \
	}
#else
#define FN_GLGETACTIVEATOMICCOUNTERBUFFERIV(program, bufferIndex, pname, params) \
	fn.glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, params)
#endif

/* -------------------------------------------------------------------------- */

/*
 * Version 4.3
 */

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETFRAMEBUFFERPARAMETERIV(target, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetFramebufferParameteriv(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETFRAMEBUFFERPARAMETERIV(target, pname, params) \
	fn.glGetFramebufferParameteriv(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETDEBUGMESSAGELOG(count, bufSize, sources, types, ids, severities, lengths, messageLog) \
	GLenum *psources; \
	GLenum *ptypes; \
	GLuint *pids; \
	GLenum *pseverities; \
	GLsizei *plengths; \
	if (sources ) { \
		psources = (GLenum *)malloc(count * sizeof(*psources)); \
		if (psources == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		psources = NULL; \
	} \
	if (types) { \
		ptypes = (GLenum *)malloc(count * sizeof(*ptypes)); \
		if (ptypes == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		ptypes = NULL; \
	} \
	if (ids) { \
		pids = (GLuint *)malloc(count * sizeof(*pids)); \
		if (pids == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pids = NULL; \
	} \
	if (severities) { \
		pseverities = (GLenum *)malloc(count * sizeof(*pseverities)); \
		if (pseverities == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		pseverities = NULL; \
	} \
	if (lengths) { \
		plengths = (GLsizei *)malloc(count * sizeof(*plengths)); \
		if (plengths == NULL) { glSetError(GL_OUT_OF_MEMORY); return 0; } \
	} else { \
		plengths = NULL; \
	} \
	count = fn.glGetDebugMessageLog(count, bufSize, psources, ptypes, pids, pseverities, plengths, messageLog); \
	if (psources) { Host2AtariIntPtr(count, psources, sources); free(psources); } \
	if (ptypes) { Host2AtariIntPtr(count, ptypes, types); free(ptypes); } \
	if (pids) { Host2AtariIntPtr(count, pids, ids); free(pids); } \
	if (pseverities) { Host2AtariIntPtr(count, pseverities, severities); free(pseverities); } \
	if (plengths) { Host2AtariIntPtr(count, plengths, lengths); free(plengths); } \
	return count
#else
#define FN_GLGETDEBUGMESSAGELOG(count, bufSize, sources, types, ids, severities, lengths, messageLog) \
	return fn.glGetDebugMessageLog(count, bufSize, sources, types, ids, severities, lengths, messageLog)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLDEBUGMESSAGECONTROL(source, type, severity, count, ids, enabled) \
	GLuint *tmp; \
	if (ids && count) { \
		tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntPtr(count, ids, tmp); \
	} else { \
		tmp = NULL; \
	} \
	fn.glDebugMessageControl(source, type, severity, count, tmp, enabled); \
	free(tmp)
#else
#define FN_GLDEBUGMESSAGECONTROL(source, type, severity, count, ids, enabled) \
	fn.glDebugMessageControl(source, type, severity, count, ids, enabled)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOBJECTLABEL(identifier, name, bufSize, length, label) \
	GLint const size = 1;\
	GLsizei tmp[size]; \
	fn.glGetObjectLabel(identifier, name, bufSize, tmp, label); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETOBJECTLABEL(identifier, name, bufSize, length, label) \
	fn.glGetObjectLabel(identifier, name, bufSize, length, label)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLOBJECTPTRLABEL(identifier, length, label) \
	fn.glObjectPtrLabel(identifier, length, label)
#else
#define FN_GLOBJECTPTRLABEL(identifier, length, label) \
	fn.glObjectPtrLabel(identifier, length, label)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOBJECTPTRLABEL(identifier, bufSize, length, label) \
	GLint const size = 1;\
	GLsizei tmp[size]; \
	fn.glGetObjectPtrLabel(identifier, bufSize, tmp, label); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETOBJECTPTRLABEL(identifier, bufSize, length, label) \
	fn.glGetObjectPtrLabel(identifier, bufSize, length, label)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETINTERNALFORMATI64V(target, internalformat, pname, bufSize, params) \
	GLint const size = 1; \
	if (params) { \
		GLint64 tmp[size]; \
		fn.glGetInternalformati64v(target, internalformat, pname, bufSize, tmp); \
		Host2AtariInt64Ptr(size, tmp, params); \
	}
#else
#define FN_GLGETINTERNALFORMATI64V(target, internalformat, pname, bufSize, params) \
	fn.glGetInternalformati64v(target, internalformat, pname, bufSize, params)
#endif

/*
 * The parameters addressed by indirect are packed into a structure that takes the form (in C):
 *
 *  typedef  struct {
 *      uint  num_groups_x;
 *      uint  num_groups_y;
 *      uint  num_groups_z;
 *  } DispatchIndirectCommand;
 */
#define FN_GLDISPATCHCOMPUTEINDIRECT(indirect) \
	if (contexts[cur_context].buffer_bindings.dispatch_indirect.id) { \
		fn.glDispatchComputeIndirect(indirect); \
	} else { \
		glSetError(GL_INVALID_OPERATION); \
	}

#define FN_GLINVALIDATEBUFFERDATA(buffer) \
	gl_buffer_t *buf = gl_get_buffer(buffer); \
	if (buf) { \
		free(buf->atari_buffer); \
		buf->atari_buffer = NULL; \
		free(buf->host_buffer); \
		buf->host_buffer = NULL; \
		buf->size = 0; \
	}

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINVALIDATEFRAMEBUFFER(target, numAttachments, attachments) \
	GLint size = numAttachments; \
	if (size <= 0) return; \
	if (attachments) { \
		GLenum tmp[size]; \
		Atari2HostIntPtr(size, attachments, tmp); \
		fn.glInvalidateFramebuffer(target, numAttachments, tmp); \
	}
#else
#define FN_GLINVALIDATEFRAMEBUFFER(target, numAttachments, attachments) \
	fn.glInvalidateFramebuffer(target, numAttachments, attachments)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLINVALIDATESUBFRAMEBUFFER(target, numAttachments, attachments, x, y, width, height) \
	GLint size = numAttachments; \
	if (size <= 0) return; \
	if (attachments) { \
		GLenum tmp[size]; \
		Atari2HostIntPtr(size, attachments, tmp); \
		fn.glInvalidateSubFramebuffer(target, numAttachments, tmp, x, y, width, height); \
	}
#else
#define FN_GLINVALIDATESUBFRAMEBUFFER(target, numAttachments, attachments, x, y, width, height) \
	fn.glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height)
#endif

#define FN_GLMULTIDRAWARRAYSINDIRECT(mode, indirect, drawcount, stride) \
	/* \
	 * The parameters addressed by indirect are packed into a structure that takes the form (in C): \
	 * \
	 *    typedef  struct { \
	 *        uint  count; \
	 *        uint  primCount; \
	 *        uint  first; \
	 *        uint  baseInstance; \
	 *    } DrawArraysIndirectCommand; \
	 */ \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		indirect = Host2AtariAddr(indirect); \
		fn.glMultiDrawArraysIndirect(mode, indirect, drawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 4 * sizeof(Uint32); \
		for (GLsizei n = 0; n < drawcount; n++) { \
			GLuint tmp[4]; \
			Atari2HostIntPtr(4, (const GLuint *)indirect, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawArraysInstancedBaseInstance(mode, tmp[2], count, tmp[1], tmp[3]); \
			indirect = (const char *)indirect + stride; \
		} \
	}

/*
 * The parameters addressed by indirect are packed into a structure that takes the form (in C):
 *
 *    typedef  struct {
 *        uint  count;
 *        uint  primCount;
 *        uint  firstIndex;
 *        uint  baseVertex;
 *        uint  baseInstance;
 *    } DrawElemntsIndirectCommand;
 */
#define FN_GLMULTIDRAWELEMENTSINDIRECT(mode, type, indirect, drawcount, stride) \
	if (contexts[cur_context].buffer_bindings.draw_indirect.id) { \
		indirect = Host2AtariAddr(indirect); \
		fn.glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride); \
	} else if (indirect) { \
		if (stride == 0) stride = 5 * sizeof(Uint32); \
		for (GLsizei n = 0; n < drawcount; n++) { \
			GLuint tmp[5]; \
			Atari2HostIntPtr(5, (const GLuint *)indirect, tmp); \
			GLuint count = tmp[0]; \
			convertClientArrays(count); \
			fn.glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, (const void *)(uintptr_t)tmp[2], tmp[1], tmp[3], tmp[4]); \
			indirect = (const char *)indirect + stride; \
		} \
	}

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMINTERFACEIV(program, programInterface, pname, params) \
	GLint const size = 1; \
	if (params) { \
		GLint tmp[size]; \
		fn.glGetProgramInterfaceiv(program, programInterface, pname, params); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETPROGRAMINTERFACEIV(program, programInterface, pname, params) \
	fn.glGetProgramInterfaceiv(program, programInterface, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMRESOURCENAME(program, programInterface, index, bufSize, length, name) \
	GLint const size = 1; \
	GLsizei l = 0; \
	fn.glGetProgramResourceName(program, programInterface, index, bufSize, &l, name); \
	if (length) Host2AtariIntPtr(size, &l, length)
#else
#define FN_GLGETPROGRAMRESOURCENAME(program, programInterface, index, bufSize, length, name) \
	fn.glGetProgramResourceName(program, programInterface, index, bufSize, length, name)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPROGRAMRESOURCEIV(program, programInterface, index, propCount, props, bufSize, length, params) \
	GLint const size = propCount; \
	if (size <= 0) return; \
	GLenum tmp[size]; \
	GLsizei l = 0; \
	if (props) { Atari2HostIntPtr(size, props, tmp); props = tmp; } \
	fn.glGetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, &l, params); \
	if (length) Host2AtariIntPtr(1, &l, length); \
	if (params) Host2AtariIntPtr(l / sizeof(GLint), params, params)
#else
#define FN_GLGETPROGRAMRESOURCEIV(program, programInterface, index, propCount, props, bufSize, length, params) \
	fn.glGetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, length, params)
#endif

/* TODO */
#define FN_GLBINDVERTEXBUFFER(bindingindex, buffer, offset, stride) \
	fn.glBindVertexBuffer(bindingindex, buffer, offset, stride)

/* -------------------------------------------------------------------------- */

/*
 * Version 4.4
 */
/* nothing to do */
#define FN_GLBUFFERSTORAGE(target, size, data, flags) \
	fn.glBufferStorage(target, size, data, flags)

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLEARTEXIMAGE(texture, level, format, type, data) \
	/* \
	 * FIXME: we need the dimensions of the texture, \
	 * which are only avaiable through GL 4.5 glGetTextureParameter() \
	 */ \
	fn.glClearTexImage(texture, level, format, type, data)
#else
#define FN_GLCLEARTEXIMAGE(texture, level, format, type, data) \
	fn.glClearTexImage(texture, level, format, type, data)
#endif

#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV
#define FN_GLCLEARTEXSUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data) \
	void *tmp = convertPixels(width, height, depth, format, type, data); \
	fn.glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, tmp); \
	if (tmp != data) free(tmp)
#else
#define FN_GLCLEARTEXSUBIMAGE(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data) \
	fn.glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data)
#endif

#define FN_GLBINDBUFFERSBASE(target, first, count, buffers) \
	if (buffers) { \
		GLuint *tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, buffers, tmp); \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, tmp[i], first + i, 0); \
		fn.glBindBuffersBase(target, first, count, tmp); \
		free(tmp); \
	} else { \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, 0, first + i, 0); \
		fn.glBindBuffersBase(target, first, count, buffers); \
	}

#define FN_GLBINDBUFFERSRANGE(target, first, count, buffers, offsets, sizes) \
	GLuint *pbuffers; \
	GLintptr *poffsets; \
	GLsizeiptr *psizes; \
	if (buffers) { \
		pbuffers = (GLuint *)malloc(count * sizeof(*pbuffers)); \
		if (!pbuffers) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, buffers, pbuffers); \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, pbuffers[i], first + i, 0); \
	} else { \
		pbuffers = NULL; \
		for (GLsizei i = 0; i < count; i++) \
			gl_bind_buffer(target, 0, first + i, 0); \
	} \
	if (offsets) { \
		poffsets = (GLintptr *)malloc(count * sizeof(*poffsets)); \
		if (!poffsets) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *aoffsets = (const Uint32 *)offsets; \
		for (int i = 0; i < count; i++) \
			poffsets[i] = SDL_SwapBE32(aoffsets[i]); \
	} else { \
		poffsets = NULL; \
	} \
	if (sizes) { \
		psizes = (GLsizeiptr *)malloc(count * sizeof(*psizes)); \
		if (!psizes) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *asizes = (const Uint32 *)sizes; \
		for (int i = 0; i < count; i++) \
			psizes[i] = SDL_SwapBE32(asizes[i]); \
	} else { \
		psizes = NULL; \
	} \
	fn.glBindBuffersRange(target, first, count, pbuffers, poffsets, psizes); \
	free(psizes); \
	free(poffsets); \
	free(pbuffers)

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDTEXTURES(first, count, textures) \
	if (textures) { \
		GLuint *tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, textures, tmp); \
		fn.glBindTextures(first, count, tmp); \
		free(tmp); \
	} else { \
		fn.glBindTextures(first, count, textures); \
	}
#else
#define FN_GLBINDTEXTURES(first, count, samples) \
	fn.glBindTextures(first, count, textures)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDSAMPLERS(first, count, samplers) \
	if (samplers) { \
		GLuint *tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, samplers, tmp); \
		fn.glBindSamplers(first, count, tmp); \
		free(tmp); \
	} else { \
		fn.glBindSamplers(first, count, samplers); \
	}
#else
#define FN_GLBINDSAMPLERS(first, count, samples) \
	fn.glBindSamplers(first, count, samplers)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLBINDIMAGETEXTURES(first, count, textures) \
	if (textures) { \
		GLuint *tmp = (GLuint *)malloc(count * sizeof(*tmp)); \
		if (!tmp) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, textures, tmp); \
		fn.glBindImageTextures(first, count, tmp); \
		free(tmp); \
	} else { \
		fn.glBindImageTextures(first, count, textures); \
	}
#else
#define FN_GLBINDIMAGETEXTURES(first, count, textures) \
	fn.glBindImageTextures(first, count, textures)
#endif

#define FN_GLBINDVERTEXBUFFERS(first, count, buffers, offsets, sizes) \
	GLuint *pbuffers; \
	GLintptr *poffsets; \
	GLsizei *psizes; \
	if (buffers) { \
		pbuffers = (GLuint *)malloc(count * sizeof(*pbuffers)); \
		if (!pbuffers) { glSetError(GL_OUT_OF_MEMORY); return; } \
		Atari2HostIntArray(count, buffers, pbuffers); \
	} else { \
		pbuffers = NULL; \
	} \
	if (offsets) { \
		poffsets = (GLintptr *)malloc(count * sizeof(*poffsets)); \
		if (!poffsets) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *aoffsets = (const Uint32 *)offsets; \
		for (int i = 0; i < count; i++) \
			poffsets[i] = SDL_SwapBE32(aoffsets[i]); \
	} else { \
		poffsets = NULL; \
	} \
	if (sizes) { \
		psizes = (GLsizei *)malloc(count * sizeof(*psizes)); \
		if (!psizes) { glSetError(GL_OUT_OF_MEMORY); return; } \
		const Uint32 *asizes = (const Uint32 *)sizes; \
		for (int i = 0; i < count; i++) \
			psizes[i] = SDL_SwapBE32(asizes[i]); \
	} else { \
		psizes = NULL; \
	} \
	fn.glBindVertexBuffers(first, count, pbuffers, poffsets, psizes); \
	free(psizes); \
	free(poffsets); \
	free(pbuffers)

/* -------------------------------------------------------------------------- */

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMATERIALFV(face, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMaterialfv(face, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETMATERIALFV(face, pname, params) \
	fn.glGetMaterialfv(face, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMATERIALIV(face, pname, params) \
	GLint size = nfglGetNumParams(pname); \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetMaterialiv(face, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETMATERIALIV(face, pname, params) \
	fn.glGetMaterialiv(face, pname, params)
#endif


/* glGetMaterialxOES??? should be *params */

/*
 * If a non-zero named buffer object is bound to the GL_PIXEL_PACK_BUFFER
 * target (see glBindBuffer) while minimum and maximum pixel values are
 * requested, values is treated as a byte offset into the buffer object's
 * data store.     
 */
#if NFOSMESA_NEED_INT_CONV || NFOSMESA_NEED_FLOAT_CONV || NFOSMESA_NEED_DOUBLE_CONV
#define FN_GLGETMINMAX(target, reset, format, type, values) \
	GLsizei size, count; \
	GLint width = 2; \
	GLubyte result[4 * 2 * sizeof(GLdouble)]; \
	const void *src; \
	void *dst; \
	 \
	if (!pixelParams(format, type, size, count)) \
		return; \
	count *= width; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetMinmax(target, reset, format, type, values); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)values; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)values; \
	} else { \
		fn.glGetMinmax(target, reset, format, type, result); \
		src = result; \
		dst = values; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst)
#define FN_GLGETMINMAXEXT(target, reset, format, type, values) \
	GLsizei size, count; \
	GLint width = 2; \
	GLubyte result[4 * 2 * sizeof(GLdouble)]; \
	const void *src; \
	void *dst; \
	 \
	if (!pixelParams(format, type, size, count)) \
		return; \
	count *= width; \
	if (contexts[cur_context].buffer_bindings.pixel_pack.id) { \
		fn.glGetMinmaxEXT(target, reset, format, type, values); \
		src = (const char *)contexts[cur_context].buffer_bindings.pixel_pack.host_pointer + (uintptr_t)values; \
		dst = (char *)contexts[cur_context].buffer_bindings.pixel_pack.atari_pointer + (uintptr_t)values; \
	} else { \
		fn.glGetMinmaxEXT(target, reset, format, type, result); \
		src = result; \
		dst = values; \
	} \
	if (type == GL_FLOAT) \
		Host2AtariFloatArray(count, (const GLfloat *)src, (GLfloat *)dst); \
	else if (size == 2) \
		Host2AtariShortPtr(count, (const GLushort *)src, (Uint16 *)dst); \
	else /* if (size == 4) */ \
		Atari2HostIntArray(count, (const GLuint *)src, (Uint32 *)dst)
#else
#define FN_GLGETMINMAX(target, reset, format, type, values) \
	fn.glGetMinmax(target, reset, format, type, values)
#define FN_GLGETMINMAXEXT(target, reset, format, type, values) \
	fn.glGetMinmaxEXT(target, reset, format, type, values)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMINMAXPARAMETERFV(target, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMinmaxParameterfv(target, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#define FN_GLGETMINMAXPARAMETERFVEXT(target, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMinmaxParameterfvEXT(target, pname, tmp); \
		Host2AtariFloatArray(size, tmp, params); \
	}
#else
#define FN_GLGETMINMAXPARAMETERFV(target, pname, params) \
	fn.glGetMinmaxParameterfv(target, pname, params)
#define FN_GLGETMINMAXPARAMETERFVEXT(target, pname, params) \
	fn.glGetMinmaxParameterfvEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMINMAXPARAMETERIV(target, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetMinmaxParameteriv(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#define FN_GLGETMINMAXPARAMETERIVEXT(target, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetMinmaxParameterivEXT(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETMINMAXPARAMETERIV(target, pname, params) \
	fn.glGetMinmaxParameteriv(target, pname, params)
#define FN_GLGETMINMAXPARAMETERIVEXT(target, pname, params) \
	fn.glGetMinmaxParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETMULTISAMPLEFV(pname, index, val) \
	GLint size = 2; \
	if (val) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMultisamplefv(pname, index, tmp); \
		Host2AtariFloatArray(size, tmp, val); \
	}
#define FN_GLGETMULTISAMPLEFVNV(pname, index, val) \
	GLint size = 2; \
	if (val) { \
		GLfloat tmp[MAX(size, 16)]; \
		fn.glGetMultisamplefvNV(pname, index, tmp); \
		Host2AtariFloatArray(size, tmp, val); \
	}
#else
#define FN_GLGETMULTISAMPLEFV(pname, index, val) \
	fn.glGetMultisamplefv(pname, index, val)
#define FN_GLGETMULTISAMPLEFVNV(pname, index, val) \
	fn.glGetMultisamplefvNV(pname, index, val)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETMULTISAMPLEIVNV(pname, index, val) \
	GLint size = 2; \
	if (val) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetMultisampleivNV(pname, index, tmp); \
		Host2AtariIntPtr(size, tmp, val); \
	}
#else
#define FN_GLGETMULTISAMPLEIVNV(pname, index, val) \
	fn.glGetMultisampleivNV(pname, index, val)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETRENDERBUFFERPARAMETERIVEXT(target, pname, params) \
	GLint size = 1; \
	if (params) { \
		GLint tmp[MAX(size, 16)]; \
		fn.glGetRenderbufferParameterivEXT(target, pname, tmp); \
		Host2AtariIntPtr(size, tmp, params); \
	}
#else
#define FN_GLGETRENDERBUFFERPARAMETERIVEXT(target, pname, params) \
	fn.glGetRenderbufferParameterivEXT(target, pname, params)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETOBJECTLABELEXT(identifier, name, bufSize, length, label) \
	GLint const size = 1;\
	GLsizei tmp[size]; \
	fn.glGetObjectLabelEXT(identifier, name, bufSize, tmp, label); \
	if (length) Host2AtariIntPtr(size, tmp, length)
#else
#define FN_GLGETOBJECTLABELEXT(identifier, name, bufSize, length, label) \
	fn.glGetObjectLabelEXT(identifier, name, bufSize, length, label)
#endif

#if NFOSMESA_NEED_FLOAT_CONV
#define FN_GLGETPIXELMAPFV(map, values) \
	GLint const size = nfglPixelmapSize(map);\
	if (size <= 0) return; \
	GLfloat tmp[size]; \
	fn.glGetPixelMapfv(map, tmp); \
	if (values) Host2AtariFloatArray(size, tmp, values)
#else
#define FN_GLGETPIXELMAPFV(map, values) \
	fn.glGetPixelMapfv(map, values)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELMAPUIV(map, values) \
	GLint const size = nfglPixelmapSize(map);\
	if (size <= 0) return; \
	GLuint tmp[size]; \
	fn.glGetPixelMapuiv(map, tmp); \
	if (values) Host2AtariIntPtr(size, tmp, values)
#else
#define FN_GLGETPIXELMAPUIV(map, values) \
	fn.glGetPixelMapuiv(map, values)
#endif

#if NFOSMESA_NEED_INT_CONV
#define FN_GLGETPIXELMAPUSV(map, values) \
	GLint const size = nfglPixelmapSize(map);\
	if (size <= 0) return; \
	GLushort tmp[size]; \
	fn.glGetPixelMapusv(map, tmp); \
	if (values) Host2AtariShortPtr(size, tmp, values)
#else
#define FN_GLGETPIXELMAPUSV(map, values) \
	fn.glGetPixelMapusv(map, values)
#endif

#include "nfosmesa/call-gl.c"
