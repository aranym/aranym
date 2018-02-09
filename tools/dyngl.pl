#!/usr/bin/perl -w

#
# This script is used to parse GL header(s) and
# generate several output files from it.
#
# Output files are:
#
# -protos:
#     Extract the function prototypes only. The generated file
#     is used then as reference for generating the other files.
#
#     Usage: dyngl.pl -protos /usr/include/GL/gl.h /usr/include/GL/glext.h /usr/include/GL/osmesa.h > tools/glfuncs.h
#
#     This file MUST NOT automatically be recreated by make, for several reasons:
#        - The gl headers may be different from system to system. However, the NFOSMesa interface expect
#          a certain list of functions to be available.
#        - This script (and make) may not know where the headers are actually stored,
#          only the compiler knows.
#     When regenerating this file, check the following list:
#        - Keep a reference of the previous version
#        - Functions that are considered obsolete by OpenGL might have been removed from the headers.
#          In that case, since they must still be exported, they should be added to the %missing
#          hash below.
#        - Take a look at the arguments and return types of new functions.
#          - Integral types of 8-64 bits should be handled already. However,
#            if the promoted argument is > 32 bits, and declared by some typedef (like GLint64),
#            you may have to change gen_dispatch()
#          - Arguments that are pointers to some data may need big->little endian conversion
#            before passing it to the hosts OSMesa library. 
#          - Return types that are pointers usually dont work, because that
#            memory would not point to emulated Atari memory. Look at glGetString()
#            how this (sometimes) can be solved. Otherwise, if you decide not to
#            export such a function, add it to the %blacklist hash.
#          - add enums for new functions to atari/nfosmesa/enum-gl.h (at the end, dont reuse any existing numbers!)
#            This is not done automatically, because the numbers must not change for old functions.
#          - If new types are used in the prototypes, add them to atari/nfosmesa/gltypes.h
#
#     The current version (API Version 3) was generated from <GL/gl.h> on Linux,
#     glext.h from Khronos group (http://www.opengl.org/registry/api/GL/glext.h, Date 2018-01-14
#     and osmesa.h from Mesa 11.2.
#
# -macros:
#     Generate macro calls that are used to generate several tables.
#     The macro must be defined before including the generated file
#     and has the signature:
#        GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#     with
#        type: return type of function
#        gl: group the function belongs to, either gl, glu or OSMesa
#        name: name of function, with (gl|glu) prefix removed
#        export: exported name of function. Different only for some
#                functions which take float arguments that are exported from LDG
#                and conflict with OpenGL functions of the same name
#                that take double arguments.
#        upper: name of function in uppercase
#        proto: prototype of the function, including parenthesis
#        args: argument names of the function
#        first: address of the first argument
#        ret: code to return the function result, or GL_void_return
#     Optionally, OSMESA_PROC() can be defined, with the same
#     signature, to process OSMesa functions.
#     There is also a second set of macro calls GL_PROCM generated,
#     with the same signature, but replacing "memptr" for any argument
#     in the prototype that is a pointer type
#
#     Usage: dyngl.pl -macros tools/glfuncs.h > atari/nfosmesa/glfuncs.h
#
#  -calls:
#     Generate the function calls that are used to implement the NFOSMesa feature.
#
#     Usage: dyngl.pl -calls tools/glfuncs.h > src/natfeat/nfosmesa/call-gl.c
#
#  -dispatch:
#     Generate the case statements that are used to implement the NFOSMesa feature.
#
#     Usage: dyngl.pl -dispatch tools/glfuncs.h > src/natfeat/nfosmesa/dispatch-gl.c
#
#  -ldgheader:
#     Generate the atari header file needed to use osmesa.ldg.
#
#     Usage: dyngl.pl -ldgheader tools/glfuncs.h > atari/nfosmesa/ldg/osmesa.h
#
#  -ldgsource:
#     Generate the atari source file that dynamically loads the functions
#     from osmesa.ldg.
#
#     Usage: dyngl.pl -ldgsource tools/glfuncs.h > atari/nfosmesa/osmesa_load.c
#
#  -tinyldgheader:
#     Generate the atari header file needed to use tiny_gl.ldg.
#
#     Usage: dyngl.pl -tinyldgheader tools/glfuncs.h > atari/nfosmesa/ldg/tiny_gl.h
#
#  -tinyldgsource:
#     Generate the atari source file that dynamically loads the functions
#     from tiny_gl.ldg.
#
#     Usage: dyngl.pl -tinyldgsource tools/glfuncs.h > atari/nfosmesa/tinygl_load.c
#
#  -tinyldglink:
#     Generate the atari source file that exports the functions from tiny_gl.ldg.
#
#     Usage: dyngl.pl -tinyldglink tools/glfuncs.h > atari/nfosmesa/link-tinygl.h
#
#  -tinyslbheader:
#     Generate the atari header file needed to use tiny_gl.slb.
#
#     Usage: dyngl.pl -tinyslbheader tools/glfuncs.h > atari/nfosmesa/slb/tiny_gl.h
#
#  -tinyslbsource:
#     Generate the atari source file that dynamically loads the functions
#     from tiny_gl.slb.
#
#     Usage: dyngl.pl -tinyslbsource tools/glfuncs.h > atari/nfosmesa/tinygl_loadslb.c
#
#  -slbheader:
#     Generate the atari header file needed to use osmesa.slb.
#
#     Usage: dyngl.pl -slbheader tools/glfuncs.h > atari/nfosmesa/slb/osmesa.h
#
#  -slbsource:
#     Generate the atari source file that dynamically loads the functions
#     from osmesa.slb.
#
#     Usage: dyngl.pl -slbsource tools/glfuncs.h > atari/nfosmesa/osmesa_loadslb.c
#

use strict;
use Data::Dumper;


my $me = "dyngl.pl";

my %functions;
my $warnings = 0;
my $enumfile;
my $inc_gltypes = "atari/nfosmesa/gltypes.h";

#
# functions which are not yet implemented,
# because they would return a host address,
#
my %blacklist = (
	'glGetString' => 1,     # handled separately
	'glGetStringi' => 1,    # handled separately
	'glCreateSyncFromCLeventARB' => 1,
	'glDebugMessageCallback' => 1,
	'glDebugMessageCallbackAMD' => 1,
	'glDebugMessageCallbackARB' => 1,
	'glGetBufferSubData' => 1,
	'glGetBufferSubDataARB' => 1,
	'glGetNamedBufferSubDataEXT' => 1,
	'glGetNamedBufferSubData' => 1,
	'glMapBuffer' => 1,
	'glMapBufferARB' => 1,
	'glMapBufferRange' => 1,
	'glMapObjectBufferATI' => 1,
	'glMapNamedBufferEXT' => 1,
	'glMapNamedBufferRangeEXT' => 1,
	'glMapNamedBuffer' => 1,
	'glMapNamedBufferRange' => 1,
	'glMapTexture2DINTEL' => 1,
	'glImportSyncEXT' => 1,
	'glProgramCallbackMESA' => 1,
	'glTextureRangeAPPLE' => 1,
	'glGetTexParameterPointervAPPLE' => 1,
	'glInstrumentsBufferSGIX' => 1,
	# GL_NV_vdpau_interop
	'glVDPAUInitNV' => 1,
	'glVDPAUFiniNV' => 1,
	'glVDPAURegisterVideoSurfaceNV' => 1,
	'glVDPAURegisterOutputSurfaceNV' => 1,
	'glVDPAUIsSurfaceNV' => 1,
	'glVDPAUUnregisterSurfaceNV' => 1,
	'glVDPAUGetSurfaceivNV' => 1,
	'glVDPAUSurfaceAccessNV' => 1,
	'glVDPAUMapSurfacesNV' => 1,
	'glVDPAUUnmapSurfacesNV' => 1,
	# GL_NV_draw_vulkan_image
	'glGetVkProcAddrNV' => 1,
	# from GL_EXT_memory_object; could not figure out yet
	# how to get at the size of those objects
	'glGetUnsignedBytevEXT' => 1,
	'glGetUnsignedBytei_vEXT' => 1,
);

#
# typedefs from headers that are already pointer types
#
my %pointer_types = (
#	'GLsync' => 1, # declared as pointer type, but actually is a handle
	'GLDEBUGPROC' => 1,
	'GLDEBUGPROCARB' => 1,
	'GLDEBUGPROCAMD' => 1,
	'GLprogramcallbackMESA' => 1,
	'GLeglClientBufferEXT' => 1,
	'GLVULKANPROCNV' => 1,
);

#
# 8-bit types that dont have endian issues
#
my %byte_types = (
	'GLubyte' => 1,
	'const GLubyte' => 1,
	'GLchar' => 1,
	'const GLchar' => 1,
	'GLbyte' => 1,
	'const GLbyte' => 1,
	'char' => 1,
	'GLcharARB' => 1,
	'const GLcharARB' => 1,
	'GLboolean' => 1,
);

#
# integral types that need to be promoted to
# 32bit type when passed by value
#
my %short_types = (
	'GLshort' => 'GLshort32',
	'GLushort' => 'GLushort32',
	'GLboolean' => 'GLboolean32',
	'GLchar' => 'GLchar32',
	'char' => 'GLchar32',
	'GLubyte' => 'GLubyte32',
	'GLbyte' => 'GLbyte32',
	'GLhalfNV' => 'GLhalfNV32',
);

#
# >32bit types that need special handling
#
my %longlong_types = (
	'GLint64' => 1,
	'GLint64EXT' => 1,
	'GLuint64' => 1,
	'GLuint64EXT' => 1,
);
my %longlong_rettypes = (
	'GLint64' => 1,
	'GLint64EXT' => 1,
	'GLuint64' => 1,
	'GLuint64EXT' => 1,
	'GLintptr' => 1,
	'GLsizeiptr' => 1,
	'GLintptrARB' => 1,
	'GLsizeiptrARB' => 1,
);

#
# format specifiers to print the various types,
# only used for debug output
#
my %printf_formats = (
	'GLshort32' => '%d',
	'GLushort32' => '%u',
	'GLboolean32' => '%d',
	'GLchar32' => '%c',
	'GLubyte32' => '%u',
	'GLbyte32' => '%d',
	'GLhalfNV32' => '%u',
	'GLint' => '%d',
	'GLsizei' => '%d',
	'GLuint' => '%u',
	'GLenum' => '%s', # will be translated using gl_enum_name
	'GLbitfield' => '0x%x',
	'GLfloat' => '%f',
	'const GLfloat' => '%f',  # used in glClearNamedFramebufferfi
	'GLclampf' => '%f',
	'GLdouble' => '%f',
	'GLclampd' => '%f',
	'GLfixed' => '0x%x',
	'GLintptr' => '%" PRI_IPTR "',
	'GLintptrARB' => '%" PRI_IPTR "',
	'GLsizeiptr' => '%" PRI_IPTR "',
	'GLsizeiptrARB' => '%" PRI_IPTR "',
	'GLint64' => '%" PRId64 "',
	'GLint64EXT' => '%" PRId64 "',
	'GLuint64' => '%" PRIu64 "',
	'GLuint64EXT' => '%" PRIu64 "',
	'long' => '%ld',
	'GLhandleARB' => '%u',
	'GLsync' => '" PRI_PTR "',
	'GLvdpauSurfaceNV' => '" PRI_IPTR "',
	'GLDEBUGPROC' => '" PRI_PTR "',
	'GLDEBUGPROCAMD' => '" PRI_PTR "',
	'GLDEBUGPROCARB' => '" PRI_PTR "',
	'GLprogramcallbackMESA' => '" PRI_PTR "',
	'GLeglClientBufferEXT' => '" PRI_PTR "',
	'GLVULKANPROCNV' => '" PRI_PTR "',
	'OSMesaContext' => '%u',  # type is a pointer, but we return a handle instead
);


#
# emit header
#
sub print_header()
{
	my $files = join(' ', @ARGV);
	$files = '<stdin>' unless ($files ne "");
print << "EOF";
/*
 * -- DO NOT EDIT --
 * Generated by $me from $files
 */

EOF
}


sub warn($)
{
	my ($msg) = @_;
	print STDERR "$me: warning: $msg\n";
	++$warnings;
}

my %missing = (
	'glDeleteObjectBufferATI' => {
		'name' => 'DeleteObjectBufferATI',
		'gl' => 'gl',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLuint', 'name' => 'buffer', 'pointer' => 0 },
		],
	},
	'glGetActiveUniformBlockIndex' => {
		'name' => 'GetActiveUniformBlockIndex',
		'gl' => 'gl',
		'type' => 'GLuint',
		'params' => [
			{ 'type' => 'GLuint', 'name' => 'program', 'pointer' => 0 },
			{ 'type' => 'const GLchar', 'name' => 'uniformBlockName', 'pointer' => 1 },
		],
	},
	'glSamplePassARB' => {
		'name' => 'SamplePassARB',
		'gl' => 'gl',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLenum', 'name' => 'mode', 'pointer' => 0 },
		],
	},
	'glTexScissorFuncINTEL' => {
		'name' => 'TexScissorFuncINTEL',
		'gl' => 'gl',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLenum', 'name' => 'target', 'pointer' => 0 },
			{ 'type' => 'GLenum', 'name' => 'lfunc', 'pointer' => 0 },
			{ 'type' => 'GLenum', 'name' => 'hfunc', 'pointer' => 0 },
		],
	},
	'glTexScissorINTEL' => {
		'name' => 'TexScissorINTEL',
		'gl' => 'gl',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLenum', 'name' => 'target', 'pointer' => 0 },
			{ 'type' => 'GLclampf', 'name' => 'tlow', 'pointer' => 0 },
			{ 'type' => 'GLclampf', 'name' => 'thigh', 'pointer' => 0 },
		],
	},
	'glTextureFogSGIX' => {
		'name' => 'TextureFogSGIX',
		'gl' => 'gl',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLenum', 'name' => 'pname', 'pointer' => 0 },
		],
	},
	'gluLookAt' => {
		'name' => 'LookAt',
		'gl' => 'glu',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLdouble', 'name' => 'eyeX', 'pointer' => 0 },
			{ 'type' => 'GLdouble', 'name' => 'eyeY', 'pointer' => 0 },
			{ 'type' => 'GLdouble', 'name' => 'eyeZ', 'pointer' => 0 },
			{ 'type' => 'GLdouble', 'name' => 'centerX', 'pointer' => 0 },
			{ 'type' => 'GLdouble', 'name' => 'centerY', 'pointer' => 0 },
			{ 'type' => 'GLdouble', 'name' => 'centerZ', 'pointer' => 0 },
			{ 'type' => 'GLdouble', 'name' => 'upX', 'pointer' => 0 },
			{ 'type' => 'GLdouble', 'name' => 'upY', 'pointer' => 0 },
			{ 'type' => 'GLdouble', 'name' => 'upZ', 'pointer' => 0 },
		],
	},
);


