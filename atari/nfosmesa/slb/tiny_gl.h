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



#if !defined(__MSHORT__) && (defined(__PUREC__) && __PUREC__ < 0x400)
# define __MSHORT__ 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef APIENTRY
#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#define APIENTRY __stdcall
#elif defined(__PUREC__)
#define APIENTRY __CDECL
#else
#define APIENTRY
#endif
#endif
#ifndef CALLBACK
#define CALLBACK APIENTRY
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif
#if !defined(__CDECL)
#if defined(__PUREC__)
#define __CDECL cdecl
#else
#define __CDECL
#endif
#endif


#ifndef GL_VERSION_1_1
#define GL_VERSION_1_1 1
#ifdef __MSHORT__
typedef long GLint;					/* 4-byte signed */
typedef unsigned long GLuint;		/* 4-byte unsigned */
typedef unsigned long GLenum;		/* 4-byte unsigned */
typedef unsigned long GLbitfield;	/* 4-byte unsigned */
typedef long GLsizei;
#else
typedef int GLint;					/* 4-byte signed */
typedef unsigned int GLuint;		/* 4-byte unsigned */
typedef unsigned int GLenum;		/* 4-byte unsigned */
typedef unsigned int GLbitfield;	/* 4-byte unsigned */
typedef int GLsizei;
#endif
typedef unsigned char GLboolean;
typedef signed char GLbyte;			/* 1-byte signed */
typedef unsigned char GLubyte;		/* 1-byte unsigned */
typedef short GLshort;				/* 2-byte signed */
typedef unsigned short GLushort;	/* 2-byte unsigned */
typedef float GLfloat;				/* single precision float */
typedef float GLclampf;				/* single precision float in [0, 1] */
typedef double GLdouble;			/* double precision float */
typedef double GLclampd;			/* double precision float in [0, 1] */
typedef void GLvoid;


/* Boolean values */
#define GL_FALSE			0
#define GL_TRUE				1

/* Data types */
#define GL_BYTE								0x1400
#define GL_UNSIGNED_BYTE					0x1401
#define GL_SHORT							0x1402
#define GL_UNSIGNED_SHORT					0x1403
#define GL_INT								0x1404
#define GL_UNSIGNED_INT						0x1405
#define GL_FLOAT							0x1406
#define GL_DOUBLE							0x140A
#define GL_2_BYTES							0x1407
#define GL_3_BYTES							0x1408
#define GL_4_BYTES							0x1409

/* Primitives */
#define GL_LINES							0x0001
#define GL_POINTS							0x0000
#define GL_LINE_STRIP						0x0003
#define GL_LINE_LOOP						0x0002
#define GL_TRIANGLES						0x0004
#define GL_TRIANGLE_STRIP					0x0005
#define GL_TRIANGLE_FAN						0x0006
#define GL_QUADS							0x0007
#define GL_QUAD_STRIP						0x0008
#define GL_POLYGON							0x0009
#define GL_EDGE_FLAG						0x0B43

/* Vertex Arrays */
#define GL_VERTEX_ARRAY						0x8074
#define GL_NORMAL_ARRAY						0x8075
#define GL_COLOR_ARRAY						0x8076
#define GL_INDEX_ARRAY						0x8077
#define GL_TEXTURE_COORD_ARRAY				0x8078
#define GL_EDGE_FLAG_ARRAY					0x8079
#define GL_VERTEX_ARRAY_SIZE				0x807A
#define GL_VERTEX_ARRAY_TYPE				0x807B
#define GL_VERTEX_ARRAY_STRIDE				0x807C
#define GL_VERTEX_ARRAY_COUNT				0x807D
#define GL_NORMAL_ARRAY_TYPE				0x807E
#define GL_NORMAL_ARRAY_STRIDE				0x807F
#define GL_NORMAL_ARRAY_COUNT				0x8080
#define GL_COLOR_ARRAY_SIZE					0x8081
#define GL_COLOR_ARRAY_TYPE					0x8082
#define GL_COLOR_ARRAY_STRIDE				0x8083
#define GL_COLOR_ARRAY_COUNT				0x8084
#define GL_INDEX_ARRAY_TYPE					0x8085
#define GL_INDEX_ARRAY_STRIDE				0x8086
#define GL_INDEX_ARRAY_COUNT				0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE			0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE			0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE		0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT		0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE			0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT			0x808D
#define GL_VERTEX_ARRAY_POINTER				0x808E
#define GL_NORMAL_ARRAY_POINTER				0x808F
#define GL_COLOR_ARRAY_POINTER				0x8090
#define GL_INDEX_ARRAY_POINTER				0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER		0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER			0x8093
#define GL_V2F								0x2A20
#define GL_V3F								0x2A21
#define GL_C4UB_V2F							0x2A22
#define GL_C4UB_V3F							0x2A23
#define GL_C3F_V3F							0x2A24
#define GL_N3F_V3F							0x2A25
#define GL_C4F_N3F_V3F						0x2A26
#define GL_T2F_V3F							0x2A27
#define GL_T4F_V4F							0x2A28
#define GL_T2F_C4UB_V3F						0x2A29
#define GL_T2F_C3F_V3F						0x2A2A
#define GL_T2F_N3F_V3F						0x2A2B
#define GL_T2F_C4F_N3F_V3F					0x2A2C
#define GL_T4F_C4F_N3F_V4F					0x2A2D

/* Matrix Mode */
#define GL_MATRIX_MODE						0x0BA0
#define GL_MODELVIEW						0x1700
#define GL_PROJECTION						0x1701
#define GL_TEXTURE							0x1702

/* Points */
#define GL_POINT_SMOOTH						0x0B10
#define GL_POINT_SIZE						0x0B11
#define GL_POINT_SIZE_GRANULARITY 			0x0B13
#define GL_POINT_SIZE_RANGE					0x0B12

/* Lines */
#define GL_LINE_SMOOTH						0x0B20
#define GL_LINE_STIPPLE						0x0B24
#define GL_LINE_STIPPLE_PATTERN				0x0B25
#define GL_LINE_STIPPLE_REPEAT				0x0B26
#define GL_LINE_WIDTH						0x0B21
#define GL_LINE_WIDTH_GRANULARITY			0x0B23
#define GL_LINE_WIDTH_RANGE					0x0B22

/* Polygons */
#define GL_POINT							0x1B00
#define GL_LINE								0x1B01
#define GL_FILL								0x1B02
#define GL_CCW								0x0901
#define GL_CW								0x0900
#define GL_FRONT							0x0404
#define GL_BACK								0x0405
#define GL_CULL_FACE						0x0B44
#define GL_CULL_FACE_MODE					0x0B45
#define GL_POLYGON_SMOOTH					0x0B41
#define GL_POLYGON_STIPPLE					0x0B42
#define GL_FRONT_FACE						0x0B46
#define GL_POLYGON_MODE						0x0B40
#define GL_POLYGON_OFFSET_FACTOR			0x8038
#define GL_POLYGON_OFFSET_UNITS				0x2A00
#define GL_POLYGON_OFFSET_POINT				0x2A01
#define GL_POLYGON_OFFSET_LINE				0x2A02
#define GL_POLYGON_OFFSET_FILL				0x8037

/* Display Lists */
#define GL_COMPILE							0x1300
#define GL_COMPILE_AND_EXECUTE				0x1301
#define GL_LIST_BASE						0x0B32
#define GL_LIST_INDEX						0x0B33
#define GL_LIST_MODE						0x0B30

/* Depth buffer */
#define GL_NEVER							0x0200
#define GL_LESS								0x0201
#define GL_GEQUAL							0x0206
#define GL_LEQUAL							0x0203
#define GL_GREATER							0x0204
#define GL_NOTEQUAL							0x0205
#define GL_EQUAL							0x0202
#define GL_ALWAYS							0x0207
#define GL_DEPTH_TEST						0x0B71
#define GL_DEPTH_BITS						0x0D56
#define GL_DEPTH_CLEAR_VALUE				0x0B73
#define GL_DEPTH_FUNC						0x0B74
#define GL_DEPTH_RANGE						0x0B70
#define GL_DEPTH_WRITEMASK					0x0B72
#define GL_DEPTH_COMPONENT					0x1902

