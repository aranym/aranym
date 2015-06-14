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
#          - add enums for new functions to enum-gl.h (at the end, dont reuse any existing numbers!)
#            This is not done automatically, because the numbers must not change for old functions.
#          - If new types are used in the prototypes, add them to atari/nfosmesa/gltypes.h
#
#     The current version (API Version 2) was generated from <GL/gl.h> on Linux,
#     glext.h from Khronos group (http://www.opengl.org/registry/api/GL/glext.h, $Revision: 26745 $Date: 2014-05-21 03:12:26)
#     and osmesa.h from Mesa 10.1.4.
#
# -macros:
#     Generate macro calls that are used to generate several tables.
#     The macro must be defined before including the generated file
#     and has the signature:
#        GL_PROC(type, gl, name, export, upper, params, first, ret)
#     with
#        type: return type of function
#        gl: group the function belongs to, either gl, glu or OSMesa
#        name: name of function, with (gl|glu) prefix removed
#        export: exported name of function. Different only for some
#                functions which take float arguments that are exported from LDG
#                and conflict with OpenGL functions of the same name
#                that take double arguments.
#        upper: name of function in uppercase
#        params: prototype of the function, including parenthesis
#        first: address of the first argument
#        ret: code to return the function result, or GL_void_return
#     Optionally, OSMESA_PROC() can be defined, with the same
#     signature, to process OSMesa functions.
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

use strict;

my $me = "dyngl.pl";

my %functions;
my $warnings = 0;

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
	'glFenceSync' => 1,
	'glMapBuffer' => 1,
	'glMapBufferARB' => 1,
	'glMapBufferRange' => 1,
	'glMapObjectBufferATI' => 1,
	'glMapNamedBufferEXT' => 1,
	'glMapNamedBufferRangeEXT' => 1,
	'glMapTexture2DINTEL' => 1,
	'glImportSyncEXT' => 1,
	'glProgramCallbackMESA' => 1,
);

#
# typedefs from headers that are already pointer types
#
my %pointer_types = (
	'GLsync' => 1,
	'GLDEBUGPROC' => 1,
	'GLDEBUGPROCARB' => 1,
	'GLDEBUGPROCAMD' => 1,
	'GLprogramcallbackMESA' => 1,
);