#
# hash of trivial lengths of pointer arguments
#
my %paramlens = (
	# GL_NV_register_combiners
	'glCombinerParameterfvNV' => [ { 'name' => 'params', 'inout' => 'in', 'len' => 'nfglGetNumParams(pname)' } ],
	'glCombinerParameterivNV' => [ { 'name' => 'params', 'inout' => 'in', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetCombinerInputParameterfvNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetCombinerInputParameterivNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetCombinerOutputParameterfvNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetCombinerOutputParameterivNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetFinalCombinerInputParameterfvNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetFinalCombinerInputParameterivNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],

	# GL_NV_register_combiners2
	'glCombinerStageParameterfvNV' => [ { 'name' => 'params', 'inout' => 'in', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetCombinerStageParameterfvNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],

	# GL_ARB_transpose_matrix
	'glLoadTransposeMatrixfARB' => [ { 'name' => 'm', 'inout' => 'in', 'len' => '16' } ],
	'glLoadTransposeMatrixdARB' => [ { 'name' => 'm', 'inout' => 'in', 'len' => '16' } ],
	'glMultTransposeMatrixdARB' => [ { 'name' => 'm', 'inout' => 'in', 'len' => '16' } ],
	'glMultTransposeMatrixfARB' => [ { 'name' => 'm', 'inout' => 'in', 'len' => '16' } ],

	# GL_ARB_shader_objects
	'glGetInfoLogARB' => [
		{ 'name' => 'length', 'inout' => 'out', 'len' => '1' },
		{ 'name' => 'infoLog', 'inout' => 'out', 'len' => 'maxLength', 'outlen' => '__length_tmp[0] + 1' },
	],
	'glUniform1fvARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glUniform1ivARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glUniform2fvARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glUniform2ivARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glUniform3fvARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glUniform3ivARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glUniform4fvARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glUniform4ivARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glUniformMatrix2fvARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glUniformMatrix3fvARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '9 * count' } ],
	'glUniformMatrix4fvARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '16 * count' } ],
	'glGetObjectParameterfvARB' => [ { 'name' => 'params', 'inout' => 'out', 'len' => '1' } ],
	'glGetObjectParameterivARB' => [ { 'name' => 'params', 'inout' => 'out', 'len' => '1' } ],
	'glGetUniformfvARB' => [ { 'name' => 'params', 'inout' => 'out', 'len' => '1' } ],
	'glGetUniformivARB' => [ { 'name' => 'params', 'inout' => 'out', 'len' => '1' } ],
	'glGetShaderSourceARB' => [
		{ 'name' => 'length', 'inout' => 'out', 'len' => '1' },
		{ 'name' => 'source', 'inout' => 'out', 'len' => 'maxLength', 'outlen' => '__length_tmp[0] + 1' },
	],
	'glGetUniformLocationARB' => [ { 'name' => 'name', 'inout' => 'in', 'len' => 'safe_strlen(name)' } ],
	'glGetActiveUniformARB' => [
		{ 'name' => 'length', 'inout' => 'out', 'len' => '1' },
		{ 'name' => 'size', 'inout' => 'out', 'len' => '1' },
		{ 'name' => 'type', 'inout' => 'out', 'len' => '1' },
		{ 'name' => 'name', 'inout' => 'out', 'len' => 'maxLength', 'outlen' => '__length_tmp[0] + 1' },
	],
	'glGetAttachedObjectsARB' => [
		{ 'name' => 'count', 'inout' => 'out', 'len' => '1' },
		{ 'name' => 'obj', 'inout' => 'out', 'len' => 'maxCount', 'outlen' => '__count_tmp[0]' },
	],

	# GL_ARB_multitexture
	'glMultiTexCoord1dvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '1' } ],
	'glMultiTexCoord1fvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '1' } ],
	'glMultiTexCoord1ivARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '1' } ],
	'glMultiTexCoord1svARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '1' } ],
	'glMultiTexCoord2dvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '2' } ],
	'glMultiTexCoord2fvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '2' } ],
	'glMultiTexCoord2ivARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '2' } ],
	'glMultiTexCoord2svARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '2' } ],
	'glMultiTexCoord3dvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '3' } ],
	'glMultiTexCoord3fvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '3' } ],
	'glMultiTexCoord3ivARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '3' } ],
	'glMultiTexCoord3svARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '3' } ],
	'glMultiTexCoord4dvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '4' } ],
	'glMultiTexCoord4fvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '4' } ],
	'glMultiTexCoord4svARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '4' } ],
	'glMultiTexCoord4ivARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => '4' } ],

	# GL_EXT_cull_vertex
	'glCullParameterdvEXT' => [ { 'name' => 'params', 'inout' => 'in', 'len' => 'nfglGetNumParams(pname)' } ],
	'glCullParameterfvEXT' => [ { 'name' => 'params', 'inout' => 'in', 'len' => 'nfglGetNumParams(pname)' } ],

	# GL_EXT_debug_marker
	'glInsertEventMarkerEXT' => [ { 'name' => 'marker', 'inout' => 'in', 'len' => 'length > 0 ? length : safe_strlen(marker) + 1' } ],
	'glPushGroupMarkerEXT' => [ { 'name' => 'marker', 'inout' => 'in', 'len' => 'length > 0 ? length : safe_strlen(marker) + 1' } ],

	# GL_ARB_debug_output
	'glDebugMessageControlARB' => [ { 'name' => 'ids', 'inout' => 'in', 'len' => 'count' } ],
	'glDebugMessageInsertARB' => [ { 'name' => 'buf', 'inout' => 'in', 'len' => 'length >= 0 ? length : safe_strlen(buf) + 1' } ],
	'glGetDebugMessageLogARB' => [
		{ 'name' => 'sources', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'types', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'ids', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'severities', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'lengths', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'messageLog', 'inout' => 'out', 'len' => 'bufSize' },
	],

	# GL_AMD_debug_output
	'glDebugMessageEnableAMD' => [ { 'name' => 'ids', 'inout' => 'in', 'len' => 'count' } ],
	'glDebugMessageInsertAMD' => [ { 'name' => 'buf', 'inout' => 'in', 'len' => 'length >= 0 ? length : safe_strlen(buf) + 1' } ],
	'glGetDebugMessageLogAMD' => [
		{ 'name' => 'categories', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'ids', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'severities', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'lengths', 'inout' => 'out', 'len' => 'count', 'outlen' => '__ret' },
		{ 'name' => 'message', 'inout' => 'out', 'len' => 'bufsize' },
	],

	# GL_APPLE_fence
	'glGenFencesAPPLE' => [ { 'name' => 'fences', 'inout' => 'out', 'len' => 'n' } ],
	'glDeleteFencesAPPLE' => [ { 'name' => 'fences', 'inout' => 'in', 'len' => 'n' } ],

	# GL_NV_fence
	'glDeleteFencesNV' => [ { 'name' => 'fences', 'inout' => 'in', 'len' => 'n' } ],
	'glGenFencesNV' => [ { 'name' => 'fences', 'inout' => 'out', 'len' => 'n' } ],
	'glGetFenceivNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],

	# GL_SGIS_detail_texture
	'glDetailTexFuncSGIS' => [ { 'name' => 'points', 'inout' => 'in', 'len' => 'n * 2' } ],

	# GL version 4.6
	'glSpecializeShader' => [
		{ 'name' => 'pEntryPoint', 'inout' => 'in', 'len' => 'safe_strlen(pEntryPoint) + 1' },
		{ 'name' => 'pConstantIndex', 'inout' => 'in', 'len' => 'numSpecializationConstants' },
		{ 'name' => 'pConstantValue', 'inout' => 'in', 'len' => 'numSpecializationConstants' },
	],

	# GL_ARB_gl_spirv
	'glSpecializeShaderARB' => [
		{ 'name' => 'pEntryPoint', 'inout' => 'in', 'len' => 'safe_strlen(pEntryPoint) + 1' },
		{ 'name' => 'pConstantIndex', 'inout' => 'in', 'len' => 'numSpecializationConstants' },
		{ 'name' => 'pConstantValue', 'inout' => 'in', 'len' => 'numSpecializationConstants' },
	],

	# GL_AMD_gpu_shader_int64
	'glUniform1i64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glUniform2i64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glUniform3i64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glUniform4i64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glUniform1ui64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glUniform2ui64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glUniform3ui64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glUniform4ui64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glProgramUniform1i64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glProgramUniform2i64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glProgramUniform3i64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glProgramUniform4i64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glProgramUniform1ui64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glProgramUniform2ui64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glProgramUniform3ui64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glProgramUniform4ui64vNV' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],

	# GL_ARB_gpu_shader_int64
	'glUniform1i64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glUniform2i64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glUniform3i64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glUniform4i64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glUniform1ui64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glUniform2ui64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glUniform3ui64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glUniform4ui64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glProgramUniform1i64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glProgramUniform2i64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glProgramUniform3i64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glProgramUniform4i64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],
	'glProgramUniform1ui64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '1 * count' } ],
	'glProgramUniform2ui64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '2 * count' } ],
	'glProgramUniform3ui64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '3 * count' } ],
	'glProgramUniform4ui64vARB' => [ { 'name' => 'value', 'inout' => 'in', 'len' => '4 * count' } ],

	# GL_ARB_sample_locations
	'glFramebufferSampleLocationsfvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => 'count' } ],
	'glNamedFramebufferSampleLocationsfvARB' => [ { 'name' => 'v', 'inout' => 'in', 'len' => 'count' } ],
	
	# GL_EXT_memory_object
	'glDeleteMemoryObjectsEXT' => [ { 'name' => 'memoryObjects', 'inout' => 'in', 'len' => 'n' } ],
	'glCreateMemoryObjectsEXT' => [ { 'name' => 'memoryObjects', 'inout' => 'out', 'len' => 'n' } ],
	'glMemoryObjectParameterivEXT' => [ { 'name' => 'params', 'inout' => 'in', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetMemoryObjectParameterivEXT' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	
	# GL_EXT_memory_object_win32
	'glImportMemoryWin32NameEXT' => [ { 'name' => 'name', 'inout' => 'in', 'len' => 'safe_strlen(name) + 1', 'basetype' => 'GLchar' } ],
	
	# GL_EXT_semaphore
	'glGenSemaphoresEXT' => [ { 'name' => 'semaphores', 'inout' => 'out', 'len' => 'n' } ],
	'glDeleteSemaphoresEXT' => [ { 'name' => 'semaphores', 'inout' => 'in', 'len' => 'n' } ],
	'glSemaphoreParameterui64vEXT' => [ { 'name' => 'params', 'inout' => 'in', 'len' => 'nfglGetNumParams(pname)' } ],
	'glGetSemaphoreParameterui64vEXT' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glWaitSemaphoreEXT' => [
		{ 'name' => 'buffers', 'inout' => 'in', 'len' => 'numBufferBarriers' },
		{ 'name' => 'textures', 'inout' => 'in', 'len' => 'numTextureBarriers' },
		{ 'name' => 'srcLayouts', 'inout' => 'in', 'len' => 'numTextureBarriers' },
	],
	'glSignalSemaphoreEXT' => [
		{ 'name' => 'buffers', 'inout' => 'in', 'len' => 'numBufferBarriers' },
		{ 'name' => 'textures', 'inout' => 'in', 'len' => 'numTextureBarriers' },
		{ 'name' => 'dstLayouts', 'inout' => 'in', 'len' => 'numTextureBarriers' },
	],

	# GL_EXT_semaphore_win32
	'glImportSemaphoreWin32NameEXT' => [ { 'name' => 'name', 'inout' => 'in', 'len' => 'safe_strlen(name) + 1', 'basetype' => 'GLchar' } ],
	
	# GL_NVX_linked_gpu_multicast
	'glLGPUNamedBufferSubDataNVX' => [ { 'name' => 'data', 'inout' => 'in', 'len' => 'size', 'basetype' => 'GLbyte' } ],

	# GL_NV_gpu_multicast
	'glMulticastBufferSubDataNV' => [ { 'name' => 'data', 'inout' => 'in', 'len' => 'size', 'basetype' => 'GLbyte' } ],
	'glMulticastFramebufferSampleLocationsfvNV' => [ { 'name' => 'v', 'inout' => 'in', 'len' => 'count' } ],
	'glMulticastGetQueryObjectivNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glMulticastGetQueryObjectuivNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glMulticastGetQueryObjecti64vNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],
	'glMulticastGetQueryObjectui64vNV' => [ { 'name' => 'params', 'inout' => 'out', 'len' => 'nfglGetNumParams(pname)' } ],

	# GL_NV_query_resource
	'glQueryResourceNV' => [ { 'name' => 'buffer', 'inout' => 'out', 'len' => 'bufSize' } ],

	# GL_NV_query_resource_tag
	'glGenQueryResourceTagNV' => [ { 'name' => 'tagIds', 'inout' => 'out', 'len' => 'n' } ],
	'glDeleteQueryResourceTagNV' => [ { 'name' => 'tagIds', 'inout' => 'in', 'len' => 'n' } ],
	'glQueryResourceTagNV' => [ { 'name' => 'tagString', 'inout' => 'in', 'len' => 'safe_strlen(tagString) + 1' } ],

	# GL_AMD_framebuffer_sample_positions 1
	'glFramebufferSamplePositionsfvAMD' => [ { 'name' => 'values', 'inout' => 'in', 'len' => 'numsamples' } ],
	'glNamedFramebufferSamplePositionsfvAMD' => [ { 'name' => 'values', 'inout' => 'in', 'len' => 'numsamples' } ],
	'glGetFramebufferParameterfvAMD' => [ { 'name' => 'values', 'inout' => 'out', 'len' => 'numsamples' } ],
	'glGetNamedFramebufferParameterfvAMD' => [ { 'name' => 'values', 'inout' => 'out', 'len' => 'numsamples' } ],

	# GL_EXT_window_rectangles
	'glWindowRectanglesEXT' => [ { 'name' => 'box', 'inout' => 'in', 'len' => '4 * count' } ],
);


sub add_missing($)
{
	my ($missing) = @_;
	my $function_name;
	my $gl;
	
	foreach my $key (keys %{$missing}) {
		my $ent = $missing->{$key};
		$gl = $ent->{gl};
		$function_name = $gl . $ent->{name};
		if (!defined($functions{$function_name})) {
			$functions{$function_name} = $ent;
		}
	}
}



my %copy_funcs = (
	'GLbyte' =>      { 'ifdef' => 'NFOSMESA_NEED_BYTE_CONV',   'copyin' => 'Atari2HostByteArray',   'copyout' => 'Host2AtariByteArray',   },
	'GLubyte' =>     { 'ifdef' => 'NFOSMESA_NEED_BYTE_CONV',   'copyin' => 'Atari2HostByteArray',   'copyout' => 'Host2AtariByteArray',   },
	'GLchar' =>      { 'ifdef' => 'NFOSMESA_NEED_BYTE_CONV',   'copyin' => 'Atari2HostByteArray',   'copyout' => 'Host2AtariByteArray',   },
	'GLcharARB' =>   { 'ifdef' => 'NFOSMESA_NEED_BYTE_CONV',   'copyin' => 'Atari2HostByteArray',   'copyout' => 'Host2AtariByteArray',   },
	'GLshort' =>     { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostShortArray',  'copyout' => 'Host2AtariIntArray',    },
	'GLushort' =>    { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostShortArray',  'copyout' => 'Host2AtariIntArray',    },
	'GLint' =>       { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostIntArray',    'copyout' => 'Host2AtariIntArray',    },
	'GLsizei' =>     { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostIntArray',    'copyout' => 'Host2AtariIntArray',    },
	'GLuint' =>      { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostIntArray',    'copyout' => 'Host2AtariIntArray',    },
	'GLenum' =>      { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostIntArray',    'copyout' => 'Host2AtariIntArray',    },
	'GLintptr' =>    { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostIntptrArray', 'copyout' => 'Host2AtariIntptrArray', },
	'GLint64' =>     { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostInt64Array',  'copyout' => 'Host2AtariInt64Array',  },
	'GLint64EXT' =>  { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostInt64Array',  'copyout' => 'Host2AtariInt64Array',  },
	'GLuint64' =>    { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostInt64Array',  'copyout' => 'Host2AtariInt64Array',  },
	'GLuint64EXT' => { 'ifdef' => 'NFOSMESA_NEED_INT_CONV',    'copyin' => 'Atari2HostInt64Array',  'copyout' => 'Host2AtariInt64Array',  },
	'GLfloat' =>     { 'ifdef' => 'NFOSMESA_NEED_FLOAT_CONV',  'copyin' => 'Atari2HostFloatArray',  'copyout' => 'Host2AtariFloatArray',  },
	'GLdouble' =>    { 'ifdef' => 'NFOSMESA_NEED_DOUBLE_CONV', 'copyin' => 'Atari2HostDoubleArray', 'copyout' => 'Host2AtariDoubleArray', },
	'GLhandleARB' => { 'ifdef' => 'NFOSMESA_NEED_INT_CONV || defined(__APPLE__)', 'copyin' => 'Atari2HostHandleARB', 'copyout' => 'Host2AtariHandleARB', },
);

sub sort_by_name
{
	my $ret = $a cmp $b;
	return $ret;
}

sub sort_by_value
{
	my $ret = $functions{$a}->{funcno} <=> $functions{$b}->{funcno};
	return $ret;
}

sub add_paramlens($)
{
	my ($lens) = @_;
	my $function_name;
	
	foreach my $key (keys %{$lens}) {
		my $len = $lens->{$key};
		$function_name = $key;
		my $ent = $functions{$function_name};
		if (!defined($ent)) {
			die "$me: unknown function $function_name";
		}
		my $params = $ent->{params};
		my $argcount = $#$params + 1;
		my $lenargcount = $#$len + 1;
		my $argc;
		my $param;
		my %ifdefs = ();
		for (my $lenargc = 0; $lenargc < $lenargcount; $lenargc++)
		{
			my $lenparam = $len->[$lenargc];
			for ($argc = 0; $argc < $argcount; $argc++)
			{
				$param = $params->[$argc];
				last if ($param->{name} eq $lenparam->{name});
			}
			die "$me: unknown parameter $lenparam->{name} in $function_name" unless ($argc < $argcount);
			die "$me: parameter $lenparam->{name} in $function_name is not pointer" unless ($param->{pointer});
			$param->{len} = $lenparam->{len};
			my $type = $param->{type};
			if (!defined($lenparam->{inout}))
			{
				if ($type =~ m/const/)
				{
					$lenparam->{inout} = 'in';
				} else
				{
					$lenparam->{inout} = 'out';
				}
			} else
			{
				if ($type =~ m/const/)
				{
					if ($lenparam->{inout} ne 'in')
					{
						&warn("$me: $function_name($param->{name}): output type '$type' declared 'const'");
					}
				} else {
					if ($lenparam->{inout} eq 'in')
					{
						# &warn("$me: $function_name($param->{name}): input type '$type' not declared 'const'");
					}
				}
			}
			my $basetype = $param->{type};
			$basetype =~ s/const//;
			$basetype =~ s/^ *//;
			$basetype =~ s/ *$//;
			if (defined($lenparam->{basetype}))
			{
				$basetype = $lenparam->{basetype};
			}
			die "$me: $function_name($param->{name}): don't know how to convert type '$basetype'" unless (defined($copy_funcs{$basetype}));
			$param->{inout} = $lenparam->{inout};
			$param->{outlen} = $lenparam->{outlen} if defined($lenparam->{outlen});
			$param->{basetype} = $basetype;
			$ifdefs{$copy_funcs{$basetype}->{ifdef}} = 1;
		}
		my $allfound = 1;
		for ($argc = 0; $argc < $argcount; $argc++)
		{
			$param = $params->[$argc];
			if ($param->{pointer} && !defined($param->{len}))
			{
				$allfound = 0;
			}
		}
		if ($allfound)
		{
			$ent->{autogen} = 1;
			$ent->{ifdefs} = join(' || ', sort { sort_by_name } keys %ifdefs);
		} else
		{
			&warn("$me: $function_name cannot be generated automatically");
		}
	}
}


#
# fix short types to their promoted 32bit type
#
sub fix_promotions()
{
	foreach my $key (keys %functions) {
		my $ent = $functions{$key};
		my $params = $ent->{params};
		my $argcount = $#$params + 1;
		for (my $argc = 0; $argc < $argcount; $argc++)
		{
			my $param = $params->[$argc];
			my $pointer = $param->{pointer};
			my $type = $param->{type};
			if (! $pointer && defined($short_types{$type}))
			{
# print "changing $ent->{name}($type $param->{name}) to $short_types{$type}\n";
				$param->{type} = $short_types{$type};
			}
		}
	}
}


#
# generate the strings for the prototype and the arguments
#
sub gen_params()
{
	my $ent;
	
	foreach my $key (keys %functions) {
		$ent = $functions{$key};
		my $params = $ent->{params};
		my $argcount = $#$params + 1;
		my $args = "";
		my $noconv_args = "";
		my $debug_args = "";
		my $prototype = "";
		my $prototype_mem = "";
		my $any_pointer = 0;
		my $printf_format = "";
		my $format;
		my $type_mem;
		for (my $argc = 0; $argc < $argcount; $argc++)
		{
			my $param = $params->[$argc];
			my $pointer = $param->{pointer};
			my $type = $param->{type};
			my $name = $param->{name};
			$args .= ", " if ($argc != 0);
			$noconv_args .= ", " if ($argc != 0);
			$debug_args .= ", " if ($argc != 0);
			if ($pointer) {
				if (1 || !defined($byte_types{$type})) {
					$any_pointer = 2;
				} elsif ($any_pointer == 0) {
					$any_pointer = 1;
				}
				if ($pointer > 1) {
					$pointer = "";
				} else {
					$pointer = "*";
				}
				$debug_args .= 'AtariOffset(' . $name . ')';
				$format = '" PRI_PTR "';
				$type_mem = "memptr";
				if (defined($param->{len}))
				{
					if ($param->{'inout'} eq 'in' || $param->{'inout'} eq 'inout')
					{
						$args .= '__' . $name. '_ptmp';
					} else
					{
						$args .= '__' . $name. '_tmp';
					}
				} else
				{
					$args .= $name;
				}
				$noconv_args .= "HostAddr($name, $type ${pointer})";
			} else {
				$type_mem = "$type";
				if ($type eq 'GLsync' || $type eq 'GLhandleARB')
				{
					$debug_args .= '(unsigned int)(uintptr_t)' . $name;
				} elsif ($type eq 'GLenum')
				{
					$debug_args .= "gl_enum_name($name)";
				} elsif (defined($pointer_types{$type}))
				{
					$debug_args .= 'AtariOffset(' . $name . ')';
					$type_mem = "memptr";
				} else
				{
					$debug_args .= $name;
				}
				$pointer = "";
				$format = $printf_formats{$type};
				if (!defined($format))
				{
					&warn("$key: dont know how to printf values of type $type");
					$format = "%d";
				}
				$args .= $name;
				$noconv_args .= $name;
			}
			$prototype .= ", " if ($argc != 0);
			$prototype .= "${type} ${pointer}${name}";
			$prototype_mem .= ", " if ($argc != 0);
			$prototype_mem .= "${type_mem} ${name}";
			$printf_format .= ", " if ($argc != 0);
			$printf_format .= $format;
		}
		$prototype = "void" unless ($prototype ne "");
		$prototype_mem = "void" unless ($prototype_mem ne "");
		$ent->{args} = $args;
		$ent->{debug_args} = $debug_args;
		$ent->{noconv_args} = $noconv_args;
		$ent->{proto} = $prototype;
		$ent->{proto_mem} = $prototype_mem;
		$ent->{any_pointer} = $any_pointer;
		$ent->{printf_format} = $printf_format;
	}
	# hack for exception_error, which has a function pointer as argument
	$ent = $functions{"exception_error"};
	$ent->{args} = "exception";
}


#
# Read include file(s)
#
sub read_includes()
{
	my $filename;
	my $return_type;
	my $function_name;
	my $gl;

	while ($filename = shift @ARGV)
	{
		my $line;
		
		if ( ! defined(open(FILE, $filename)) ) {
			die "$me: couldn't open $filename: $!\n";
		}
		
		while ($line = <FILE>) {
			if ($line =~ /^GLAPI/ ) {
				my @params;
				my $prototype;
				
				while (! ($line =~ /\);/)) {
					chomp($line);
					$line .= " " . <FILE>;
				}
				chomp($line);
				$line =~ s/\t/ /g;
				$line =~ s/\n//g;
				$line =~ s/ +/ /g;
				$line =~ s/ *$//;
				$line =~ s/\( /(/g;
				$line =~ s/ \(/(/g;
				$line =~ s/ \)/)/g;
		
				# Add missing parameter names (for glext.h)
				my $letter = 'a';
				while ($line =~ /[,\( ] *GL\w+\** *\**,/ ) {
					$line =~ s/([,\( ] *GL\w+\** *\**),/$1 $letter,/;
					$letter++;
				}
				while ($line =~ /[,\( ] *GL\w+\** *\**\)/ ) {
					$line =~ s/([,\( ] *GL\w+\** *\**)\)/$1 $letter\)/;
					$letter++;
				}
				while ($line =~ /[,\( ] *GL\w+\** *const *\*,/ ) {
					$line =~ s/([,\( ] *GL\w+\** *const *\*),/$1 $letter,/;
					$letter++;
				}
		
				if ($line =~ /^GLAPI *([a-zA-Z0-9_ *]+).* +(glu|gl|OSMesa)(\w+) *\((.*)\);/) {
					$return_type = $1;
					$gl = $2;
					$function_name = $3;
					$prototype = $4;
				} else {
					&warn("ignoring $line");
					next;
				}
				$return_type =~ s/ *$//;
				$return_type =~ s/GLAPIENTRY$//;
				$return_type =~ s/APIENTRY$//;
				$return_type =~ s/ *$//;
				$return_type =~ s/^ *//;
				$prototype =~ s/ *$//;
				$prototype =~ s/^ *//;
		
				# Remove parameter types
				@params = ();
				my $any_pointer = 0;
				if ( $prototype eq "void" ) {
					# Remove void list of parameters
				} else {
					# Remove parameters type
					foreach my $param (split(',', $prototype)) {
						$param =~ s/ *$//;
						$param =~ s/^ *//;
						if ($param =~ /([a-zA-Z0-9_ *]*[ *])([a-zA-Z0-9_]+)/)
						{
							my $type = $1;
							my $name = $2;
							$type =~ s/^ *//;
							$type =~ s/ *$//;
							my $pointer = ($type =~ s/\*$//);
							if ($pointer) {
								$type =~ s/ *$//;
							}
							if ($param =~ /\[([0-9]+)\]$/) {
								$pointer = 1;
							}
							my %param = ('type' => $type, 'name' => $name, 'pointer' => $pointer);
							push @params, \%param;
						} else {
							die "$me: can't parse parameter $param in $function_name";
						}
					}
				}
		
				my %ent = ('type' => $return_type, 'name' => $function_name, 'params' => \@params, 'proto' => $prototype, 'gl' => $gl, 'any_pointer' => $any_pointer);
				$function_name = $gl . $function_name;
				
				# ignore some functions that sometimes appear in gl headers but are actually only GLES
				next if ($function_name eq 'glEGLImageTargetRenderbufferStorageOES');
				next if ($function_name eq 'glEGLImageTargetTexture2DOES');
				
				$functions{$function_name} = \%ent;
			}
		}
		close(FILE);
	}
	
	add_missing(\%missing);
	fix_promotions();
}


sub read_enums()
{
	my $key;
	my $ent;
	my $line;
	my $name;
	my $funcno;
	my $function_name;
	my %byupper;
	
	die "$me: no enum file" unless defined($enumfile);

	foreach my $key (keys %functions) {
		$functions{$key}->{funcno} = 0;
	}
	if ( ! defined(open(FILE, $enumfile)) ) {
		die "$me: couldn't open $enumfile: $!\n";
	}

	foreach my $key (keys %functions) {
		my $uppername = uc($key);
		# 'swapbuffer' is mangled to 'tinyglswapbuffer' to avoid name clashes
		if ($key eq 'swapbuffer') {
			$uppername = 'TINYGL' . $uppername;
		}
		# glGetString and glGetStringi are passed
		# as LENGLGETSTRING and LENGLGETSTRINGI
		if ($key eq 'glGetString' || $key eq 'glGetStringi') {
			$uppername = 'LEN' . $uppername;
		}
		$byupper{$uppername} = $key;
	}
	# some functions from oldmesa have no direct NFAPI call
	$functions{'OSMesaCreateLDG'}->{funcno} = 1384;
	$functions{'OSMesaDestroyLDG'}->{funcno} = 1385;
	$functions{'max_width'}->{funcno} = 1386;
	$functions{'max_height'}->{funcno} = 1387;
	$functions{'information'}->{funcno} = 1388;
	$functions{'exception_error'}->{funcno} = 1389;
	
	while ($line = <FILE>) {
		if ($line =~ /.*NFOSMESA_([a-zA-Z0-9_]+) = ([0-9]+).*/) {
			$name = $1;
			$funcno = $2;
			if (exists($byupper{$name})) {
				$functions{$byupper{$name}}->{funcno} = $funcno;
				$functions{$byupper{$name}}->{nfapi} = 'NFOSMESA_' . $name;
			}
		}
	}
	close(FILE);
	
	my $errors = 0;
	foreach my $key (keys %functions) {
		if (!$functions{$key}->{funcno})
		{
			&warn("no function number for $key");
			++$errors;
		}
	}
	die "$errors errors" unless ($errors == 0);
}

#
# functions that need special attention;
# each one needs a corresponding macro
#
# Most of these macros are for converting input/output arrays
# to/from host format. For functions that don't need conversion
# (ie. all parameters are passed as values)
# no entry needs to be made here.
# If there is an entry, AND its value is != 0,
# a macro FN_UPPERCASENAME has to be defined in nfosmesa.cpp.
# In some cases, the length of the array can be deduced from
# some of the other parameters; in such cases, you should make
# an entry to the paramlens hash above, and set the entry here to 0,
# or remove it. The script will then generate the conversion code.
#
my %macros = (
	# GL_ARB_window_pos
	'glWindowPos2dvARB' => 1,
	'glWindowPos2fvARB' => 1,
	'glWindowPos2ivARB' => 1,
	'glWindowPos2svARB' => 1,
	'glWindowPos3dvARB' => 1,
	'glWindowPos3fvARB' => 1,
	'glWindowPos3ivARB' => 1,
	'glWindowPos3svARB' => 1,
	
	# GL_MESA_window_pos
	'glWindowPos2dvMESA' => 1,
	'glWindowPos2fvMESA' => 1,
	'glWindowPos2ivMESA' => 1,
	'glWindowPos2svMESA' => 1,
	'glWindowPos3dvMESA' => 1,
	'glWindowPos3fvMESA' => 1,
	'glWindowPos3ivMESA' => 1,
	'glWindowPos3svMESA' => 1,
	'glWindowPos4dvMESA' => 1,
	'glWindowPos4fvMESA' => 1,
	'glWindowPos4ivMESA' => 1,
	'glWindowPos4svMESA' => 1,

	# GL_EXT_color_subtable
	'glColorSubTableEXT' => 1,
	'glCopyColorSubTableEXT' => 0,

	# GL_NV_register_combiners
	'glCombinerParameterfvNV' => 2,
	'glCombinerParameterfNV' => 0,
	'glCombinerParameterivNV' => 2,
	'glCombinerParameteriNV' => 0,
	'glCombinerInputNV' => 0,
	'glCombinerOutputNV' => 0,
	'glFinalCombinerInputNV' => 0,
	'glGetCombinerInputParameterfvNV' => 2,
	'glGetCombinerInputParameterivNV' => 2,
	'glGetCombinerOutputParameterfvNV' => 2,
	'glGetCombinerOutputParameterivNV' => 2,
	'glGetFinalCombinerInputParameterfvNV' => 2,
	'glGetFinalCombinerInputParameterivNV' => 2,

	# GL_NV_register_combiners2
	'glCombinerStageParameterfvNV' => 2,
	'glGetCombinerStageParameterfvNV' => 2,

	# GL_ARB_multitexture
	'glActiveTextureARB' => 0,
	'glClientActiveTextureARB' => 0,
	'glMultiTexCoord1dARB' => 0,
	'glMultiTexCoord1dvARB' => 2,
	'glMultiTexCoord1fARB' => 0,
	'glMultiTexCoord1fvARB' => 2,
	'glMultiTexCoord1iARB' => 0,
	'glMultiTexCoord1ivARB' => 2,
	'glMultiTexCoord1sARB' => 0,
	'glMultiTexCoord1svARB' => 2,
	'glMultiTexCoord2dARB' => 0,
	'glMultiTexCoord2dvARB' => 2,
	'glMultiTexCoord2fARB' => 0,
	'glMultiTexCoord2fvARB' => 2,
	'glMultiTexCoord2iARB' => 0,
	'glMultiTexCoord2ivARB' => 2,
	'glMultiTexCoord2sARB' => 0,
	'glMultiTexCoord2svARB' => 2,
	'glMultiTexCoord3dARB' => 0,
	'glMultiTexCoord3dvARB' => 2,
	'glMultiTexCoord3fARB' => 0,
	'glMultiTexCoord3fvARB' => 2,
	'glMultiTexCoord3iARB' => 0,
	'glMultiTexCoord3ivARB' => 2,
	'glMultiTexCoord3sARB' => 0,
	'glMultiTexCoord3svARB' => 2,
	'glMultiTexCoord4dARB' => 0,
	'glMultiTexCoord4dvARB' => 2,
	'glMultiTexCoord4fARB' => 0,
	'glMultiTexCoord4fvARB' => 2,
	'glMultiTexCoord4iARB' => 0,
	'glMultiTexCoord4ivARB' => 2,
	'glMultiTexCoord4sARB' => 0,
	'glMultiTexCoord4svARB' => 2,
	
	# GL_ARB_transpose_matrix
	'glLoadTransposeMatrixdARB' => 2,
	'glLoadTransposeMatrixfARB' => 2,
	'glMultTransposeMatrixdARB' => 2,
	'glMultTransposeMatrixfARB' => 2,

	# GL_EXT_cull_vertex
	'glCullParameterdvEXT' => 2,
	'glCullParameterfvEXT' => 2,

	# GL_EXT_debug_marker
	'glInsertEventMarkerEXT' => 2,
	'glPushGroupMarkerEXT' => 2,
	'glPopGroupMarkerEXT' => 0,
	
	# GL_ARB_debug_output
	'glDebugMessageControlARB' => 2,
	'glDebugMessageInsertARB' => 2,
	'glDebugMessageCallbackARB' => 1,
	'glGetDebugMessageLogARB' => 2,

	# GL_AMD_debug_output
	'glDebugMessageEnableAMD' => 2,
	'glDebugMessageInsertAMD' => 2,
	'glDebugMessageCallbackAMD' => 1,
	'glGetDebugMessageLogAMD' => 2,

	# GL_SGIX_polynomial_ffd
	'glDeformationMap3dSGIX' => 1,
	'glDeformationMap3fSGIX' => 1,
	'glDeformSGIX' => 0,
	'glLoadIdentityDeformationMapSGIX' => 0,

	# GL_APPLE_fence
	'glGenFencesAPPLE' => 2,
	'glDeleteFencesAPPLE' => 2,
	'glSetFenceAPPLE' => 0,
	'glIsFenceAPPLE' => 0,
	'glTestFenceAPPLE' => 0,
	'glFinishFenceAPPLE' => 0,
	'glTestObjectAPPLE' => 0,
	'glFinishObjectAPPLE' => 0,

	# GL_NV_fence
	'glDeleteFencesNV' => 2,
	'glGenFencesNV' => 2,
	'glIsFenceNV' => 0,
	'glTestFenceNV' => 0,
	'glGetFenceivNV' => 2,
	'glFinishFenceNV' => 0,
	'glSetFenceNV' => 0,
	
	# GL_EXT_framebuffer_object
	'glIsRenderbufferEXT' => 0,
	'glBindRenderbufferEXT' => 0,
	'glDeleteRenderbuffersEXT' => 1,
	'glGenRenderbuffersEXT' => 1,
	'glRenderbufferStorageEXT' => 0,
	'glGetRenderbufferParameterivEXT' => 1,
	'glIsFramebufferEXT' => 0,
	'glBindFramebufferEXT' => 0,
	'glDeleteFramebuffersEXT' => 1,
	'glGenFramebuffersEXT' => 1,
	'glCheckFramebufferStatusEXT' => 0,
	'glFramebufferTexture1DEXT' => 0,
	'glFramebufferTexture2DEXT' => 0,
	'glFramebufferTexture3DEXT' => 0,
	'glFramebufferRenderbufferEXT' => 0,
	'glGetFramebufferAttachmentParameterivEXT' => 1,
	'glGenerateMipmapEXT' => 0,

	# GL_AMD_name_gen_delete
	'glGenNamesAMD' => 1,
	'glDeleteNamesAMD' => 1,
	'glIsNameAMD' => 0,
	
	# GL_APPLE_vertex_array_object
	'glBindVertexArrayAPPLE' => 0,
	'glDeleteVertexArraysAPPLE' => 1,
	'glGenVertexArraysAPPLE' => 1,
	'glIsVertexArrayAPPLE' => 0,
	
	# GL_SGIS_detail_texture
	'glDetailTexFuncSGIS' => 0,
	'glGetDetailTexFuncSGIS' => 1,
	
	# GL_ARB_draw_buffers
	'glDrawBuffersARB' => 1,

	# GL_ATI_draw_buffers
	'glDrawBuffersATI' => 1,

	# GL_ARB_draw_instanced
	'glDrawArraysInstancedARB' => 1,
	'glDrawElementsInstancedARB' => 1,

	# GL_EXT_draw_instanced
	'glDrawArraysInstancedEXT' => 1,
	'glDrawElementsInstancedEXT' => 1,

	# GL_EXT_draw_range_elements
	'glDrawRangeElementsEXT' => 1,

	# GL_EXT_fog_coord
	'glFogCoordfEXT' => 0,
	'glFogCoordfvEXT' => 1,
	'glFogCoorddEXT' => 0,
	'glFogCoorddvEXT' => 1,
	'glFogCoordPointerEXT' => 1,
	
	# GL_SGIS_fog_function
	'glFogFuncSGIS' => 1,
	'glGetFogFuncSGIS' => 1,
	
	# GL_SGIX_fragment_lighting
	'glFragmentColorMaterialSGIX' => 0,
	'glFragmentLightfSGIX' => 0,
	'glFragmentLightfvSGIX' => 1,
	'glFragmentLightiSGIX' => 0,
	'glFragmentLightivSGIX' => 1,
	'glFragmentLightModelfSGIX' => 0,
	'glFragmentLightModelfvSGIX' => 1,
	'glFragmentLightModeliSGIX' => 0,
	'glFragmentLightModelivSGIX' => 1,
	'glFragmentMaterialfSGIX' => 0,
	'glFragmentMaterialfvSGIX' => 1,
	'glFragmentMaterialiSGIX' => 0,
	'glFragmentMaterialivSGIX' => 1,
	'glGetFragmentLightfvSGIX' => 1,
	'glGetFragmentLightivSGIX' => 1,
	'glGetFragmentMaterialfvSGIX' => 1,
	'glGetFragmentMaterialivSGIX' => 1,
	'glLightEnviSGIX' => 0,

	# GL_EXT_coordinate_frame
	'glTangent3bEXT' => 0,
	'glTangent3bvEXT' => 1,
	'glTangent3dEXT' => 0,
	'glTangent3dvEXT' => 1,
	'glTangent3fEXT' => 0,
	'glTangent3fvEXT' => 1,
	'glTangent3iEXT' => 0,
	'glTangent3ivEXT' => 1,
	'glTangent3sEXT' => 0,
	'glTangent3svEXT' => 1,
	'glBinormal3bEXT' => 0,
	'glBinormal3bvEXT' => 1,
	'glBinormal3dEXT' => 0,
	'glBinormal3dvEXT' => 1,
	'glBinormal3fEXT' => 0,
	'glBinormal3fvEXT' => 1,
	'glBinormal3iEXT' => 0,
	'glBinormal3ivEXT' => 1,
	'glBinormal3sEXT' => 0,
	'glBinormal3svEXT' => 1,
	'glTangentPointerEXT' => 1,
	'glBinormalPointerEXT' => 1,
	
	# GL_IBM_vertex_array_lists
	'glColorPointerListIBM' => 1,
	'glSecondaryColorPointerListIBM' => 1,
	'glEdgeFlagPointerListIBM' => 1,
	'glFogCoordPointerListIBM' => 1,
	'glIndexPointerListIBM' => 1,
	'glNormalPointerListIBM' => 1,
	'glTexCoordPointerListIBM' => 1,
	'glVertexPointerListIBM' => 1,

	# GL_INTEL_parallel_arrays
	'glVertexPointervINTEL' => 1,
	'glNormalPointervINTEL' => 1,
	'glColorPointervINTEL' => 1,
	'glTexCoordPointervINTEL' => 1,

	# GL_EXT_vertex_array
	'glArrayElementEXT' => 1,
	'glColorPointerEXT' => 1,
	'glDrawArraysEXT' => 1,
	'glEdgeFlagPointerEXT' => 1,
	'glGetPointervEXT' => 1,
	'glIndexPointerEXT' => 1,
	'glNormalPointerEXT' => 1,
	'glTexCoordPointerEXT' => 1,
	'glVertexPointerEXT' => 1,

	# GL_EXT_compiled_vertex_array
	'glLockArraysEXT' => 1,
	'glUnlockArraysEXT' => 0,
	
	# GL_APPLE_element_array
	'glElementPointerAPPLE' => 1,
	'glDrawElementArrayAPPLE' => 1,
	'glDrawRangeElementArrayAPPLE' => 1,
	'glMultiDrawElementArrayAPPLE' => 1,
	'glMultiDrawRangeElementArrayAPPLE' => 1,
	
	# GL_ATI_element_array
	'glElementPointerATI' => 1,
	'glDrawElementArrayATI' => 1,
	'glDrawRangeElementArrayATI' => 1,
	
	# GL_NV_evaluators
	'glMapParameterfvNV' => 1,
	'glGetMapParameterfvNV' => 1,
	'glMapParameterivNV' => 1,
	'glGetMapParameterivNV' => 1,
	'glMapAttribParameterfvNV' => 1,
	'glGetMapAttribParameterfvNV' => 1,
	'glMapAttribParameterivNV' => 1,
	'glGetMapAttribParameterivNV' => 1,
	'glMapControlPointsNV' => 1,
	'glGetMapControlPointsNV' => 1,
	
	# GL_NV_explicit_multisample
	'glGetMultisamplefvNV' => 1,
	'glSampleMaskIndexedNV' => 0,
	'glTexRenderbufferNV' => 0,

	# GL_INTEL_performance_query
	'glBeginPerfQueryINTEL' => 0,
	'glCreatePerfQueryINTEL' => 1,
	'glDeletePerfQueryINTEL' => 0,
	'glEndPerfQueryINTEL' => 0,
	'glGetFirstPerfQueryIdINTEL' => 1,
	'glGetNextPerfQueryIdINTEL' => 1,
	'glGetPerfCounterInfoINTEL' => 1,
	'glGetPerfQueryDataINTEL' => 1,
	'glGetPerfQueryIdByNameINTEL' => 1,
	'glGetPerfQueryInfoINTEL' => 1,
	
	# GL_EXT_direct_state_access
	'glMultiTexCoordPointerEXT' => 1,
	'glMultiTexEnvfvEXT' => 1,
	'glMultiTexEnvivEXT' => 1,
	'glMultiTexGendvEXT' => 1,
	'glMultiTexGenfvEXT' => 1,
	'glMultiTexGenivEXT' => 1,
	'glGetMultiTexEnvfvEXT' => 1,
	'glGetMultiTexEnvivEXT' => 1,
	'glGetMultiTexGendvEXT' => 1,
	'glGetMultiTexGenfvEXT' => 1,
	'glGetMultiTexGenivEXT' => 1,
	'glMultiTexParameterivEXT' => 1,
	'glMultiTexParameterfvEXT' => 1,
	'glMultiTexImage1DEXT' => 1,
	'glMultiTexImage2DEXT' => 1,
	'glMultiTexImage3DEXT' => 1,
	'glMultiTexSubImage1DEXT' => 1,
	'glMultiTexSubImage2DEXT' => 1,
	'glMultiTexSubImage3DEXT' => 1,
	'glGetMultiTexImageEXT' => 1,
	'glGetMultiTexParameterfvEXT' => 1,
	'glGetMultiTexParameterivEXT' => 1,
	'glGetMultiTexLevelParameterfvEXT' => 1,
	'glGetMultiTexLevelParameterivEXT' => 1,
	'glGetDoubleIndexedvEXT' => 1,
	'glGetFloatIndexedvEXT' => 1,
	'glGetPointerIndexedvEXT' => 1,
	'glGetBooleanIndexedvEXT' => 1,
	'glGetIntegerIndexedvEXT' => 1,
	'glGetCompressedMultiTexImageEXT' => 1,
	'glGetCompressedTextureImageEXT' => 1,
	'glCompressedMultiTexImage1DEXT' => 1,
	'glCompressedMultiTexImage2DEXT' => 1,
	'glCompressedMultiTexImage3DEXT' => 1,
	'glCompressedMultiTexSubImage1DEXT' => 1,
	'glCompressedMultiTexSubImage2DEXT' => 1,
	'glCompressedMultiTexSubImage3DEXT' => 1,
	'glCompressedTextureImage1DEXT' => 1,
	'glCompressedTextureImage2DEXT' => 1,
	'glCompressedTextureImage3DEXT' => 1,
	'glCompressedTextureSubImage1DEXT' => 1,
	'glCompressedTextureSubImage2DEXT' => 1,
	'glCompressedTextureSubImage3DEXT' => 1,
	'glMatrixLoadfEXT' => 1,
	'glMatrixLoaddEXT' => 1,
	'glMatrixMultfEXT' => 1,
	'glMatrixMultdEXT' => 1,
	'glMatrixLoadTransposefEXT' => 1,
	'glMatrixLoadTransposedEXT' => 1,
	'glMatrixMultTransposefEXT' => 1,
	'glMatrixMultTransposedEXT' => 1,
	'glNamedBufferDataEXT' => 1,
	'glNamedBufferSubDataEXT' => 1,
	'glGetNamedBufferParameterivEXT' => 1,
	'glGetNamedBufferPointervEXT' => 1,
	'glGetNamedBufferSubDataEXT' => 1,
	'glGetNamedFramebufferAttachmentParameterivEXT' => 1,
	'glGetNamedFramebufferParameterivEXT' => 1,
	'glGetNamedRenderbufferParameterivEXT' => 1,
	'glProgramUniform1fvEXT' => 1,
	'glProgramUniform2fvEXT' => 1,
	'glProgramUniform3fvEXT' => 1,
	'glProgramUniform4fvEXT' => 1,
	'glProgramUniform1ivEXT' => 1,
	'glProgramUniform2ivEXT' => 1,
	'glProgramUniform3ivEXT' => 1,
	'glProgramUniform4ivEXT' => 1,
	'glProgramUniform1uivEXT' => 1,
	'glProgramUniform2uivEXT' => 1,
	'glProgramUniform3uivEXT' => 1,
	'glProgramUniform4uivEXT' => 1,
	'glProgramUniform1dvEXT' => 1,
	'glProgramUniform2dvEXT' => 1,
	'glProgramUniform3dvEXT' => 1,
	'glProgramUniform4dvEXT' => 1,
	'glProgramUniformMatrix2fvEXT' => 1,
	'glProgramUniformMatrix3fvEXT' => 1,
	'glProgramUniformMatrix4fvEXT' => 1,
	'glProgramUniformMatrix2x3fvEXT' => 1,
	'glProgramUniformMatrix3x2fvEXT' => 1,
	'glProgramUniformMatrix2x4fvEXT' => 1,
	'glProgramUniformMatrix4x2fvEXT' => 1,
	'glProgramUniformMatrix3x4fvEXT' => 1,
	'glProgramUniformMatrix4x3fvEXT' => 1,
	'glProgramUniformMatrix2dvEXT' => 1,
	'glProgramUniformMatrix3dvEXT' => 1,
	'glProgramUniformMatrix4dvEXT' => 1,
	'glProgramUniformMatrix2x3dvEXT' => 1,
	'glProgramUniformMatrix2x4dvEXT' => 1,
	'glProgramUniformMatrix3x2dvEXT' => 1,
	'glProgramUniformMatrix3x4dvEXT' => 1,
	'glProgramUniformMatrix4x2dvEXT' => 1,
	'glProgramUniformMatrix4x3dvEXT' => 1,
	'glTextureParameterIivEXT' => 1,
	'glTextureParameterIuivEXT' => 1,
	'glGetTextureParameterIivEXT' => 1,
	'glGetTextureParameterIuivEXT' => 1,
	'glGetMultiTexParameterIivEXT' => 1,
	'glGetMultiTexParameterIuivEXT' => 1,
	'glMultiTexParameterIivEXT' => 1,
	'glMultiTexParameterIuivEXT' => 1,
	'glNamedProgramLocalParameters4fvEXT' => 1,
	'glNamedProgramLocalParametersI4ivEXT' => 1,
	'glNamedProgramLocalParametersI4uivEXT' => 1,
	'glNamedProgramLocalParameterI4ivEXT' => 1,
	'glNamedProgramLocalParameterI4uivEXT' => 1,
	'glNamedProgramLocalParameter4dvEXT' => 1,
	'glNamedProgramLocalParameter4fvEXT' => 1,
	'glGetNamedProgramLocalParameterIivEXT' => 1,
	'glGetNamedProgramLocalParameterIuivEXT' => 1,
	'glGetNamedProgramLocalParameterdvEXT' => 1,
	'glGetNamedProgramLocalParameterfvEXT' => 1,
	'glGetFloati_vEXT' => 1,
	'glGetDoublei_vEXT' => 1,
	'glGetPointeri_vEXT' => 1,
	'glGetNamedProgramStringEXT' => 1,
	'glNamedProgramStringEXT' => 1,
	'glGetNamedProgramivEXT' => 1,
	'glFramebufferDrawBuffersEXT' => 1,
	'glGetFramebufferParameterivEXT' => 1,
	'glGetVertexArrayIntegervEXT' => 1,
	'glGetVertexArrayPointervEXT' => 1,
	'glGetVertexArrayIntegeri_vEXT' => 1,
	'glGetVertexArrayPointeri_vEXT' => 1,
	'glGetTextureImageEXT' => 1,
	'glGetTextureParameterfvEXT' => 1,
	'glGetTextureParameterivEXT' => 1,
	'glGetTextureLevelParameterfvEXT' => 1,
	'glGetTextureLevelParameterivEXT' => 1,
	'glNamedBufferStorageEXT' => 1,
	'glTextureImage1DEXT' => 1,
	'glTextureImage2DEXT' => 1,
	'glTextureImage3DEXT' => 1,
	'glTextureParameterfvEXT' => 1,
	'glTextureParameterivEXT' => 1,
	'glTextureSubImage1DEXT' => 1,
	'glTextureSubImage2DEXT' => 1,
	'glTextureSubImage3DEXT' => 1,
	
	# GL_ARB_clear_buffer_object
	'glClearBufferData' => 1,
	'glClearBufferSubData' => 1,
	'glClearNamedBufferSubDataEXT' => 1,
	'glClearNamedBufferDataEXT' => 1,
	
	# GL_EXT_gpu_shader4
	'glGetUniformuivEXT' => 1,
	'glBindFragDataLocationEXT' => 1,
	'glGetFragDataLocationEXT' => 1,
	'glUniform1uiEXT' => 0,
	'glUniform2uiEXT' => 0,
	'glUniform3uiEXT' => 0,
	'glUniform4uiEXT' => 0,
	'glUniform1uivEXT' => 1,
	'glUniform2uivEXT' => 1,
	'glUniform3uivEXT' => 1,
	'glUniform4uivEXT' => 1,

	# GL_AMD_gpu_shader_int64
	'glUniform1i64NV' => 0,
	'glUniform2i64NV' => 0,
	'glUniform3i64NV' => 0,
	'glUniform4i64NV' => 0,
	'glUniform1i64vNV' => 0,
	'glUniform2i64vNV' => 0,
	'glUniform3i64vNV' => 0,
	'glUniform4i64vNV' => 0,
	'glUniform1ui64NV' => 0,
	'glUniform2ui64NV' => 0,
	'glUniform3ui64NV' => 0,
	'glUniform4ui64NV' => 0,
	'glUniform1ui64vNV' => 0,
	'glUniform2ui64vNV' => 0,
	'glUniform3ui64vNV' => 0,
	'glUniform4ui64vNV' => 0,
	'glGetUniformi64vNV' => 1,
	'glGetUniformui64vNV' => 1,
	'glProgramUniform1i64NV' => 0,
	'glProgramUniform2i64NV' => 0,
	'glProgramUniform3i64NV' => 0,
	'glProgramUniform4i64NV' => 0,
	'glProgramUniform1i64vNV' => 0,
	'glProgramUniform2i64vNV' => 0,
	'glProgramUniform3i64vNV' => 0,
	'glProgramUniform4i64vNV' => 0,
	'glProgramUniform1ui64NV' => 0,
	'glProgramUniform2ui64NV' => 0,
	'glProgramUniform3ui64NV' => 0,
	'glProgramUniform4ui64NV' => 0,
	'glProgramUniform1ui64vNV' => 0,
	'glProgramUniform2ui64vNV' => 0,
	'glProgramUniform3ui64vNV' => 0,
	'glProgramUniform4ui64vNV' => 0,

	# GL_EXT_vertex_shader
	'glBeginVertexShaderEXT' => 0,
	'glEndVertexShaderEXT' => 0,
	'glBindVertexShaderEXT' => 0,
	'glGenVertexShadersEXT' => 0,
	'glDeleteVertexShaderEXT' => 0,
	'glShaderOp1EXT' => 0,
	'glShaderOp2EXT' => 0,
	'glShaderOp3EXT' => 0,
	'glSwizzleEXT' => 0,
	'glWriteMaskEXT' => 0,
	'glInsertComponentEXT' => 0,
	'glExtractComponentEXT' => 0,
	'glGenSymbolsEXT' => 0,
	'glSetInvariantEXT' => 1,
	'glSetLocalConstantEXT' => 1,
	'glVariantbvEXT' => 1,
	'glVariantsvEXT' => 1,
	'glVariantivEXT' => 1,
	'glVariantfvEXT' => 1,
	'glVariantdvEXT' => 1,
	'glVariantubvEXT' => 1,
	'glVariantusvEXT' => 1,
	'glVariantuivEXT' => 1,
	'glVariantPointerEXT' => 1,
	'glEnableVariantClientStateEXT' => 1,
	'glDisableVariantClientStateEXT' => 1,
	'glBindLightParameterEXT' => 0,
	'glBindMaterialParameterEXT' => 0,
	'glBindTexGenParameterEXT' => 0,
	'glBindTextureUnitParameterEXT' => 0,
	'glBindParameterEXT' => 0,
	'glIsVariantEnabledEXT' => 0,
	'glGetVariantBooleanvEXT' => 1,
	'glGetVariantIntegervEXT' => 1,
	'glGetVariantFloatvEXT' => 1,
	'glGetVariantPointervEXT' => 1,
	'glGetInvariantBooleanvEXT' => 1,
	'glGetInvariantIntegervEXT' => 1,
	'glGetInvariantFloatvEXT' => 1,
	'glGetLocalConstantBooleanvEXT' => 1,
	'glGetLocalConstantIntegervEXT' => 1,
	'glGetLocalConstantFloatvEXT' => 1,

	# GL_ARB_shading_language_include
	'glNamedStringARB' => 1,
	'glDeleteNamedStringARB' => 1,
	'glCompileShaderIncludeARB' => 1,
	'glIsNamedStringARB' => 1,
	'glGetNamedStringARB' => 1,
	'glGetNamedStringivARB' => 1,
	
	# GL_ATI_vertex_streams
	'glVertexStream1sATI' => 0,
	'glVertexStream1svATI' => 1,
	'glVertexStream1iATI' => 0,
	'glVertexStream1ivATI' => 1,
	'glVertexStream1fATI' => 0,
	'glVertexStream1fvATI' => 1,
	'glVertexStream1dATI' => 0,
	'glVertexStream1dvATI' => 1,
	'glVertexStream2sATI' => 0,
	'glVertexStream2svATI' => 1,
	'glVertexStream2iATI' => 0,
	'glVertexStream2ivATI' => 1,
	'glVertexStream2fATI' => 0,
	'glVertexStream2fvATI' => 1,
	'glVertexStream2dATI' => 0,
	'glVertexStream2dvATI' => 1,
	'glVertexStream3sATI' => 0,
	'glVertexStream3svATI' => 1,
	'glVertexStream3iATI' => 0,
	'glVertexStream3ivATI' => 1,
	'glVertexStream3fATI' => 0,
	'glVertexStream3fvATI' => 1,
	'glVertexStream3dATI' => 0,
	'glVertexStream3dvATI' => 1,
	'glVertexStream4sATI' => 0,
	'glVertexStream4svATI' => 1,
	'glVertexStream4iATI' => 0,
	'glVertexStream4ivATI' => 1,
	'glVertexStream4fATI' => 0,
	'glVertexStream4fvATI' => 1,
	'glVertexStream4dATI' => 0,
	'glVertexStream4dvATI' => 1,
	'glNormalStream3bATI' => 0,
	'glNormalStream3bvATI' => 1,
	'glNormalStream3sATI' => 0,
	'glNormalStream3svATI' => 1,
	'glNormalStream3iATI' => 0,
	'glNormalStream3ivATI' => 1,
	'glNormalStream3fATI' => 0,
	'glNormalStream3fvATI' => 1,
	'glNormalStream3dATI' => 0,
	'glNormalStream3dvATI' => 1,
	'glClientActiveVertexStreamATI' => 0,
	'glVertexBlendEnviATI' => 0,
	'glVertexBlendEnvfATI' => 0,
	
	# GL_EXT_secondary_color
	'glSecondaryColor3bEXT' => 0,
	'glSecondaryColor3bvEXT' => 1,
	'glSecondaryColor3dEXT' => 0,
	'glSecondaryColor3dvEXT' => 1,
	'glSecondaryColor3fEXT' => 0,
	'glSecondaryColor3fvEXT' => 1,
	'glSecondaryColor3iEXT' => 0,
	'glSecondaryColor3ivEXT' => 1,
	'glSecondaryColor3sEXT' => 0,
	'glSecondaryColor3svEXT' => 1,
	'glSecondaryColor3ubEXT' => 0,
	'glSecondaryColor3ubvEXT' => 1,
	'glSecondaryColor3uiEXT' => 0,
	'glSecondaryColor3uivEXT' => 1,
	'glSecondaryColor3usEXT' => 0,
	'glSecondaryColor3usvEXT' => 1,
	'glSecondaryColorPointerEXT' => 1,
	
	# GL_EXT_separate_shader_objects
	'glCreateShaderProgramEXT' => 1,
	
	# GL_ATI_vertex_array_object
	'glNewObjectBufferATI' => 1,
	'glArrayObjectATI' => 1,
	'glUpdateObjectBufferATI' => 1,
	'glGetObjectBufferfvATI' => 1,
	'glGetObjectBufferivATI' => 1,
	'glGetArrayObjectfvATI' => 1,
	'glGetArrayObjectivATI' => 1,
	'glGetVariantArrayObjectfvATI' => 1,
	'glGetVariantArrayObjectivATI' => 1,
	'glFreeObjectBufferATI' => 1,
	'glDeleteObjectBufferATI' => 1,
	
	# GL_ATI_vertex_array_object
	'glVertexAttribArrayObjectATI' => 1,
	'glGetVertexAttribArrayObjectfvATI' => 1,
	'glGetVertexAttribArrayObjectivATI' => 1,
	
	# GL_ARB_shader_objects
	'glDeleteObjectARB' => 0,
	'glGetHandleARB' => 0,
	'glDetachObjectARB' => 0,
	'glCreateShaderObjectARB' => 0,
	'glShaderSourceARB' => 1,
	'glCompileShaderARB' => 0,
	'glCreateProgramObjectARB' => 0,
	'glAttachObjectARB' => 0,
	'glLinkProgramARB' => 0,
	'glUseProgramObjectARB' => 0,
	'glValidateProgramARB' => 0,
	'glUniform1fARB' => 0,
	'glUniform2fARB' => 0,
	'glUniform3fARB' => 0,
	'glUniform4fARB' => 0,
	'glUniform1iARB' => 0,
	'glUniform2iARB' => 0,
	'glUniform3iARB' => 0,
	'glUniform4iARB' => 0,
	'glUniform1fvARB' => 2,
	'glUniform2fvARB' => 2,
	'glUniform3fvARB' => 2,
	'glUniform4fvARB' => 2,
	'glUniform1ivARB' => 2,
	'glUniform2ivARB' => 2,
	'glUniform3ivARB' => 2,
	'glUniform4ivARB' => 2,
	'glUniformMatrix2fvARB' => 2,
	'glUniformMatrix3fvARB' => 2,
	'glUniformMatrix4fvARB' => 2,
	'glGetObjectParameterfvARB' => 2,
	'glGetObjectParameterivARB' => 2,
	'glGetInfoLogARB' => 2,
	'glGetAttachedObjectsARB' => 2,
	'glGetUniformLocationARB' => 2,
	'glGetActiveUniformARB' => 2,
	'glGetUniformfvARB' => 2,
	'glGetUniformivARB' => 2,
	'glGetShaderSourceARB' => 2,
	
	# GL_APPLE_object_purgeable
	'glObjectPurgeableAPPLE' => 0,
	'glObjectUnpurgeableAPPLE' => 0,
	'glGetObjectParameterivAPPLE' => 1,
	
	# GL_NV_occlusion_query
	'glGenOcclusionQueriesNV' => 1,
	'glDeleteOcclusionQueriesNV' => 1,
	'glIsOcclusionQueryNV' => 0,
	'glBeginOcclusionQueryNV' => 0,
	'glEndOcclusionQueryNV' => 0,
	'glGetOcclusionQueryivNV' => 1,
	'glGetOcclusionQueryuivNV' => 1,

	# GL_NV_path_rendering
	'glGenPathsNV' => 0,
	'glDeletePathsNV' => 0,
	'glIsPathNV' => 0,
	'glPathCommandsNV' => 1,
	'glPathCoordsNV' => 1,
	'glPathSubCommandsNV' => 1,
	'glPathSubCoordsNV' => 1,
	'glPathStringNV' => 1,
	'glPathGlyphsNV' => 1,
	'glPathGlyphRangeNV' => 1,
	'glWeightPathsNV' => 1,
	'glCopyPathNV' => 0,
	'glInterpolatePathsNV' => 0,
	'glTransformPathNV' => 1,
	'glPathParameterivNV' => 1,
	'glPathParameteriNV' => 0,
	'glPathParameterfvNV' => 1,
	'glPathParameterfNV' => 0,
	'glPathDashArrayNV' => 1,
	'glPathStencilFuncNV' => 0,
	'glPathStencilDepthOffsetNV' => 0,
	'glStencilFillPathNV' => 0,
	'glStencilStrokePathNV' => 0,
	'glStencilFillPathInstancedNV' => 1,
	'glStencilStrokePathInstancedNV' => 1,
	'glPathCoverDepthFuncNV' => 0,
	'glCoverFillPathNV' => 0,
	'glCoverStrokePathNV' => 0,
	'glCoverFillPathInstancedNV' => 1,
	'glCoverStrokePathInstancedNV' => 1,
	'glGetPathParameterivNV' => 1,
	'glGetPathParameterfvNV' => 1,
	'glGetPathCommandsNV' => 1,
	'glGetPathCoordsNV' => 1,
	'glGetPathDashArrayNV' => 1,
	'glGetPathMetricsNV' => 1,
	'glGetPathMetricRangeNV' => 1,
	'glGetPathSpacingNV' => 1,
	'glGetPathLengthNV' => 0,
	'glIsPointInFillPathNV' => 0,
	'glIsPointInStrokePathNV' => 0,
	'glPointAlongPathNV' => 1,
	'glMatrixLoad3x2fNV' => 1,
	'glMatrixLoad3x3fNV' => 1,
	'glMatrixLoadTranspose3x3fNV' => 1,
	'glMatrixMult3x2fNV' => 1,
	'glMatrixMult3x3fNV' => 1,
	'glMatrixMultTranspose3x3fNV' => 1,
	'glStencilThenCoverFillPathNV' => 0,
	'glStencilThenCoverStrokePathNV' => 0,
	'glStencilThenCoverFillPathInstancedNV' => 1,
	'glStencilThenCoverStrokePathInstancedNV' => 1,
	'glPathGlyphIndexRangeNV' => 1,
	'glPathGlyphIndexArrayNV' => 1,
	'glPathMemoryGlyphIndexArrayNV' => 1,
	'glProgramPathFragmentInputGenNV' => 1,
	'glGetProgramResourcefvNV' => 1,
	'glPathColorGenNV' => 1,
	'glPathTexGenNV' => 1,
	'glPathFogGenNV' => 0,
	'glGetPathColorGenivNV' => 1,
	'glGetPathColorGenfvNV' => 1,
	'glGetPathTexGenivNV' => 1,
	'glGetPathTexGenfvNV' => 1,
	
	# GL_NV_internalformat_sample_query
	'glGetInternalformatSampleivNV' => 1,
	
	# GL_AMD_performance_monitor
	'glGetPerfMonitorGroupsAMD' => 1,
	'glGetPerfMonitorCountersAMD' => 1,
	'glGetPerfMonitorGroupStringAMD' => 1,
	'glGetPerfMonitorCounterStringAMD' => 1,
	'glGetPerfMonitorCounterInfoAMD' => 1,
	'glGenPerfMonitorsAMD' => 1,
	'glDeletePerfMonitorsAMD' => 1,
	'glSelectPerfMonitorCountersAMD' => 1,
	'glBeginPerfMonitorAMD' => 0,
	'glEndPerfMonitorAMD' => 0,
	'glGetPerfMonitorCounterDataAMD' => 1,
	
	# GL_OES_single_precision
	'glClearDepthfOES' => 0,
	'glClipPlanefOES' => 1,
	'glDepthRangefOES' => 0,
	'glFrustumfOES' => 0,
	'glGetClipPlanefOES' => 1,
	'glOrthofOES' => 0,

	# GL_NV_half_float
	'glVertex2hNV' => 0,
	'glVertex2hvNV' => 1,
	'glVertex3hNV' => 0,
	'glVertex3hvNV' => 1,
	'glVertex4hNV' => 0,
	'glVertex4hvNV' => 1,
	'glNormal3hNV' => 0,
	'glNormal3hvNV' => 1,
	'glColor3hNV' => 0,
	'glColor3hvNV' => 1,
	'glColor4hNV' => 0,
	'glColor4hvNV' => 1,
	'glTexCoord1hNV' => 0,
	'glTexCoord1hvNV' => 1,
	'glTexCoord2hNV' => 0,
	'glTexCoord2hvNV' => 1,
	'glTexCoord3hNV' => 0,
	'glTexCoord3hvNV' => 1,
	'glTexCoord4hNV' => 0,
	'glTexCoord4hvNV' => 1,
	'glMultiTexCoord1hNV' => 0,
	'glMultiTexCoord1hvNV' => 1,
	'glMultiTexCoord2hNV' => 0,
	'glMultiTexCoord2hvNV' => 1,
	'glMultiTexCoord3hNV' => 0,
	'glMultiTexCoord3hvNV' => 1,
	'glMultiTexCoord4hNV' => 0,
	'glMultiTexCoord4hvNV' => 1,
	'glFogCoordhNV' => 0,
	'glFogCoordhvNV' => 1,
	'glSecondaryColor3hNV' => 0,
	'glSecondaryColor3hvNV' => 1,
	'glVertexWeighthNV' => 0,
	'glVertexWeighthvNV' => 1,
	'glVertexAttrib1hNV' => 0,
	'glVertexAttrib1hvNV' => 1,
	'glVertexAttrib2hNV' => 0,
	'glVertexAttrib2hvNV' => 1,
	'glVertexAttrib3hNV' => 0,
	'glVertexAttrib3hvNV' => 1,
	'glVertexAttrib4hNV' => 0,
	'glVertexAttrib4hvNV' => 1,
	'glVertexAttribs1hvNV' => 1,
	'glVertexAttribs2hvNV' => 1,
	'glVertexAttribs3hvNV' => 1,
	'glVertexAttribs4hvNV' => 1,
	
	# GL_EXT_histogram
	'glGetHistogramEXT' => 1,
	'glGetHistogramParameterfvEXT' => 1,
	'glGetHistogramParameterivEXT' => 1,
	'glGetMinmaxEXT' => 1,
	'glGetMinmaxParameterfvEXT' => 1,
	'glGetMinmaxParameterivEXT' => 1,
	'glHistogramEXT' => 0,
	'glMinmaxEXT' => 0,
	'glResetHistogramEXT' => 0,
	'glResetMinmaxEXT' => 0,

	# GL_OES_fixed_point
	'glGetFixedvOES' => 1,
	'glGetPixelMapxv' => 1,
	'glPixelMapx' => 1,
	'glTexEnvxvOES' => 1,
	'glGetTexEnvxvOES' => 1,
	'glColor3xvOES' => 1,
	'glColor4xvOES' => 1,
	'glConvolutionParameterxvOES' => 1,
	'glEvalCoord1xvOES' => 1,
	'glEvalCoord2xvOES' => 1,
	'glFogxvOES' => 1,
	'glGetConvolutionParameterxvOES' => 1,
	'glGetHistogramParameterxvOES' => 1,
	'glGetMapxvOES' => 1,
	'glClipPlanexOES' => 1,
	'glFeedbackBufferxOES' => 1,
	'glGetClipPlanexOES' => 1,
	'glGetLightxOES' => 1,
	'glTexGenxvOES' => 1,
	'glGetTexGenxvOES' => 1,
	'glGetTexLevelParameterxvOES' => 1,
	'glTexParameterxvOES' => 1,
	'glGetTexParameterxvOES' => 1,
	'glIndexxvOES' => 1,
	'glLightModelxvOES' => 1,
	'glLightxvOES' => 1,
	'glLoadMatrixxOES' => 1,
	'glLoadTransposeMatrixxOES' => 1,
	'glMaterialxvOES' => 1,
	'glMultMatrixxOES' => 1,
	'glMultTransposeMatrixxOES' => 1,
	'glMultiTexCoord1xvOES' => 1,
	'glMultiTexCoord2xvOES' => 1,
	'glMultiTexCoord3xvOES' => 1,
	'glMultiTexCoord4xvOES' => 1,
	'glNormal3xvOES' => 1,
	'glPointParameterxvOES' => 1,
	'glPrioritizeTexturesxOES' => 1,
	'glRasterPos2xvOES' => 1,
	'glRasterPos3xvOES' => 1,
	'glRasterPos4xvOES' => 1,
	'glRectxvOES' => 1,
	'glTexCoord1xvOES' => 1,
	'glTexCoord2xvOES' => 1,
	'glTexCoord3xvOES' => 1,
	'glTexCoord4xvOES' => 1,
	'glVertex2xvOES' => 1,
	'glVertex3xvOES' => 1,
	'glVertex4xvOES' => 1,
	'glBitmapxOES' => 1,

	# GL_OES_byte_coordinates
	'glMultiTexCoord1bOES' => 0,
	'glMultiTexCoord1bvOES' => 1,
	'glMultiTexCoord2bOES' => 0,
	'glMultiTexCoord2bvOES' => 1,
	'glMultiTexCoord3bOES' => 0,
	'glMultiTexCoord3bvOES' => 1,
	'glMultiTexCoord4bOES' => 0,
	'glMultiTexCoord4bvOES' => 1,
	'glTexCoord1bOES' => 0,
	'glTexCoord1bvOES' => 1,
	'glTexCoord2bOES' => 0,
	'glTexCoord2bvOES' => 1,
	'glTexCoord3bOES' => 0,
	'glTexCoord3bvOES' => 1,
	'glTexCoord4bOES' => 0,
	'glTexCoord4bvOES' => 1,
	'glVertex2bOES' => 0,
	'glVertex2bvOES' => 1,
	'glVertex3bOES' => 0,
	'glVertex3bvOES' => 1,
	'glVertex4bOES' => 0,
	'glVertex4bvOES' => 1,
	
	# GL_APPLE_vertex_array_range
	'glVertexArrayRangeAPPLE' => 1,
	'glFlushVertexArrayRangeAPPLE' => 1,
	'glVertexArrayParameteriAPPLE' => 0,

	# GL_NV_vertex_array_range
	'glFlushVertexArrayRangeNV' => 0,
	'glVertexArrayRangeNV' => 1,
	
	# GL_SGIX_reference_plane
	'glReferencePlaneSGIX' => 1,
	
	# GL_OES_query_matrix
	'glQueryMatrixxOES' => 1,

	# GL_SUN_triangle_list
	'glReplacementCodeuiSUN' => 0,
	'glReplacementCodeusSUN' => 0,
	'glReplacementCodeubSUN' => 0,
	'glReplacementCodeuivSUN' => 1,
	'glReplacementCodeusvSUN' => 1,
	'glReplacementCodeubvSUN' => 1,
	'glReplacementCodePointerSUN' => 1,
	
	# GL_SUN_vertex
	'glColor4ubVertex2fSUN' => 0,
	'glColor4ubVertex2fvSUN' => 1,
	'glColor4ubVertex3fSUN' => 0,
	'glColor4ubVertex3fvSUN' => 1,
	'glColor3fVertex3fSUN' => 0,
	'glColor3fVertex3fvSUN' => 1,
	'glNormal3fVertex3fSUN' => 0,
	'glNormal3fVertex3fvSUN' => 1,
	'glColor4fNormal3fVertex3fSUN' => 0,
	'glColor4fNormal3fVertex3fvSUN' => 1,
	'glTexCoord2fVertex3fSUN' => 0,
	'glTexCoord2fVertex3fvSUN' => 1,
	'glTexCoord4fVertex4fSUN' => 0,
	'glTexCoord4fVertex4fvSUN' => 1,
	'glTexCoord2fColor4ubVertex3fSUN' => 0,
	'glTexCoord2fColor4ubVertex3fvSUN' => 1,
	'glTexCoord2fColor3fVertex3fSUN' => 0,
	'glTexCoord2fColor3fVertex3fvSUN' => 1,
	'glTexCoord2fNormal3fVertex3fSUN' => 0,
	'glTexCoord2fNormal3fVertex3fvSUN' => 1,
	'glTexCoord2fColor4fNormal3fVertex3fSUN' => 0,
	'glTexCoord2fColor4fNormal3fVertex3fvSUN' => 1,
	'glTexCoord4fColor4fNormal3fVertex4fSUN' => 0,
	'glTexCoord4fColor4fNormal3fVertex4fvSUN' => 1,
	'glReplacementCodeuiVertex3fSUN' => 0,
	'glReplacementCodeuiVertex3fvSUN' => 1,
	'glReplacementCodeuiColor4ubVertex3fSUN' => 0,
	'glReplacementCodeuiColor4ubVertex3fvSUN' => 1,
	'glReplacementCodeuiColor3fVertex3fSUN' => 0,
	'glReplacementCodeuiColor3fVertex3fvSUN' => 1,
	'glReplacementCodeuiNormal3fVertex3fSUN' => 0,
	'glReplacementCodeuiNormal3fVertex3fvSUN' => 1,
	'glReplacementCodeuiColor4fNormal3fVertex3fSUN' => 0,
	'glReplacementCodeuiColor4fNormal3fVertex3fvSUN' => 1,
	'glReplacementCodeuiTexCoord2fVertex3fSUN' => 0,
	'glReplacementCodeuiTexCoord2fVertex3fvSUN' => 1,
	'glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN' => 0,
	'glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN' => 1,
	'glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN' => 0,
	'glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN' => 1,

	# GL_ATI_fragment_shader
	'glGenFragmentShadersATI' => 0,
	'glBindFragmentShaderATI' => 0,
	'glDeleteFragmentShaderATI' => 0,
	'glBeginFragmentShaderATI' => 0,
	'glEndFragmentShaderATI' => 0,
	'glPassTexCoordATI' => 0,
	'glSampleMapATI' => 0,
	'glColorFragmentOp1ATI' => 0,
	'glColorFragmentOp2ATI' => 0,
	'glColorFragmentOp3ATI' => 0,
	'glAlphaFragmentOp1ATI' => 0,
	'glAlphaFragmentOp2ATI' => 0,
	'glAlphaFragmentOp3ATI' => 0,
	'glSetFragmentShaderConstantATI' => 1,
	
	# GL_AMD_sample_positions
	'glSetMultisamplefvAMD' => 1,
	
	# GL_SGIX_sprite
	'glSpriteParameterfSGIX' => 0,
	'glSpriteParameterfvSGIX' => 1,
	'glSpriteParameteriSGIX' => 0,
	'glSpriteParameterivSGIX' => 1,
	
	# GL_GREMEDY_string_marker
	'glStringMarkerGREMEDY' => 1,
	
	# GL_SGIS_pixel_texture
	'glPixelTexGenParameteriSGIS' => 0,
	'glPixelTexGenParameterivSGIS' => 1,
	'glPixelTexGenParameterfSGIS' => 0,
	'glPixelTexGenParameterfvSGIS' => 1,
	'glGetPixelTexGenParameterivSGIS' => 1,
	'glGetPixelTexGenParameterfvSGIS' => 1,
	
	# GL_SGIX_instruments
	'glGetInstrumentsSGIX' => 0,
	'glInstrumentsBufferSGIX' => 1,
	'glPollInstrumentsSGIX' => 1,
	'glReadInstrumentsSGIX' => 0,
	'glStartInstrumentsSGIX' => 0,
	'glStopInstrumentsSGIX' => 0,
	
	# GL_EXT_paletted_texture
	'glColorTableEXT' => 1,
	'glGetColorTableEXT' => 1,
	'glGetColorTableParameterivEXT' => 1,
	'glGetColorTableParameterfvEXT' => 1,
	
	# GL_SGI_color_table
	'glColorTableSGI' => 1,
	'glColorTableParameterfvSGI' => 1,
	'glColorTableParameterivSGI' => 1,
	'glCopyColorTableSGI' => 0,
	'glGetColorTableSGI' => 1,
	'glGetColorTableParameterfvSGI' => 1,
	'glGetColorTableParameterivSGI' => 1,

	# GL_EXT_pixel_transform
	'glPixelTransformParameteriEXT' => 0,
	'glPixelTransformParameterfEXT' => 0,
	'glPixelTransformParameterivEXT' => 1,
	'glPixelTransformParameterfvEXT' => 1,
	'glGetPixelTransformParameterivEXT' => 1,
	'glGetPixelTransformParameterfvEXT' => 1,

	# GL_NV_gpu_program4
	'glProgramLocalParameterI4iNV' => 0,
	'glProgramLocalParameterI4ivNV' => 1,
	'glProgramLocalParametersI4ivNV' => 1,
	'glProgramLocalParameterI4uiNV' => 0,
	'glProgramLocalParameterI4uivNV' => 1,
	'glProgramLocalParametersI4uivNV' => 1,
	'glProgramEnvParameterI4iNV' => 0,
	'glProgramEnvParameterI4ivNV' => 1,
	'glProgramEnvParametersI4ivNV' => 1,
	'glProgramEnvParameterI4uiNV' => 0,
	'glProgramEnvParameterI4uivNV' => 1,
	'glProgramEnvParametersI4uivNV' => 1,
	'glGetProgramLocalParameterIivNV' => 1,
	'glGetProgramLocalParameterIuivNV' => 1,
	'glGetProgramEnvParameterIivNV' => 1,
	'glGetProgramEnvParameterIuivNV' => 1,
	
	# GL_NV_gpu_program4
	'glProgramSubroutineParametersuivNV' => 1,
	'glGetProgramSubroutineParameteruivNV' => 1,
	
	# GL_ARB_fragment_program
	'glProgramStringARB' => 1,
	'glBindProgramARB' => 0,
	'glDeleteProgramsARB' => 1,
	'glGenProgramsARB' => 1,
	'glProgramEnvParameter4dARB' => 0,
	'glProgramEnvParameter4dvARB' => 1,
	'glProgramEnvParameter4fARB' => 0,
	'glProgramEnvParameter4fvARB' => 1,
	'glProgramLocalParameter4dARB' => 0,
	'glProgramLocalParameter4dvARB' => 1,
	'glProgramLocalParameter4fARB' => 0,
	'glProgramLocalParameter4fvARB' => 1,
	'glGetProgramEnvParameterdvARB' => 1,
	'glGetProgramEnvParameterfvARB' => 1,
	'glGetProgramLocalParameterdvARB' => 1,
	'glGetProgramLocalParameterfvARB' => 1,
	'glGetProgramivARB' => 1,
	'glGetProgramStringARB' => 1,
	'glIsProgramARB' => 0,
	
	# GL_NV_fragment_program
	'glProgramNamedParameter4fNV' => 1,
	'glProgramNamedParameter4fvNV' => 1,
	'glProgramNamedParameter4dNV' => 1,
	'glProgramNamedParameter4dvNV' => 1,
	'glGetProgramNamedParameterfvNV' => 1,
	'glGetProgramNamedParameterdvNV' => 1,
	
	# GL_NV_vertex_program
	'glAreProgramsResidentNV' => 1,
	'glBindProgramNV' => 0,
	'glDeleteProgramsNV' => 1,
	'glExecuteProgramNV' => 1,
	'glGenProgramsNV' => 1,
	'glGetProgramParameterdvNV' => 1,
	'glGetProgramParameterfvNV' => 1,
	'glGetProgramivNV' => 1,
	'glGetProgramStringNV' => 1,
	'glGetTrackMatrixivNV' => 1,
	'glGetVertexAttribdvNV' => 1,
	'glGetVertexAttribfvNV' => 1,
	'glGetVertexAttribivNV' => 1,
	'glGetVertexAttribPointervNV' => 1,
	'glIsProgramNV' => 0,
	'glLoadProgramNV' => 1,
	'glProgramParameter4dNV' => 0,
	'glProgramParameter4dvNV' => 1,
	'glProgramParameter4fNV' => 0,
	'glProgramParameter4fvNV' => 1,
	'glProgramParameters4dvNV' => 1,
	'glProgramParameters4fvNV' => 1,
	'glRequestResidentProgramsNV' => 1,
	'glTrackMatrixNV' => 0,
	'glVertexAttribPointerNV' => 1,
	'glVertexAttrib1dNV' => 0,
	'glVertexAttrib1dvNV' => 1,
	'glVertexAttrib1fNV' => 0,
	'glVertexAttrib1fvNV' => 1,
	'glVertexAttrib1sNV' => 0,
	'glVertexAttrib1svNV' => 1,
	'glVertexAttrib2dNV' => 0,
	'glVertexAttrib2dvNV' => 1,
	'glVertexAttrib2fNV' => 0,
	'glVertexAttrib2fvNV' => 1,
	'glVertexAttrib2sNV' => 0,
	'glVertexAttrib2svNV' => 1,
	'glVertexAttrib3dNV' => 0,
	'glVertexAttrib3dvNV' => 1,
	'glVertexAttrib3fNV' => 0,
	'glVertexAttrib3fvNV' => 1,
	'glVertexAttrib3sNV' => 0,
	'glVertexAttrib3svNV' => 1,
	'glVertexAttrib4dNV' => 0,
	'glVertexAttrib4dvNV' => 1,
	'glVertexAttrib4fNV' => 0,
	'glVertexAttrib4fvNV' => 1,
	'glVertexAttrib4sNV' => 0,
	'glVertexAttrib4svNV' => 1,
	'glVertexAttrib4ubNV' => 0,
	'glVertexAttrib4ubvNV' => 1,
	'glVertexAttribs1dvNV' => 1,
	'glVertexAttribs1fvNV' => 1,
	'glVertexAttribs1svNV' => 1,
	'glVertexAttribs2dvNV' => 1,
	'glVertexAttribs2fvNV' => 1,
	'glVertexAttribs2svNV' => 1,
	'glVertexAttribs3dvNV' => 1,
	'glVertexAttribs3fvNV' => 1,
	'glVertexAttribs3svNV' => 1,
	'glVertexAttribs4dvNV' => 1,
	'glVertexAttribs4fvNV' => 1,
	'glVertexAttribs4svNV' => 1,
	'glVertexAttribs4ubvNV' => 1,
	
	# GL_NV_vertex_program4
	'glVertexAttribI1iEXT' => 0,
	'glVertexAttribI2iEXT' => 0,
	'glVertexAttribI3iEXT' => 0,
	'glVertexAttribI4iEXT' => 0,
	'glVertexAttribI1uiEXT' => 0,
	'glVertexAttribI2uiEXT' => 0,
	'glVertexAttribI3uiEXT' => 0,
	'glVertexAttribI4uiEXT' => 0,
	'glVertexAttribI1ivEXT' => 1,
	'glVertexAttribI2ivEXT' => 1,
	'glVertexAttribI3ivEXT' => 1,
	'glVertexAttribI4ivEXT' => 1,
	'glVertexAttribI1uivEXT' => 1,
	'glVertexAttribI2uivEXT' => 1,
	'glVertexAttribI3uivEXT' => 1,
	'glVertexAttribI4uivEXT' => 1,
	'glVertexAttribI4bvEXT' => 1,
	'glVertexAttribI4svEXT' => 1,
	'glVertexAttribI4ubvEXT' => 1,
	'glVertexAttribI4usvEXT' => 1,
	'glVertexAttribIPointerEXT' => 1,
	'glGetVertexAttribIivEXT' => 1,
	'glGetVertexAttribIuivEXT' => 1,
	
	# GL_EXT_vertex_attrib_64bit
	'glVertexAttribL1dEXT' => 0,
	'glVertexAttribL2dEXT' => 0,
	'glVertexAttribL3dEXT' => 0,
	'glVertexAttribL4dEXT' => 0,
	'glVertexAttribL1dvEXT' => 1,
	'glVertexAttribL2dvEXT' => 1,
	'glVertexAttribL3dvEXT' => 1,
	'glVertexAttribL4dvEXT' => 1,
	'glVertexAttribLPointerEXT' => 1,
	'glGetVertexAttribLdvEXT' => 1,
	
	# GL_NV_vertex_attrib_integer_64bit
	'glVertexAttribL1i64NV' => 0,
	'glVertexAttribL2i64NV' => 0,
	'glVertexAttribL3i64NV' => 0,
	'glVertexAttribL4i64NV' => 0,
	'glVertexAttribL1i64vNV' => 1,
	'glVertexAttribL2i64vNV' => 1,
	'glVertexAttribL3i64vNV' => 1,
	'glVertexAttribL4i64vNV' => 1,
	'glVertexAttribL1ui64NV' => 0,
	'glVertexAttribL2ui64NV' => 0,
	'glVertexAttribL3ui64NV' => 0,
	'glVertexAttribL4ui64NV' => 0,
	'glVertexAttribL1ui64vNV' => 1,
	'glVertexAttribL2ui64vNV' => 1,
	'glVertexAttribL3ui64vNV' => 1,
	'glVertexAttribL4ui64vNV' => 1,
	'glGetVertexAttribLi64vNV' => 1,
	'glGetVertexAttribLui64vNV' => 1,
	'glVertexAttribLFormatNV' => 0,
	
	# GL_NV_vertex_buffer_unified_memory
	'glBufferAddressRangeNV' => 0,
	'glVertexFormatNV' => 0,
	'glNormalFormatNV' => 0,
	'glColorFormatNV' => 0,
	'glIndexFormatNV' => 0,
	'glTexCoordFormatNV' => 0,
	'glEdgeFlagFormatNV' => 0,
	'glSecondaryColorFormatNV' => 0,
	'glFogCoordFormatNV' => 0,
	'glVertexAttribFormatNV' => 0,
	'glVertexAttribIFormatNV' => 0,
	'glGetIntegerui64i_vNV' => 1,

	# GL_ARB_bindless_texture
	'glGetTextureHandleARB' => 1,
	'glGetTextureSamplerHandleARB' => 1,
	'glMakeTextureHandleResidentARB' => 0,
	'glMakeTextureHandleNonResidentARB' => 0,
	'glGetImageHandleARB' => 1,
	'glMakeImageHandleResidentARB' => 0,
	'glMakeImageHandleNonResidentARB' => 0,
	'glUniformHandleui64ARB' => 0,
	'glUniformHandleui64vARB' => 1,
	'glProgramUniformHandleui64ARB' => 0,
	'glProgramUniformHandleui64vARB' => 1,
	'glIsTextureHandleResidentARB' => 0,
	'glIsImageHandleResidentARB' => 0,
	'glVertexAttribL1ui64ARB' => 0,
	'glVertexAttribL1ui64vARB' => 1,
	'glGetVertexAttribLui64vARB' => 1,
	
	# GL_NV_bindless_texture
	'glGetTextureHandleNV' => 1,
	'glGetTextureSamplerHandleNV' => 1,
	'glMakeTextureHandleResidentNV' => 0,
	'glMakeTextureHandleNonResidentNV' => 0,
	'glGetImageHandleNV' => 1,
	'glMakeImageHandleResidentNV' => 0,
	'glMakeImageHandleNonResidentNV' => 0,
	'glUniformHandleui64NV' => 0,
	'glUniformHandleui64vNV' => 1,
	'glProgramUniformHandleui64NV' => 0,
	'glProgramUniformHandleui64vNV' => 1,
	'glIsTextureHandleResidentNV' => 0,
	'glIsImageHandleResidentNV' => 0,

	# GL_NV_shader_buffer_load
	'glMakeBufferResidentNV' => 0,
	'glMakeBufferNonResidentNV' => 0,
	'glIsBufferResidentNV' => 0,
	'glMakeNamedBufferResidentNV' => 0,
	'glMakeNamedBufferNonResidentNV' => 0,
	'glIsNamedBufferResidentNV' => 0,
	'glGetBufferParameterui64vNV' => 1,
	'glGetNamedBufferParameterui64vNV' => 1,
	'glGetIntegerui64vNV' => 1,
	'glUniformui64NV' => 0,
	'glUniformui64vNV' => 1,
	'glProgramUniformui64NV' => 0,
	'glProgramUniformui64vNV' => 1,

	# GL_ARB_matrix_palette
	'glCurrentPaletteMatrixARB' => 0,
	'glMatrixIndexubvARB' => 1,
	'glMatrixIndexusvARB' => 1,
	'glMatrixIndexuivARB' => 1,
	'glMatrixIndexPointerARB' => 1,
	
	# GL_ARB_vertex_shader
	'glBindAttribLocationARB' => 1,
	'glGetActiveAttribARB' => 1,
	'glGetAttribLocationARB' => 1,

	# GL_ARB_vertex_program
	'glVertexAttrib1dARB' => 0,
	'glVertexAttrib1dvARB' => 1,
	'glVertexAttrib1fARB' => 0,
	'glVertexAttrib1fvARB' => 1,
	'glVertexAttrib1sARB' => 0,
	'glVertexAttrib1svARB' => 1,
	'glVertexAttrib2dARB' => 0,
	'glVertexAttrib2dvARB' => 1,
	'glVertexAttrib2fARB' => 0,
	'glVertexAttrib2fvARB' => 1,
	'glVertexAttrib2sARB' => 0,
	'glVertexAttrib2svARB' => 1,
	'glVertexAttrib3dARB' => 0,
	'glVertexAttrib3dvARB' => 1,
	'glVertexAttrib3fARB' => 0,
	'glVertexAttrib3fvARB' => 1,
	'glVertexAttrib3sARB' => 0,
	'glVertexAttrib3svARB' => 1,
	'glVertexAttrib4NbvARB' => 1,
	'glVertexAttrib4NivARB' => 1,
	'glVertexAttrib4NsvARB' => 1,
	'glVertexAttrib4NubARB' => 0,
	'glVertexAttrib4NubvARB' => 1,
	'glVertexAttrib4NuivARB' => 1,
	'glVertexAttrib4NusvARB' => 1,
	'glVertexAttrib4bvARB' => 1,
	'glVertexAttrib4dARB' => 0,
	'glVertexAttrib4dvARB' => 1,
	'glVertexAttrib4fARB' => 0,
	'glVertexAttrib4fvARB' => 1,
	'glVertexAttrib4ivARB' => 1,
	'glVertexAttrib4sARB' => 0,
	'glVertexAttrib4svARB' => 1,
	'glVertexAttrib4ubvARB' => 1,
	'glVertexAttrib4uivARB' => 1,
	'glVertexAttrib4usvARB' => 1,
	'glVertexAttribPointerARB' => 1,
	'glEnableVertexAttribArrayARB' => 0,
	'glDisableVertexAttribArrayARB' => 0,
	'glGetVertexAttribdvARB' => 1,
	'glGetVertexAttribfvARB' => 1,
	'glGetVertexAttribivARB' => 1,
	'glGetVertexAttribPointervARB' => 1,
	
	# GL_NV_video_capture
	'glBeginVideoCaptureNV' => 0,
	'glBindVideoCaptureStreamBufferNV' => 0,
	'glBindVideoCaptureStreamTextureNV' => 0,
	'glEndVideoCaptureNV' => 0,
	'glGetVideoCaptureivNV' => 1,
	'glGetVideoCaptureStreamivNV' => 1,
	'glGetVideoCaptureStreamfvNV' => 1,
	'glGetVideoCaptureStreamdvNV' => 1,
	'glVideoCaptureNV' => 1,
	'glVideoCaptureStreamParameterivNV' => 1,
	'glVideoCaptureStreamParameterfvNV' => 1,
	'glVideoCaptureStreamParameterdvNV' => 1,
	
	# GL_NV_present_video
	'glPresentFrameKeyedNV' => 0,
	'glPresentFrameDualFillNV' => 0,
	'glGetVideoivNV' => 1,
	'glGetVideouivNV' => 1,
	'glGetVideoi64vNV' => 1,
	'glGetVideoui64vNV' => 1,
	
	# GL_NV_vdpau_interop
	'glVDPAUInitNV' => 1,
	'glVDPAUFiniNV' => 1,
	'glVDPAURegisterVideoSurfaceNV' => 1,
	'glVDPAURegisterOutputSurfaceNV' => 1,
	'glVDPAUIsSurfaceNV' => 1,
	'glVDPAUUnregisterSurfaceNV' => 1,
	'glVDPAUGetSurfaceivNV' => 1,
	'glVDPAUSurfaceAccessNV' => 1,
	'glVDPAUMapSurfacesNV' => 1,
	'glVDPAUUnmapSurfacesNV' => 1,
	
	# GL_MESA_program_debug
	'glProgramCallbackMESA' => 1,
	'glGetProgramRegisterfvMESA' => 1,
	
	# GL_MESA_shader_debug
	'glCreateDebugObjectMESA' => 0,
	'glClearDebugLogMESA' => 0,
	'glGetDebugLogMESA' => 1,
	'glGetDebugLogLengthMESA' => 0,

	# GL_EXT_debug_label
	'glLabelObjectEXT' => 1,
	'glGetObjectLabelEXT' => 1,

	# GL_EXT_timer_query
	'glGetQueryObjecti64vEXT' => 1,
	'glGetQueryObjectui64vEXT' => 1,

	# GL_ARB_occlusion_query
	'glGenQueriesARB' => 1,
	'glDeleteQueriesARB' => 1,
	'glIsQueryARB' => 0,
	'glBeginQueryARB' => 0,
	'glEndQueryARB' => 0,
	'glGetQueryivARB' => 1,
	'glGetQueryObjectivARB' => 1,
	'glGetQueryObjectuivARB' => 1,

	# GL_EXT_convolution
	'glConvolutionFilter1DEXT' => 1,
	'glConvolutionFilter2DEXT' => 1,
	'glConvolutionParameterfEXT' => 0,
	'glConvolutionParameterfvEXT' => 1,
	'glConvolutionParameteriEXT' => 0,
	'glConvolutionParameterivEXT' => 1,
	'glCopyConvolutionFilter1DEXT' => 0,
	'glCopyConvolutionFilter2DEXT' => 0,
	'glGetConvolutionFilterEXT' => 1,
	'glGetConvolutionParameterfvEXT' => 1,
	'glGetConvolutionParameterivEXT' => 1,
	'glGetSeparableFilterEXT' => 1,
	'glSeparableFilter2DEXT' => 1,

	# GL_ARB_texture_compression
	'glCompressedTexImage1DARB' => 1,
	'glCompressedTexImage2DARB' => 1,
	'glCompressedTexImage3DARB' => 1,
	'glCompressedTexSubImage1DARB' => 1,
	'glCompressedTexSubImage2DARB' => 1,
	'glCompressedTexSubImage3DARB' => 1,
	'glGetCompressedTexImageARB' => 1,
	
	# GL_SGIS_sharpen_texture
	'glSharpenTexFuncSGIS' => 1,
	'glGetSharpenTexFuncSGIS' => 1,
	
	# GL_ATI_envmap_bumpmap
	'glTexBumpParameterivATI' => 1,
	'glTexBumpParameterfvATI' => 1,
	'glGetTexBumpParameterivATI' => 1,
	'glGetTexBumpParameterfvATI' => 1,
	
	# GL_SGIS_texture_filter4
	'glGetTexFilterFuncSGIS' => 1,
	'glTexFilterFuncSGIS' => 1,
	
	# GL_EXT_subtexture
	'glTexSubImage1DEXT' => 1,
	'glTexSubImage2DEXT' => 1,
	
	# GL_EXT_texture_object
	'glAreTexturesResidentEXT' => 1,
	'glBindTextureEXT' => 0,
	'glDeleteTexturesEXT' => 1,
	'glGenTexturesEXT' => 1,
	'glIsTextureEXT' => 0,
	'glPrioritizeTexturesEXT' => 1,

	# GL_EXT_texture3D
	'glTexImage3DEXT' => 1,
	'glTexSubImage3DEXT' => 1,

	# GL_SGIS_texture4D
	'glTexImage4DSGIS' => 1,
	'glTexSubImage4DSGIS' => 1,
	
	# GL_EXT_texture_integer
	'glTexParameterIivEXT' => 1,
	'glTexParameterIuivEXT' => 1,
	'glGetTexParameterIivEXT' => 1,
	'glGetTexParameterIuivEXT' => 1,
	'glClearColorIiEXT' => 0,
	'glClearColorIuiEXT' => 0,
	
	# GL_APPLE_texture_range
	'glTextureRangeAPPLE' => 1,
	'glGetTexParameterPointervAPPLE' => 1,
	
	# GL_EXT_transform_feedback
	'glBeginTransformFeedbackEXT' => 0,
	'glEndTransformFeedbackEXT' => 0,
	'glBindBufferRangeEXT' => 1,
	'glBindBufferOffsetEXT' => 0,
	'glBindBufferBaseEXT' => 1,
	'glTransformFeedbackVaryingsEXT' => 1,
	'glGetTransformFeedbackVaryingEXT' => 1,

	# GL_NV_transform_feedback
	'glBeginTransformFeedbackNV' => 0,
	'glEndTransformFeedbackNV' => 0,
	'glTransformFeedbackAttribsNV' => 1,
	'glBindBufferRangeNV' => 1,
	'glBindBufferOffsetNV' => 0,
	'glBindBufferBaseNV' => 1,
	'glTransformFeedbackVaryingsNV' => 1,
	'glActiveVaryingNV' => 1,
	'glGetVaryingLocationNV' => 1,
	'glGetActiveVaryingNV' => 1,
	'glGetTransformFeedbackVaryingNV' => 1,
	'glTransformFeedbackStreamAttribsNV' => 1,
	
	# GL_NV_transform_feedback2
	'glBindTransformFeedbackNV' => 0,
	'glDeleteTransformFeedbacksNV' => 1,
	'glGenTransformFeedbacksNV' => 1,
	'glIsTransformFeedbackNV' => 0,
	'glPauseTransformFeedbackNV' => 0,
	'glResumeTransformFeedbackNV' => 0,
	'glDrawTransformFeedbackNV' => 0,
	
	# GL_ARB_robustness
	'glGetGraphicsResetStatusARB' => 0,
	'glGetnTexImageARB' => 1,
	'glReadnPixelsARB' => 1,
	'glGetnCompressedTexImageARB' => 1,
	'glGetnUniformfvARB' => 1,
	'glGetnUniformivARB' => 1,
	'glGetnUniformuivARB' => 1,
	'glGetnUniformdvARB' => 1,
	'glGetnMapdvARB' => 1,
	'glGetnMapfvARB' => 1,
	'glGetnMapivARB' => 1,
	'glGetnPixelMapfvARB' => 1,
	'glGetnPixelMapuivARB' => 1,
	'glGetnPixelMapusvARB' => 1,
	'glGetnPolygonStippleARB' => 1,
	'glGetnColorTableARB' => 1,
	'glGetnConvolutionFilterARB' => 1,
	'glGetnSeparableFilterARB' => 1,
	'glGetnHistogramARB' => 1,
	'glGetnMinmaxARB' => 1,

	# GL_SGIX_igloo_interface
	'glIglooInterfaceSGIX' => 1,
	
	# GL_SGIX_list_priority
	'glGetListParameterfvSGIX' => 1,
	'glGetListParameterivSGIX' => 1,
	'glListParameterfSGIX' => 0,
	'glListParameterfvSGIX' => 1,
	'glListParameteriSGIX' => 0,
	'glListParameterivSGIX' => 1,
	
	# GL_HP_image_transform
	'glImageTransformParameteriHP' => 0,
	'glImageTransformParameterfHP' => 0,
	'glImageTransformParameterivHP' => 1,
	'glImageTransformParameterfvHP' => 1,
	'glGetImageTransformParameterivHP' => 1,
	'glGetImageTransformParameterfvHP' => 1,
	
	# GL_APPLE_vertex_program_evaluators
	'glEnableVertexAttribAPPLE' => 0,
	'glDisableVertexAttribAPPLE' => 0,
	'glIsVertexAttribEnabledAPPLE' => 0,
	'glMapVertexAttrib1dAPPLE' => 1,
	'glMapVertexAttrib1fAPPLE' => 1,
	'glMapVertexAttrib2dAPPLE' => 1,
	'glMapVertexAttrib2fAPPLE' => 1,
	
	# GL_EXT_multi_draw_arrays
	'glMultiDrawArraysEXT' => 1,
	'glMultiDrawElementsEXT' => 1,

	# GL_AMD_multi_draw_indirect
	'glMultiDrawArraysIndirectAMD' => 1,
	'glMultiDrawElementsIndirectAMD' => 1,
	
	# GL_NV_bindless_multi_draw_indirect
	'glMultiDrawArraysIndirectBindlessNV' => 1,
	'glMultiDrawElementsIndirectBindlessNV' => 1,
	
	# GL_IBM_multimode_draw_arrays
	'glMultiModeDrawArraysIBM' => 1,
	'glMultiModeDrawElementsIBM' => 1,
	
	# GL_NV_pixel_data_range
	'glPixelDataRangeNV' => 1,
	'glFlushPixelDataRangeNV' => 0,
	
	# GL_ARB_point_parameters
	'glPointParameterfARB' => 0,
	'glPointParameterfvARB' => 1,

	# GL_EXT_point_parameters
	'glPointParameterfEXT' => 0,
	'glPointParameterfvEXT' => 1,
	
	# GL_SGIS_point_parameters
	'glPointParameterfSGIS' => 0,
	'glPointParameterfvSGIS' => 1,
	
	# GL_NV_point_sprite
	'glPointParameteriNV' => 0,
	'glPointParameterivNV' => 1,
	
	# GL_APPLE_vertex_point_size
	'glPointSizePointerAPPLE' => 1,
	'glVertexPointSizefAPPLE' => 0,

	# GL_SGIX_async
	'glAsyncMarkerSGIX' => 0,
	'glFinishAsyncSGIX' => 1,
	'glPollAsyncSGIX' => 1,
	'glGenAsyncMarkersSGIX' => 0,
	'glDeleteAsyncMarkersSGIX' => 0,
	'glIsAsyncMarkerSGIX' => 0,

	# GL_NV_parameter_buffer_object
	'glProgramBufferParametersfvNV' => 1,
	'glProgramBufferParametersIivNV' => 1,
	'glProgramBufferParametersIuivNV' => 1,
	
	# GL_EXT_gpu_program_parameters
	'glProgramEnvParameters4fvEXT' => 1,
	'glProgramLocalParameters4fvEXT' => 1,

	# GL_MESA_trace
	'glEnableTraceMESA' => 0,
	'glDisableTraceMESA' => 0,
	'glNewTraceMESA' => 1,
	'glEndTraceMESA' => 0,
	'glTraceAssertAttribMESA' => 0,
	'glTraceCommentMESA' => 1,
	'glTraceTextureMESA' => 1,
	'glTraceListMESA' => 1,
	'glTracePointerMESA' => 1,
	'glTracePointerRangeMESA' => 1,

	# GL_EXT_vertex_weighting
	'glVertexWeightfEXT' => 0,
	'glVertexWeightfvEXT' => 1,
	'glVertexWeightPointerEXT' => 1,
	
	# GL_ARB_vertex_buffer_object
	'glBindBufferARB' => 1,
	'glDeleteBuffersARB' => 1,
	'glGenBuffersARB' => 1,
	'glIsBufferARB' => 0,
	'glBufferDataARB' => 1,
	'glBufferSubDataARB' => 1,
	'glGetBufferSubDataARB' => 1,
	'glMapBufferARB' => 1,
	'glUnmapBufferARB' => 0,
	'glGetBufferParameterivARB' => 1,
	'glGetBufferPointervARB' => 1,

	# GL_ARB_vertex_blend
	'glWeightbvARB' => 1,
	'glWeightsvARB' => 1,
	'glWeightivARB' => 1,
	'glWeightfvARB' => 1,
	'glWeightdvARB' => 1,
	'glWeightubvARB' => 1,
	'glWeightusvARB' => 1,
	'glWeightuivARB' => 1,
	'glWeightPointerARB' => 1,
	'glVertexBlendARB' => 0,

	# GL_ARB_sparse_buffer
	'glBufferPageCommitmentARB' => 0,
	'glNamedBufferPageCommitmentEXT' => 0,
	'glNamedBufferPageCommitmentARB' => 0,
	
	# GL_KHR_blend_equation_advanced
	'glBlendBarrierKHR' => 0,
	
	# GL_EXT_polygon_offset_clamp
	'glPolygonOffsetClampEXT' => 0,
	
	# GL_EXT_raster_multisample
	'glRasterSamplesEXT' => 0,
	
	# GL_NV_bindless_multi_draw_indirect_count
	'glMultiDrawArraysIndirectBindlessCountNV' => 1,
	'glMultiDrawElementsIndirectBindlessCountNV' => 1,
	
	# GL_NV_command_list
	'glCreateStatesNV' => 1,
	'glDeleteStatesNV' => 1,
	'glIsStateNV' => 0,
	'glStateCaptureNV' => 0,
	'glGetCommandHeaderNV' => 0,
	'glGetStageIndexNV' => 0,
	'glDrawCommandsNV' => 1,
	'glDrawCommandsAddressNV' => 1,
	'glDrawCommandsStatesNV' => 1,
	'glDrawCommandsStatesAddressNV' => 1,
	'glCreateCommandListsNV' => 1,
	'glDeleteCommandListsNV' => 1,
	'glIsCommandListNV' => 0,
	'glListDrawCommandsStatesClientNV' => 1,
	'glCommandListSegmentsNV' => 0,
	'glCompileCommandListNV' => 0,
	'glCallCommandListNV' => 0,
	
	# GL_NV_conservative_raster
	'glSubpixelPrecisionBiasNV' => 0,
	
	# GL_NV_fragment_coverage_to_color
	'glFragmentCoverageColorNV' => 0,
	
	# GL_NV_framebuffer_mixed_samples
	'glCoverageModulationTableNV' => 1,
	'glGetCoverageModulationTableNV' => 1,
	'glCoverageModulationNV' => 0,
	
	# GL_NV_sample_locations
	'glFramebufferSampleLocationsfvNV' => 1,
	'glNamedFramebufferSampleLocationsfvNV' => 1,
	'glResolveDepthValuesNV' => 0,
	
	# GL_OVR_multiview
	'glFramebufferTextureMultiviewOVR' => 0,

	# ARB_indirect_parameters
	'glMultiDrawArraysIndirectCountARB' => 1,
	'glMultiDrawElementsIndirectCountARB' => 1,
	
	# GL_ARB_gl_spirv
	'glSpecializeShaderARB' => 0,
	
	# GL_ARB_ES3_2_compatibility
	'glPrimitiveBoundingBoxARB' => 0,
	
	# GL_ARB_gpu_shader_int64
	'glUniform1i64ARB' => 0,
	'glUniform2i64ARB' => 0,
	'glUniform3i64ARB' => 0,
	'glUniform4i64ARB' => 0,
	'glUniform1i64vARB' => 0,
	'glUniform2i64vARB' => 0,
	'glUniform3i64vARB' => 0,
	'glUniform4i64vARB' => 0,
	'glUniform1ui64ARB' => 0,
	'glUniform2ui64ARB' => 0,
	'glUniform3ui64ARB' => 0,
	'glUniform4ui64ARB' => 0,
	'glUniform1ui64vARB' => 0,
	'glUniform2ui64vARB' => 0,
	'glUniform3ui64vARB' => 0,
	'glUniform4ui64vARB' => 0,
	'glGetUniformi64vARB' => 1,
	'glGetUniformui64vARB' => 1,
	'glGetnUniformi64vARB' => 1,
	'glGetnUniformui64vARB' => 1,
	'glProgramUniform1i64ARB' => 0,
	'glProgramUniform2i64ARB' => 0,
	'glProgramUniform3i64ARB' => 0,
	'glProgramUniform4i64ARB' => 0,
	'glProgramUniform1i64vARB' => 0,
	'glProgramUniform2i64vARB' => 0,
	'glProgramUniform3i64vARB' => 0,
	'glProgramUniform4i64vARB' => 0,
	'glProgramUniform1ui64ARB' => 0,
	'glProgramUniform2ui64ARB' => 0,
	'glProgramUniform3ui64ARB' => 0,
	'glProgramUniform4ui64ARB' => 0,
	'glProgramUniform1ui64vARB' => 0,
	'glProgramUniform2ui64vARB' => 0,
	'glProgramUniform3ui64vARB' => 0,
	'glProgramUniform4ui64vARB' => 0,
	
	# GL_ARB_parallel_shader_compile
	'glMaxShaderCompilerThreadsARB' => 0,
	
	# GL_ARB_sample_locations
	'glFramebufferSampleLocationsfvARB' => 0,
	'glNamedFramebufferSampleLocationsfvARB' => 0,
	'glEvaluateDepthValuesARB' => 0,
	
	# GL_KHR_parallel_shader_compile
	'glMaxShaderCompilerThreadsKHR' => 0,
	
	# GL_EXT_external_buffer
	'glBufferStorageExternalEXT' => 1,
	'glNamedBufferStorageExternalEXT' => 1,
	
	# GL_EXT_memory_object
	'glGetUnsignedBytevEXT' => 0,
	'glGetUnsignedBytei_vEXT' => 0,
	'glDeleteMemoryObjectsEXT' => 0,
	'glIsMemoryObjectEXT' => 0,
	'glCreateMemoryObjectsEXT' => 0,
	'glMemoryObjectParameterivEXT' => 0,
	'glGetMemoryObjectParameterivEXT' => 0,
	'glTexStorageMem2DEXT' => 0,
	'glTexStorageMem2DMultisampleEXT' => 0,
	'glTexStorageMem3DEXT' => 0,
	'glTexStorageMem3DMultisampleEXT' => 0,
	'glBufferStorageMemEXT' => 0,
	'glTextureStorageMem2DEXT' => 0,
	'glTextureStorageMem2DMultisampleEXT' => 0,
	'glTextureStorageMem3DEXT' => 0,
	'glTextureStorageMem3DMultisampleEXT' => 0,
	'glNamedBufferStorageMemEXT' => 0,
	'glTexStorageMem1DEXT' => 0,
	'glTextureStorageMem1DEXT' => 0,
	
	# GL_EXT_memory_object_fd
	'glImportMemoryFdEXT' => 0,
	
	# GL_EXT_memory_object_win32
	'glImportMemoryWin32HandleEXT' => 1,
	'glImportMemoryWin32NameEXT' => 0,

	# GL_EXT_semaphore
	'glGenSemaphoresEXT' => 0,
	'glDeleteSemaphoresEXT' => 0,
	'glIsSemaphoreEXT' => 0,
	'glSemaphoreParameterui64vEXT' => 0,
	'glGetSemaphoreParameterui64vEXT' => 0,
	'glWaitSemaphoreEXT' => 0,
	'glSignalSemaphoreEXT' => 0,
	
	# GL_EXT_semaphore_fd
	'glImportSemaphoreFdEXT' => 0,
	
	# GL_EXT_semaphore_win32
	'glImportSemaphoreWin32HandleEXT' => 1,
	'glImportSemaphoreWin32NameEXT' => 0,
	
	# GL_EXT_win32_keyed_mutex
	'glAcquireKeyedMutexWin32EXT' => 0,
	'glReleaseKeyedMutexWin32EXT' => 0,
	
	# GL_NVX_linked_gpu_multicast
	'glLGPUNamedBufferSubDataNVX' => 0,
	'glLGPUCopyImageSubDataNVX' => 0,
	'glLGPUInterlockNVX' => 0,
	
	# GL_NV_alpha_to_coverage_dither_control
	'glAlphaToCoverageDitherControlNV' => 0,

	# GL_NV_draw_vulkan_image
	'glDrawVkImageNV' => 0,
	'glGetVkProcAddrNV' => 0,
	'glWaitVkSemaphoreNV' => 0,
	'glSignalVkSemaphoreNV' => 0,
	'glSignalVkFenceNV' => 0,

	# GL_NV_gpu_multicast
	'glRenderGpuMaskNV' => 0,
	'glMulticastBufferSubDataNV' => 0,
	'glMulticastCopyBufferSubDataNV' => 0,
	'glMulticastCopyImageSubDataNV' => 0,
	'glMulticastBlitFramebufferNV' => 0,
	'glMulticastFramebufferSampleLocationsfvNV' => 0,
	'glMulticastBarrierNV' => 0,
	'glMulticastWaitSyncNV' => 0,
	'glMulticastGetQueryObjectivNV' => 0,
	'glMulticastGetQueryObjectuivNV' => 0,
	'glMulticastGetQueryObjecti64vNV' => 0,
	'glMulticastGetQueryObjectui64vNV' => 0,

	# GL_NV_query_resource
	'glQueryResourceNV' => 0,

	# GL_NV_query_resource_tag
	'glGenQueryResourceTagNV' => 0,
	'glDeleteQueryResourceTagNV' => 0,
	'glQueryResourceTagNV' => 0,

	# GL_AMD_framebuffer_sample_positions 1
	'glFramebufferSamplePositionsfvAMD' => 0,
	'glNamedFramebufferSamplePositionsfvAMD' => 0,
	'glGetFramebufferParameterfvAMD' => 0,
	'glGetNamedFramebufferParameterfvAMD' => 0,

	# GL_EXT_window_rectangles
	'glWindowRectanglesEXT' => 0,

	# GL_INTEL_framebuffer_CMAA
	'glApplyFramebufferAttachmentCMAAINTEL' => 0,

	# GL_NV_viewport_swizzle
	'glViewportSwizzleNV' => 0,

	# GL_NV_clip_space_w_scaling
	'glViewportPositionWScaleNV' => 0,

	# GL_NV_conservative_raster_dilate
	'glConservativeRasterParameterfNV' => 0,

	# GL_NV_conservative_raster_pre_snap_triangles
	'glConservativeRasterParameteriNV' => 0,
	
	# Version 1.1
	'glAccum' => 0,
	'glAlphaFunc' => 0,
	'glAreTexturesResident' => 1,
	'glArrayElement' => 1,
	'glBegin' => 0,
	'glBindTexture' => 0,
	'glBitmap' => 1,
	'glBlendFunc' => 0,
	'glCallList' => 0,
	'glCallLists' => 1,
	'glClear' => 0,
	'glClearAccum' => 0,
	'glClearColor' => 0,
	'glClearDepth' => 0,
	'glClearIndex' => 0,
	'glClearStencil' => 0,
	'glClipPlane' => 1,
	'glColor3b' => 0,
	'glColor3bv' => 1,
	'glColor3d' => 0,
	'glColor3dv' => 1,
	'glColor3f' => 0,
	'glColor3fv' => 1,
	'glColor3i' => 0,
	'glColor3iv' => 1,
	'glColor3s' => 0,
	'glColor3sv' => 1,
	'glColor3ub' => 0,
	'glColor3ubv' => 1,
	'glColor3ui' => 0,
	'glColor3uiv' => 1,
	'glColor3us' => 0,
	'glColor3usv' => 1,
	'glColor4b' => 0,
	'glColor4bv' => 1,
	'glColor4d' => 0,
	'glColor4dv' => 1,
	'glColor4f' => 0,
	'glColor4fv' => 1,
	'glColor4i' => 0,
	'glColor4iv' => 1,
	'glColor4s' => 0,
	'glColor4sv' => 1,
	'glColor4ub' => 0,
	'glColor4ubv' => 1,
	'glColor4ui' => 0,
	'glColor4uiv' => 1,
	'glColor4us' => 0,
	'glColor4usv' => 1,
	'glColorMask' => 0,
	'glColorMaterial' => 0,
	'glColorPointer' => 1,
	'glCopyPixels' => 0,
	'glCopyTexImage1D' => 0,
	'glCopyTexImage2D' => 0,
	'glCopyTexSubImage1D' => 0,
	'glCopyTexSubImage2D' => 0,
	'glCullFace' => 0,
	'glDeleteLists' => 0,
	'glDeleteTextures' => 1,
	'glDepthFunc' => 0,
	'glDepthMask' => 0,
	'glDepthRange' => 0,
	'glDisable' => 1,
	'glDisableClientState' => 1,
	'glDrawArrays' => 1,
	'glDrawBuffer' => 0,
	'glDrawElements' => 1,
	'glDrawPixels' => 1,
	'glEdgeFlag' => 0,
	'glEdgeFlagPointer' => 1,
	'glEdgeFlagv' => 1,
	'glEnable' => 1,
	'glEnableClientState' => 1,
	'glEnd' => 0,
	'glEndList' => 0,
	'glEvalCoord1d' => 0,
	'glEvalCoord1dv' => 1,
	'glEvalCoord1f' => 0,
	'glEvalCoord1fv' => 1,
	'glEvalCoord2d' => 0,
	'glEvalCoord2dv' => 1,
	'glEvalCoord2f' => 0,
	'glEvalCoord2fv' => 1,
	'glEvalMesh1' => 0,
	'glEvalMesh2' => 0,
	'glEvalPoint1' => 0,
	'glEvalPoint2' => 0,
	'glFeedbackBuffer' => 1,
	'glFinish' => 1,
	'glFlush' => 1,
	'glFogf' => 0,
	'glFogfv' => 1,
	'glFogi' => 0,
	'glFogiv' => 1,
	'glFrontFace' => 0,
	'glFrustum' => 0,
	'glGenLists' => 0,
	'glGenTextures' => 1,
	'glGetBooleanv' => 1,
	'glGetClipPlane' => 1,
	'glGetDoublev' => 1,
	'glGetError' => 1,
	'glGetFloatv' => 1,
	'glGetIntegerv' => 1,
	'glGetLightfv' => 1,
	'glGetLightiv' => 1,
	'glGetMapdv' => 1,
	'glGetMapfv' => 1,
	'glGetMapiv' => 1,
	'glGetMaterialfv' => 1,
	'glGetMaterialiv' => 1,
	'glGetPixelMapfv' => 1,
	'glGetPixelMapuiv' => 1,
	'glGetPixelMapusv' => 1,
	'glGetPointerv' => 1,
	'glGetPolygonStipple' => 1,
	'glGetString' => 1,
	'glGetTexEnvfv' => 1,
	'glGetTexEnviv' => 1,
	'glGetTexGendv' => 1,
	'glGetTexGenfv' => 1,
	'glGetTexGeniv' => 1,
	'glGetTexImage' => 1,
	'glGetTexLevelParameterfv' => 1,
	'glGetTexLevelParameteriv' => 1,
	'glGetTexParameterfv' => 1,
	'glGetTexParameteriv' => 1,
	'glHint' => 0,
	'glIndexMask' => 0,
	'glIndexPointer' => 1,
	'glIndexd' => 0,
	'glIndexdv' => 1,
	'glIndexf' => 0,
	'glIndexfv' => 1,
	'glIndexi' => 0,
	'glIndexiv' => 1,
	'glIndexs' => 0,
	'glIndexsv' => 1,
	'glIndexub' => 0,
	'glIndexubv' => 1,
	'glInitNames' => 0,
	'glInterleavedArrays' => 1,
	'glIsEnabled' => 1,
	'glIsList' => 0,
	'glIsTexture' => 0,
	'glLightModelf' => 0,
	'glLightModelfv' => 1,
	'glLightModeli' => 0,
	'glLightModeliv' => 1,
	'glLightf' => 0,
	'glLightfv' => 1,
	'glLighti' => 0,
	'glLightiv' => 1,
	'glLineStipple' => 0,
	'glLineWidth' => 0,
	'glListBase' => 0,
	'glLoadIdentity' => 0,
	'glLoadMatrixd' => 1,
	'glLoadMatrixf' => 1,
	'glLoadName' => 0,
	'glLogicOp' => 0,
	'glMap1d' => 1,
	'glMap1f' => 1,
	'glMap2d' => 1,
	'glMap2f' => 1,
	'glMapGrid1d' => 0,
	'glMapGrid1f' => 0,
	'glMapGrid2d' => 0,
	'glMapGrid2f' => 0,
	'glMaterialf' => 0,
	'glMaterialfv' => 1,
	'glMateriali' => 0,
	'glMaterialiv' => 1,
	'glMatrixMode' => 0,
	'glMultMatrixd' => 1,
	'glMultMatrixf' => 1,
	'glNewList' => 0,
	'glNormal3b' => 0,
	'glNormal3bv' => 1,
	'glNormal3d' => 0,
	'glNormal3dv' => 1,
	'glNormal3f' => 0,
	'glNormal3fv' => 1,
	'glNormal3i' => 0,
	'glNormal3iv' => 1,
	'glNormal3s' => 0,
	'glNormal3sv' => 1,
	'glNormalPointer' => 1,
	'glOrtho' => 0,
	'glPassThrough' => 0,
	'glPixelMapfv' => 1,
	'glPixelMapuiv' => 1,
	'glPixelMapusv' => 1,
	'glPixelStoref' => 0,
	'glPixelStorei' => 0,
	'glPixelTransferf' => 0,
	'glPixelTransferi' => 0,
	'glPixelZoom' => 0,
	'glPointSize' => 0,
	'glPolygonMode' => 0,
	'glPolygonOffset' => 0,
	'glPolygonStipple' => 1,
	'glPopAttrib' => 0,
	'glPopClientAttrib' => 0,
	'glPopMatrix' => 0,
	'glPopName' => 0,
	'glPrioritizeTextures' => 1,
	'glPushAttrib' => 0,
	'glPushClientAttrib' => 0,
	'glPushMatrix' => 0,
	'glPushName' => 0,
	'glRasterPos2d' => 0,
	'glRasterPos2dv' => 1,
	'glRasterPos2f' => 0,
	'glRasterPos2fv' => 1,
	'glRasterPos2i' => 0,
	'glRasterPos2iv' => 1,
	'glRasterPos2s' => 0,
	'glRasterPos2sv' => 1,
	'glRasterPos3d' => 0,
	'glRasterPos3dv' => 1,
	'glRasterPos3f' => 0,
	'glRasterPos3fv' => 1,
	'glRasterPos3i' => 0,
	'glRasterPos3iv' => 1,
	'glRasterPos3s' => 0,
	'glRasterPos3sv' => 1,
	'glRasterPos4d' => 0,
	'glRasterPos4dv' => 1,
	'glRasterPos4f' => 0,
	'glRasterPos4fv' => 1,
	'glRasterPos4i' => 0,
	'glRasterPos4iv' => 1,
	'glRasterPos4s' => 0,
	'glRasterPos4sv' => 1,
	'glReadBuffer' => 0,
	'glReadPixels' => 1,
	'glRectd' => 0,
	'glRectdv' => 1,
	'glRectf' => 0,
	'glRectfv' => 1,
	'glRecti' => 0,
	'glRectiv' => 1,
	'glRects' => 0,
	'glRectsv' => 1,
	'glRenderMode' => 1,
	'glRotated' => 0,
	'glRotatef' => 0,
	'glScaled' => 0,
	'glScalef' => 0,
	'glScissor' => 0,
	'glSelectBuffer' => 1,
	'glShadeModel' => 0,
	'glStencilFunc' => 0,
	'glStencilMask' => 0,
	'glStencilOp' => 0,
	'glTexCoord1d' => 0,
	'glTexCoord1dv' => 1,
	'glTexCoord1f' => 0,
	'glTexCoord1fv' => 1,
	'glTexCoord1i' => 0,
	'glTexCoord1iv' => 1,
	'glTexCoord1s' => 0,
	'glTexCoord1sv' => 1,
	'glTexCoord2d' => 0,
	'glTexCoord2dv' => 1,
	'glTexCoord2f' => 0,
	'glTexCoord2fv' => 1,
	'glTexCoord2i' => 0,
	'glTexCoord2iv' => 1,
	'glTexCoord2s' => 0,
	'glTexCoord2sv' => 1,
	'glTexCoord3d' => 0,
	'glTexCoord3dv' => 1,
	'glTexCoord3f' => 0,
	'glTexCoord3fv' => 1,
	'glTexCoord3i' => 0,
	'glTexCoord3iv' => 1,
	'glTexCoord3s' => 0,
	'glTexCoord3sv' => 1,
	'glTexCoord4d' => 0,
	'glTexCoord4dv' => 1,
	'glTexCoord4f' => 0,
	'glTexCoord4fv' => 1,
	'glTexCoord4i' => 0,
	'glTexCoord4iv' => 1,
	'glTexCoord4s' => 0,
	'glTexCoord4sv' => 1,
	'glTexCoordPointer' => 1,
	'glTexEnvf' => 0,
	'glTexEnvfv' => 1,
	'glTexEnvi' => 0,
	'glTexEnviv' => 1,
	'glTexGend' => 0,
	'glTexGendv' => 1,
	'glTexGenf' => 0,
	'glTexGenfv' => 1,
	'glTexGeni' => 0,
	'glTexGeniv' => 1,
	'glTexImage1D' => 1,
	'glTexImage2D' => 1,
	'glTexParameterf' => 0,
	'glTexParameterfv' => 1,
	'glTexParameteri' => 0,
	'glTexParameteriv' => 1,
	'glTexSubImage1D' => 1,
	'glTexSubImage2D' => 1,
	'glTranslated' => 0,
	'glTranslatef' => 0,
	'glVertex2d' => 0,
	'glVertex2dv' => 1,
	'glVertex2f' => 0,
	'glVertex2fv' => 1,
	'glVertex2i' => 0,
	'glVertex2iv' => 1,
	'glVertex2s' => 0,
	'glVertex2sv' => 1,
	'glVertex3d' => 0,
	'glVertex3dv' => 1,
	'glVertex3f' => 0,
	'glVertex3fv' => 1,
	'glVertex3i' => 0,
	'glVertex3iv' => 1,
	'glVertex3s' => 0,
	'glVertex3sv' => 1,
	'glVertex4d' => 0,
	'glVertex4dv' => 1,
	'glVertex4f' => 0,
	'glVertex4fv' => 1,
	'glVertex4i' => 0,
	'glVertex4iv' => 1,
	'glVertex4s' => 0,
	'glVertex4sv' => 1,
	'glVertexPointer' => 1,
	'glViewport' => 0,

	# Version 1.2
	'glBlendColor' => 0,
	'glBlendEquation' => 0,
	'glDrawRangeElements' => 1,
	'glTexImage3D' => 1,
	'glTexSubImage3D' => 1,
	'glCopyTexSubImage3D' => 0,
	'glColorTable' => 1,
	'glColorTableParameterfv' => 1,
	'glColorTableParameteriv' => 1,
	'glCopyColorTable' => 0,
	'glGetColorTable' => 1,
	'glGetColorTableParameterfv' => 1,
	'glGetColorTableParameteriv' => 1,
	'glColorSubTable' => 1,
	'glCopyColorSubTable' => 0,
	'glConvolutionFilter1D' => 1,
	'glConvolutionFilter2D' => 1,
	'glConvolutionParameterf' => 0,
	'glConvolutionParameterfv' => 1,
	'glConvolutionParameteri' => 0,
	'glConvolutionParameteriv' => 1,
	'glCopyConvolutionFilter1D' => 0,
	'glCopyConvolutionFilter2D' => 0,
	'glGetConvolutionFilter' => 1,
	'glGetConvolutionParameterfv' => 1,
	'glGetConvolutionParameteriv' => 1,
	'glGetSeparableFilter' => 1,
	'glSeparableFilter2D' => 1,
	'glGetHistogram' => 1,
	'glGetHistogramParameterfv' => 1,
	'glGetHistogramParameteriv' => 1,
	'glGetMinmax' => 1,
	'glGetMinmaxParameterfv' => 1,
	'glGetMinmaxParameteriv' => 1,
	'glHistogram' => 0,
	'glMinmax' => 0,
	'glResetHistogram' => 0,
	'glResetMinmax' => 0,
	
	# Version 1.3
	'glActiveTexture' => 0,
	'glSampleCoverage' => 0,
	'glSamplePass' => 0,
	'glCompressedTexImage2D' => 1,
	'glCompressedTexImage1D' => 1,
	'glCompressedTexImage3D' => 1,
	'glCompressedTexSubImage1D' => 1,
	'glCompressedTexSubImage2D' => 1,
	'glCompressedTexSubImage3D' => 1,
	'glGetCompressedTexImage' => 1,
	'glClientActiveTexture' => 0,
	'glMultiTexCoord1d' => 0,
	'glMultiTexCoord1dv' => 1,
	'glMultiTexCoord1f' => 0,
	'glMultiTexCoord1fv' => 1,
	'glMultiTexCoord1i' => 0,
	'glMultiTexCoord1iv' => 1,
	'glMultiTexCoord1s' => 0,
	'glMultiTexCoord1sv' => 1,
	'glMultiTexCoord2d' => 0,
	'glMultiTexCoord2dv' => 1,
	'glMultiTexCoord2f' => 0,
	'glMultiTexCoord2fv' => 1,
	'glMultiTexCoord2i' => 0,
	'glMultiTexCoord2iv' => 1,
	'glMultiTexCoord2s' => 0,
	'glMultiTexCoord2sv' => 1,
	'glMultiTexCoord3d' => 0,
	'glMultiTexCoord3dv' => 1,
	'glMultiTexCoord3f' => 0,
	'glMultiTexCoord3fv' => 1,
	'glMultiTexCoord3i' => 0,
	'glMultiTexCoord3iv' => 1,
	'glMultiTexCoord3s' => 0,
	'glMultiTexCoord3sv' => 1,
	'glMultiTexCoord4d' => 0,
	'glMultiTexCoord4dv' => 1,
	'glMultiTexCoord4f' => 0,
	'glMultiTexCoord4fv' => 1,
	'glMultiTexCoord4i' => 0,
	'glMultiTexCoord4iv' => 1,
	'glMultiTexCoord4s' => 0,
	'glMultiTexCoord4sv' => 1,
	'glLoadTransposeMatrixf' => 1,
	'glLoadTransposeMatrixd' => 1,
	'glMultTransposeMatrixf' => 1,
	'glMultTransposeMatrixd' => 1,

	# Version 1.4
	'glBlendFuncSeparate' => 0,
	'glMultiDrawArrays' => 1,
	'glMultiDrawElements' => 1,
	'glPointParameterf' => 0,
	'glPointParameterfv' => 1,
	'glPointParameteri' => 0,
	'glPointParameteriv' => 1,
	'glFogCoordf' => 0,
	'glFogCoordfv' => 1,
	'glFogCoordd' => 0,
	'glFogCoorddv' => 1,
	'glFogCoordPointer' => 1,
	'glSecondaryColor3b' => 0,
	'glSecondaryColor3bv' => 1,
	'glSecondaryColor3d' => 0,
	'glSecondaryColor3dv' => 1,
	'glSecondaryColor3f' => 0,
	'glSecondaryColor3fv' => 1,
	'glSecondaryColor3i' => 0,
	'glSecondaryColor3iv' => 1,
	'glSecondaryColor3s' => 0,
	'glSecondaryColor3sv' => 1,
	'glSecondaryColor3ub' => 0,
	'glSecondaryColor3ubv' => 1,
	'glSecondaryColor3ui' => 0,
	'glSecondaryColor3uiv' => 1,
	'glSecondaryColor3us' => 0,
	'glSecondaryColor3usv' => 1,
	'glSecondaryColorPointer' => 1,
	'glWindowPos2d' => 0,
	'glWindowPos2dv' => 1,
	'glWindowPos2f' => 0,
	'glWindowPos2fv' => 1,
	'glWindowPos2i' => 0,
	'glWindowPos2iv' => 1,
	'glWindowPos2s' => 0,
	'glWindowPos2sv' => 1,
	'glWindowPos3d' => 0,
	'glWindowPos3dv' => 1,
	'glWindowPos3f' => 0,
	'glWindowPos3fv' => 1,
	'glWindowPos3i' => 0,
	'glWindowPos3iv' => 1,
	'glWindowPos3s' => 0,
	'glWindowPos3sv' => 1,
	
	# Version 1.5
	'glGenQueries' => 1,
	'glDeleteQueries' => 1,
	'glIsQuery' => 0,
	'glBeginQuery' => 0,
	'glEndQuery' => 0,
	'glGetQueryiv' => 1,
	'glGetQueryObjectiv' => 1,
	'glGetQueryObjectuiv' => 1,
	'glBindBuffer' => 1,
	'glDeleteBuffers' => 1,
	'glGenBuffers' => 1,
	'glIsBuffer' => 0,
	'glBufferData' => 1,
	'glBufferSubData' => 1,
	'glGetBufferSubData' => 1,
	'glMapBuffer' => 1,
	'glUnmapBuffer' => 0,
	'glGetBufferParameteriv' => 1,
	'glGetBufferPointerv' => 1,
	
	# Version 2.0
	'glBlendEquationSeparate' => 0,
	'glDrawBuffers' => 1,
	'glStencilOpSeparate' => 0,
	'glStencilFuncSeparate' => 0,
	'glStencilMaskSeparate' => 0,
	'glAttachShader' => 0,
	'glBindAttribLocation' => 1,
	'glCompileShader' => 0,
	'glCreateProgram' => 0,
	'glCreateShader' => 0,
	'glDeleteProgram' => 0,
	'glDeleteShader' => 0,
	'glDetachShader' => 0,
	'glDisableVertexAttribArray' => 0,
	'glEnableVertexAttribArray' => 0,
	'glGetActiveAttrib' => 1,
	'glGetActiveUniform' => 1,
	'glGetAttachedShaders' => 1,
	'glGetAttribLocation' => 1,
	'glGetProgramiv' => 1,
	'glGetProgramInfoLog' => 1,
	'glGetShaderiv' => 1,
	'glGetShaderInfoLog' => 1,
	'glGetShaderSource' => 1,
	'glGetUniformLocation' => 1,
	'glGetUniformfv' => 1,
	'glGetUniformiv' => 1,
	'glGetVertexAttribdv' => 1,
	'glGetVertexAttribfv' => 1,
	'glGetVertexAttribiv' => 1,
	'glGetVertexAttribPointerv' => 1,
	'glIsProgram' => 0,
	'glIsShader' => 0,
	'glLinkProgram' => 0,
	'glShaderSource' => 1,
	'glUseProgram' => 0,
	'glUniform1f' => 0,
	'glUniform2f' => 0,
	'glUniform3f' => 0,
	'glUniform4f' => 0,
	'glUniform1i' => 0,
	'glUniform2i' => 0,
	'glUniform3i' => 0,
	'glUniform4i' => 0,
	'glUniform1fv' => 1,
	'glUniform2fv' => 1,
	'glUniform3fv' => 1,
	'glUniform4fv' => 1,
	'glUniform1iv' => 1,
	'glUniform2iv' => 1,
	'glUniform3iv' => 1,
	'glUniform4iv' => 1,
	'glUniformMatrix2fv' => 1,
	'glUniformMatrix3fv' => 1,
	'glUniformMatrix4fv' => 1,
	'glValidateProgram' => 0,
	'glVertexAttrib1d' => 0,
	'glVertexAttrib1dv' => 1,
	'glVertexAttrib1f' => 0,
	'glVertexAttrib1fv' => 1,
	'glVertexAttrib1s' => 0,
	'glVertexAttrib1sv' => 1,
	'glVertexAttrib2d' => 0,
	'glVertexAttrib2dv' => 1,
	'glVertexAttrib2f' => 0,
	'glVertexAttrib2fv' => 1,
	'glVertexAttrib2s' => 0,
	'glVertexAttrib2sv' => 1,
	'glVertexAttrib3d' => 0,
	'glVertexAttrib3dv' => 1,
	'glVertexAttrib3f' => 0,
	'glVertexAttrib3fv' => 1,
	'glVertexAttrib3s' => 0,
	'glVertexAttrib3sv' => 1,
	'glVertexAttrib4Nbv' => 1,
	'glVertexAttrib4Niv' => 1,
	'glVertexAttrib4Nsv' => 1,
	'glVertexAttrib4Nub' => 0,
	'glVertexAttrib4Nubv' => 1,
	'glVertexAttrib4Nuiv' => 1,
	'glVertexAttrib4Nusv' => 1,
	'glVertexAttrib4bv' => 1,
	'glVertexAttrib4d' => 0,
	'glVertexAttrib4dv' => 1,
	'glVertexAttrib4f' => 0,
	'glVertexAttrib4fv' => 1,
	'glVertexAttrib4iv' => 1,
	'glVertexAttrib4s' => 0,
	'glVertexAttrib4sv' => 1,
	'glVertexAttrib4ubv' => 1,
	'glVertexAttrib4uiv' => 1,
	'glVertexAttrib4usv' => 1,
	'glVertexAttribPointer' => 1,
	
	# Version 2.1
	'glUniformMatrix2x3fv' => 1,
	'glUniformMatrix3x2fv' => 1,
	'glUniformMatrix2x4fv' => 1,
	'glUniformMatrix4x2fv' => 1,
	'glUniformMatrix3x4fv' => 1,
	'glUniformMatrix4x3fv' => 1,

	# Version 3.0
	'glColorMaski' => 0,
	'glGetBooleani_v' => 1,
	'glGetIntegeri_v' => 1,
	'glEnablei' => 0,
	'glDisablei' => 0,
	'glIsEnabledi' => 0,
	'glBeginTransformFeedback' => 0,
	'glEndTransformFeedback' => 0,
	'glBindBufferRange' => 1,
	'glBindBufferBase' => 1,
	'glTransformFeedbackVaryings' => 1,
	'glGetTransformFeedbackVarying' => 1,
	'glClampColor' => 0,
	'glBeginConditionalRender' => 0,
	'glEndConditionalRender' => 0,
	'glVertexAttribIPointer' => 1,
	'glGetVertexAttribIiv' => 1,
	'glGetVertexAttribIuiv' => 1,
	'glVertexAttribI1i' => 0,
	'glVertexAttribI2i' => 0,
	'glVertexAttribI3i' => 0,
	'glVertexAttribI4i' => 0,
	'glVertexAttribI1ui' => 0,
	'glVertexAttribI2ui' => 0,
	'glVertexAttribI3ui' => 0,
	'glVertexAttribI4ui' => 0,
	'glVertexAttribI1iv' => 1,
	'glVertexAttribI2iv' => 1,
	'glVertexAttribI3iv' => 1,
	'glVertexAttribI4iv' => 1,
	'glVertexAttribI1uiv' => 1,
	'glVertexAttribI2uiv' => 1,
	'glVertexAttribI3uiv' => 1,
	'glVertexAttribI4uiv' => 1,
	'glVertexAttribI4bv' => 1,
	'glVertexAttribI4sv' => 1,
	'glVertexAttribI4ubv' => 1,
	'glVertexAttribI4usv' => 1,
	'glGetUniformuiv' => 1,
	'glBindFragDataLocation' => 1,
	'glGetFragDataLocation' => 1,
	'glUniform1ui' => 0,
	'glUniform2ui' => 0,
	'glUniform3ui' => 0,
	'glUniform4ui' => 0,
	'glUniform1uiv' => 1,
	'glUniform2uiv' => 1,
	'glUniform3uiv' => 1,
	'glUniform4uiv' => 1,
	'glTexParameterIiv' => 1,
	'glTexParameterIuiv' => 1,
	'glGetTexParameterIiv' => 1,
	'glGetTexParameterIuiv' => 1,
	'glClearBufferiv' => 1,
	'glClearBufferuiv' => 1,
	'glClearBufferfv' => 1,
	'glClearBufferfi' => 0,
	'glGetStringi' => 0,  # handled separately
	'glIsRenderbuffer' => 0,
	'glBindRenderbuffer' => 0,
	'glDeleteRenderbuffers' => 1,
	'glGenRenderbuffers' => 1,
	'glRenderbufferStorage' => 0,
	'glGetRenderbufferParameteriv' => 1,
	'glIsFramebuffer' => 0,
	'glBindFramebuffer' => 0,
	'glDeleteFramebuffers' => 1,
	'glGenFramebuffers' => 1,
	'glCheckFramebufferStatus' => 0,
	'glFramebufferTexture1D' => 0,
	'glFramebufferTexture2D' => 0,
	'glFramebufferTexture3D' => 0,
	'glFramebufferRenderbuffer' => 0,
	'glGetFramebufferAttachmentParameteriv' => 1,
	'glGenerateMipmap' => 0,
	'glBlitFramebuffer' => 0,
	'glRenderbufferStorageMultisample' => 0,
	'glFramebufferTextureLayer' => 0,
	'glMapBufferRange' => 1,
	'glFlushMappedBufferRange' => 0,
	'glBindVertexArray' => 0,
	'glDeleteVertexArrays' => 1,
	'glGenVertexArrays' => 1,
	'glIsVertexArray' => 0,
	
	# Version 3.1
	'glDrawArraysInstanced' => 1,
	'glDrawElementsInstanced' => 1,
	'glTexBuffer' => 0,
	'glPrimitiveRestartIndex' => 0,
	'glCopyBufferSubData' => 0,
	'glGetUniformIndices' => 1,
	'glGetActiveUniformsiv' => 1,
	'glGetActiveUniformName' => 1,
	'glGetUniformBlockIndex' => 1,
	'glGetActiveUniformBlockiv' => 1,
	'glGetActiveUniformBlockIndex' => 1,
	'glGetActiveUniformBlockName' => 1,
	'glUniformBlockBinding' => 0,
	
	# Version 3.2
	'glDrawElementsBaseVertex' => 1,
	'glDrawRangeElementsBaseVertex' => 1,
	'glDrawElementsInstancedBaseVertex' => 1,
	'glMultiDrawElementsBaseVertex' => 1,
	'glProvokingVertex' => 0,
	'glFenceSync' => 0,
	'glIsSync' => 0,
	'glDeleteSync' => 0,
	'glClientWaitSync' => 0,
	'glWaitSync' => 0,
	'glGetInteger64v' => 1,
	'glGetSynciv' => 1,
	'glGetInteger64i_v' => 1,
	'glGetBufferParameteri64v' => 1,
	'glFramebufferTexture' => 0,
	'glTexImage2DMultisample' => 0,
	'glTexImage3DMultisample' => 0,
	'glGetMultisamplefv' => 1,
	'glSampleMaski' => 0,
	
	# Version 3.3
	'glBindFragDataLocationIndexed' => 1,
	'glGetFragDataIndex' => 1,
	'glGenSamplers' => 1,
	'glDeleteSamplers' => 1,
	'glIsSampler' => 0,
	'glBindSampler' => 0,
	'glSamplerParameteri' => 0,
	'glSamplerParameteriv' => 1,
	'glSamplerParameterf' => 0,
	'glSamplerParameterfv' => 1,
	'glSamplerParameterIiv' => 1,
	'glSamplerParameterIuiv' => 1,
	'glGetSamplerParameteriv' => 1,
	'glGetSamplerParameterIiv' => 1,
	'glGetSamplerParameterfv' => 1,
	'glGetSamplerParameterIuiv' => 1,
	'glQueryCounter' => 0,
	'glGetQueryObjecti64v' => 1,
	'glGetQueryObjectui64v' => 1,
	'glVertexAttribDivisor' => 0,
	'glVertexAttribP1ui' => 0,
	'glVertexAttribP1uiv' => 1,
	'glVertexAttribP2ui' => 0,
	'glVertexAttribP2uiv' => 1,
	'glVertexAttribP3ui' => 0,
	'glVertexAttribP3uiv' => 1,
	'glVertexAttribP4ui' => 0,
	'glVertexAttribP4uiv' => 1,
	'glVertexP2ui' => 0,
	'glVertexP2uiv' => 1,
	'glVertexP3ui' => 0,
	'glVertexP3uiv' => 1,
	'glVertexP4ui' => 0,
	'glVertexP4uiv' => 1,
	'glTexCoordP1ui' => 0,
	'glTexCoordP1uiv' => 1,
	'glTexCoordP2ui' => 0,
	'glTexCoordP2uiv' => 1,
	'glTexCoordP3ui' => 0,
	'glTexCoordP3uiv' => 1,
	'glTexCoordP4ui' => 0,
	'glTexCoordP4uiv' => 1,
	'glMultiTexCoordP1ui' => 0,
	'glMultiTexCoordP1uiv' => 1,
	'glMultiTexCoordP2ui' => 0,
	'glMultiTexCoordP2uiv' => 1,
	'glMultiTexCoordP3ui' => 0,
	'glMultiTexCoordP3uiv' => 1,
	'glMultiTexCoordP4ui' => 0,
	'glMultiTexCoordP4uiv' => 1,
	'glNormalP3ui' => 0,
	'glNormalP3uiv' => 1,
	'glColorP3ui' => 0,
	'glColorP3uiv' => 1,
	'glColorP4ui' => 0,
	'glColorP4uiv' => 1,
	'glSecondaryColorP3ui' => 0,
	'glSecondaryColorP3uiv' => 1,
	
	# Version 4.0
	'glMinSampleShading' => 0,
	'glBlendEquationi' => 0,
	'glBlendEquationSeparatei' => 0,
	'glBlendFunci' => 0,
	'glBlendFuncSeparatei' => 0,
	'glDrawArraysIndirect' => 1,
	'glDrawElementsIndirect' => 1,
	'glUniform1d' => 0,
	'glUniform2d' => 0,
	'glUniform3d' => 0,
	'glUniform4d' => 0,
	'glUniform1dv' => 1,
	'glUniform2dv' => 1,
	'glUniform3dv' => 1,
	'glUniform4dv' => 1,
	'glUniformMatrix2dv' => 1,
	'glUniformMatrix3dv' => 1,
	'glUniformMatrix4dv' => 1,
	'glUniformMatrix2x3dv' => 1,
	'glUniformMatrix2x4dv' => 1,
	'glUniformMatrix3x2dv' => 1,
	'glUniformMatrix3x4dv' => 1,
	'glUniformMatrix4x2dv' => 1,
	'glUniformMatrix4x3dv' => 1,
	'glGetUniformdv' => 1,
	'glGetSubroutineUniformLocation' => 1,
	'glGetSubroutineIndex' => 1,
	'glGetActiveSubroutineUniformiv' => 1,
	'glGetActiveSubroutineUniformName' => 1,
	'glGetActiveSubroutineName' => 1,
	'glUniformSubroutinesuiv' => 1,
	'glGetUniformSubroutineuiv' => 1,
	'glGetProgramStageiv' => 1,
	'glPatchParameteri' => 0,
	'glPatchParameterfv' => 1,
	'glBindTransformFeedback' => 0,
	'glDeleteTransformFeedbacks' => 1,
	'glGenTransformFeedbacks' => 1,
	'glIsTransformFeedback' => 0,
	'glPauseTransformFeedback' => 0,
	'glResumeTransformFeedback' => 0,
	'glDrawTransformFeedback' => 0,
	'glDrawTransformFeedbackStream' => 0,
	'glBeginQueryIndexed' => 0,
	'glEndQueryIndexed' => 0,
	'glGetQueryIndexediv' => 1,
	
	# Version 4.1
	'glReleaseShaderCompiler' => 0,
	'glShaderBinary' => 1,
	'glGetShaderPrecisionFormat' => 1,
	'glDepthRangef' => 0,
	'glClearDepthf' => 0,
	'glGetProgramBinary' => 1,
	'glProgramBinary' => 1,
	'glProgramParameteri' => 0,
	'glUseProgramStages' => 0,
	'glActiveShaderProgram' => 0,
	'glCreateShaderProgramv' => 1,
	'glBindProgramPipeline' => 0,
	'glDeleteProgramPipelines' => 1,
	'glGenProgramPipelines' => 1,
	'glIsProgramPipeline' => 0,
	'glGetProgramPipelineiv' => 1,
	'glProgramUniform1i' => 0,
	'glProgramUniform1iv' => 1,
	'glProgramUniform1f' => 0,
	'glProgramUniform1fv' => 1,
	'glProgramUniform1d' => 0,
	'glProgramUniform1dv' => 1,
	'glProgramUniform1ui' => 0,
	'glProgramUniform1uiv' => 1,
	'glProgramUniform2i' => 0,
	'glProgramUniform2iv' => 1,
	'glProgramUniform2f' => 0,
	'glProgramUniform2fv' => 1,
	'glProgramUniform2d' => 0,
	'glProgramUniform2dv' => 1,
	'glProgramUniform2ui' => 0,
	'glProgramUniform2uiv' => 1,
	'glProgramUniform3i' => 0,
	'glProgramUniform3iv' => 1,
	'glProgramUniform3f' => 0,
	'glProgramUniform3fv' => 1,
	'glProgramUniform3d' => 0,
	'glProgramUniform3dv' => 1,
	'glProgramUniform3ui' => 0,
	'glProgramUniform3uiv' => 1,
	'glProgramUniform4i' => 0,
	'glProgramUniform4iv' => 1,
	'glProgramUniform4f' => 0,
	'glProgramUniform4fv' => 1,
	'glProgramUniform4d' => 0,
	'glProgramUniform4dv' => 1,
	'glProgramUniform4ui' => 0,
	'glProgramUniform4uiv' => 1,
	'glProgramUniformMatrix2fv' => 1,
	'glProgramUniformMatrix3fv' => 1,
	'glProgramUniformMatrix4fv' => 1,
	'glProgramUniformMatrix2dv' => 1,
	'glProgramUniformMatrix3dv' => 1,
	'glProgramUniformMatrix4dv' => 1,
	'glProgramUniformMatrix2x3fv' => 1,
	'glProgramUniformMatrix3x2fv' => 1,
	'glProgramUniformMatrix2x4fv' => 1,
	'glProgramUniformMatrix4x2fv' => 1,
	'glProgramUniformMatrix3x4fv' => 1,
	'glProgramUniformMatrix4x3fv' => 1,
	'glProgramUniformMatrix2x3dv' => 1,
	'glProgramUniformMatrix3x2dv' => 1,
	'glProgramUniformMatrix2x4dv' => 1,
	'glProgramUniformMatrix4x2dv' => 1,
	'glProgramUniformMatrix3x4dv' => 1,
	'glProgramUniformMatrix4x3dv' => 1,
	'glValidateProgramPipeline' => 0,
	'glGetProgramPipelineInfoLog' => 1,
	'glVertexAttribL1d' => 0,
	'glVertexAttribL2d' => 0,
	'glVertexAttribL3d' => 0,
	'glVertexAttribL4d' => 0,
	'glVertexAttribL1dv' => 1,
	'glVertexAttribL2dv' => 1,
	'glVertexAttribL3dv' => 1,
	'glVertexAttribL4dv' => 1,
	'glVertexAttribLPointer' => 1,
	'glGetVertexAttribLdv' => 1,
	'glViewportArrayv' => 1,
	'glViewportIndexedf' => 0,
	'glViewportIndexedfv' => 1,
	'glScissorArrayv' => 1,
	'glScissorIndexed' => 0,
	'glScissorIndexedv' => 1,
	'glDepthRangeArrayv' => 1,
	'glDepthRangeIndexed' => 0,
	'glGetFloati_v' => 1,
	'glGetDoublei_v' => 1,
	
	# Version 4.2
	'glDrawArraysInstancedBaseInstance' => 0,
	'glDrawElementsInstancedBaseInstance' => 1,
	'glDrawElementsInstancedBaseVertexBaseInstance' => 1,
	'glGetInternalformativ' => 1,
	'glGetActiveAtomicCounterBufferiv' => 1,
	'glBindImageTexture' => 0,
	'glMemoryBarrier' => 0,
	'glTexStorage1D' => 0,
	'glTexStorage2D' => 0,
	'glTexStorage3D' => 0,
	'glDrawTransformFeedbackInstanced' => 0,
	'glDrawTransformFeedbackStreamInstanced' => 0,
	
	# Version 4.3
	'glDispatchCompute' => 0,
	'glDispatchComputeIndirect' => 1,
	'glCopyImageSubData' => 0,
	'glFramebufferParameteri' => 0,
	'glGetFramebufferParameteriv' => 1,
	'glGetInternalformati64v' => 1,
	'glInvalidateTexSubImage' => 0,
	'glInvalidateTexImage' => 0,
	'glInvalidateBufferSubData' => 0,
	'glInvalidateBufferData' => 1,
	'glInvalidateFramebuffer' => 1,
	'glInvalidateSubFramebuffer' => 1,
	'glMultiDrawArraysIndirect' => 1,
	'glMultiDrawElementsIndirect' => 1,
	'glGetProgramInterfaceiv' => 1,
	'glGetProgramResourceIndex' => 1,
	'glGetProgramResourceName' => 1,
	'glGetProgramResourceiv' => 1,
	'glGetProgramResourceLocation' => 1,
	'glGetProgramResourceLocationIndex' => 1,
	'glShaderStorageBlockBinding' => 0,
	'glTexBufferRange' => 0,
	'glTexStorage2DMultisample' => 0,
	'glTexStorage3DMultisample' => 0,
	'glTextureView' => 0,
	'glBindVertexBuffer' => 1,
	'glVertexAttribFormat' => 0,
	'glVertexAttribIFormat' => 0,
	'glVertexAttribLFormat' => 0,
	'glVertexAttribBinding' => 0,
	'glVertexBindingDivisor' => 0,
	'glDebugMessageControl' => 1,
	'glDebugMessageInsert' => 1,
	'glDebugMessageCallback' => 1,
	'glGetDebugMessageLog' => 1,
	'glPushDebugGroup' => 1,
	'glObjectLabel' => 1,
	'glGetObjectLabel' => 1,
	'glObjectPtrLabel' => 1,
	'glGetObjectPtrLabel' => 1,
	
	# Version 4.4
	'glBufferStorage' => 1,
	'glClearTexImage' => 1,
	'glClearTexSubImage' => 1,
	'glBindBuffersBase' => 1,
	'glBindBuffersRange' => 1,
	'glBindTextures' => 1,
	'glBindSamplers' => 1,
	'glBindImageTextures' => 1,
	'glBindVertexBuffers' => 1,
	
	# Version 4.5
	'glBindTextureUnit' => 0,
	'glBlitNamedFramebuffer' => 0,
	'glCheckNamedFramebufferStatus' => 0,
	'glClearNamedBufferData' => 1,
	'glClearNamedBufferSubData' => 1,
	'glClearNamedFramebufferfi' => 0,
	'glClearNamedFramebufferfv' => 1,
	'glClearNamedFramebufferiv' => 1,
	'glClearNamedFramebufferuiv' => 1,
	'glClipControl' => 0,
	'glCompressedTextureSubImage1D' => 1,
	'glCompressedTextureSubImage2D' => 1,
	'glCompressedTextureSubImage3D' => 1,
	'glCopyNamedBufferSubData' => 0,
	'glCopyTextureSubImage1D' => 0,
	'glCopyTextureSubImage2D' => 0,
	'glCopyTextureSubImage3D' => 0,
	'glCreateBuffers' => 1,
	'glCreateFramebuffers' => 1,
	'glCreateProgramPipelines' => 1,
	'glCreateQueries' => 1,
	'glCreateRenderbuffers' => 1,
	'glCreateSamplers' => 1,
	'glCreateTextures' => 1,
	'glCreateTransformFeedbacks' => 1,
	'glCreateVertexArrays' => 1,
	'glDisableVertexArrayAttrib' => 0,
	'glEnableVertexArrayAttrib' => 0,
	'glFlushMappedNamedBufferRange' => 0,
	'glGenerateTextureMipmap' => 0,
	'glGetCompressedTextureImage' => 1,
	'glGetCompressedTextureSubImage' => 1,
	'glGetGraphicsResetStatus' => 0,
	'glGetNamedBufferParameteri64v' => 1,
	'glGetNamedBufferParameteriv' => 1,
	'glGetNamedBufferPointerv' => 1,
	'glGetNamedBufferSubData' => 1,
	'glGetNamedFramebufferAttachmentParameteriv' => 1,
	'glGetNamedFramebufferParameteriv' => 1,
	'glGetNamedRenderbufferParameteriv' => 1,
	'glGetQueryBufferObjecti64v' => 0,
	'glGetQueryBufferObjectiv' => 0,
	'glGetQueryBufferObjectui64v' => 0,
	'glGetQueryBufferObjectuiv' => 0,
	'glGetTextureImage' => 1,
	'glGetTextureLevelParameterfv' => 1,
	'glGetTextureLevelParameteriv' => 1,
	'glGetTextureParameterIiv' => 1,
	'glGetTextureParameterIuiv' => 1,
	'glGetTextureParameterfv' => 1,
	'glGetTextureParameteriv' => 1,
	'glGetTextureSubImage' => 1,
	'glGetTransformFeedbacki64_v' => 1,
	'glGetTransformFeedbacki_v' => 1,
	'glGetTransformFeedbackiv' => 1,
	'glGetVertexArrayIndexed64iv' => 1,
	'glGetVertexArrayIndexediv' => 1,
	'glGetVertexArrayiv' => 1,
	'glGetnColorTable' => 1,
	'glGetnCompressedTexImage' => 1,
	'glGetnConvolutionFilter' => 1,
	'glGetnHistogram' => 1,
	'glGetnMapdv' => 1,
	'glGetnMapfv' => 1,
	'glGetnMapiv' => 1,
	'glGetnMinmax' => 1,
	'glGetnPixelMapfv' => 1,
	'glGetnPixelMapuiv' => 1,
	'glGetnPixelMapusv' => 1,
	'glGetnPolygonStipple' => 1,
	'glGetnSeparableFilter' => 1,
	'glGetnTexImage' => 1,
	'glGetnUniformdv' => 1,
	'glGetnUniformfv' => 1,
	'glGetnUniformiv' => 1,
	'glGetnUniformuiv' => 1,
	'glInvalidateNamedFramebufferData' => 1,
	'glInvalidateNamedFramebufferSubData' => 1,
	'glMapNamedBuffer' => 1,
	'glMapNamedBufferRange' => 1,
	'glMemoryBarrierByRegion' => 0,
	'glNamedBufferData' => 1,
	'glNamedBufferStorage' => 1,
	'glNamedBufferSubData' => 1,
	'glNamedFramebufferDrawBuffer' => 0,
	'glNamedFramebufferDrawBuffers' => 1,
	'glNamedFramebufferParameteri' => 0,
	'glNamedFramebufferReadBuffer' => 0,
	'glNamedFramebufferRenderbuffer' => 0,
	'glNamedFramebufferTexture' => 0,
	'glNamedFramebufferTextureLayer' => 0,
	'glNamedRenderbufferStorage' => 0,
	'glNamedRenderbufferStorageMultisample' => 0,
	'glReadnPixels' => 1,
	'glTextureBarrier' => 0,
	'glTextureBuffer' => 0,
	'glTextureBufferRange' => 0,
	'glTextureParameterIiv' => 1,
	'glTextureParameterIuiv' => 1,
	'glTextureParameterf' => 0,
	'glTextureParameterfv' => 1,
	'glTextureParameteri' => 0,
	'glTextureParameteriv' => 1,
	'glTextureStorage1D' => 0,
	'glTextureStorage2D' => 0,
	'glTextureStorage2DMultisample' => 0,
	'glTextureStorage3D' => 0,
	'glTextureStorage3DMultisample' => 0,
	'glTextureSubImage1D' => 1,
	'glTextureSubImage2D' => 1,
	'glTextureSubImage3D' => 1,
	'glTransformFeedbackBufferBase' => 0,
	'glTransformFeedbackBufferRange' => 0,
	'glUnmapNamedBuffer' => 0,
	'glVertexArrayAttribBinding' => 0,
	'glVertexArrayAttribFormat' => 0,
	'glVertexArrayAttribIFormat' => 0,
	'glVertexArrayAttribLFormat' => 0,
	'glVertexArrayBindingDivisor' => 0,
	'glVertexArrayElementBuffer' => 0,
	'glVertexArrayVertexBuffer' => 0,
	'glVertexArrayVertexBuffers' => 1,

	# Version 4.6
	'glSpecializeShader' => 0,
	'glMultiDrawArraysIndirectCount' => 1,
	'glMultiDrawElementsIndirectCount' => 1,
	'glPolygonOffsetClamp' => 0,
);


#
# subset of functions that should be exported to TinyGL
#
my %tinygl = (
	'information' => 1,
	'glBegin' => 2,
	'glClear' => 3,
	'glClearColor' => 4,
	'glColor3f' => 5,
	'glDisable' => 6,
	'glEnable' => 7,
	'glEnd' => 8,
	'glLightfv' => 9,
	'glLoadIdentity' => 10,
	'glMaterialfv' => 11,
	'glMatrixMode' => 12,
	'glOrthof' => 13,
	'glPopMatrix' => 14,
	'glPushMatrix' => 15,
	'glRotatef' => 16,
	'glTexEnvi' => 17,
	'glTexParameteri' => 18,
	'glTranslatef' => 19,
	'glVertex2f' => 20,
	'glVertex3f' => 21,
	'OSMesaCreateLDG' => 22,
	'OSMesaDestroyLDG' => 23,
	'glArrayElement' => 24,
	'glBindTexture' => 25,
	'glCallList' => 26,
	'glClearDepthf' => 27,
	'glColor4f' => 28,
	'glColor3fv' => 29,
	'glColor4fv' => 30,
	'glColorMaterial' => 31,
	'glColorPointer' => 32,
	'glCullFace' => 33,
	'glDeleteTextures' => 34,
	'glDisableClientState' => 35,
	'glEnableClientState' => 36,
	'glEndList' => 37,
	'glEdgeFlag' => 38,
	'glFlush' => 39,
	'glFrontFace' => 40,
	'glFrustumf' => 41,
	'glGenLists' => 42,
	'glGenTextures' => 43,
	'glGetFloatv' => 44,
	'glGetIntegerv' => 45,
	'glHint' => 46,
	'glInitNames' => 47,
	'glIsList' => 48,
	'glLightf' => 49,
	'glLightModeli' => 50,
	'glLightModelfv' => 51,
	'glLoadMatrixf' => 52,
	'glLoadName' => 53,
	'glMaterialf' => 54,
	'glMultMatrixf' => 55,
	'glNewList' => 56,
	'glNormal3f' => 57,
	'glNormal3fv' => 58,
	'glNormalPointer' => 59,
	'glPixelStorei' => 60,
	'glPolygonMode' => 61,
	'glPolygonOffset' => 62,
	'glPopName' => 63,
	'glPushName' => 64,
	'glRenderMode' => 65,
	'glSelectBuffer' => 66,
	'glScalef' => 67,
	'glShadeModel' => 68,
	'glTexCoord2f' => 69,
	'glTexCoord4f' => 70,
	'glTexCoord2fv' => 71,
	'glTexCoordPointer' => 72,
	'glTexImage2D' => 73,
	'glVertex4f' => 74,
	'glVertex3fv' => 75,
	'glVertexPointer' => 76,
	'glViewport' => 77,
	'swapbuffer' => 78,
	'max_width' => 79,
	'max_height' => 80,
	'glDeleteLists' => 81,
	'gluLookAtf' => 82,
	'exception_error' => 83,
);

#
# functions that are exported under a different name
#
my %floatfuncs = (
	'glOrtho' => 'glOrthod',
	'glFrustum' => 'glFrustumd',
	'glClearDepth' => 'glClearDepthd',
	'gluLookAt' => 'gluLookAtd',
);


my %oldmesa = (
	'OSMesaCreateLDG' => {
		'name' => 'OSMesaCreateLDG',
		'gl' => '',
		'type' => 'void *',
		'params' => [
			{ 'type' => 'GLenum', 'name' => 'format', 'pointer' => 0 },
			{ 'type' => 'GLenum', 'name' => 'type', 'pointer' => 0 },
			{ 'type' => 'GLint', 'name' => 'width', 'pointer' => 0 },
			{ 'type' => 'GLint', 'name' => 'height', 'pointer' => 0 },
		],
	},
	'OSMesaDestroyLDG' => {
		'name' => 'OSMesaDestroyLDG',
		'gl' => '',
		'type' => 'void',
		'params' => [
		],
	},
	'max_width' => {
		'name' => 'max_width',
		'gl' => '',
		'type' => 'GLsizei',
		'params' => [
		],
	},
	'max_height' => {
		'name' => 'max_height',
		'gl' => '',
		'type' => 'GLsizei',
		'params' => [
		],
	},
	'glOrthof' => {
		'name' => 'Orthof',
		'gl' => 'gl',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLfloat', 'name' => 'left', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'right', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'bottom', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'top', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'near_val', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'far_val', 'pointer' => 0 },
		],
	},
	'glFrustumf' => {
		'name' => 'Frustumf',
		'gl' => 'gl',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLfloat', 'name' => 'left', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'right', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'bottom', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'top', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'near_val', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'far_val', 'pointer' => 0 },
		],
	},
	'gluLookAtf' => {
		'name' => 'LookAtf',
		'gl' => 'glu',
		'type' => 'void',
		'params' => [
			{ 'type' => 'GLfloat', 'name' => 'eyeX', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'eyeY', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'eyeZ', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'centerX', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'centerY', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'centerZ', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'upX', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'upY', 'pointer' => 0 },
			{ 'type' => 'GLfloat', 'name' => 'upZ', 'pointer' => 0 },
		],
	},
	'swapbuffer' => {
		'name' => 'swapbuffer',
		'gl' => '',
		'type' => 'void',
		'params' => [
			{ 'type' => 'void', 'name' => 'buffer', 'pointer' => 1 },
		],
	},
	'exception_error' => {
		'name' => 'exception_error',
		'gl' => '',
		'type' => 'void',
		'params' => [
			{ 'type' => 'void (CALLBACK *exception)(GLenum param)', 'name' => '', 'pointer' => 2 },
		],
	},
	'information' => {
		'name' => 'information',
		'gl' => '',
		'type' => 'void',
		'params' => [
		],
	},
#	'glInit' => {
#		'name' => 'glInit',
#		'gl' => '',
#		'type' => 'void',
#		'params' => [
#			{ 'type' => 'void', 'name' => 'zbuffer', 'pointer' => 1 },
#		],
#	},
#	'glClose' => {
#		'name' => 'glClose',
#		'gl' => '',
#		'type' => 'void',
#		'params' => [
#		],
#	},
#	'glDebug' => {
#		'name' => 'glDebug',
#		'gl' => '',
#		'type' => 'void',
#		'params' => [
#			{ 'type' => 'GLint', 'name' => 'mode', 'pointer' => 0 },
#		],
#	},
);


#
# generate functions
#
sub gen_calls() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $ret;
	my $retvar;
	my $prototype;
	my $prototype_mem;
	my $return_type;
	my $function_name;
	my $uppername;
	my $gl;
	my $args;
	my $debug_args;
	my $noconv_args;
	my $printf_format;
	my $conversions_needed = 0;
	my $num_longlongs = 0;
	my $num_autogen = 0;
	
	add_paramlens(\%paramlens);
	gen_params();
	foreach my $key (sort { sort_by_name } keys %functions) {
		my $ent = $functions{$key};
		$gl = $ent->{gl};
		next if ($gl ne 'gl' && $gl ne 'glu');
		$function_name = $gl . $ent->{name};
		$return_type = $ent->{type};
		$prototype = $ent->{proto};
		$prototype_mem = $ent->{proto_mem};
		$args = $ent->{args};
		$noconv_args = $ent->{noconv_args};
		$debug_args = $ent->{debug_args};
		$printf_format = $ent->{printf_format};
		$uppername = uc($function_name);
	
		if ($return_type eq "void")
		{
			$retvar = "";
			$ret = "";
		} else {
			$retvar = "$return_type __ret = ";
			$ret = "return __ret";
		}
		print("#if 0\n") if ($gl ne 'gl' || defined($blacklist{$function_name}));
		if (defined($longlong_types{$return_type}) && !defined($blacklist{$function_name}) && !defined($macros{$function_name}))
		{
			print "/* FIXME: $return_type cannot be returned */\n";
			++$num_longlongs;
		}
		if ($prototype ne $prototype_mem)
		{
			print "#if NFOSMESA_POINTER_AS_MEMARG\n";
			print "$return_type OSMesaDriver::nf$function_name($prototype_mem)\n";
			print "#else\n";
			print "$return_type OSMesaDriver::nf$function_name($prototype)\n";
			print "#endif\n";
		} else
		{
			print "$return_type OSMesaDriver::nf$function_name($prototype)\n";
		}
		print "{\n";
		print "\tD(bug(\"nfosmesa: $function_name($printf_format)\"";
		print ", " unless ($args eq "");
		print "$debug_args));\n";
		if (defined($macros{$function_name}) && $macros{$function_name} == 1) {
			print "FN_${uppername}(${args});\n";
		} else {
			if ($ent->{any_pointer} == 2 && !defined($blacklist{$function_name}) && !defined($ent->{autogen}))
			{
				&warn("$function_name missing conversion");
				print "\t/* TODO: NFOSMESA_${uppername} may need conversion */\n";
				++$conversions_needed;
			}
			if (defined($ent->{autogen}))
			{
				my $params = $ent->{params};
				my $argcount = $#$params + 1;
				my $argc;
				my $param;
				printf "#if $ent->{ifdefs}\n";
				for ($argc = 0; $argc < $argcount; $argc++)
				{
					$param = $params->[$argc];
					my $name = $param->{name};
					if ($param->{pointer} && defined($param->{len}))
					{
						my $init = '';
						my $len = $param->{len};
						if ($len eq '1') {
							if ($param->{inout} eq 'out' || $param->{inout} eq 'inout')
							{
								$init = ' = { 0 }';
							}
						} elsif ($len =~ /\(/) {
							;
						} else {
							$len = "MAX($len, 0)";
						}
						print "\tGLint const __${name}_size = $len;\n";
						print "\t" . $param->{basetype} . ' __' . ${name} . '_tmp[__' . ${name} . '_size]' . "$init;\n";
						if ($param->{inout} eq 'in' || $param->{inout} eq 'inout')
						{
							print "\t$param->{basetype} *__${name}_ptmp = $copy_funcs{$param->{basetype}}->{copyin}(__${name}_size, $name, __${name}_tmp);\n";
						}
					}
				}
				print "\t${retvar}fn.${function_name}(${args});\n";
				for ($argc = 0; $argc < $argcount; $argc++)
				{
					$param = $params->[$argc];
					my $name = $param->{name};
					if ($param->{pointer} && defined($param->{len}))
					{
						if ($param->{inout} eq 'out' || $param->{inout} eq 'inout')
						{
							my $len;
							if (defined($param->{outlen})) {
								$len = "MIN($param->{outlen}, $param->{len})";
							} else {
								$len = "__${name}_size";
							}
							print "\t$copy_funcs{$param->{basetype}}->{copyout}($len, __${name}_tmp, $name);\n";
						}
					}
				}
				printf "#else\n";
				print "\t${retvar}fn.${function_name}(${noconv_args});\n";
				printf "#endif\n";
				++$num_autogen;
			} else {
				print "\t${retvar}fn.${function_name}(${args});\n";
			}
			print "\t$ret;\n" unless($ret eq '');
		}
		print "}\n";
		print("#endif\n") if ($gl ne 'gl' || defined($blacklist{$function_name}));
		print "\n";
		$gl_count++ if ($gl eq "gl");
		$glu_count++ if ($gl eq "glu");
	}
	print STDERR "$conversions_needed function(s) may need conversion macros\n" unless($conversions_needed == 0);
	print STDERR "$num_longlongs function(s) may need attention\n" unless($num_longlongs == 0);
#
# emit trailer
#
	print << "EOF";

/* Functions generated: $gl_count GL + $glu_count GLU */
/* Automatically generated: $num_autogen */
EOF
}


sub add_glgetstring() {
	my $funcno;
	
	$funcno = $functions{'glGetString'}->{funcno};
	{
		my %ent = (
			'name' => 'GetString',
			'gl' => 'LenGl',
			'type' => 'GLuint',
			'params' => [
				{ 'type' => 'OSMesaContext', 'name' => 'ctx', 'pointer' => 0 },
				{ 'type' => 'GLenum', 'name' => 'name', 'pointer' => 0 },
			],
			'funcno' => $funcno,
		);
		$functions{'glGetString'} = \%ent;
	}
	{
		my %ent = (
			'name' => 'GetString',
			'gl' => 'PutGl',
			'type' => 'void',
			'params' => [
				{ 'type' => 'OSMesaContext', 'name' => 'ctx', 'pointer' => 0 },
				{ 'type' => 'GLenum', 'name' => 'name', 'pointer' => 0 },
				{ 'type' => 'GLubyte', 'name' => 'buffer', 'pointer' => 1 },
			],
			'funcno' => $funcno + 1,
		);
		$functions{'putglGetString'} = \%ent;
	}
	
	$funcno = $functions{'glGetStringi'}->{funcno};
	{
		my %ent = (
			'name' => 'GetStringi',
			'gl' => 'LenGl',
			'type' => 'GLuint',
			'params' => [
				{ 'type' => 'OSMesaContext', 'name' => 'ctx', 'pointer' => 0 },
				{ 'type' => 'GLenum', 'name' => 'name', 'pointer' => 0 },
				{ 'type' => 'GLuint', 'name' => 'index', 'pointer' => 0 },
			],
			'funcno' => $funcno,
		);
		$functions{'glGetStringi'} = \%ent;
	}
	{
		my %ent = (
			'name' => 'GetStringi',
			'gl' => 'PutGl',
			'type' => 'void',
			'params' => [
				{ 'type' => 'OSMesaContext', 'name' => 'ctx', 'pointer' => 0 },
				{ 'type' => 'GLenum', 'name' => 'name', 'pointer' => 0 },
				{ 'type' => 'GLuint', 'name' => 'index', 'pointer' => 0 },
				{ 'type' => 'GLubyte', 'name' => 'buffer', 'pointer' => 1 },
			],
			'funcno' => $funcno + 1,
		);
		$functions{'putglGetStringi'} = \%ent;
	}
}


#
# generate table of # of stacked parameters,
# and now also table of function names
#
sub gen_paramcount() {
	my $prefix = "NFOSMESA_";
	my $params;
	my $gl;
	my $glx;
	my $function_name;
	my $maxcount;
	my $lastfunc;
	my $funcno;
	my $uppername;
	my $first;
	
	add_missing(\%oldmesa);
	read_enums();
	add_glgetstring();
	gen_params();

	print "static unsigned char const paramcount[NFOSMESA_LAST] = {\n";
	print "\t0, /* GETVERSION */\n";
	$maxcount = 0;
	$lastfunc = 1;
	
	$first = 1;
	foreach my $key (sort { sort_by_value } keys %functions) {
		my $ent = $functions{$key};
		$gl = $ent->{gl};

		if (!defined($gl) || $gl eq '')
		{
			$function_name = $key;
			$uppername = $function_name;
		} else
		{
			$function_name = $gl . $ent->{name};
			$uppername = ${prefix} . uc($function_name);
		}

		$funcno = $ent->{funcno};
		while ($lastfunc != $funcno)
		{
			printf ",\n"  unless($first);
			printf "\t0";
			++$lastfunc;
			$first = 0;
		}
		$params = $ent->{params};
		
		my $argcount = $#$params + 1;
		
		my $paramnum = 0;
		if ($argcount > 0) {
			for (my $argc = 0; $argc < $argcount; $argc++)
			{
				my $type = $params->[$argc]->{type};
				my $name = $params->[$argc]->{name};
				my $pointer = $params->[$argc]->{pointer} ? "*" : "";
				if ($pointer ne "" || defined($pointer_types{$type}))
				{
					++$paramnum;
				} elsif ($type eq 'GLdouble' || $type eq 'GLclampd')
				{
					$paramnum += 2;
				} elsif (defined($longlong_types{$type}))
				{
					$paramnum += 2;
				} elsif ($type eq 'GLhandleARB')
				{
					++$paramnum;
				} elsif ($type eq 'GLsync')
				{
					++$paramnum;
				} else
				{
					++$paramnum;
				}
			}
		}
		printf ",\n" unless($first);
		print "\t$paramnum /* ${uppername} */";
		$maxcount = $paramnum if ($paramnum > $maxcount);
		$lastfunc = $funcno + 1;
		$first = 0;
	}
	print "\n};\n";
	print "#define NFOSMESA_MAXPARAMS $maxcount\n";
	print "\n\n";
	print "static struct {\n";
	print "\tconst char *name;\n";
	print "\tunsigned int funcno;\n";
	print "} const gl_functionnames[] = {\n";
	$first = 1;
	foreach my $key (sort { sort_by_name } keys %functions) {
		my $ent = $functions{$key};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");

		# skip the TinyGL only functions(like OSMesaCreatLDG) for this
		next if (!defined($gl) || $gl eq '');
		$function_name = $gl . $ent->{name};
		$uppername = ${prefix} . uc($function_name);

		# sigh. another hack for GetString etc.
		next if ($function_name eq 'PutGlGetString');
		next if ($function_name eq 'PutGlGetStringi');
		$function_name = 'glGetString' if ($function_name eq 'LenGlGetString');
		$function_name = 'glGetStringi' if ($function_name eq 'LenGlGetStringi');
		
		$funcno = $ent->{funcno};
		printf ",\n"  unless($first);
		print "\t{ \"${function_name}\", $uppername }";
		$first = 0;
	}
	print "\n};\n";
}


#
# generate case statements
#
sub gen_dispatch() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $ret;
	my $prefix = "NFOSMESA_";
	my $uppername;
	my $params;
	my $return_type;
	my $function_name;
	my $gl;
	
	gen_params();
	foreach my $key (sort { sort_by_name } keys %functions) {
		my $ent = $functions{$key};
		$gl = $ent->{gl};
		next if ($gl ne 'gl' && $gl ne 'glu');
		$function_name = $gl . $ent->{name};
		$return_type = $ent->{type};
		$params = $ent->{params};
		
		$uppername = uc($function_name);
		print("#if 0\n") if (defined($blacklist{$function_name}));
		print "\t\tcase $prefix$uppername:\n";
		if ($return_type eq "void")
		{
			$ret = "";
		} else {
			$ret = "ret = ";
			if ($return_type =~ /\*/ || defined($pointer_types{$return_type}) || $return_type eq 'GLhandleARB' || $return_type eq 'GLsync')
			{
				$ret .= '(uint32)(uintptr_t)';
			}
		}
		my $argcount = $#$params + 1;
		print "\t\t\tD(funcname = \"${function_name}\");\n";
		print "\t\t\tif (GL_ISAVAILABLE(${function_name}))\n" if ($gl eq 'gl');
		print "\t\t\t${ret}nf${function_name}(";
		
		if ($argcount > 0) {
			my $paramnum = 0;
			my $indent = "\n\t\t\t\t";
			for (my $argc = 0; $argc < $argcount; $argc++)
			{
				my $type = $params->[$argc]->{type};
				my $name = $params->[$argc]->{name};
				my $pointer = $params->[$argc]->{pointer} ? "*" : "";
				my $comment = " /* ${type} ${pointer}${name} */";
				my $comma = ($argc < ($argcount - 1)) ? "," : "";
				if ($params->[$argc]->{pointer} || defined($pointer_types{$type}))
				{
					print "${indent}getStackedPointer($paramnum, ${type} ${pointer})$comma $comment";
					++$paramnum;
				} elsif ($type eq 'GLdouble' || $type eq 'GLclampd')
				{
					print "${indent}getStackedDouble($paramnum)$comma $comment";
					$paramnum += 2;
				} elsif ($type eq 'GLfloat' || $type eq 'GLclampf')
				{
					print "${indent}getStackedFloat($paramnum)$comma $comment";
					++$paramnum;
				} elsif (defined($longlong_types{$type}))
				{
					print "${indent}getStackedParameter64($paramnum)$comma $comment";
					$paramnum += 2;
				} elsif ($type eq 'GLhandleARB')
				{
					# legacy MacOSX headers declare GLhandleARB as void *
					print "${indent}(GLhandleARB)(uintptr_t)getStackedParameter($paramnum)$comma $comment";
					++$paramnum;
				} elsif ($type eq 'GLsync')
				{
					# GLsync declared as pointer type on host; need a cast here
					print "${indent}(GLsync)(uintptr_t)getStackedParameter($paramnum)$comma $comment";
					++$paramnum;
				} else
				{
					print "${indent}getStackedParameter($paramnum)$comma $comment";
					++$paramnum;
				}
			}
		}
		print ");\n";
		print "\t\t\tbreak;\n";
		print("#endif\n") if (defined($blacklist{$function_name}));
		
		$gl_count++ if ($gl eq "gl");
		$glu_count++ if ($gl eq "glu");
	}

#
# emit trailer
#
	print << "EOF";

/* Functions generated: $gl_count GL + $glu_count GLU */
EOF
}


#
# generate prototypes
#
sub gen_protos() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $osmesa_count = 0;
	my $prototype;
	my $return_type;
	my $function_name;
	my $gl;
	my $lastgl;
	
	gen_params();
	$lastgl = "";
	foreach my $key (sort { sort_by_name } keys %functions) {
		my $ent = $functions{$key};
		$gl = $ent->{gl};
		if ($gl ne $lastgl)
		{
			print "\n";
			$lastgl = $gl;
		}
		$function_name = $gl . $ent->{name};
		$return_type = $ent->{type};
		$prototype = $ent->{proto};
	
		print "GLAPI $return_type APIENTRY ${function_name}($prototype);\n";
		$gl_count++ if ($gl eq "gl");
		$glu_count++ if ($gl eq "glu");
		$osmesa_count++ if ($gl eq "OSMesa");
	}

#
# emit trailer
#
	print << "EOF";

/* Functions generated: $osmesa_count OSMesa + $gl_count GL + $glu_count GLU */
EOF
}


sub first_param_addr($)
{
	my ($ent) = @_;
	my $first_param;
	if ($ent->{args} eq "")
	{
		$first_param = 'NULL';
	} elsif ($ent->{name} eq "exception_error")
	{
		# hack for exception_error, which has a function pointer as argument
		$first_param = '&exception';
	} else {
		my $params = $ent->{params};
		$first_param = '&' . $params->[0]->{name};
	}
	return $first_param;
}

#
# generate prototype macros
#
sub gen_macros() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $osmesa_count = 0;
	my $first_param;
	my $ret;
	my $prototype;
	my $prototype_mem;
	my $return_type;
	my $function_name;
	my $complete_name;
	my $export;
	my $gl;
	my $lastgl;
	my $prefix;
	
	gen_params();
#
# emit header
#
	print << "EOF";
#ifndef GL_PROCM
#define GL_PROCM(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef GL_GETSTRING
#define GL_GETSTRING(type, gl, name, export, upper, proto, args, first, ret) GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#define GL_GETSTRINGI(type, gl, name, export, upper, proto, args, first, ret) GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef GLU_PROC
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret) GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef GLU_PROCM
#define GLU_PROCM(type, gl, name, export, upper, proto, args, first, ret) GL_PROCM(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef GL_PROC64
#define GL_PROC64(type, gl, name, export, upper, proto, args, first, ret) GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef GL_PROC64M
#define GL_PROC64M(type, gl, name, export, upper, proto, args, first, ret) GL_PROCM(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef OSMESA_PROC
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef OSMESA_PROCM
#define OSMESA_PROCM(type, gl, name, export, upper, proto, args, first, ret)
#endif
#undef GL_void_return
#define GL_void_return
#ifndef NOTHING
#define NOTHING void
#endif
#ifndef AND
#define AND
#endif
EOF

	$lastgl = "";
	foreach my $key (sort { sort_by_name } keys %functions) {
		my $ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		my $args = $ent->{args};
		my $params = $ent->{params};
		$prototype = $ent->{proto};
		$prototype_mem = $ent->{proto_mem};
		$first_param = first_param_addr($ent);
		$gl = $ent->{gl};
		if ($lastgl ne $gl)
		{
			print "\n";
			$lastgl = $gl;
		}
		$complete_name = $gl . $function_name;
		if ($return_type eq 'void') {
			# GL_void_return defined to nothing in header, to avoid empty macro argument
			$ret = 'GL_void_return';
		} else {
			$ret = "return ($return_type)";
		}
		$prefix = uc($gl);

		# hack for old library exporting several functions as taking float arguments
		$complete_name .= "f" if (defined($floatfuncs{$complete_name}));
		$export = $gl . $function_name;
		$export = $floatfuncs{$export} if (defined($floatfuncs{$export}));
		$export = "information" if ($function_name eq "tinyglinformation");

		if ($prototype eq "void")
		{
			$prototype = "NOTHING";
			$prototype_mem = "NOTHING";
		} else {
			$prototype = "AND " . $prototype;
			$prototype_mem = "AND " . $prototype_mem;
		}

		my $skip_for_tiny = ! defined($tinygl{$gl . $function_name});
		print("#ifndef NO_" . uc($gl . $function_name) . "\n");
		print("#if !defined(TINYGL_ONLY)\n") if ($skip_for_tiny);
		if ($complete_name eq 'glGetString')
		{
			# hack for glGetString() which is handled separately
			print "${prefix}_GETSTRING($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), ($args), $first_param, $ret)\n";
		} elsif ($complete_name eq 'glGetStringi')
		{
			# hack for glGetStringi() which is handled separately
			print "${prefix}_GETSTRINGI($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), ($args), $first_param, $ret)\n";
		} else
		{
			if (defined($longlong_rettypes{$return_type}))
			{
				$prefix .= '_PROC64';
			} else {
				$prefix .= '_PROC';
			}
			print "${prefix}($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), ($args), $first_param, $ret)\n";
			print "${prefix}M($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype_mem), ($args), $first_param, $ret)\n";
		}
		print("#endif\n") if ($skip_for_tiny);
		print("#endif\n");
		$gl_count++ if ($gl eq "gl");
		$glu_count++ if ($gl eq "glu");
		$osmesa_count++ if ($gl eq "OSMesa");
	}

#
# emit trailer
#
	print << "EOF";

/* Functions generated: $osmesa_count OSMesa + $gl_count GL + $glu_count GLU */

#undef GL_PROC
#undef GL_PROCM
#undef GL_PROC64
#undef GL_PROC64M
#undef GLU_PROC
#undef GLU_PROCM
#undef GL_GETSTRING
#undef GL_GETSTRINGI
#undef OSMESA_PROC
#undef OSMESA_PROCM
#undef GL_void_return
EOF
}


#
# generate prototype macros
#
sub gen_macros_bynumber() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $osmesa_count = 0;
	my $first_param;
	my $ret;
	my $prototype;
	my $return_type;
	my $function_name;
	my $complete_name;
	my $export;
	my $gl;
	my $prefix;
	my $lastfunc;
	my $funcno;
	my $empty_slots;
	
	add_missing(\%oldmesa);
	read_enums();
	add_glgetstring();
	gen_params();
#
# emit header
#
	print << "EOF";
#ifndef LENGL_PROC
#define LENGL_PROC(type, gl, name, export, upper, proto, args, first, ret) GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef PUTGL_PROC
#define PUTGL_PROC(type, gl, name, export, upper, proto, args, first, ret) GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef GLU_PROC
#define GLU_PROC(type, gl, name, export, upper, proto, args, first, ret) GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef GL_PROC64
#define GL_PROC64(type, gl, name, export, upper, proto, args, first, ret) GL_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#ifndef OSMESA_PROC
#define OSMESA_PROC(type, gl, name, export, upper, proto, args, first, ret)
#endif
#undef GL_void_return
#define GL_void_return
#ifndef NOTHING
#define NOTHING void
#endif
#ifndef AND
#define AND
#endif
EOF

	$lastfunc = 1;
	$empty_slots = 0;
	foreach my $key (sort { sort_by_value } keys %functions) {
		my $ent = $functions{$key};
		$function_name = $ent->{name};
		$funcno = $ent->{funcno};
		while ($lastfunc != $funcno)
		{
			printf "/* %4d */ NO_PROC\n", $lastfunc;
			++$lastfunc;
			++$empty_slots;
		}
		printf "/* %4d */ ", $funcno;
		if (!$function_name)
		{
			printf "NO_PROC /* $key */\n";
			++$empty_slots;
		} else
		{
			$return_type = $ent->{type};
			my $args = $ent->{args};
			my $params = $ent->{params};
			$prototype = $ent->{proto};
			$first_param = first_param_addr($ent);
			$gl = $ent->{gl};
			$complete_name = $gl . $function_name;
			if ($return_type eq 'void') {
				# GL_void_return defined to nothing in header, to avoid empty macro argument
				$ret = 'GL_void_return';
			} else {
				$ret = "return ($return_type)";
			}
			$prefix = uc($gl);
	
			# hack for old library exporting several functions as taking float arguments
			$complete_name .= "f" if (defined($floatfuncs{$complete_name}));
			$export = $gl . $function_name;
			$export = $floatfuncs{$export} if (defined($floatfuncs{$export}));
	
			if ($prototype eq "void")
			{
				$prototype = "NOTHING";
			} else {
				$prototype = "AND " . $prototype;
			}
	
			if ($complete_name eq 'glGetString')
			{
				# hack for glGetString() which is handled separately
				print "${prefix}_GETSTRING($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), ($args), $first_param, $ret)\n";
			} elsif ($complete_name eq 'glGetStringi')
			{
				# hack for glGetStringi() which is handled separately
				print "${prefix}_GETSTRINGI($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), ($args), $first_param, $ret)\n";
			} else
			{
				if ($prefix eq '')
				{
					$prefix = 'TINYGL_PROC';
					$gl = 'tinygl_';
			    } elsif (defined($longlong_rettypes{$return_type}))
				{
					$prefix .= '_PROC64';
				} else {
					$prefix .= '_PROC';
				}
				print "${prefix}($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), ($args), $first_param, $ret)\n";
			}
			$gl_count++ if ($gl eq "gl");
			$glu_count++ if ($gl eq "glu");
			$osmesa_count++ if ($gl eq "OSMesa");
		}
		$lastfunc = $funcno + 1;
	}

#
# emit trailer
#
	print << "EOF";

/* Functions generated: $osmesa_count OSMesa + $gl_count GL + $glu_count GLU + $empty_slots empty */

#undef GL_PROC
#undef GL_PROC64
#undef GLU_PROC
#undef GL_GETSTRING
#undef GL_GETSTRINGI
#undef LENGL_PROC
#undef PUTGL_PROC
#undef OSMESA_PROC
#undef NO_PROC
#undef TINYGL_PROC
#undef GL_void_return
EOF
}


my $ldg_trailer = '
/*
 * Functions exported from TinyGL that take float arguments,
 * and conflict with OpenGL functions of the same name
 */
#undef glFrustum
#define glFrustum glFrustumf
#undef glClearDepth
#define glClearDepth glClearDepthf
#undef glOrtho
#define glOrtho glOrthof
#undef gluLookAt
#define gluLookAt gluLookAtf

/* fix bad name */
#undef information
#define tinyglinformation gl.information

';

my $tinygl_trailer = '
/*
 * no-ops in TinyGL
 */
#undef glFinish
#define glFinish()

/* fix bad name */
#undef information
#define tinyglinformation gl.information

/*
 * Functions from OpenGL that are not implemented in TinyGL
 */
#define glPointSize(size) glPointSize_not_supported_by_tinygl()
#define glLineWidth(width) glLineWidth_not_supported_by_tinygl()
#define glDepthFunc(func) glDepthFunc_not_supported_by_tinygl()
#define glBlendFunc(sfactor, dfactor) glBlendFunc_not_supported_by_tinygl()
#define glTexEnvf(target, pname, param) glTexEnvf_not_supported_by_tinygl()
#define glVertex2i(x, y) glVertex2i_not_supported_by_tinygl()
#define glDepthMask(flag) glDepthMask_not_supported_by_tinygl()
#define glFogi(pname, param) glFogi_not_supported_by_tinygl()
#define glFogf(pname, param) glFogf_not_supported_by_tinygl()
#define glFogiv(pname, params) glFogiv_not_supported_by_tinygl()
#define glFogfv(pname, params) glFogfv_not_supported_by_tinygl()
#define glRasterPos2f(x, y) glRasterPos2f_not_supported_by_tinygl()
#define glPolygonStipple(mask) glPolygonStipple_not_supported_by_tinygl()
#define glTexParameterf(target, pname, param) glTexParameterf_not_supported_by_tinygl()

';


sub gen_ldgheader() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $osmesa_count = 0;
	my $ent;
	my $prototype;
	my $return_type;
	my $function_name;
	my $gl;
	my $glx;
	my $args;
	my $key;
	my $lastfunc;
	my $funcno;
	
	add_missing(\%oldmesa);
	read_enums();
	gen_params();

#
# emit header
#
	print << "EOF";
#ifndef __NFOSMESA_H__
#define __NFOSMESA_H__

#include <gem.h>
#include <stddef.h>
#include <stdint.h>
#include <ldg.h>

#if !defined(__MSHORT__) && (defined(__PUREC__) && __PUREC__ < 0x400)
# define __MSHORT__ 1
#endif

#if (!defined(__PUREC__) || !defined(__MSHORT__) || defined(__GEMLIB__)) && !defined(__USE_GEMLIB)
#define __USE_GEMLIB 1
#endif
#ifndef _WORD
# if defined(__GEMLIB__) || defined(__USE_GEMLIB)
#  define _WORD short
# else
#  define _WORD int
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*
 * load & initialize NFOSmesa
 */
struct gl_public *ldg_load_osmesa(const char *libname, _WORD *gl);

/*
 * init NFOSmesa from already loaded lib
 */
int ldg_init_osmesa(LDG *lib);

/*
 * unload NFOSmesa
 */
void ldg_unload_osmesa(struct gl_public *pub, _WORD *gl);


#ifdef __cplusplus
}
#endif


EOF

	my $filename = $inc_gltypes;
	if ( ! defined(open(INC, $filename)) ) {
		&warn("cannot include \"$filename\" in output file");
		print '#include "gltypes.h"' . "\n";
	} else {
		my $line;
		print "\n";
		while ($line = <INC>) {
			chomp($line);
			print "$line\n";
		}
		print "\n";
		close(INC)
	}

	foreach $key (sort { sort_by_name } keys %floatfuncs) {
		print "#undef ${key}\n";
	}

	print << "EOF";

#ifdef __cplusplus
extern "C" {
#endif

struct _gl_osmesa {
EOF

	$lastfunc = 1;
	foreach $key (sort { sort_by_value } keys %functions) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$funcno = $ent->{funcno};
		while ($lastfunc != $funcno)
		{
			printf "\t/* %4d */ void *__unused_%d;\n", $lastfunc - 1, $lastfunc - 1;
			++$lastfunc;
		}
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		printf "\t/* %4d */ ${return_type} APIENTRY (*${glx}${function_name})(${prototype});\n", $funcno - 1;
		$gl_count++ if ($gl eq "gl");
		$glu_count++ if ($gl eq "glu");
		$osmesa_count++ if ($gl eq "OSMesa");
		$lastfunc = $funcno + 1;
	}

	print << "EOF";
	unsigned int __numfuncs;
	OSMESAproc APIENTRY (*__old_OSMesaGetProcAddress)(const char *funcName);
};

extern struct _gl_osmesa gl;


#ifndef NFOSMESA_NO_MANGLE
EOF

	foreach $key (sort { sort_by_value } keys %functions) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "#undef ${gl}${function_name}\n";
		print "#define ${gl}${function_name} (gl.${glx}${function_name})\n";
	}

#
# emit trailer
#
	print << "EOF";
#endif

$ldg_trailer

/* Functions generated: $osmesa_count OSMesa + $gl_count GL + $glu_count GLU */

#endif /* __NFOSMESA_H__ */
EOF
}


sub sort_tinygl_by_value
{
	my $ret = $tinygl{$a} <=> $tinygl{$b};
	return $ret;
}


sub gen_tinyldgheader() {
	my $tinygl_count = 0;
	my $ent;
	my $prototype;
	my $return_type;
	my $function_name;
	my $gl;
	my $glx;
	my $args;
	my $key;
	
	add_missing(\%oldmesa);
	gen_params();

#
# emit header
#
	print << "EOF";
#ifndef __LDG_TINY_GL_H__
#define __LDG_TINY_GL_H__
#ifndef __TINY_GL_H__
# define __TINY_GL_H__ 1
#endif

#include <gem.h>
#include <stddef.h>
#include <ldg.h>

#if !defined(__MSHORT__) && (defined(__PUREC__) && __PUREC__ < 0x400)
# define __MSHORT__ 1
#endif

#if (!defined(__PUREC__) || !defined(__MSHORT__) || defined(__GEMLIB__)) && !defined(__USE_GEMLIB)
#define __USE_GEMLIB 1
#endif
#ifndef _WORD
# if defined(__GEMLIB__) || defined(__USE_GEMLIB)
#  define _WORD short
# else
#  define _WORD int
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * load & initialize TinyGL
 */
struct gl_public *ldg_load_tiny_gl(const char *libname, _WORD *gl);

/*
 * init TinyGL from already loaded lib
 */
int ldg_init_tiny_gl(LDG *lib);

/*
 * unload TinyGL
 */
void ldg_unload_tiny_gl(struct gl_public *pub, _WORD *gl);

#ifdef __cplusplus
}
#endif


EOF

	my $filename = $inc_gltypes;
	if ( ! defined(open(INC, $filename)) ) {
		&warn("cannot include \"$filename\" in output file");
		print '#include "gltypes.h"' . "\n";
	} else {
		my $line;
		print "\n";
		while ($line = <INC>) {
			chomp($line);
			print "$line\n";
		}
		print "\n";
		close(INC)
	}

	foreach $key (sort { sort_by_name } keys %floatfuncs) {
		print "#undef ${key}\n";
	}

	print << "EOF";

/*
 * old LDG functions
 */
#ifdef __cplusplus
extern "C" {
#endif

GLAPI void *GLAPIENTRY OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height);
GLAPI void GLAPIENTRY OSMesaDestroyLDG(void);
GLAPI GLsizei GLAPIENTRY max_width(void);
GLAPI GLsizei GLAPIENTRY max_height(void);
GLAPI void GLAPIENTRY swapbuffer(void *buf);
GLAPI void GLAPIENTRY exception_error(void CALLBACK (*exception)(GLenum param));
GLAPI void GLAPIENTRY tinyglinformation(void);

GLAPI void GLAPIENTRY glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
GLAPI void GLAPIENTRY glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
GLAPI void GLAPIENTRY glClearDepthf(GLfloat depth);
GLAPI void GLAPIENTRY gluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ);

struct _gl_tiny {
EOF

	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		if (!defined($functions{$key}))
		{
			die "$me: $key should be exported to tinygl but is not defined";
		}
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "\t${return_type} APIENTRY (*${glx}${function_name})(${prototype});\n";
		$tinygl_count++;
	}

	print << "EOF";

};

extern struct _gl_tiny gl;

#ifdef __cplusplus
}
#endif


#ifndef NFOSMESA_NO_MANGLE
EOF

	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "#undef ${gl}${function_name}\n";
		print "#define ${gl}${function_name} (gl.${glx}${function_name})\n";
	}

#
# emit trailer
#
	print << "EOF";

#endif

$ldg_trailer
$tinygl_trailer

/* Functions generated: $tinygl_count */

#endif /* __LDG_TINY_GL_H__ */
EOF
} # end of gen_tinyldgheader


sub gen_tinyldglink() {
	my $tinygl_count = 0;
	my $ent;
	my $prototype;
	my $prototype_desc;
	my $return_type;
	my $function_name;
	my $complete_name;
	my $gl;
	my $glx;
	my $args;
	my $ret;
	my $key;
	
	add_missing(\%oldmesa);
	gen_params();

	print "#undef NUM_TINYGL_PROCS\n";
	print "\n";
	
	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		if (!defined($functions{$key}))
		{
			die "$me: $key should be exported to tinygl but is not defined";
		}
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		if ($return_type eq "void")
		{
			$ret = "voidf";
		} else {
			$ret = "return ";
		}
		$prototype_desc = $prototype;
		if ($prototype eq "void")
		{
			$prototype = "NOTHING";
		} else {
			$prototype = "AND " . $prototype;
		}
		$complete_name = $gl . $function_name;
		# remove trailing 'f' from export name
		if (substr($complete_name, -1) eq "f" && defined($floatfuncs{substr($complete_name, 0, length($complete_name) - 1)})) {
			chop($function_name);
		}
		$glx = "";
		$glx = "tinygl" if ($complete_name eq "information" || $complete_name eq "swapbuffer" || $complete_name eq "exception_error");
		print "GL_PROC($return_type, $ret, \"${gl}${function_name}\", ${glx}${complete_name}, \"${return_type} ${complete_name}(${prototype_desc})\", ($prototype), ($args))\n";
		$tinygl_count++;
	}

	print << "EOF";

#undef GL_PROC
#undef GL_PROC64
#define NUM_TINYGL_PROCS $tinygl_count

/* Functions generated: $tinygl_count */

EOF
} # end of gen_tinyldglink


sub ldg_export($)
{
	my ($ent) = @_;
	my $function_name = $ent->{name};
	my $prototype = $ent->{proto};
	my $return_type = $ent->{type};
	my $gl = $ent->{gl};
	my $glx = $gl;
	$glx = "" if ($glx eq "gl");
	my $lookup = "${gl}${function_name}";
	if (defined($floatfuncs{$lookup}))
	{
		# GL compatible glOrtho() is exported as "glOrthod"
		$lookup = $floatfuncs{$lookup};
	} elsif (substr($lookup, -1) eq "f" && defined($floatfuncs{substr($lookup, 0, length($lookup) - 1)}))
	{
		# the function exported as "glOrtho" actually is glOrthof
		chop($lookup);
	} elsif ($lookup eq "tinyglinformation")
	{
		$lookup = "information";
	}
	print "\tglp->${glx}${function_name} = ($return_type APIENTRY (*)($prototype)) ldg_find(\"${lookup}\", lib);\n";
	print "\tGL_CHECK(glp->${glx}${function_name});\n";
}


sub gen_ldgsource() {
	my $ent;
	my $key;
	my $numfuncs;
	my $function_name;
	my $gl;
	my $glx;
	
	add_missing(\%oldmesa);
	read_enums();
	gen_params();

#
# emit header
#
	print << "EOF";
/* Bindings of osmesa.ldg
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <stdlib.h>
#include <ldg.h>
#define NFOSMESA_NO_MANGLE
#include <ldg/osmesa.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

struct _gl_osmesa gl;

#if defined(__PUREC__) && !defined(__AHCC__)
/*
 * Pure-C is not able to compile the result if you enable it,
 * probable the function gets too large
 */
# define GL_CHECK(x)
#else
# define GL_CHECK(x) if (x == 0) result = FALSE
#endif


static APIENTRY OSMESAproc real_OSMesaGetProcAddress(const char *funcname)
{
	unsigned int func = (unsigned int)((*gl.__old_OSMesaGetProcAddress)(funcname));
	if (func == 0 || func > gl.__numfuncs)
		return 0;
	--func;
	return (OSMESAproc)(((void **)(&gl))[func]);
}


int ldg_init_osmesa(LDG *lib)
{
	int result = TRUE;
	struct _gl_osmesa *glp = &gl;
	
EOF

	foreach $key (sort { sort_by_name } keys %floatfuncs) {
		print "#undef ${key}\n";
	}
	foreach $key (sort { sort_by_value } keys %functions) {
		$ent = $functions{$key};
		$numfuncs = $ent->{funcno};
		ldg_export($ent);
	}

#
# emit trailer
#
	print << "EOF";
	gl.__numfuncs = $numfuncs;
	gl.__old_OSMesaGetProcAddress = gl.OSMesaGetProcAddress;
	gl.OSMesaGetProcAddress = real_OSMesaGetProcAddress;
	return result;
}
#undef GL_CHECK


struct gl_public *ldg_load_osmesa(const char *libname, _WORD *gl)
{
	LDG *lib;
	struct gl_public *pub = NULL;
	size_t len;
	
	if (libname == NULL)
		libname = "osmesa.ldg";
	if (gl == NULL)
		gl = ldg_global;
	lib = ldg_open(libname, gl);
	if (lib != NULL)
	{
		long APIENTRY (*glInit)(void *) = (long APIENTRY (*)(void *))ldg_find("glInit", lib);
		if (glInit)
		{
			len = (*glInit)(NULL);
		} else
		{
			len = sizeof(*pub);
		}
		pub = (struct gl_public *)calloc(1, len);
		if (pub)
		{
			pub->m_alloc = malloc;
			pub->m_free = free;
			if (glInit)
				(*glInit)(pub);
			pub->libhandle = lib;
			ldg_init_osmesa(lib);
		} else
		{
			ldg_close(lib, gl);
		}
	}
	return pub;
}


void ldg_unload_osmesa(struct gl_public *pub, _WORD *gl)
{
	if (pub != NULL)
	{
		if (pub->libhandle != NULL)
		{
			if (gl == NULL)
				gl = ldg_global;
			ldg_close((LDG *)pub->libhandle, gl);
		}
		pub->m_free(pub);
	}
}


#ifdef TEST

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	struct gl_public *pub;
	OSMESAproc p;
	OSMesaContext ctx;
	void *buffer;
	int width = 32;
	int height = 32;
	
	pub = ldg_load_osmesa(0, 0);
	if (pub == NULL)
		pub = ldg_load_osmesa("c:/gemsys/ldg/osmesa.ldg", 0);
	if (pub == NULL)
	{
		fprintf(stderr, "osmesa.ldg not found\\n");
		return 1;
	}
	ctx = gl.OSMesaCreateContextExt(OSMESA_RGB, 16, 0, 0, NULL);
	if (ctx == NULL)
	{
		fprintf(stderr, "can't create context\\n");
		return 1;
	}
	buffer = malloc(width * height * 4);
	gl.OSMesaMakeCurrent(ctx, buffer, OSMESA_RGB, width, height);
	printf("GL_RENDERER   = %s\\n", (const char *) gl.GetString(GL_RENDERER));
	printf("GL_VERSION    = %s\\n", (const char *) gl.GetString(GL_VERSION));
	printf("GL_VENDOR     = %s\\n", (const char *) gl.GetString(GL_VENDOR));
	printf("GL_EXTENSIONS = %s\\n", (const char *) gl.GetString(GL_EXTENSIONS));
EOF
	foreach $key (sort { sort_by_name } keys %functions) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "\tp = gl.OSMesaGetProcAddress(\"${gl}${function_name}\");\n";
		printf "\tprintf(\"%-30s: %%p\\n\", p);\n", ${gl} . ${function_name};
	}
	print << "EOF";
	gl.OSMesaDestroyContext(ctx);
	ldg_unload_osmesa(pub, NULL);
	return 0;
}