/* Lighting */
#define GL_LIGHTING							0x0B50
#define GL_LIGHT0							0x4000
#define GL_LIGHT1							0x4001
#define GL_LIGHT2							0x4002
#define GL_LIGHT3							0x4003
#define GL_LIGHT4							0x4004
#define GL_LIGHT5							0x4005
#define GL_LIGHT6							0x4006
#define GL_LIGHT7							0x4007
#define GL_SPOT_EXPONENT					0x1205
#define GL_SPOT_CUTOFF						0x1206
#define GL_CONSTANT_ATTENUATION				0x1207
#define GL_LINEAR_ATTENUATION				0x1208
#define GL_QUADRATIC_ATTENUATION			0x1209
#define GL_AMBIENT							0x1200
#define GL_DIFFUSE							0x1201
#define GL_SPECULAR							0x1202
#define GL_SHININESS						0x1601
#define GL_EMISSION							0x1600
#define GL_POSITION							0x1203
#define GL_SPOT_DIRECTION					0x1204
#define GL_AMBIENT_AND_DIFFUSE				0x1602
#define GL_COLOR_INDEXES					0x1603
#define GL_LIGHT_MODEL_TWO_SIDE				0x0B52
#define GL_LIGHT_MODEL_LOCAL_VIEWER			0x0B51
#define GL_LIGHT_MODEL_AMBIENT				0x0B53
#define GL_FRONT_AND_BACK					0x0408
#define GL_SHADE_MODEL						0x0B54
#define GL_FLAT								0x1D00
#define GL_SMOOTH							0x1D01
#define GL_COLOR_MATERIAL					0x0B57
#define GL_COLOR_MATERIAL_FACE				0x0B55
#define GL_COLOR_MATERIAL_PARAMETER			0x0B56
#define GL_NORMALIZE						0x0BA1

/* User clipping planes */
#define GL_CLIP_PLANE0						0x3000
#define GL_CLIP_PLANE1						0x3001
#define GL_CLIP_PLANE2						0x3002
#define GL_CLIP_PLANE3						0x3003
#define GL_CLIP_PLANE4						0x3004
#define GL_CLIP_PLANE5						0x3005

/* Accumulation buffer */
#define GL_ACCUM_RED_BITS					0x0D58
#define GL_ACCUM_GREEN_BITS					0x0D59
#define GL_ACCUM_BLUE_BITS					0x0D5A
#define GL_ACCUM_ALPHA_BITS					0x0D5B
#define GL_ACCUM_CLEAR_VALUE				0x0B80
#define GL_ACCUM							0x0100
#define GL_ADD								0x0104
#define GL_LOAD								0x0101
#define GL_MULT								0x0103
#define GL_RETURN							0x0102

/* Alpha testing */
#define GL_ALPHA_TEST						0x0BC0
#define GL_ALPHA_TEST_REF					0x0BC2
#define GL_ALPHA_TEST_FUNC					0x0BC1

/* Blending */
#define GL_BLEND							0x0BE2
#define GL_BLEND_SRC						0x0BE1
#define GL_BLEND_DST						0x0BE0
#define GL_ZERO								0
#define GL_ONE								1
#define GL_SRC_COLOR						0x0300
#define GL_ONE_MINUS_SRC_COLOR				0x0301
#define GL_DST_COLOR						0x0306
#define GL_ONE_MINUS_DST_COLOR				0x0307
#define GL_SRC_ALPHA						0x0302
#define GL_ONE_MINUS_SRC_ALPHA				0x0303
#define GL_DST_ALPHA						0x0304
#define GL_ONE_MINUS_DST_ALPHA				0x0305
#define GL_SRC_ALPHA_SATURATE				0x0308
#define GL_CONSTANT_COLOR					0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR			0x8002
#define GL_CONSTANT_ALPHA					0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA			0x8004

/* Render Mode */
#define GL_FEEDBACK							0x1C01
#define GL_RENDER							0x1C00
#define GL_SELECT							0x1C02

/* Feedback */
#define GL_2D								0x0600
#define GL_3D								0x0601
#define GL_3D_COLOR							0x0602
#define GL_3D_COLOR_TEXTURE					0x0603
#define GL_4D_COLOR_TEXTURE					0x0604
#define GL_POINT_TOKEN						0x0701
#define GL_LINE_TOKEN						0x0702
#define GL_LINE_RESET_TOKEN					0x0707
#define GL_POLYGON_TOKEN					0x0703
#define GL_BITMAP_TOKEN						0x0704
#define GL_DRAW_PIXEL_TOKEN					0x0705
#define GL_COPY_PIXEL_TOKEN					0x0706
#define GL_PASS_THROUGH_TOKEN				0x0700
#define GL_FEEDBACK_BUFFER_POINTER			0x0DF0
#define GL_FEEDBACK_BUFFER_SIZE				0x0DF1
#define GL_FEEDBACK_BUFFER_TYPE				0x0DF2

/* Fog */
#define GL_FOG								0x0B60
#define GL_FOG_MODE							0x0B65
#define GL_FOG_DENSITY						0x0B62
#define GL_FOG_COLOR						0x0B66
#define GL_FOG_INDEX						0x0B61
#define GL_FOG_START						0x0B63
#define GL_FOG_END							0x0B64
#define GL_LINEAR							0x2601
#define GL_EXP								0x0800
#define GL_EXP2								0x0801

/* Logic Ops */
#define GL_LOGIC_OP							0x0BF1
#define GL_INDEX_LOGIC_OP					0x0BF1
#define GL_COLOR_LOGIC_OP					0x0BF2
#define GL_LOGIC_OP_MODE					0x0BF0
#define GL_CLEAR							0x1500
#define GL_SET								0x150F
#define GL_COPY								0x1503
#define GL_COPY_INVERTED					0x150C
#define GL_NOOP								0x1505
#define GL_INVERT							0x150A
#define GL_AND								0x1501
#define GL_NAND								0x150E
#define GL_OR								0x1507
#define GL_NOR								0x1508
#define GL_XOR								0x1506
#define GL_EQUIV							0x1509
#define GL_AND_REVERSE						0x1502
#define GL_AND_INVERTED						0x1504
#define GL_OR_REVERSE						0x150B
#define GL_OR_INVERTED						0x150D

/* Stencil */
#define GL_STENCIL_TEST						0x0B90
#define GL_STENCIL_WRITEMASK				0x0B98
#define GL_STENCIL_BITS						0x0D57
#define GL_STENCIL_FUNC						0x0B92
#define GL_STENCIL_VALUE_MASK				0x0B93
#define GL_STENCIL_REF						0x0B97
#define GL_STENCIL_FAIL						0x0B94
#define GL_STENCIL_PASS_DEPTH_PASS			0x0B96
#define GL_STENCIL_PASS_DEPTH_FAIL			0x0B95
#define GL_STENCIL_CLEAR_VALUE				0x0B91
#define GL_STENCIL_INDEX					0x1901
#define GL_KEEP								0x1E00
#define GL_REPLACE							0x1E01
#define GL_INCR								0x1E02
#define GL_DECR								0x1E03