#
# 8-bit types that dont have endian issues
#
my %byte_types = (
	'GLubyte' => 1,
	'GLchar' => 1,
	'GLbyte' => 1,
	'char' => 1,
	'GLcharARB' => 1,
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
	'GLenum' => '0x%x',
	'GLbitfield' => '0x%x',
	'GLfloat' => '%f',
	'GLclampf' => '%f',
	'GLdouble' => '%f',
	'GLclampd' => '%f',
	'GLfixed' => '0x%x',
	'GLintptr' => '%ld',
	'GLintptrARB' => '%ld',
	'GLsizeiptr' => '%ld',
	'GLsizeiptrARB' => '%ld',
	'GLint64' => '%" PRId64 "',
	'GLint64EXT' => '%" PRId64 "',
	'GLuint64' => '%" PRIu64 "',
	'GLuint64EXT' => '%" PRIu64 "',
	'long' => '%ld',
	'GLhandleARB' => '%u',
	'GLsync' => '%p',
	'GLvdpauSurfaceNV' => '%ld',
	'GLDEBUGPROC' => '%p',
	'GLDEBUGPROCAMD' => '%p',
	'GLDEBUGPROCARB' => '%p',
	'GLprogramcallbackMESA' => '%p',
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
		my $prototype = "";
		my $any_pointer = 0;
		my $printf_format = "";
		my $format;
		for (my $argc = 0; $argc < $argcount; $argc++)
		{
			my $param = $params->[$argc];
			my $pointer = $param->{pointer};
			my $type = $param->{type};
			my $name = $param->{name};
			$args .= ", " if ($argc != 0);
			$args .= $name;
			if ($pointer) {
				if (!defined($byte_types{$type})) {
					$any_pointer = 2;
				} elsif ($any_pointer == 0) {
					$any_pointer = 1;
				}
				if ($pointer > 1) {
					$pointer = "";
				} else {
					$pointer = "*";
				}
				$format = "%p";
			} else {
				$pointer = "";
				if (!defined($printf_formats{$type}))
				{
					&warn("dont know how to printf values of type $type");
					$format = "%d";
				} else
				{
					$format = $printf_formats{$type}
				}
			}
			$prototype .= ", " if ($argc != 0);
			$prototype .= "${type} ${pointer}${name}";
			$printf_format .= ", " if ($argc != 0);
			$printf_format .= $format;
		}
		$prototype = "void" unless ($prototype ne "");
		$ent->{args} = $args;
		$ent->{proto} = $prototype;
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

#
# functions that need special attention;
# each one needs a corresponding macro
#
my %macros = (
	'glAreTexturesResident' => 1,
	'glAreTexturesResidentEXT' => 1,
	'glArrayElement' => 1,
	'glArrayElementEXT' => 1,
	'glBinormal3dvEXT' => 1,
	'glBinormal3fvEXT' => 1,
	'glBinormal3ivEXT' => 1,
	'glBinormal3svEXT' => 1,
	'glCallLists' => 1,
	'glClipPlane' => 1,
	'glClipPlanefOES' => 1,
	'glClipPlanexOES' => 1,
	'glColor3dv' => 1,
	'glColor3fVertex3fvSUN' => 1,
	'glColor3fv' => 1,
	'glColor3hvNV' => 1,
	'glColor3iv' => 1,
	'glColor3sv' => 1,
	'glColor3uiv' => 1,
	'glColor3usv' => 1,
	'glColor3xvOES' => 1,
	'glColor4dv' => 1,
	'glColor4fNormal3fVertex3fvSUN' => 1,
	'glColor4fv' => 1,
	'glColor4hvNV' => 1,
	'glColor4iv' => 1,
	'glColor4sv' => 1,
	'glColor4ubVertex2fvSUN' => 1,
	'glColor4ubVertex3fvSUN' => 1,
	'glColor4uiv' => 1,
	'glColor4usv' => 1,
	'glColor4xvOES' => 1,
	'glColorP3uiv' => 1,
	'glColorP4uiv' => 1,
	'glColorPointer' => 1,
	'glColorPointerEXT' => 1,
	'glColorSubTable' => 1,
	'glColorTable' => 1,
	'glDeleteTextures' => 1,
	'glDeleteTexturesEXT' => 1,
	'glDisableClientState' => 1,
	'glDrawArrays' => 1,
	'glDrawElements' => 1,
	'glDrawRangeElements' => 1,
	'glEdgeFlagPointer' => 1,
	'glEnableClientState' => 1,
	'glEvalCoord1dv' => 1,
	'glEvalCoord1fv' => 1,
	'glEvalCoord2dv' => 1,
	'glEvalCoord2fv' => 1,
	'glFinish' => 1,
	'glFlush' => 1,
	'glFogCoordhvNV' => 1,
	'glFogfv' => 1,
	'glFogiv' => 1,
	'glGenTextures' => 1,
	'glGetDoublev' => 1,
	'glGetFloatv' => 1,
	'glGetIntegerv' => 1,
	'glIndexPointer' => 1,
	'glIndexdv' => 1,
	'glIndexfv' => 1,
	'glIndexiv' => 1,
	'glIndexsv' => 1,
	'glInterleavedArrays' => 1,
	'glLightModelfv' => 1,
	'glLightModeliv' => 1,
	'glLightfv' => 1,
	'glLightiv' => 1,
	'glLoadMatrixd' => 1,
	'glLoadMatrixf' => 1,
	'glLoadTransposeMatrixd' => 1,
	'glLoadTransposeMatrixdARB' => 1,
	'glLoadTransposeMatrixf' => 1,
	'glLoadTransposeMatrixfARB' => 1,
	'glMap1d' => 1,
	'glMap1f' => 1,
	'glMap2d' => 1,
	'glMap2f' => 1,
	'glMaterialfv' => 1,
	'glMaterialiv' => 1,
	'glMultMatrixd' => 1,
	'glMultMatrixf' => 1,
	'glMultTransposeMatrixd' => 1,
	'glMultTransposeMatrixdARB' => 1,
	'glMultTransposeMatrixf' => 1,
	'glMultTransposeMatrixfARB' => 1,
	'glMultiTexCoord1dv' => 1,
	'glMultiTexCoord1dvARB' => 1,
	'glMultiTexCoord1fv' => 1,
	'glMultiTexCoord1fvARB' => 1,
	'glMultiTexCoord1hvNV' => 1,
	'glMultiTexCoord1iv' => 1,
	'glMultiTexCoord1ivARB' => 1,
	'glMultiTexCoord1sv' => 1,
	'glMultiTexCoord1svARB' => 1,
	'glMultiTexCoord2dv' => 1,
	'glMultiTexCoord2dvARB' => 1,
	'glMultiTexCoord2fv' => 1,
	'glMultiTexCoord2fvARB' => 1,
	'glMultiTexCoord2hvNV' => 1,
	'glMultiTexCoord2iv' => 1,
	'glMultiTexCoord2ivARB' => 1,
	'glMultiTexCoord2sv' => 1,
	'glMultiTexCoord2svARB' => 1,
	'glMultiTexCoord3dv' => 1,
	'glMultiTexCoord3dvARB' => 1,
	'glMultiTexCoord3fv' => 1,
	'glMultiTexCoord3fvARB' => 1,
	'glMultiTexCoord3hvNV' => 1,
	'glMultiTexCoord3iv' => 1,
	'glMultiTexCoord3ivARB' => 1,
	'glMultiTexCoord3sv' => 1,
	'glMultiTexCoord3svARB' => 1,
	'glMultiTexCoord4dv' => 1,
	'glMultiTexCoord4dvARB' => 1,
	'glMultiTexCoord4fv' => 1,
	'glMultiTexCoord4fvARB' => 1,
	'glMultiTexCoord4hvNV' => 1,
	'glMultiTexCoord4iv' => 1,
	'glMultiTexCoord4ivARB' => 1,
	'glMultiTexCoord4sv' => 1,
	'glMultiTexCoord4svARB' => 1,
	'glNormal3dv' => 1,
	'glNormal3fVertex3fvSUN' => 1,
	'glNormal3fv' => 1,
	'glNormal3hvNV' => 1,
	'glNormal3iv' => 1,
	'glNormal3sv' => 1,
	'glNormalPointer' => 1,
	'glNormalPointerEXT' => 1,
	'glNormalStream3dvATI' => 1,
	'glNormalStream3fvATI' => 1,
	'glNormalStream3ivATI' => 1,
	'glNormalStream3svATI' => 1,
	'glPixelMapfv' => 1,
	'glPixelMapuiv' => 1,
	'glPixelMapusv' => 1,
	'glPrioritizeTextures' => 1,
	'glPrioritizeTexturesEXT' => 1,
	'glProgramEnvParameter4dvARB' => 1,
	'glProgramEnvParameter4fvARB' => 1,
	'glProgramLocalParameter4dvARB' => 1,
	'glProgramLocalParameter4fvARB' => 1,
	'glProgramNamedParameter4dvNV' => 1,
	'glProgramNamedParameter4fvNV' => 1,
	'glProgramParameter4dvNV' => 1,
	'glProgramParameter4fvNV' => 1,
	'glProgramParameters4dvNV' => 1,
	'glProgramParameters4fvNV' => 1,
	'glRasterPos2dv' => 1,
	'glRasterPos2fv' => 1,
	'glRasterPos2iv' => 1,
	'glRasterPos2sv' => 1,
	'glRasterPos3dv' => 1,
	'glRasterPos3fv' => 1,
	'glRasterPos3iv' => 1,
	'glRasterPos3sv' => 1,
	'glRasterPos4dv' => 1,
	'glRasterPos4fv' => 1,
	'glRasterPos4iv' => 1,
	'glRasterPos4sv' => 1,
	'glRectdv' => 1,
	'glRectfv' => 1,
	'glRectiv' => 1,
	'glRectsv' => 1,
	'glSecondaryColor3dv' => 1,
	'glSecondaryColor3dvEXT' => 1,
	'glSecondaryColor3fv' => 1,
	'glSecondaryColor3fvEXT' => 1,
	'glSecondaryColor3hvNV' => 1,
	'glSecondaryColor3iv' => 1,
	'glSecondaryColor3ivEXT' => 1,
	'glSecondaryColor3sv' => 1,
	'glSecondaryColor3svEXT' => 1,
	'glSecondaryColor3uiv' => 1,
	'glSecondaryColor3uivEXT' => 1,
	'glSecondaryColor3usv' => 1,
	'glSecondaryColor3usvEXT' => 1,
	'glTangent3dvEXT' => 1,
	'glTangent3fvEXT' => 1,
	'glTangent3ivEXT' => 1,
	'glTangent3svEXT' => 1,
	'glTexCoord1dv' => 1,
	'glTexCoord1fv' => 1,
	'glTexCoord1hvNV' => 1,
	'glTexCoord1iv' => 1,
	'glTexCoord1sv' => 1,
	'glTexCoord2dv' => 1,
	'glTexCoord2fColor3fVertex3fvSUN' => 1,
	'glTexCoord2fColor4fNormal3fVertex3fvSUN' => 1,
	'glTexCoord2fColor4ubVertex3fvSUN' => 1,
	'glTexCoord2fNormal3fVertex3fvSUN' => 1,
	'glTexCoord2fVertex3fvSUN' => 1,
	'glTexCoord2fv' => 1,
	'glTexCoord2hvNV' => 1,
	'glTexCoord2iv' => 1,
	'glTexCoord2sv' => 1,
	'glTexCoord3dv' => 1,
	'glTexCoord3fv' => 1,
	'glTexCoord3hvNV' => 1,
	'glTexCoord3iv' => 1,
	'glTexCoord3sv' => 1,
	'glTexCoord4dv' => 1,
	'glTexCoord4fColor4fNormal3fVertex4fvSUN' => 1,
	'glTexCoord4fVertex4fvSUN' => 1,
	'glTexCoord4fv' => 1,
	'glTexCoord4hvNV' => 1,
	'glTexCoord4iv' => 1,
	'glTexCoord4sv' => 1,
	'glTexCoordPointer' => 1,
	'glTexCoordPointerEXT' => 1,
	'glTexEnvfv' => 1,
	'glTexEnviv' => 1,
	'glTexGendv' => 1,
	'glTexGenfv' => 1,
	'glTexGeniv' => 1,
	'glTexParameterfv' => 1,
	'glTexParameteriv' => 1,
	'glUniform1fv' => 1,
	'glUniform1fvARB' => 1,
	'glUniform1iv' => 1,
	'glUniform1ivARB' => 1,
	'glUniform2fv' => 1,
	'glUniform2fvARB' => 1,
	'glUniform2iv' => 1,
	'glUniform2ivARB' => 1,
	'glUniform3fv' => 1,
	'glUniform3fvARB' => 1,
	'glUniform3iv' => 1,
	'glUniform3ivARB' => 1,
	'glUniform4fv' => 1,
	'glUniform4fvARB' => 1,
	'glUniform4iv' => 1,
	'glUniform4ivARB' => 1,
	'glVertex2dv' => 1,
	'glVertex2fv' => 1,
	'glVertex2hvNV' => 1,
	'glVertex2iv' => 1,
	'glVertex2sv' => 1,
	'glVertex3dv' => 1,
	'glVertex3fv' => 1,
	'glVertex3hvNV' => 1,
	'glVertex3iv' => 1,
	'glVertex3sv' => 1,
	'glVertex4dv' => 1,
	'glVertex4fv' => 1,
	'glVertex4hvNV' => 1,
	'glVertex4iv' => 1,
	'glVertex4sv' => 1,
	'glVertexAttrib1dv' => 1,
	'glVertexAttrib1dvARB' => 1,
	'glVertexAttrib1dvNV' => 1,
	'glVertexAttrib1fv' => 1,
	'glVertexAttrib1fvARB' => 1,
	'glVertexAttrib1fvNV' => 1,
	'glVertexAttrib1hvNV' => 1,
	'glVertexAttrib1sv' => 1,
	'glVertexAttrib1svARB' => 1,
	'glVertexAttrib1svNV' => 1,
	'glVertexAttrib2dv' => 1,
	'glVertexAttrib2dvARB' => 1,
	'glVertexAttrib2dvNV' => 1,
	'glVertexAttrib2fv' => 1,
	'glVertexAttrib2fvARB' => 1,
	'glVertexAttrib2fvNV' => 1,
	'glVertexAttrib2hvNV' => 1,
	'glVertexAttrib2sv' => 1,
	'glVertexAttrib2svARB' => 1,
	'glVertexAttrib2svNV' => 1,
	'glVertexAttrib3dv' => 1,
	'glVertexAttrib3dvARB' => 1,
	'glVertexAttrib3dvNV' => 1,
	'glVertexAttrib3fv' => 1,
	'glVertexAttrib3fvARB' => 1,
	'glVertexAttrib3fvNV' => 1,
	'glVertexAttrib3hvNV' => 1,
	'glVertexAttrib3sv' => 1,
	'glVertexAttrib3svARB' => 1,
	'glVertexAttrib3svNV' => 1,
	'glVertexAttrib4Niv' => 1,
	'glVertexAttrib4NivARB' => 1,
	'glVertexAttrib4Nsv' => 1,
	'glVertexAttrib4NsvARB' => 1,
	'glVertexAttrib4Nuiv' => 1,
	'glVertexAttrib4NuivARB' => 1,
	'glVertexAttrib4Nusv' => 1,
	'glVertexAttrib4NusvARB' => 1,
	'glVertexAttrib4dv' => 1,
	'glVertexAttrib4dvARB' => 1,
	'glVertexAttrib4dvNV' => 1,
	'glVertexAttrib4fv' => 1,
	'glVertexAttrib4fvARB' => 1,
	'glVertexAttrib4fvNV' => 1,
	'glVertexAttrib4hvNV' => 1,
	'glVertexAttrib4iv' => 1,
	'glVertexAttrib4ivARB' => 1,
	'glVertexAttrib4sv' => 1,
	'glVertexAttrib4svARB' => 1,
	'glVertexAttrib4svNV' => 1,
	'glVertexAttrib4uiv' => 1,
	'glVertexAttrib4uivARB' => 1,
	'glVertexAttrib4usv' => 1,
	'glVertexAttrib4usvARB' => 1,
	'glVertexAttribs1dvNV' => 1,
	'glVertexAttribs1fvNV' => 1,
	'glVertexAttribs1hvNV' => 1,
	'glVertexAttribs1svNV' => 1,
	'glVertexAttribs2dvNV' => 1,
	'glVertexAttribs2fvNV' => 1,
	'glVertexAttribs2hvNV' => 1,
	'glVertexAttribs2svNV' => 1,
	'glVertexAttribs3dvNV' => 1,
	'glVertexAttribs3fvNV' => 1,
	'glVertexAttribs3hvNV' => 1,
	'glVertexAttribs3svNV' => 1,
	'glVertexAttribs4dvNV' => 1,
	'glVertexAttribs4fvNV' => 1,
	'glVertexAttribs4hvNV' => 1,
	'glVertexAttribs4svNV' => 1,
	'glVertexPointer' => 1,
	'glVertexPointerEXT' => 1,
	'glVertexStream1dvATI' => 1,
	'glVertexStream1fvATI' => 1,
	'glVertexStream1ivATI' => 1,
	'glVertexStream1svATI' => 1,
	'glVertexStream2dvATI' => 1,
	'glVertexStream2fvATI' => 1,
	'glVertexStream2ivATI' => 1,
	'glVertexStream2svATI' => 1,
	'glVertexStream3dvATI' => 1,
	'glVertexStream3fvATI' => 1,
	'glVertexStream3ivATI' => 1,
	'glVertexStream3svATI' => 1,
	'glVertexStream4dvATI' => 1,
	'glVertexStream4fvATI' => 1,
	'glVertexStream4ivATI' => 1,
	'glVertexStream4svATI' => 1,
	'glVertexWeightfvEXT' => 1,
	'glVertexWeighthvNV' => 1,
	'glWindowPos2dv' => 1,
	'glWindowPos2dvARB' => 1,
	'glWindowPos2dvMESA' => 1,
	'glWindowPos2fv' => 1,
	'glWindowPos2fvARB' => 1,
	'glWindowPos2fvMESA' => 1,
	'glWindowPos2iv' => 1,
	'glWindowPos2ivARB' => 1,
	'glWindowPos2ivMESA' => 1,
	'glWindowPos2sv' => 1,
	'glWindowPos2svARB' => 1,
	'glWindowPos2svMESA' => 1,
	'glWindowPos3dv' => 1,
	'glWindowPos3dvARB' => 1,
	'glWindowPos3dvMESA' => 1,
	'glWindowPos3fv' => 1,
	'glWindowPos3fvARB' => 1,
	'glWindowPos3fvMESA' => 1,
	'glWindowPos3iv' => 1,
	'glWindowPos3ivARB' => 1,
	'glWindowPos3ivMESA' => 1,
	'glWindowPos3sv' => 1,
	'glWindowPos3svARB' => 1,
	'glWindowPos3svMESA' => 1,
	'glWindowPos4dvMESA' => 1,
	'glWindowPos4dvMESA' => 1,
	'glWindowPos4fvMESA' => 1,
	'glWindowPos4ivMESA' => 1,
	'glWindowPos4svMESA' => 1,
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


sub sort_by_name
{
	my $ret = $a cmp $b;
	return $ret;
}

#
# generate functions
#
sub gen_calls() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $ret;
	my $prototype;
	my $return_type;
	my $function_name;
	my $uppername;
	my $gl;
	my $args;
	my $printf_format;
	my $conversions_needed = 0;
	
	gen_params();
	foreach my $key (sort { sort_by_name } keys %functions) {
		my $ent = $functions{$key};
		$gl = $ent->{gl};
		next if ($gl ne 'gl' && $gl ne 'glu');
		$function_name = $gl . $ent->{name};
		$return_type = $ent->{type};
		$prototype = $ent->{proto};
		$args = $ent->{args};
		$printf_format = $ent->{printf_format};
		$uppername = uc($function_name);
	
		if ($return_type eq "void")
		{
			$ret = "";
		} else {
			$ret = "return ";
		}
		print("#if 0\n") if ($gl ne 'gl' || defined($blacklist{$function_name}));
		print "$return_type OSMesaDriver::nf$function_name($prototype)\n";
		print "{\n";
		print "\tD(bug(\"nfosmesa: $function_name($printf_format)\"";
		print ", " unless ($args eq "");
		print "$args));\n";
		if (defined($macros{$function_name})) {
			print "FN_${uppername}(${args});\n";
		} else {
			if ($ent->{any_pointer} == 2)
			{
#				&warn("$function_name may need conversion");
				print "\t/* TODO: NFOSMESA_${uppername} may need conversion */\n";
				++$conversions_needed;
			}
			print "\t${ret}fn.${function_name}(${args});\n";
		}
		print "}\n";
		print("#endif\n") if ($gl ne 'gl' || defined($blacklist{$function_name}));
		print "\n";
		$gl_count++ if ($gl eq "gl");
		$glu_count++ if ($gl eq "glu");
	}
	print STDERR "$conversions_needed function(s) may need conversion macros\n" unless($conversions_needed == 0);
#
# emit trailer
#
	print << "EOF";

/* Functions generated: $gl_count GL + $glu_count GLU */
EOF
}


#
# generate case statements
#
sub gen_dispatch() {
	my $gl_count = 0;
	my $glu_count = 0;
	my $ret;
	my $prototype;
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
		$prototype = $ent->{proto};
		$params = $ent->{params};
		
		$uppername = uc($function_name);
		print("#if 0\n") if (defined($blacklist{$function_name}));
		print "\t\tcase $prefix$uppername:\n";
		if ($return_type eq "void")
		{
			$ret = "";
		} else {
			$ret = "ret = ";
			if ($return_type =~ /\*/ || defined($pointer_types{$return_type}) || $return_type eq 'GLhandleARB')
			{
				$ret .= '(uint32)(uintptr_t)';
			}
		}
		my $argcount = $#$params + 1;
		print "\t\t\t${ret}nf${function_name}(";
		
		if ($argcount > 0) {
			my $paramnum = 0;
			print "\n\t\t\t\t";
			for (my $argc = 0; $argc < $argcount; $argc++)
			{
				print ",\n\t\t\t\t" unless ($argc == 0);
				my $type = $params->[$argc]->{type};
				my $name = $params->[$argc]->{name};
				my $pointer = $params->[$argc]->{pointer} ? "*" : "";
				if ($pointer ne "" || defined($pointer_types{$type})) {
					print "(${type} ${pointer})getStackedPointer($paramnum)";
					++$paramnum;
				} elsif ($type eq 'GLdouble' || $type eq 'GLclampd')
				{
					print "getStackedDouble($paramnum)";
					$paramnum += 2;
				} elsif ($type eq 'GLfloat' || $type eq 'GLclampf')
				{
					print "getStackedFloat($paramnum)";
					++$paramnum;
				} elsif ($type eq 'GLint64' || $type eq 'GLint64EXT' || $type eq 'GLuint64' || $type eq 'GLuint64EXT')
				{
					print "getStackedParameter64($paramnum)";
					$paramnum += 2;
				} elsif ($type eq 'GLhandleARB')
				{
					# legacy MacOSX headers declare GLhandleARB as void *
					print "(GLhandleARB)getStackedParameter($paramnum)";
					++$paramnum;
				} else
				{
					print "getStackedParameter($paramnum)";
					++$paramnum;
				}
				print " /* ${type} ${pointer}${name} */";
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
#ifndef GL_GETSTRING
#define GL_GETSTRING(type, gl, name, export, upper, params, first, ret) GL_PROC(type, gl, name, export, upper, params, first, ret)
#define GL_GETSTRINGI(type, gl, name, export, upper, params, first, ret) GL_PROC(type, gl, name, export, upper, params, first, ret)
#endif
#ifndef GLU_PROC
#define GLU_PROC(type, gl, name, export, upper, params, first, ret) GL_PROC(type, gl, name, export, upper, params, first, ret)
#endif
#ifndef OSMESA_PROC
#define OSMESA_PROC(type, gl, name, export, upper, params, first, ret)
#endif
#undef GL_void_return
#define GL_void_return

EOF

	$lastgl = "";
	foreach my $key (sort { sort_by_name } keys %functions) {
		my $ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		my $args = $ent->{args};
		my $params = $ent->{params};
		$prototype = $ent->{proto};
		if ($args eq "") {
			$first_param = 'NULL';
		} else {
			$first_param = '&' . $params->[0]->{name};
		}
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
		$export = $floatfuncs{$complete_name} if (defined($floatfuncs{$complete_name}));

		print("#if !defined(TINYGL_ONLY)\n") if (! defined($tinygl{$complete_name}));
		if ($complete_name eq 'glGetString')
		{
			# hack for glGetString() which is handled separately
			print "${prefix}_GETSTRING($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), $first_param, $ret)\n";
		} elsif ($complete_name eq 'glGetStringi')
		{
			# hack for glGetStringi() which is handled separately
			print "${prefix}_GETSTRINGI($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), $first_param, $ret)\n";
		} else
		{
			print "${prefix}_PROC($return_type, $gl, $function_name, $export, " . uc($function_name) . ", ($prototype), $first_param, $ret)\n";
		}
		print("#endif\n") if (! defined($tinygl{$complete_name}));
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
#undef GLU_PROC
#undef GL_GETSTRING
#undef GL_GETSTRINGI
#undef OSMESA_PROC
#undef GL_void_return
EOF
}


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
#		'name' => 'glInit',
#		'gl' => '',
#		'type' => 'void',
#		'params' => [
#			{ 'type' => 'GLint', 'name' => 'mode', 'pointer' => 0 },
#		],
#	},
);


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

';

my $tinygl_trailer = '
/*
 * no-ops in TinyGL
 */
#undef glFinish
#define glFinish()

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
	
	add_missing(\%oldmesa);
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

/*
 * load & initialize NFOSmesa
 */
LDG *ldg_load_osmesa(const char *libname, _WORD *gl);

/*
 * init NFOSmesa from already loaded lib
 */
int ldg_init_osmesa(LDG *lib);

/*
 * unload NFOSmesa
 */
void ldg_unload_osmesa(LDG *lib, _WORD *gl);


EOF

	my $filename = "atari/nfosmesa/gltypes.h";
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

	print << "EOF";

struct _gl_osmesa {
EOF

	foreach $key (sort { sort_by_name } keys %functions) {
		$ent = $functions{$key};
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$args = $ent->{args};
		$prototype = $ent->{proto};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "\t${return_type} APIENTRY (*${glx}${function_name})(${prototype});\n";
		$gl_count++ if ($gl eq "gl");
		$glu_count++ if ($gl eq "glu");
		$osmesa_count++ if ($gl eq "OSMesa");
	}

	print << "EOF";

};

extern struct _gl_osmesa gl;


#ifndef NFOSMESA_NO_MANGLE
EOF

	foreach $key (sort { sort_by_name } keys %functions) {
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

/*
 * load & initialize TinyGL
 */
LDG *ldg_load_tiny_gl(const char *libname, _WORD *gl);

/*
 * init TinyGL from already loaded lib
 */
int ldg_init_tiny_gl(LDG *lib);

/*
 * unload TinyGL
 */
void ldg_unload_tiny_gl(LDG *lib, _WORD *gl);


EOF

	my $filename = "atari/nfosmesa/gltypes.h";
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
GLAPI void GLAPIENTRY information(void);

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
		print "GL_PROC($return_type, $ret, \"${gl}${function_name}\", ${glx}${complete_name}, \"${return_type} ${complete_name}(${prototype})\", ($prototype), ($args))\n";
		$tinygl_count++;
	}

	print << "EOF";

#undef GL_PROC
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
	}
	print "\tgl.${glx}${function_name} = ($return_type APIENTRY (*)($prototype)) ldg_find(\"${lookup}\", lib);\n";
	print "\tGL_CHECK(gl.${glx}${function_name});\n";
}


sub gen_ldgsource() {
	my $ent;
	my $key;
	
	add_missing(\%oldmesa);
	gen_params();

#
# emit header
#
	print << "EOF";
/* Bindings of osmesa.ldg
 * Compile this module and link it with the application client
 */

#include <gem.h>
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

int ldg_init_osmesa(LDG *lib)
{
	int result = TRUE;
	
EOF

	foreach $key (keys %floatfuncs) {
		print "#undef ${key}\n";
	}
	foreach $key (sort { sort_by_name } keys %functions) {
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


LDG *ldg_load_osmesa(const char *libname, _WORD *gl)
{
	LDG *lib;
	
	if (libname == NULL)
		libname = "osmesa.ldg";
	if (gl == NULL)
		gl = ldg_global;
	lib = ldg_open(libname, gl);
	if (lib != NULL)
		ldg_init_osmesa(lib);
	return lib;
}


void ldg_unload_osmesa(LDG *lib, _WORD *gl)
{
	if (lib != NULL)
	{
		if (gl == NULL)
			gl = ldg_global;
		ldg_close(lib, gl);
	}
}


#ifdef TEST

#include <mint/arch/nf_ops.h>

int main(void)
{
	LDG *lib;
	
	lib = ldg_load_osmesa(0, 0);
	if (lib == NULL)
		lib = ldg_load_osmesa("c:/gemsys/ldg/osmesa.ldg", 0);
	if (lib == NULL)
	{
		nf_debugprintf("osmesa.ldg not found\\n");
		return 1;
	}
	nf_debugprintf("%s: %lx\\n", "glBegin", gl.Begin);
	nf_debugprintf("%s: %lx\\n", "glOrtho", gl.Ortho);
	nf_debugprintf("%s: %lx\\n", "glOrthof", gl.Orthof);
	ldg_unload_osmesa(lib, NULL);
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
	
EOF

	foreach $key (keys %floatfuncs) {
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


LDG *ldg_load_tiny_gl(const char *libname, _WORD *gl)
{
	LDG *lib;
	
	if (libname == NULL)
		libname = "tiny_gl.ldg";
	if (gl == NULL)
		gl = ldg_global;
	lib = ldg_open(libname, gl);
	if (lib != NULL)
		ldg_init_tiny_gl(lib);
	return lib;
}


void ldg_unload_tiny_gl(LDG *lib, _WORD *gl)
{
	if (lib != NULL)
	{
		if (gl == NULL)
			gl = ldg_global;
		ldg_close(lib, gl);
	}
}


#ifdef TEST

#include <mint/arch/nf_ops.h>

int main(void)
{
	LDG *lib;
	
	lib = ldg_load_tiny_gl(0, 0);
	if (lib == NULL)
		lib = ldg_load_tiny_gl("c:/gemsys/ldg/tin_gl.ldg", 0);
	if (lib == NULL)
	{
		nf_debugprintf("tiny_gl.ldg not found\\n");
		return 1;
	}
	nf_debugprintf("%s: %lx\\n", "glBegin", gl.Begin);
	nf_debugprintf("%s: %lx\\n", "glOrthof", gl.Orthof);
	ldg_unload_tiny_gl(lib, NULL);
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

/*
 * load & initialize TinyGL
 */
long slb_load_tiny_gl(const char *path, long min_version);

/*
 * unload TinyGL
 */
void slb_unload_tiny_gl(void);


EOF

	my $filename = "atari/nfosmesa/gltypes.h";
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
GLAPI void GLAPIENTRY information(void);

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
extern SLB_HANDLE gl_slb;
extern SLB_EXEC gl_exec;

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


sub gen_tinyslbsource() {
	my $ent;
	my $key;
	my $funcno;
	my $function_name;
	my $prototype;
	my $return_type;
	my $gl;
	my $glx;
	my $args;
	my $params;
	my $nargs;
	my $argcount;
	my $ret;
	
	add_missing(\%oldmesa);
	gen_params();

#
# emit header
#
	print << "EOF";
/* Bindings of tiny_gl.slb
 * Compile this module and link it with the application client
 */

#include <gem.h>
#include <mint/slb.h>
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

#ifndef __slb_lexec_defined
typedef long  __CDECL (*SLB_LEXEC)(SLB_HANDLE slb, long fn, long nwords, ...);
#define __slb_lexec_defined 1
#endif


EOF

	foreach $key (keys %floatfuncs) {
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
		$function_name = $ent->{name};
		$return_type = $ent->{type};
		$funcno = $tinygl{$key} - 1;
		$prototype = $ent->{proto};
		$params = $ent->{params};
		$args = $ent->{args};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "static $return_type APIENTRY exec_${gl}${function_name}($prototype)\n";
		print "{\n";
		$argcount = $#$params + 1;
		$nargs = 0;
		for (my $argc = 0; $argc < $argcount; $argc++)
		{
			my $param = $params->[$argc];
			my $type = $param->{type};
			my $name = $param->{name};
			my $pointer = $param->{pointer};
			if ($type eq 'GLdouble' || $type eq 'GLclampd' || $type eq 'GLint64' || $type eq 'GLint64EXT' || $type eq 'GLuint64' || $type eq 'GLuint64EXT')
			{
				$nargs += 2;
			} else {
				$nargs += 1;
			}
		}
		print "\tSLB_LEXEC exec = (SLB_LEXEC)gl_exec;\n";
		if ($return_type eq 'void') {
			$ret = '';
		} else {
			$ret = "return ($return_type)";
		}
		$args = ", " . $args unless ($args eq "");
		print "\t${ret}(*exec)(gl_slb, $funcno, SLB_NARGS($nargs)$args);\n";
		print "}\n\n";
	}

	print << "EOF";
static void slb_init_tiny_gl(void)
{
EOF

	foreach $key (sort { sort_tinygl_by_value } keys %tinygl) {
		if (!defined($functions{$key}))
		{
			die "$me: $key should be exported to tinygl but is not defined";
		}
		$ent = $functions{$key};
		$funcno = $tinygl{$key} - 1;
		$function_name = $ent->{name};
		$gl = $ent->{gl};
		$glx = $gl;
		$glx = "" if ($glx eq "gl");
		print "\tgl.${glx}${function_name} = exec_${gl}${function_name};\n";
	}

#
# emit trailer
#
	print << "EOF";
}


long slb_load_tiny_gl(const char *path, long min_version)
{
	long ret;
	
	ret = Slbopen("tiny_gl.slb", path, min_version, &gl_slb, &gl_exec);
	if (ret >= 0)
	{
		slb_init_tiny_gl();
	}
	return ret;
}


void slb_unload_tiny_gl(void)
{
	if (gl_slb != NULL)
	{
		Slbclose(gl_slb);
		gl_slb = 0;
	}
}
EOF
}  # end of generated source, and also of gen_tinyslbsource



sub usage()
{
	print "Usage: $me [-protos|-macros|-calls|-dispatch]\n";
}

#
# main()
#
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
	gen_macros();
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
} elsif ($ARGV[0] eq '-tinyslbsource') {
	shift @ARGV;
	read_includes();
	gen_tinyslbsource();
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