#endif
EOF
} # end of generated source, and also of gen_ldgsource


sub gen_tinyldgsource() {
	my $ent;
	my $key;
	
	add_missing(\%oldmesa);
	gen_params();

#
# emit header
#
	print << "EOF";
/* Bindings of tiny_gl.ldg
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <stdlib.h>
#include <ldg.h>
#define NFOSMESA_NO_MANGLE
#include <ldg/tiny_gl.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

struct _gl_tiny gl;

#define GL_CHECK(x) if (x == 0) result = FALSE

int ldg_init_tiny_gl(LDG *lib)
{
	int result = TRUE;
	struct _gl_tiny *glp = &gl;
	
EOF

	foreach $key (sort { sort_by_name } keys %floatfuncs) {
		print "#undef ${key}\n";
	}
	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		if (!defined($functions{$key}))
		{
			die "$me: $key should be exported to tinygl but is not defined";
		}
		$ent = $functions{$key};
		ldg_export($ent);
	}

#
# emit trailer
#
	print << "EOF";
	return result;
}
#undef GL_CHECK


struct gl_public *ldg_load_tiny_gl(const char *libname, _WORD *gl)
{
	LDG *lib;
	struct gl_public *pub = NULL;
	size_t len;
	
	if (libname == NULL)
		libname = "tiny_gl.ldg";
	if (gl == NULL)
		gl = ldg_global;
	lib = ldg_open(libname, gl);
	if (lib != NULL)
	{
		long APIENTRY (*glInit)(void *) = (long APIENTRY (*)(void *))ldg_find("glInit", lib);
		if (glInit)
		{
			len = (*glInit)(NULL);
		} else
		{
			len = sizeof(*pub);
		}
		pub = (struct gl_public *)calloc(1, len);
		if (pub)
		{
			pub->m_alloc = malloc;
			pub->m_free = free;
			if (glInit)
				(*glInit)(pub);
			pub->libhandle = lib;
			ldg_init_tiny_gl(lib);
		} else
		{
			ldg_close(lib, gl);
		}
	}
	return pub;
}


void ldg_unload_tiny_gl(struct gl_public *pub, _WORD *gl)
{
	if (pub != NULL)
	{
		if (pub->libhandle != NULL)
		{
			if (gl == NULL)
				gl = ldg_global;
			ldg_close((LDG *)pub->libhandle, gl);
		}
		pub->m_free(pub);
	}
}


#ifdef TEST

#include <stdio.h>

int main(void)
{
	struct gl_public *pub;
	
	pub = ldg_load_tiny_gl(0, 0);
	if (pub == NULL)
		pub = ldg_load_tiny_gl("c:/gemsys/ldg/tin_gl.ldg", 0);
	if (pub == NULL)
	{
		fprintf(stderr, "tiny_gl.ldg not found\\n");
		return 1;
	}
	printf("%s: %lx\\n", "glBegin", gl.Begin);
	printf("%s: %lx\\n", "glOrthof", gl.Orthof);
	ldg_unload_tiny_gl(pub, NULL);
	return 0;
}
#endif
EOF
}  # end of generated source, and also of gen_tinyldgsource


sub gen_tinyslbheader() {
	my $tinygl_count = 0;
	my $ent;
	my $prototype;
	my $return_type;
	my $function_name;
	my $gl;
	my $glx;
	my $args;
	my $key;
	
	add_missing(\%oldmesa);
	gen_params();

#
# emit header
#
	print << "EOF";
#ifndef __SLB_TINY_GL_H__
#define __SLB_TINY_GL_H__
#ifndef __TINY_GL_H__
# define __TINY_GL_H__ 1
#endif

#include <gem.h>
#include <stddef.h>
#include <mint/slb.h>

#if !defined(__MSHORT__) && (defined(__PUREC__) && __PUREC__ < 0x400)
# define __MSHORT__ 1
#endif

#if (!defined(__PUREC__) || !defined(__MSHORT__) || defined(__GEMLIB__)) && !defined(__USE_GEMLIB)
#define __USE_GEMLIB 1
#endif
#ifndef _WORD
# if defined(__GEMLIB__) || defined(__USE_GEMLIB)
#  define _WORD short
# else
#  define _WORD int
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * load & initialize TinyGL
 */
struct gl_public *slb_load_tiny_gl(const char *path);

/*
 * unload TinyGL
 */
void slb_unload_tiny_gl(struct gl_public *pub);


#ifdef __cplusplus
}
#endif