/* Buffers, Pixel Drawing/Reading */
#define GL_NONE								0
#define GL_LEFT								0x0406
#define GL_RIGHT							0x0407
#define GL_FRONT_LEFT						0x0400
#define GL_FRONT_RIGHT						0x0401
#define GL_BACK_LEFT						0x0402
#define GL_BACK_RIGHT						0x0403
#define GL_AUX0								0x0409
#define GL_AUX1								0x040A
#define GL_AUX2								0x040B
#define GL_AUX3								0x040C
#define GL_COLOR_INDEX						0x1900
#define GL_RED								0x1903
#define GL_GREEN							0x1904
#define GL_BLUE								0x1905
#define GL_ALPHA							0x1906
#define GL_LUMINANCE						0x1909
#define GL_LUMINANCE_ALPHA					0x190A
#define GL_ALPHA_BITS						0x0D55
#define GL_RED_BITS							0x0D52
#define GL_GREEN_BITS						0x0D53
#define GL_BLUE_BITS						0x0D54
#define GL_INDEX_BITS						0x0D51
#define GL_SUBPIXEL_BITS					0x0D50
#define GL_AUX_BUFFERS						0x0C00
#define GL_READ_BUFFER						0x0C02
#define GL_DRAW_BUFFER						0x0C01
#define GL_DOUBLEBUFFER						0x0C32
#define GL_STEREO							0x0C33
#define GL_BITMAP							0x1A00
#define GL_COLOR							0x1800
#define GL_DEPTH							0x1801
#define GL_STENCIL							0x1802
#define GL_DITHER							0x0BD0
#define GL_RGB								0x1907
#define GL_RGBA								0x1908

/* Implementation limits */
#define GL_MAX_LIST_NESTING					0x0B31
#define GL_MAX_ATTRIB_STACK_DEPTH			0x0D35
#define GL_MAX_MODELVIEW_STACK_DEPTH		0x0D36
#define GL_MAX_NAME_STACK_DEPTH				0x0D37
#define GL_MAX_PROJECTION_STACK_DEPTH		0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH			0x0D39
#define GL_MAX_EVAL_ORDER					0x0D30
#define GL_MAX_LIGHTS						0x0D31
#define GL_MAX_CLIP_PLANES					0x0D32
#define GL_MAX_TEXTURE_SIZE					0x0D33
#define GL_MAX_PIXEL_MAP_TABLE				0x0D34
#define GL_MAX_VIEWPORT_DIMS				0x0D3A
#define GL_MAX_CLIENT_ATTRIB_STACK_DEPTH	0x0D3B
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH     0x80B3

/* Gets */
#define GL_ATTRIB_STACK_DEPTH				0x0BB0
#define GL_CLIENT_ATTRIB_STACK_DEPTH		0x0BB1
#define GL_COLOR_CLEAR_VALUE				0x0C22
#define GL_COLOR_WRITEMASK					0x0C23
#define GL_CURRENT_INDEX					0x0B01
#define GL_CURRENT_COLOR					0x0B00
#define GL_CURRENT_NORMAL					0x0B02
#define GL_CURRENT_RASTER_COLOR				0x0B04
#define GL_CURRENT_RASTER_DISTANCE			0x0B09
#define GL_CURRENT_RASTER_INDEX				0x0B05
#define GL_CURRENT_RASTER_POSITION			0x0B07
#define GL_CURRENT_RASTER_TEXTURE_COORDS	0x0B06
#define GL_CURRENT_RASTER_POSITION_VALID	0x0B08
#define GL_CURRENT_TEXTURE_COORDS			0x0B03
#define GL_INDEX_CLEAR_VALUE				0x0C20
#define GL_INDEX_MODE						0x0C30
#define GL_INDEX_WRITEMASK					0x0C21
#define GL_MODELVIEW_MATRIX					0x0BA6
#define GL_MODELVIEW_STACK_DEPTH			0x0BA3
#define GL_NAME_STACK_DEPTH					0x0D70
#define GL_PROJECTION_MATRIX				0x0BA7
#define GL_PROJECTION_STACK_DEPTH			0x0BA4
#define GL_RENDER_MODE						0x0C40
#define GL_RGBA_MODE						0x0C31
#define GL_TEXTURE_MATRIX					0x0BA8
#define GL_TEXTURE_STACK_DEPTH				0x0BA5
#define GL_VIEWPORT							0x0BA2
#define GL_COLOR_MATRIX                     0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH         0x80B2


/* Evaluators */
#define GL_AUTO_NORMAL						0x0D80
#define GL_MAP1_COLOR_4						0x0D90
#define GL_MAP1_GRID_DOMAIN					0x0DD0
#define GL_MAP1_GRID_SEGMENTS				0x0DD1
#define GL_MAP1_INDEX						0x0D91
#define GL_MAP1_NORMAL						0x0D92
#define GL_MAP1_TEXTURE_COORD_1				0x0D93
#define GL_MAP1_TEXTURE_COORD_2				0x0D94
#define GL_MAP1_TEXTURE_COORD_3				0x0D95
#define GL_MAP1_TEXTURE_COORD_4				0x0D96
#define GL_MAP1_VERTEX_3					0x0D97
#define GL_MAP1_VERTEX_4					0x0D98
#define GL_MAP2_COLOR_4						0x0DB0
#define GL_MAP2_GRID_DOMAIN					0x0DD2
#define GL_MAP2_GRID_SEGMENTS				0x0DD3
#define GL_MAP2_INDEX						0x0DB1
#define GL_MAP2_NORMAL						0x0DB2
#define GL_MAP2_TEXTURE_COORD_1				0x0DB3
#define GL_MAP2_TEXTURE_COORD_2				0x0DB4
#define GL_MAP2_TEXTURE_COORD_3				0x0DB5
#define GL_MAP2_TEXTURE_COORD_4				0x0DB6
#define GL_MAP2_VERTEX_3					0x0DB7
#define GL_MAP2_VERTEX_4					0x0DB8
#define GL_COEFF							0x0A00
#define GL_DOMAIN							0x0A02
#define GL_ORDER							0x0A01

/* Hints */
#define GL_FOG_HINT							0x0C54
#define GL_LINE_SMOOTH_HINT					0x0C52
#define GL_PERSPECTIVE_CORRECTION_HINT		0x0C50
#define GL_POINT_SMOOTH_HINT				0x0C51
#define GL_POLYGON_SMOOTH_HINT				0x0C53
#define GL_DONT_CARE						0x1100
#define GL_FASTEST							0x1101
#define GL_NICEST							0x1102

/* Scissor box */
#define GL_SCISSOR_TEST						0x0C11
#define GL_SCISSOR_BOX						0x0C10

/* Pixel Mode / Transfer */
#define GL_MAP_COLOR						0x0D10
#define GL_MAP_STENCIL						0x0D11
#define GL_INDEX_SHIFT						0x0D12
#define GL_INDEX_OFFSET						0x0D13
#define GL_RED_SCALE						0x0D14
#define GL_RED_BIAS							0x0D15
#define GL_GREEN_SCALE						0x0D18
#define GL_GREEN_BIAS						0x0D19
#define GL_BLUE_SCALE						0x0D1A
#define GL_BLUE_BIAS						0x0D1B
#define GL_ALPHA_SCALE						0x0D1C
#define GL_ALPHA_BIAS						0x0D1D
#define GL_DEPTH_SCALE						0x0D1E
#define GL_DEPTH_BIAS						0x0D1F
#define GL_PIXEL_MAP_S_TO_S_SIZE			0x0CB1
#define GL_PIXEL_MAP_I_TO_I_SIZE			0x0CB0
#define GL_PIXEL_MAP_I_TO_R_SIZE			0x0CB2
#define GL_PIXEL_MAP_I_TO_G_SIZE			0x0CB3
#define GL_PIXEL_MAP_I_TO_B_SIZE			0x0CB4
#define GL_PIXEL_MAP_I_TO_A_SIZE			0x0CB5
#define GL_PIXEL_MAP_R_TO_R_SIZE			0x0CB6
#define GL_PIXEL_MAP_G_TO_G_SIZE			0x0CB7
#define GL_PIXEL_MAP_B_TO_B_SIZE			0x0CB8
#define GL_PIXEL_MAP_A_TO_A_SIZE			0x0CB9
#define GL_PIXEL_MAP_S_TO_S					0x0C71
#define GL_PIXEL_MAP_I_TO_I					0x0C70
#define GL_PIXEL_MAP_I_TO_R					0x0C72
#define GL_PIXEL_MAP_I_TO_G					0x0C73
#define GL_PIXEL_MAP_I_TO_B					0x0C74
#define GL_PIXEL_MAP_I_TO_A					0x0C75
#define GL_PIXEL_MAP_R_TO_R					0x0C76
#define GL_PIXEL_MAP_G_TO_G					0x0C77
#define GL_PIXEL_MAP_B_TO_B					0x0C78
#define GL_PIXEL_MAP_A_TO_A					0x0C79
#define GL_PACK_ALIGNMENT					0x0D05
#define GL_PACK_LSB_FIRST					0x0D01
#define GL_PACK_ROW_LENGTH					0x0D02
#define GL_PACK_SKIP_PIXELS					0x0D04
#define GL_PACK_SKIP_ROWS					0x0D03
#define GL_PACK_SWAP_BYTES					0x0D00
#define GL_UNPACK_ALIGNMENT					0x0CF5
#define GL_UNPACK_LSB_FIRST					0x0CF1
#define GL_UNPACK_ROW_LENGTH				0x0CF2
#define GL_UNPACK_SKIP_PIXELS				0x0CF4
#define GL_UNPACK_SKIP_ROWS					0x0CF3
#define GL_UNPACK_SWAP_BYTES				0x0CF0
#define GL_ZOOM_X							0x0D16
#define GL_ZOOM_Y							0x0D17

/* Texture mapping */
#define GL_TEXTURE_ENV						0x2300
#define GL_TEXTURE_ENV_MODE					0x2200
#define GL_TEXTURE_1D						0x0DE0
#define GL_TEXTURE_2D						0x0DE1
#define GL_TEXTURE_WRAP_S					0x2802
#define GL_TEXTURE_WRAP_T					0x2803
#define GL_TEXTURE_MAG_FILTER				0x2800
#define GL_TEXTURE_MIN_FILTER				0x2801
#define GL_TEXTURE_ENV_COLOR				0x2201
#define GL_TEXTURE_GEN_S					0x0C60
#define GL_TEXTURE_GEN_T					0x0C61
#define GL_TEXTURE_GEN_MODE					0x2500
#define GL_TEXTURE_BORDER_COLOR				0x1004
#define GL_TEXTURE_WIDTH					0x1000
#define GL_TEXTURE_HEIGHT					0x1001
#define GL_TEXTURE_BORDER					0x1005
#define GL_TEXTURE_COMPONENTS				0x1003
#define GL_TEXTURE_RED_SIZE					0x805C
#define GL_TEXTURE_GREEN_SIZE				0x805D
#define GL_TEXTURE_BLUE_SIZE				0x805E
#define GL_TEXTURE_ALPHA_SIZE				0x805F
#define GL_TEXTURE_LUMINANCE_SIZE			0x8060
#define GL_TEXTURE_INTENSITY_SIZE			0x8061
#define GL_NEAREST_MIPMAP_NEAREST			0x2700
#define GL_NEAREST_MIPMAP_LINEAR			0x2702
#define GL_LINEAR_MIPMAP_NEAREST			0x2701
#define GL_LINEAR_MIPMAP_LINEAR				0x2703
#define GL_OBJECT_LINEAR					0x2401
#define GL_OBJECT_PLANE						0x2501
#define GL_EYE_LINEAR						0x2400
#define GL_EYE_PLANE						0x2502
#define GL_SPHERE_MAP						0x2402
#define GL_DECAL							0x2101
#define GL_MODULATE							0x2100
#define GL_NEAREST							0x2600
#define GL_REPEAT							0x2901
#define GL_CLAMP							0x2900
#define GL_S								0x2000
#define GL_T								0x2001
#define GL_R								0x2002
#define GL_Q								0x2003
#define GL_TEXTURE_GEN_R					0x0C62
#define GL_TEXTURE_GEN_Q					0x0C63

#define GL_PROXY_TEXTURE_1D					0x8063
#define GL_PROXY_TEXTURE_2D					0x8064
#define GL_TEXTURE_PRIORITY					0x8066
#define GL_TEXTURE_RESIDENT					0x8067
#define GL_TEXTURE_BINDING_1D				0x8068
#define GL_TEXTURE_BINDING_2D				0x8069

/* Internal texture formats */
#define GL_ALPHA4							0x803B
#define GL_ALPHA8							0x803C
#define GL_ALPHA12							0x803D
#define GL_ALPHA16							0x803E
#define GL_LUMINANCE4						0x803F
#define GL_LUMINANCE8						0x8040
#define GL_LUMINANCE12						0x8041
#define GL_LUMINANCE16						0x8042
#define GL_LUMINANCE4_ALPHA4				0x8043
#define GL_LUMINANCE6_ALPHA2				0x8044
#define GL_LUMINANCE8_ALPHA8				0x8045
#define GL_LUMINANCE12_ALPHA4				0x8046
#define GL_LUMINANCE12_ALPHA12				0x8047
#define GL_LUMINANCE16_ALPHA16				0x8048
#define GL_INTENSITY						0x8049
#define GL_INTENSITY4						0x804A
#define GL_INTENSITY8						0x804B
#define GL_INTENSITY12						0x804C
#define GL_INTENSITY16						0x804D
#define GL_R3_G3_B2							0x2A10
#define GL_RGB4								0x804F
#define GL_RGB5								0x8050
#define GL_RGB8								0x8051
#define GL_RGB10							0x8052
#define GL_RGB12							0x8053
#define GL_RGB16							0x8054
#define GL_RGBA2							0x8055
#define GL_RGBA4							0x8056
#define GL_RGB5_A1							0x8057
#define GL_RGBA8							0x8058
#define GL_RGB10_A2							0x8059
#define GL_RGBA12							0x805A
#define GL_RGBA16							0x805B

/* Utility */
#define GL_VENDOR							0x1F00
#define GL_RENDERER							0x1F01
#define GL_VERSION							0x1F02
#define GL_EXTENSIONS						0x1F03

/* Errors */
#define GL_NO_ERROR                         0
#define GL_INVALID_VALUE					0x0501
#define GL_INVALID_ENUM						0x0500
#define GL_INVALID_OPERATION				0x0502
#define GL_STACK_OVERFLOW					0x0503
#define GL_STACK_UNDERFLOW					0x0504
#define GL_OUT_OF_MEMORY					0x0505

/*
 * 1.0 Extensions
 */
    /* GL_EXT_blend_minmax and GL_EXT_blend_color */
#define GL_CONSTANT_COLOR_EXT				0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR_EXT		0x8002
#define GL_CONSTANT_ALPHA_EXT				0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT		0x8004
#define GL_BLEND_EQUATION_EXT				0x8009
#define GL_MIN_EXT							0x8007
#define GL_MAX_EXT							0x8008
#define GL_FUNC_ADD_EXT						0x8006
#define GL_FUNC_SUBTRACT_EXT				0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT		0x800B
#define GL_BLEND_COLOR_EXT					0x8005

/* GL_EXT_polygon_offset */
#define GL_POLYGON_OFFSET_EXT				0x8037
#define GL_POLYGON_OFFSET_FACTOR_EXT		0x8038
#define GL_POLYGON_OFFSET_BIAS_EXT			0x8039

/* GL_EXT_vertex_array */
#define GL_VERTEX_ARRAY_EXT					0x8074
#define GL_NORMAL_ARRAY_EXT					0x8075
#define GL_COLOR_ARRAY_EXT					0x8076
#define GL_INDEX_ARRAY_EXT					0x8077
#define GL_TEXTURE_COORD_ARRAY_EXT			0x8078
#define GL_EDGE_FLAG_ARRAY_EXT				0x8079
#define GL_VERTEX_ARRAY_SIZE_EXT			0x807A
#define GL_VERTEX_ARRAY_TYPE_EXT			0x807B
#define GL_VERTEX_ARRAY_STRIDE_EXT			0x807C
#define GL_VERTEX_ARRAY_COUNT_EXT			0x807D
#define GL_NORMAL_ARRAY_TYPE_EXT			0x807E
#define GL_NORMAL_ARRAY_STRIDE_EXT			0x807F
#define GL_NORMAL_ARRAY_COUNT_EXT			0x8080
#define GL_COLOR_ARRAY_SIZE_EXT				0x8081
#define GL_COLOR_ARRAY_TYPE_EXT				0x8082
#define GL_COLOR_ARRAY_STRIDE_EXT			0x8083
#define GL_COLOR_ARRAY_COUNT_EXT			0x8084
#define GL_INDEX_ARRAY_TYPE_EXT				0x8085
#define GL_INDEX_ARRAY_STRIDE_EXT			0x8086
#define GL_INDEX_ARRAY_COUNT_EXT			0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT		0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT		0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT	0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT	0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT		0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT		0x808D
#define GL_VERTEX_ARRAY_POINTER_EXT			0x808E
#define GL_NORMAL_ARRAY_POINTER_EXT			0x808F
#define GL_COLOR_ARRAY_POINTER_EXT			0x8090
#define GL_INDEX_ARRAY_POINTER_EXT			0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT	0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT		0x8093