EOF

	my $filename = $inc_gltypes;
	if ( ! defined(open(INC, $filename)) ) {
		&warn("cannot include \"$filename\" in output file");
		print '#include "gltypes.h"' . "\n";
	} else {
		my $line;
		print "\n";
		while ($line = <INC>) {
			chomp($line);
			print "$line\n";
		}
		print "\n";
		close(INC)
	}

	foreach $key (sort { sort_by_name } keys %floatfuncs) {
		print "#undef ${key}\n";
	}

	print << "EOF";

/*
 * old LDG functions
 */
#ifdef __cplusplus
extern "C" {
#endif

GLAPI void *GLAPIENTRY OSMesaCreateLDG(GLenum format, GLenum type, GLint width, GLint height);
GLAPI void GLAPIENTRY OSMesaDestroyLDG(void);
GLAPI GLsizei GLAPIENTRY max_width(void);
GLAPI GLsizei GLAPIENTRY max_height(void);
GLAPI void GLAPIENTRY swapbuffer(void *buf);
GLAPI void GLAPIENTRY exception_error(void CALLBACK (*exception)(GLenum param));
GLAPI void GLAPIENTRY tinyglinformation(void);

GLAPI void GLAPIENTRY glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
GLAPI void GLAPIENTRY glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
GLAPI void GLAPIENTRY glClearDepthf(GLfloat depth);
GLAPI void GLAPIENTRY gluLookAtf(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ);

struct _gl_tiny {
EOF

	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		if (!defined($functions{$key}))
		{
			die "$me: $key should be exported to tinygl but is not defined";
		}
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "\t${return_type} APIENTRY (*${glx}${function_name})(${prototype});\n";
		$tinygl_count++;
	}

	print << "EOF";

};