/* GL_EXT_texture_object */
#define GL_TEXTURE_PRIORITY_EXT				0x8066
#define GL_TEXTURE_RESIDENT_EXT				0x8067
#define GL_TEXTURE_1D_BINDING_EXT			0x8068
#define GL_TEXTURE_2D_BINDING_EXT			0x8069

/* GL_EXT_texture3D */
#define GL_PACK_SKIP_IMAGES_EXT				0x806B
#define GL_PACK_IMAGE_HEIGHT_EXT			0x806C
#define GL_UNPACK_SKIP_IMAGES_EXT			0x806D
#define GL_UNPACK_IMAGE_HEIGHT_EXT			0x806E
#define GL_TEXTURE_3D_EXT					0x806F
#define GL_PROXY_TEXTURE_3D_EXT				0x8070
#define GL_TEXTURE_DEPTH_EXT				0x8071
#define GL_TEXTURE_WRAP_R_EXT				0x8072
#define GL_MAX_3D_TEXTURE_SIZE_EXT			0x8073
#define GL_TEXTURE_3D_BINDING_EXT			0x806A

/* GL_EXT_paletted_texture */
#define GL_TABLE_TOO_LARGE_EXT				0x8031
#define GL_COLOR_TABLE_FORMAT_EXT			0x80D8
#define GL_COLOR_TABLE_WIDTH_EXT			0x80D9
#define GL_COLOR_TABLE_RED_SIZE_EXT			0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE_EXT		0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE_EXT		0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE_EXT	 	0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE_EXT	0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE_EXT	0x80DF
#define GL_TEXTURE_INDEX_SIZE_EXT			0x80ED
#define GL_COLOR_INDEX1_EXT					0x80E2
#define GL_COLOR_INDEX2_EXT					0x80E3
#define GL_COLOR_INDEX4_EXT					0x80E4
#define GL_COLOR_INDEX8_EXT					0x80E5
#define GL_COLOR_INDEX12_EXT				0x80E6
#define GL_COLOR_INDEX16_EXT				0x80E7

/* GL_EXT_shared_texture_palette */
#define GL_SHARED_TEXTURE_PALETTE_EXT		0x81FB

#if 0
/* GL_SGIS_texture_lod */
#define GL_TEXTURE_MIN_LOD_SGIS				0x813A
#define GL_TEXTURE_MAX_LOD_SGIS				0x813B
#define GL_TEXTURE_BASE_LEVEL_SGIS			0x813C
#define GL_TEXTURE_MAX_LEVEL_SGIS			0x813D
#endif

/* GL_EXT_point_parameters */
#define GL_POINT_SIZE_MIN_EXT				0x8126
#define GL_POINT_SIZE_MAX_EXT				0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT	0x8128
#define GL_DISTANCE_ATTENUATION_EXT			0x8129


#define GL_CURRENT_BIT				0x00000001L
#define GL_POINT_BIT				0x00000002L
#define GL_LINE_BIT					0x00000004L
#define GL_POLYGON_BIT				0x00000008L
#define GL_POLYGON_STIPPLE_BIT		0x00000010L
#define GL_PIXEL_MODE_BIT			0x00000020L
#define GL_LIGHTING_BIT				0x00000040L
#define GL_FOG_BIT					0x00000080L
#define GL_DEPTH_BUFFER_BIT			0x00000100L
#define GL_ACCUM_BUFFER_BIT			0x00000200L
#define GL_STENCIL_BUFFER_BIT		0x00000400L
#define GL_VIEWPORT_BIT				0x00000800L
#define GL_TRANSFORM_BIT			0x00001000L
#define GL_ENABLE_BIT				0x00002000L
#define GL_COLOR_BUFFER_BIT			0x00004000L
#define GL_HINT_BIT					0x00008000L
#define GL_EVAL_BIT					0x00010000L
#define GL_LIST_BIT					0x00020000L
#define GL_TEXTURE_BIT				0x00040000L
#define GL_SCISSOR_BIT				0x00080000L
#define GL_ALL_ATTRIB_BITS			0x000fffffL

#define GL_CLIENT_PIXEL_STORE_BIT	0x00000001L
#define GL_CLIENT_VERTEX_ARRAY_BIT	0x00000002L
#define GL_CLIENT_ALL_ATTRIB_BITS	0x0000FFFFL


GLAPI void GLAPIENTRY glEnable(GLenum cap);
GLAPI void GLAPIENTRY glDisable(GLenum cap);

GLAPI void GLAPIENTRY glShadeModel(GLenum mode);
GLAPI void GLAPIENTRY glCullFace(GLenum mode);
GLAPI void GLAPIENTRY glPolygonMode(GLenum face, GLenum mode);

GLAPI void GLAPIENTRY glBegin(GLenum mode);
GLAPI void GLAPIENTRY glEnd(void);

GLAPI void GLAPIENTRY glVertex2f(GLfloat x, GLfloat y);
GLAPI void GLAPIENTRY glVertex2d(GLdouble x, GLdouble y);
GLAPI void GLAPIENTRY glVertex2fv(const GLfloat *v);
GLAPI void GLAPIENTRY glVertex2dv(const GLdouble *v);

GLAPI void GLAPIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z);
GLAPI void GLAPIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z);
GLAPI void GLAPIENTRY glVertex3fv(const GLfloat *v);
GLAPI void GLAPIENTRY glVertex3dv(const GLdouble *v);

GLAPI void GLAPIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLAPI void GLAPIENTRY glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLAPI void GLAPIENTRY glVertex4fv(const GLfloat *v);
GLAPI void GLAPIENTRY glVertex4dv(const GLdouble *v);

GLAPI void GLAPIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue);
GLAPI void GLAPIENTRY glColor3d(GLdouble red, GLdouble green, GLdouble blue);
GLAPI void GLAPIENTRY glColor3fv(const GLfloat *v);
GLAPI void GLAPIENTRY glColor3dv(const GLdouble *v);

GLAPI void GLAPIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLAPI void GLAPIENTRY glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
GLAPI void GLAPIENTRY glColor4fv(const GLfloat *v);
GLAPI void GLAPIENTRY glColor4dv(const GLdouble *v);

GLAPI void GLAPIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
GLAPI void GLAPIENTRY glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz);
GLAPI void GLAPIENTRY glNormal3fv(const GLfloat *v);
GLAPI void GLAPIENTRY glNormal3dv(const GLdouble *v);

GLAPI void GLAPIENTRY glTexCoord1f(GLfloat s);
GLAPI void GLAPIENTRY glTexCoord1d(GLdouble s);
GLAPI void GLAPIENTRY glTexCoord1fv(const GLfloat *v);
GLAPI void GLAPIENTRY glTexCoord1dv(const GLdouble *v);

GLAPI void GLAPIENTRY glTexCoord2f(GLfloat s, GLfloat t);
GLAPI void GLAPIENTRY glTexCoord2d(GLdouble s, GLdouble t);
GLAPI void GLAPIENTRY glTexCoord2fv(const GLfloat *v);
GLAPI void GLAPIENTRY glTexCoord2dv(const GLdouble *v);