extern struct _gl_tiny gl;

#ifdef __cplusplus
}
#endif


#ifndef NFOSMESA_NO_MANGLE
EOF

	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "#undef ${gl}${function_name}\n";
		print "#define ${gl}${function_name} (gl.${glx}${function_name})\n";
	}

#
# emit trailer
#
	print << "EOF";

#endif

$ldg_trailer
$tinygl_trailer

/* Functions generated: $tinygl_count */

#endif /* __SLB_TINY_GL_H__ */
EOF
} # end of gen_tinyslbheader


sub gen_slbheader() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $osmesa_count = 0;
	my $ent;
	my $prototype;
	my $return_type;
	my $function_name;
	my $gl;
	my $glx;
	my $args;
	my $key;
	my $lastfunc;
	my $funcno;
	
	add_missing(\%oldmesa);
	read_enums();
	gen_params();

#
# emit header
#
	print << "EOF";
#ifndef __NFOSMESA_H__
#define __NFOSMESA_H__

#include <gem.h>
#include <stddef.h>
#include <stdint.h>
#include <mint/slb.h>

#if !defined(__MSHORT__) && (defined(__PUREC__) && __PUREC__ < 0x400)
# define __MSHORT__ 1
#endif

#if (!defined(__PUREC__) || !defined(__MSHORT__) || defined(__GEMLIB__)) && !defined(__USE_GEMLIB)
#define __USE_GEMLIB 1
#endif
#ifndef _WORD
# if defined(__GEMLIB__) || defined(__USE_GEMLIB)
#  define _WORD short
# else
#  define _WORD int
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * load & initialize NFOsmesa
 */
struct gl_public *slb_load_osmesa(const char *path);

/*
 * unload NFOsmesa
 */
void slb_unload_osmesa(struct gl_public *pub);


#ifdef __cplusplus
}
#endif


EOF

	my $filename = $inc_gltypes;
	if ( ! defined(open(INC, $filename)) ) {
		&warn("cannot include \"$filename\" in output file");
		print '#include "gltypes.h"' . "\n";
	} else {
		my $line;
		print "\n";
		while ($line = <INC>) {
			chomp($line);
			print "$line\n";
		}
		print "\n";
		close(INC)
	}

	foreach $key (sort { sort_by_name } keys %floatfuncs) {
		print "#undef ${key}\n";
	}

	print << "EOF";

#ifdef __cplusplus
extern "C" {
#endif

struct _gl_osmesa {
EOF

	$lastfunc = 1;
	foreach $key (sort { sort_by_value } keys %functions) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$funcno = $ent->{funcno};
		while ($lastfunc != $funcno)
		{
			printf "\t/* %4d */ void *__unused_%d;\n", $lastfunc - 1, $lastfunc - 1;
			++$lastfunc;
		}
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		printf "\t/* %4d */ ${return_type} APIENTRY (*${glx}${function_name})(${prototype});\n", $funcno - 1;
		$gl_count++ if ($gl eq "gl");
		$glu_count++ if ($gl eq "glu");
		$osmesa_count++ if ($gl eq "OSMesa");
		$lastfunc = $funcno + 1;
	}

	print << "EOF";
	unsigned int __numfuncs;
	OSMESAproc APIENTRY (*__old_OSMesaGetProcAddress)(const char *funcName);
};