GLAPI void GLAPIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r);
GLAPI void GLAPIENTRY glTexCoord3d(GLdouble s, GLdouble t, GLdouble r);
GLAPI void GLAPIENTRY glTexCoord3fv(const GLfloat *v);
GLAPI void GLAPIENTRY glTexCoord3dv(const GLdouble *v);

GLAPI void GLAPIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GLAPI void GLAPIENTRY glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GLAPI void GLAPIENTRY glTexCoord4fv(const GLfloat *v);
GLAPI void GLAPIENTRY glTexCoord4dv(const GLdouble *v);

GLAPI void GLAPIENTRY glEdgeFlag(GLboolean flag);

/* matrix */
GLAPI void GLAPIENTRY glMatrixMode(GLenum mode);
GLAPI void GLAPIENTRY glLoadMatrixf(const GLfloat *m);
GLAPI void GLAPIENTRY glLoadIdentity(void);
GLAPI void GLAPIENTRY glMultMatrixf(const GLfloat *m);
GLAPI void GLAPIENTRY glPushMatrix(void);
GLAPI void GLAPIENTRY glPopMatrix(void);
GLAPI void GLAPIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
GLAPI void GLAPIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z);
GLAPI void GLAPIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z);

GLAPI void GLAPIENTRY glViewport(GLint x, GLint y, GLint width, GLint height);
GLAPI void GLAPIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
GLAPI void GLAPIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);

/* lists */
GLAPI GLuint GLAPIENTRY glGenLists(GLsizei range);
GLAPI GLboolean GLAPIENTRY glIsList(GLuint list);
GLAPI void GLAPIENTRY glNewList(GLuint list, GLenum mode);
GLAPI void GLAPIENTRY glEndList(void);
GLAPI void GLAPIENTRY glCallList(GLuint list);
GLAPI void GLAPIENTRY glDeleteLists(GLuint list, GLsizei range);

/* clear */
GLAPI void GLAPIENTRY glClear(GLbitfield mask);
GLAPI void GLAPIENTRY glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
GLAPI void GLAPIENTRY glClearDepth(GLdouble depth);
GLAPI void GLAPIENTRY glClearDepthf(GLfloat depth);

/* selection */
GLAPI GLint GLAPIENTRY glRenderMode(GLenum mode);
GLAPI void GLAPIENTRY glSelectBuffer(GLsizei size, GLuint *buf);

GLAPI void GLAPIENTRY glInitNames(void);
GLAPI void GLAPIENTRY glPushName(GLuint name);
GLAPI void GLAPIENTRY glPopName(void);
GLAPI void GLAPIENTRY glLoadName(GLuint name);

/* textures */
GLAPI void GLAPIENTRY glGenTextures(GLsizei n, GLuint *textures);
GLAPI void GLAPIENTRY glDeleteTextures(GLsizei n, const GLuint *textures);
GLAPI void GLAPIENTRY glBindTexture(GLenum target, GLuint texture);
GLAPI void GLAPIENTRY glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
GLAPI void GLAPIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param);
GLAPI void GLAPIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);
GLAPI void GLAPIENTRY glPixelStorei(GLenum pname, GLint param);

/* lighting */

GLAPI void GLAPIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *v);
GLAPI void GLAPIENTRY glMaterialf(GLenum mode, GLenum pname, GLfloat v);
GLAPI void GLAPIENTRY glColorMaterial(GLenum face, GLenum mode);

GLAPI void GLAPIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *v);
GLAPI void GLAPIENTRY glLightf(GLenum light, GLenum pname, GLfloat v);
GLAPI void GLAPIENTRY glLightModeli(GLenum pname, GLint param);
GLAPI void GLAPIENTRY glLightModelfv(GLenum pname, const GLfloat *param);

/* misc */

GLAPI void GLAPIENTRY glFlush(void);
GLAPI void GLAPIENTRY glHint(GLenum target, GLenum mode);
GLAPI void GLAPIENTRY glGetIntegerv(GLenum pname, GLint *params);
GLAPI void GLAPIENTRY glGetFloatv(GLenum pname, GLfloat *v);
GLAPI void GLAPIENTRY glFrontFace(GLenum mode);

/* opengl 1.2 arrays */
GLAPI void GLAPIENTRY glEnableClientState(GLenum array);
GLAPI void GLAPIENTRY glDisableClientState(GLenum array);
GLAPI void GLAPIENTRY glArrayElement(GLint i);
GLAPI void GLAPIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLAPI void GLAPIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLAPI void GLAPIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
GLAPI void GLAPIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

/* opengl 1.2 polygon offset */
GLAPI void GLAPIENTRY glPolygonOffset(GLfloat factor, GLfloat units);

#endif /* GL_VERSION_1_1 */

#ifndef GL_VERSION_1_5
#ifndef __GLintptr_defined
#include <stddef.h>
#ifdef __APPLE__
typedef intptr_t GLsizeiptr;
typedef intptr_t GLintptr;
#else
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
#endif
#define __GLintptr_defined
#endif
#endif

#ifdef __NFOSMESA_H__
#include <GL/glext.h>
#endif

#ifndef GL_VERSION_2_0
#define GL_VERSION_2_0 1
typedef char GLchar;
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1
#ifdef __APPLE__
typedef intptr_t GLsizeiptrARB;
typedef intptr_t GLintptrARB;
#else
typedef ptrdiff_t GLsizeiptrARB;
typedef ptrdiff_t GLintptrARB;
#endif
#endif

#ifndef GL_NV_half_float
#define GL_NV_half_float 1
typedef unsigned short GLhalfNV;
#endif

#ifndef GL_NV_vdpau_interop
#define GL_NV_vdpau_interop 1
typedef GLintptr GLvdpauSurfaceNV;
#endif

#ifndef GL_ARB_cl_event
#define GL_ARB_cl_event 1
#ifdef __PUREC__
struct _cl_context { int dummy; };
struct _cl_event { int dummy; };
#else
struct _cl_context;
struct _cl_event;
#endif
#endif

#if !defined(GL_ARB_sync) && !defined(__TINY_GL_H__)
#define GL_ARB_sync 1
#if defined(__GNUC__) || (defined(LLONG_MAX) && LLONG_MAX > 2147483647L)
typedef int64_t GLint64;
typedef uint64_t GLuint64;
#else
typedef struct { long hi; unsigned long lo; } GLint64;
typedef struct { unsigned long hi; unsigned long lo; } GLuint64;
#endif
#ifdef __PUREC__
struct __GLsync { int dummy; };
#endif
typedef struct __GLsync *GLsync;
#endif

#if !defined(GL_EXT_timer_query) && !defined(__TINY_GL_H__)
#define GL_EXT_timer_query 1
typedef GLint64 GLint64EXT;
typedef GLuint64 GLuint64EXT;
#endif

#ifndef GL_OES_fixed_point
#define GL_OES_fixed_point 1
typedef GLint GLfixed;
#endif

#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects 1
typedef char GLcharARB;
#ifdef __APPLE__
typedef void *GLhandleARB;
#else
typedef GLuint GLhandleARB;
#endif
#endif

#ifndef GL_KHR_debug
#define GL_KHR_debug 1
typedef void CALLBACK (*GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message, const void *userParam);
#endif
#ifndef GL_ARB_debug_output
#define GL_ARB_debug_output 1
typedef void CALLBACK (*GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message, const void *userParam);
#endif
#ifndef GL_AMD_debug_output
#define GL_AMD_debug_output
typedef void CALLBACK (*GLDEBUGPROCAMD)(GLuint id,GLenum category, GLenum severity, GLsizei length, const GLchar *message, void *userParam);
#endif
#ifndef GL_MESA_program_debug
#define GL_MESA_program_debug 1
typedef void CALLBACK (*GLprogramcallbackMESA)(GLenum target, GLvoid *data);
#endif
#ifndef GL_EXT_external_buffer
#define GL_EXT_external_buffer 1
typedef void *GLeglClientBufferEXT;
#endif
#ifndef GL_NV_draw_vulkan_image
#define GL_NV_draw_vulkan_image 1
typedef void (APIENTRY *GLVULKANPROCNV)(void);
#endif