extern struct _gl_osmesa gl;

#ifdef __cplusplus
}
#endif


#ifndef NFOSMESA_NO_MANGLE
EOF

	foreach $key (sort { sort_by_value } keys %functions) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "#undef ${gl}${function_name}\n";
		print "#define ${gl}${function_name} (gl.${glx}${function_name})\n";
	}

#
# emit trailer
#
	print << "EOF";

#endif

$ldg_trailer

/* Functions generated: $osmesa_count OSMesa + $gl_count GL + $glu_count GLU */

#endif /* __NFOSMESA_H__ */
EOF
} # end of gen_slbheader


sub slb_export($)
{
	my ($ent) = @_;
	my $function_name = $ent->{name};
	my $return_type = $ent->{type};
	my $prototype = $ent->{proto};
	my $params = $ent->{params};
	my $argcount = $#$params + 1;
	my $args = $ent->{args};
	my $gl = $ent->{gl};
	my $funcno = $ent->{funcno};
	my $nfapi = $ent->{nfapi};
	my $glx = $gl;
	my $ret;
	my $first_param;
	my $exectype1;
	my $exectype2;
	
	$nfapi = '' if (!defined($nfapi));
	
	$glx = "" if ($glx eq "gl");
	print "static $return_type APIENTRY exec_${gl}${function_name}($prototype)\n";
	print "{\n";
	my $nargs = 0;
	for (my $argc = 0; $argc < $argcount; $argc++)
	{
		my $param = $params->[$argc];
		my $type = $param->{type};
		my $name = $param->{name};
		my $pointer = $param->{pointer};
		if ($type eq 'GLdouble' || $type eq 'GLclampd' || defined($longlong_types{$type}))
		{
			$nargs += 2;
		} else {
			$nargs += 1;
		}
	}
	$first_param = first_param_addr($ent);
	if ($args ne "") {
		$args = ", " . $args;
	}
	$exectype1 = "long  __CDECL (*";
	$exectype2 = ")(SLB_HANDLE, long, long, void *, void *)";
	
	if (defined($longlong_rettypes{$return_type}))
	{
		$exectype2 = ")(SLB_HANDLE, long, long, void *, void *, void *)";
		print "\tGLuint64 __retval = 0;\n";
	}
	print "\t${exectype1}exec${exectype2} = (${exectype1}${exectype2})gl_exec;\n";
	for (my $argc = 1; $argc < $argcount; $argc++)
	{
		my $param = $params->[$argc];
		my $name = $param->{name};
		print "\t(void)$name;\n";
	}
	if (defined($longlong_rettypes{$return_type}))
	{
		print "\t(*exec)(gl_slb, $funcno /* $nfapi */, SLB_NARGS(3), gl_pub, $first_param, &__retval);\n";
		print "\treturn __retval;\n";
	} else
	{
		if ($return_type eq 'void') {
			$ret = '';
		} else {
			$ret = "return ($return_type)";
		}
		print "\t${ret}(*exec)(gl_slb, $funcno /* $nfapi */, SLB_NARGS(2), gl_pub, $first_param);\n";
	}
	print "}\n\n";
}