/*
 * Mesa Off-Screen rendering interface.
 * 
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 * 
 */

#define OSMESA_VERSION_NUMBER(major, minor, patch) ((major) * 10000 + (minor) * 100 + patch)
#define OSMESA_VERSION OSMESA_VERSION_NUMBER(OSMESA_MAJOR_VERSION, OSMESA_MINOR_VERSION, OSMESA_PATCH_VERSION)

#if !defined(OSMESA_MAJOR_VERSION)
#define OSMESA_MAJOR_VERSION 6
#define OSMESA_MINOR_VERSION 3
#define OSMESA_PATCH_VERSION 0

/*
 * Values for the format parameter of OSMesaCreateLDG()
 *   Mesa_gl.ldg version 0.8 (Mesa 2.6)
 */
#define OSMESA_COLOR_INDEX	GL_COLOR_INDEX
#define OSMESA_RGBA			GL_RGBA
#define OSMESA_BGRA			0x1
#define OSMESA_ARGB			0x2
#define OSMESA_RGB			GL_RGB
#define OSMESA_BGR			0x4
#define OSMESA_RGB_565		0x5
#define VDI_ARGB			0x8
#define VDI_RGB				0xf
#define DIRECT_VDI_ARGB		0x10

/*
 * OSMesaPixelStore() parameters:
 */
#define OSMESA_ROW_LENGTH	0x10
#define OSMESA_Y_UP		0x11

/*
 * Accepted by OSMesaGetIntegerv:
 */
#define OSMESA_WIDTH		0x20
#define OSMESA_HEIGHT		0x21
#define OSMESA_FORMAT		0x22
#define OSMESA_TYPE			0x23
#define OSMESA_MAX_WIDTH	0x24  /* new in 4.0 */
#define OSMESA_MAX_HEIGHT	0x25  /* new in 4.0 */

#ifdef __PUREC__
struct osmesa_context { int dummy; };
#endif
typedef struct osmesa_context *OSMesaContext;

typedef void (APIENTRY *OSMESAproc)(void);

GLAPI OSMesaContext GLAPIENTRY OSMesaCreateContext( GLenum format, OSMesaContext sharelist );
GLAPI OSMesaContext GLAPIENTRY OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist);
GLAPI void GLAPIENTRY OSMesaDestroyContext( OSMesaContext ctx );
GLAPI GLboolean GLAPIENTRY OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height );
GLAPI OSMesaContext GLAPIENTRY OSMesaGetCurrentContext( void );
GLAPI void GLAPIENTRY OSMesaPixelStore( GLint pname, GLint value );
GLAPI void GLAPIENTRY OSMesaGetIntegerv( GLint pname, GLint *value );
GLAPI GLboolean GLAPIENTRY OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer );
GLAPI GLboolean GLAPIENTRY OSMesaGetColorBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer );
GLAPI OSMESAproc GLAPIENTRY OSMesaGetProcAddress( const char *funcName );
GLAPI void GLAPIENTRY OSMesaColorClamp(GLboolean enable);
GLAPI void GLAPIENTRY OSMesaPostprocess(OSMesaContext osmesa, const char *filter, GLuint enable_value);


#elif OSMESA_VERSION < OSMESA_VERSION_NUMBER(6, 3, 0)
typedef void (APIENTRY *OSMESAproc)(void);
#endif

/*
 * integral types that need to be promoted to
 * 32bit type when passed by value
 */
#ifndef GL_nfosmesa_short_promotions
#define GL_nfosmesa_short_promotions 1
#ifdef __MSHORT__
typedef long GLshort32;
typedef unsigned long GLushort32;
typedef unsigned long GLboolean32;
typedef long GLchar32;
typedef unsigned long GLubyte32;
typedef signed long GLbyte32;
typedef unsigned long GLhalfNV32;
#else
typedef GLshort GLshort32;
typedef GLushort GLushort32;
typedef GLboolean GLboolean32;
typedef GLchar GLchar32;
typedef GLubyte GLubyte32;
typedef GLbyte GLbyte32;
typedef GLhalfNV GLhalfNV32;
#endif
#endif

/*
 * Atari-specific structure for the shared libraries
 */
struct gl_public {
	void *libhandle;
	void *libexec;
	void *(*m_alloc)(size_t);
	void (*m_free)(void *);
};

#ifdef __cplusplus
}
#endif