sub gen_tinyslbsource() {
	my $ent;
	my $key;
	my $function_name;
	my $gl;
	my $glx;
	
	add_missing(\%oldmesa);
	read_enums();
	gen_params();

#
# emit header
#
	print << "EOF";
/* Bindings of tiny_gl.slb
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <stdlib.h>
#include <mint/slb.h>
#define NFOSMESA_NO_MANGLE
#include <slb/tiny_gl.h>
#include <mintbind.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

#ifdef __PUREC__
#pragma warn -stv
#endif

struct _gl_tiny gl;
static SLB_HANDLE gl_slb;
static SLB_EXEC gl_exec;
static struct gl_public *gl_pub;

/*
 * The "nwords" argument should actually only be a "short".
 * MagiC will expect it that way, with the actual arguments
 * following.
 * However, a "short" in the actual function definition
 * will be treated as promoted to int.
 * So we pass a long instead, with the upper half
 * set to 1 + nwords to account for the extra space.
 * This also has the benefit of keeping the stack longword aligned.
 */
#undef SLB_NWORDS
#define SLB_NWORDS(_nwords) ((((long)(_nwords) + 1l) << 16) | (long)(_nwords))
#undef SLB_NARGS
#define SLB_NARGS(_nargs) SLB_NWORDS(_nargs * 2)



EOF

	foreach $key (sort { sort_by_name } keys %floatfuncs) {
		print "#undef ${key}\n";
	}
	print "\n";

#
# emit wrapper functions
#
	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		if (!defined($functions{$key}))
		{
			die "$me: $key should be exported to tinygl but is not defined";
		}
		$ent = $functions{$key};
		$ent->{funcno} = $tinygl{$key};
		slb_export($ent);
	}

	print << "EOF";
static void slb_init_tiny_gl(void)
{
	struct _gl_tiny *glp = &gl;
EOF

	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		if (!defined($functions{$key}))
		{
			die "$me: $key should be exported to tinygl but is not defined";
		}
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "\tglp->${glx}${function_name} = exec_${gl}${function_name};\n";
	}

#
# emit trailer
#
	print << "EOF";
}


struct gl_public *slb_load_tiny_gl(const char *path)
{
	long ret;
	size_t len;
	struct gl_public *pub = NULL;
	
	/*
	 * Slbopen() checks the name of the file with the
	 * compiled-in library name, so there is no way
	 * to pass an alternative filename here
	 */
	ret = Slbopen("tiny_gl.slb", path, 4 /* ARANFOSMESA_NFAPI_VERSION */, &gl_slb, &gl_exec);
	if (ret >= 0)
	{
		long  __CDECL (*exec)(SLB_HANDLE, long, long, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *))gl_exec;
		
		len = (*exec)(gl_slb, 0, SLB_NARGS(1), NULL);
		pub = gl_pub = (struct gl_public *)calloc(1, len);
		if (pub)
		{
			pub->m_alloc = malloc;
			pub->m_free = free;
			pub->libhandle = gl_slb;
			pub->libexec = gl_exec;
			(*exec)(gl_slb, 0, SLB_NARGS(1), pub);
			slb_init_tiny_gl();
		}
	}
	return pub;
}


void slb_unload_tiny_gl(struct gl_public *pub)
{
	if (pub != NULL)
	{
		if (pub->libhandle != NULL)
		{
			Slbclose(pub->libhandle);
			gl_slb = 0;
		}
		pub->m_free(pub);
	}
}
EOF
}  # end of generated source, and also of gen_tinyslbsource



sub gen_slbsource() {
	my $ent;
	my $key;
	my $function_name;
	my $gl;
	my $glx;
	my $numfuncs;
	
	add_missing(\%oldmesa);
	read_enums();
	gen_params();

#
# emit header
#
	print << "EOF";
/* Bindings of osmesa.slb
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <stdlib.h>
#include <mint/slb.h>
#define NFOSMESA_NO_MANGLE
#include <slb/osmesa.h>
#include <mintbind.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

#ifdef __PUREC__
#pragma warn -stv
#endif

struct _gl_osmesa gl;
static SLB_HANDLE gl_slb;
static SLB_EXEC gl_exec;
static struct gl_public *gl_pub;

/*
 * The "nwords" argument should actually only be a "short".
 * MagiC will expect it that way, with the actual arguments
 * following.
 * However, a "short" in the actual function definition
 * will be treated as promoted to int.
 * So we pass a long instead, with the upper half
 * set to 1 + nwords to account for the extra space.
 * This also has the benefit of keeping the stack longword aligned.
 */
#undef SLB_NWORDS
#define SLB_NWORDS(_nwords) ((((long)(_nwords) + 1l) << 16) | (long)(_nwords))
#undef SLB_NARGS
#define SLB_NARGS(_nargs) SLB_NWORDS(_nargs * 2)


EOF

	foreach $key (sort { sort_by_name } keys %floatfuncs) {
		print "#undef ${key}\n";
	}
	print "\n";

#
# emit wrapper functions
#
	foreach $key (sort { sort_by_value } keys %functions) {
		$ent = $functions{$key};
		slb_export($ent);
	}

	print << "EOF";


static APIENTRY OSMESAproc real_OSMesaGetProcAddress(const char *funcname)
{
	unsigned int func = (unsigned int)((*gl.__old_OSMesaGetProcAddress)(funcname));
	if (func == 0 || func > gl.__numfuncs)
		return 0;
	--func;
	return (OSMESAproc)(((void **)(&gl))[func]);
}


static void slb_init_osmesa(void)
{
	struct _gl_osmesa *glp = &gl;
EOF

	foreach $key (sort { sort_by_value } keys %functions) {
		$ent = $functions{$key};
		$numfuncs = $ent->{funcno};
		$function_name = $ent->{name};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "\tglp->${glx}${function_name} = exec_${gl}${function_name};\n";
	}

#
# emit trailer
#
	print << "EOF";
	gl.__numfuncs = $numfuncs;
	gl.__old_OSMesaGetProcAddress = gl.OSMesaGetProcAddress;
	gl.OSMesaGetProcAddress = real_OSMesaGetProcAddress;
}


struct gl_public *slb_load_osmesa(const char *path)
{
	long ret;
	size_t len;
	struct gl_public *pub = NULL;
	
	/*
	 * Slbopen() checks the name of the file with the
	 * compiled-in library name, so there is no way
	 * to pass an alternative filename here
	 */
	ret = Slbopen("osmesa.slb", path, 4 /* ARANFOSMESA_NFAPI_VERSION */, &gl_slb, &gl_exec);
	if (ret >= 0)
	{
		long  __CDECL (*exec)(SLB_HANDLE, long, long, void *) = (long  __CDECL (*)(SLB_HANDLE, long, long, void *))gl_exec;
		
		len = (*exec)(gl_slb, 0, SLB_NARGS(1), NULL);
		pub = gl_pub = (struct gl_public *)calloc(1, len);
		if (pub)
		{
			pub->m_alloc = malloc;
			pub->m_free = free;
			pub->libhandle = gl_slb;
			pub->libexec = gl_exec;
			(*exec)(gl_slb, 0, SLB_NARGS(1), pub);
			slb_init_osmesa();
		}
	}
	return pub;
}


void slb_unload_osmesa(struct gl_public *pub)
{
	if (pub != NULL)
	{
		if (pub->libhandle != NULL)
		{
			Slbclose(pub->libhandle);
			gl_slb = 0;
		}
		pub->m_free(pub);
	}
}


#ifdef TEST

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	struct gl_public *pub;
	OSMESAproc p;
	OSMesaContext ctx;
	void *buffer;
	int width = 32;
	int height = 32;
	
	pub = slb_load_osmesa(NULL);
	if (pub == NULL)
	{
		fprintf(stderr, "osmesa.slb not found\\n");
		return 1;
	}
	ctx = gl.OSMesaCreateContextExt(OSMESA_RGB, 16, 0, 0, NULL);
	if (ctx == NULL)
	{
		fprintf(stderr, "can't create context\\n");
		return 1;
	}
	buffer = malloc(width * height * 4);
	gl.OSMesaMakeCurrent(ctx, buffer, OSMESA_RGB, width, height);
	printf("GL_RENDERER   = %s\\n", (const char *) gl.GetString(GL_RENDERER));
	printf("GL_VERSION    = %s\\n", (const char *) gl.GetString(GL_VERSION));
	printf("GL_VENDOR     = %s\\n", (const char *) gl.GetString(GL_VENDOR));
	printf("GL_EXTENSIONS = %s\\n", (const char *) gl.GetString(GL_EXTENSIONS));
EOF
	foreach $key (sort { sort_by_name } keys %functions) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "\tp = gl.OSMesaGetProcAddress(\"${gl}${function_name}\");\n";
		printf "\tprintf(\"%-30s: %%p\\n\", p);\n", ${gl} . ${function_name};
	}
	print << "EOF";
	gl.OSMesaDestroyContext(ctx);
	slb_unload_osmesa(pub);
	return 0;
}

#endif
EOF
}  # end of generated source, and also of gen_slbsource



sub usage()
{
	print "Usage: $me [-protos|-macros|-calls|-dispatch]\n";
}

#
# main()
#
if ($ARGV[0] eq '-incfile') {
	shift @ARGV;
	$inc_gltypes = $ARGV[0];
	shift @ARGV;
}
if ($ARGV[0] eq '-enums') {
	shift @ARGV;
	$enumfile = $ARGV[0];
	shift @ARGV;
}

unshift(@ARGV, '-') unless @ARGV;

if ($ARGV[0] eq '-protos') {
	shift @ARGV;
	print_header();
	read_includes();
	gen_protos();
} elsif ($ARGV[0] eq '-macros') {
	shift @ARGV;
	print_header();
	read_includes();
	if (defined($enumfile)) {
		gen_macros_bynumber();
	} else {
		gen_macros();
	}
} elsif ($ARGV[0] eq '-calls') {
	shift @ARGV;
	print_header();
	read_includes();
	gen_calls();
} elsif ($ARGV[0] eq '-dispatch') {
	shift @ARGV;
	print_header();
	read_includes();
	gen_dispatch();
} elsif ($ARGV[0] eq '-paramcount') {
	shift @ARGV;
	print_header();
	read_includes();
	gen_paramcount();
} elsif ($ARGV[0] eq '-ldgheader') {
	shift @ARGV;
	read_includes();
	gen_ldgheader();
} elsif ($ARGV[0] eq '-ldgsource') {
	shift @ARGV;
	read_includes();
	gen_ldgsource();
} elsif ($ARGV[0] eq '-tinyldgheader') {
	shift @ARGV;
	read_includes();
	gen_tinyldgheader();
} elsif ($ARGV[0] eq '-tinyldgsource') {
	shift @ARGV;
	read_includes();
	gen_tinyldgsource();
} elsif ($ARGV[0] eq '-tinyldglink') {
	shift @ARGV;
	print_header();
	read_includes();
	gen_tinyldglink();
} elsif ($ARGV[0] eq '-tinyslbheader') {
	shift @ARGV;
	read_includes();
	gen_tinyslbheader();
} elsif ($ARGV[0] eq '-slbheader') {
	shift @ARGV;
	read_includes();
	gen_slbheader();
} elsif ($ARGV[0] eq '-tinyslbsource') {
	shift @ARGV;
	read_includes();
	gen_tinyslbsource();
} elsif ($ARGV[0] eq '-slbsource') {
	shift @ARGV;
	read_includes();
	gen_slbsource();
} elsif ($ARGV[0] eq '-help' || $ARGV[0] eq '--help') {
	shift @ARGV;
	usage();
	exit(0);
} elsif ($ARGV[0] =~ /^-/) {
	die "$me: unknown option $ARGV[0]"
} else {
	print_header();
	read_includes();
	gen_protos();
}