#undef glClearDepth
#undef glFrustum
#undef glOrtho
#undef gluLookAt

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
	void APIENTRY (*information)(void);
	void APIENTRY (*Begin)(GLenum mode);
	void APIENTRY (*Clear)(GLbitfield mask);
	void APIENTRY (*ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void APIENTRY (*Color3f)(GLfloat red, GLfloat green, GLfloat blue);
	void APIENTRY (*Disable)(GLenum cap);
	void APIENTRY (*Enable)(GLenum cap);
	void APIENTRY (*End)(void);
	void APIENTRY (*Lightfv)(GLenum light, GLenum pname, const GLfloat *params);
	void APIENTRY (*LoadIdentity)(void);
	void APIENTRY (*Materialfv)(GLenum face, GLenum pname, const GLfloat *params);
	void APIENTRY (*MatrixMode)(GLenum mode);
	void APIENTRY (*Orthof)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
	void APIENTRY (*PopMatrix)(void);
	void APIENTRY (*PushMatrix)(void);
	void APIENTRY (*Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*TexEnvi)(GLenum target, GLenum pname, GLint param);
	void APIENTRY (*TexParameteri)(GLenum target, GLenum pname, GLint param);
	void APIENTRY (*Translatef)(GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*Vertex2f)(GLfloat x, GLfloat y);
	void APIENTRY (*Vertex3f)(GLfloat x, GLfloat y, GLfloat z);
	void * APIENTRY (*OSMesaCreateLDG)(GLenum format, GLenum type, GLint width, GLint height);
	void APIENTRY (*OSMesaDestroyLDG)(void);
	void APIENTRY (*ArrayElement)(GLint i);
	void APIENTRY (*BindTexture)(GLenum target, GLuint texture);
	void APIENTRY (*CallList)(GLuint list);
	void APIENTRY (*ClearDepthf)(GLfloat d);
	void APIENTRY (*Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void APIENTRY (*Color3fv)(const GLfloat *v);
	void APIENTRY (*Color4fv)(const GLfloat *v);
	void APIENTRY (*ColorMaterial)(GLenum face, GLenum mode);
	void APIENTRY (*ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*CullFace)(GLenum mode);
	void APIENTRY (*DeleteTextures)(GLsizei n, const GLuint *textures);
	void APIENTRY (*DisableClientState)(GLenum array);
	void APIENTRY (*EnableClientState)(GLenum array);
	void APIENTRY (*EndList)(void);
	void APIENTRY (*EdgeFlag)(GLboolean32 flag);
	void APIENTRY (*Flush)(void);
	void APIENTRY (*FrontFace)(GLenum mode);
	void APIENTRY (*Frustumf)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
	GLuint APIENTRY (*GenLists)(GLsizei range);
	void APIENTRY (*GenTextures)(GLsizei n, GLuint *textures);
	void APIENTRY (*GetFloatv)(GLenum pname, GLfloat *params);
	void APIENTRY (*GetIntegerv)(GLenum pname, GLint *params);
	void APIENTRY (*Hint)(GLenum target, GLenum mode);
	void APIENTRY (*InitNames)(void);
	GLboolean APIENTRY (*IsList)(GLuint list);
	void APIENTRY (*Lightf)(GLenum light, GLenum pname, GLfloat param);
	void APIENTRY (*LightModeli)(GLenum pname, GLint param);
	void APIENTRY (*LightModelfv)(GLenum pname, const GLfloat *params);
	void APIENTRY (*LoadMatrixf)(const GLfloat *m);
	void APIENTRY (*LoadName)(GLuint name);
	void APIENTRY (*Materialf)(GLenum face, GLenum pname, GLfloat param);
	void APIENTRY (*MultMatrixf)(const GLfloat *m);
	void APIENTRY (*NewList)(GLuint list, GLenum mode);
	void APIENTRY (*Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
	void APIENTRY (*Normal3fv)(const GLfloat *v);
	void APIENTRY (*NormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*PixelStorei)(GLenum pname, GLint param);
	void APIENTRY (*PolygonMode)(GLenum face, GLenum mode);
	void APIENTRY (*PolygonOffset)(GLfloat factor, GLfloat units);
	void APIENTRY (*PopName)(void);
	void APIENTRY (*PushName)(GLuint name);
	GLint APIENTRY (*RenderMode)(GLenum mode);
	void APIENTRY (*SelectBuffer)(GLsizei size, GLuint *buffer);
	void APIENTRY (*Scalef)(GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ShadeModel)(GLenum mode);
	void APIENTRY (*TexCoord2f)(GLfloat s, GLfloat t);
	void APIENTRY (*TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	void APIENTRY (*TexCoord2fv)(const GLfloat *v);
	void APIENTRY (*TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void APIENTRY (*Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*Vertex3fv)(const GLfloat *v);
	void APIENTRY (*VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*Viewport)(GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*swapbuffer)(void *buffer);
	GLsizei APIENTRY (*max_width)(void);
	GLsizei APIENTRY (*max_height)(void);
	void APIENTRY (*DeleteLists)(GLuint list, GLsizei range);
	void APIENTRY (*gluLookAtf)(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ);
	void APIENTRY (*exception_error)(void (CALLBACK *exception)(GLenum param) );

};

extern struct _gl_tiny gl;

#ifdef __cplusplus
}
#endif


#ifndef NFOSMESA_NO_MANGLE
#undef information
#define information (gl.information)
#undef glBegin
#define glBegin (gl.Begin)
#undef glClear
#define glClear (gl.Clear)
#undef glClearColor
#define glClearColor (gl.ClearColor)
#undef glColor3f
#define glColor3f (gl.Color3f)
#undef glDisable
#define glDisable (gl.Disable)
#undef glEnable
#define glEnable (gl.Enable)
#undef glEnd
#define glEnd (gl.End)
#undef glLightfv
#define glLightfv (gl.Lightfv)
#undef glLoadIdentity
#define glLoadIdentity (gl.LoadIdentity)
#undef glMaterialfv
#define glMaterialfv (gl.Materialfv)
#undef glMatrixMode
#define glMatrixMode (gl.MatrixMode)
#undef glOrthof
#define glOrthof (gl.Orthof)
#undef glPopMatrix
#define glPopMatrix (gl.PopMatrix)
#undef glPushMatrix
#define glPushMatrix (gl.PushMatrix)
#undef glRotatef
#define glRotatef (gl.Rotatef)
#undef glTexEnvi
#define glTexEnvi (gl.TexEnvi)
#undef glTexParameteri
#define glTexParameteri (gl.TexParameteri)
#undef glTranslatef
#define glTranslatef (gl.Translatef)
#undef glVertex2f
#define glVertex2f (gl.Vertex2f)
#undef glVertex3f
#define glVertex3f (gl.Vertex3f)
#undef OSMesaCreateLDG
#define OSMesaCreateLDG (gl.OSMesaCreateLDG)
#undef OSMesaDestroyLDG
#define OSMesaDestroyLDG (gl.OSMesaDestroyLDG)
#undef glArrayElement
#define glArrayElement (gl.ArrayElement)
#undef glBindTexture
#define glBindTexture (gl.BindTexture)
#undef glCallList
#define glCallList (gl.CallList)
#undef glClearDepthf
#define glClearDepthf (gl.ClearDepthf)
#undef glColor4f
#define glColor4f (gl.Color4f)
#undef glColor3fv
#define glColor3fv (gl.Color3fv)
#undef glColor4fv
#define glColor4fv (gl.Color4fv)
#undef glColorMaterial
#define glColorMaterial (gl.ColorMaterial)
#undef glColorPointer
#define glColorPointer (gl.ColorPointer)
#undef glCullFace
#define glCullFace (gl.CullFace)
#undef glDeleteTextures
#define glDeleteTextures (gl.DeleteTextures)
#undef glDisableClientState
#define glDisableClientState (gl.DisableClientState)
#undef glEnableClientState
#define glEnableClientState (gl.EnableClientState)
#undef glEndList
#define glEndList (gl.EndList)
#undef glEdgeFlag
#define glEdgeFlag (gl.EdgeFlag)
#undef glFlush
#define glFlush (gl.Flush)
#undef glFrontFace
#define glFrontFace (gl.FrontFace)
#undef glFrustumf
#define glFrustumf (gl.Frustumf)
#undef glGenLists
#define glGenLists (gl.GenLists)
#undef glGenTextures
#define glGenTextures (gl.GenTextures)
#undef glGetFloatv
#define glGetFloatv (gl.GetFloatv)
#undef glGetIntegerv
#define glGetIntegerv (gl.GetIntegerv)
#undef glHint
#define glHint (gl.Hint)
#undef glInitNames
#define glInitNames (gl.InitNames)
#undef glIsList
#define glIsList (gl.IsList)
#undef glLightf
#define glLightf (gl.Lightf)
#undef glLightModeli
#define glLightModeli (gl.LightModeli)
#undef glLightModelfv
#define glLightModelfv (gl.LightModelfv)
#undef glLoadMatrixf
#define glLoadMatrixf (gl.LoadMatrixf)
#undef glLoadName
#define glLoadName (gl.LoadName)
#undef glMaterialf
#define glMaterialf (gl.Materialf)
#undef glMultMatrixf
#define glMultMatrixf (gl.MultMatrixf)
#undef glNewList
#define glNewList (gl.NewList)
#undef glNormal3f
#define glNormal3f (gl.Normal3f)
#undef glNormal3fv
#define glNormal3fv (gl.Normal3fv)
#undef glNormalPointer
#define glNormalPointer (gl.NormalPointer)
#undef glPixelStorei
#define glPixelStorei (gl.PixelStorei)
#undef glPolygonMode
#define glPolygonMode (gl.PolygonMode)
#undef glPolygonOffset
#define glPolygonOffset (gl.PolygonOffset)
#undef glPopName
#define glPopName (gl.PopName)
#undef glPushName
#define glPushName (gl.PushName)
#undef glRenderMode
#define glRenderMode (gl.RenderMode)
#undef glSelectBuffer
#define glSelectBuffer (gl.SelectBuffer)
#undef glScalef
#define glScalef (gl.Scalef)
#undef glShadeModel
#define glShadeModel (gl.ShadeModel)
#undef glTexCoord2f
#define glTexCoord2f (gl.TexCoord2f)
#undef glTexCoord4f
#define glTexCoord4f (gl.TexCoord4f)
#undef glTexCoord2fv
#define glTexCoord2fv (gl.TexCoord2fv)
#undef glTexCoordPointer
#define glTexCoordPointer (gl.TexCoordPointer)
#undef glTexImage2D
#define glTexImage2D (gl.TexImage2D)
#undef glVertex4f
#define glVertex4f (gl.Vertex4f)
#undef glVertex3fv
#define glVertex3fv (gl.Vertex3fv)
#undef glVertexPointer
#define glVertexPointer (gl.VertexPointer)
#undef glViewport
#define glViewport (gl.Viewport)
#undef swapbuffer
#define swapbuffer (gl.swapbuffer)
#undef max_width
#define max_width (gl.max_width)
#undef max_height
#define max_height (gl.max_height)
#undef glDeleteLists
#define glDeleteLists (gl.DeleteLists)
#undef gluLookAtf
#define gluLookAtf (gl.gluLookAtf)
#undef exception_error
#define exception_error (gl.exception_error)

#endif


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



/* Functions generated: 83 */

#endif /* __SLB_TINY_GL_H__ */
