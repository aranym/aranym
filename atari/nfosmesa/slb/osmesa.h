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
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
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
typedef ptrdiff_t GLsizeiptrARB;
typedef ptrdiff_t GLintptrARB;
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
typedef GLuint GLhandleARB;
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

typedef void (*OSMESAproc)(void);

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
typedef void (*OSMESAproc)(void);
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


#ifdef __cplusplus
extern "C" {
#endif

struct _gl_osmesa {
	const GLubyte * APIENTRY (*GetString)(GLenum name);
	OSMesaContext APIENTRY (*OSMesaCreateContext)(GLenum format, OSMesaContext sharelist);
	OSMesaContext APIENTRY (*OSMesaCreateContextExt)(GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist);
	void APIENTRY (*OSMesaDestroyContext)(OSMesaContext ctx);
	GLboolean APIENTRY (*OSMesaMakeCurrent)(OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height);
	OSMesaContext APIENTRY (*OSMesaGetCurrentContext)(void);
	void APIENTRY (*OSMesaPixelStore)(GLint pname, GLint value);
	void APIENTRY (*OSMesaGetIntegerv)(GLint pname, GLint *value);
	GLboolean APIENTRY (*OSMesaGetDepthBuffer)(OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void * *buffer);
	GLboolean APIENTRY (*OSMesaGetColorBuffer)(OSMesaContext c, GLint *width, GLint *height, GLint *format, void * *buffer);
	OSMESAproc APIENTRY (*OSMesaGetProcAddress)(const char *funcName);
	void APIENTRY (*ClearIndex)(GLfloat c);
	void APIENTRY (*ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void APIENTRY (*Clear)(GLbitfield mask);
	void APIENTRY (*IndexMask)(GLuint mask);
	void APIENTRY (*ColorMask)(GLboolean32 red, GLboolean32 green, GLboolean32 blue, GLboolean32 alpha);
	void APIENTRY (*AlphaFunc)(GLenum func, GLclampf ref);
	void APIENTRY (*BlendFunc)(GLenum sfactor, GLenum dfactor);
	void APIENTRY (*LogicOp)(GLenum opcode);
	void APIENTRY (*CullFace)(GLenum mode);
	void APIENTRY (*FrontFace)(GLenum mode);
	void APIENTRY (*PointSize)(GLfloat size);
	void APIENTRY (*LineWidth)(GLfloat width);
	void APIENTRY (*LineStipple)(GLint factor, GLushort32 pattern);
	void APIENTRY (*PolygonMode)(GLenum face, GLenum mode);
	void APIENTRY (*PolygonOffset)(GLfloat factor, GLfloat units);
	void APIENTRY (*PolygonStipple)(const GLubyte *mask);
	void APIENTRY (*GetPolygonStipple)(GLubyte *mask);
	void APIENTRY (*EdgeFlag)(GLboolean32 flag);
	void APIENTRY (*EdgeFlagv)(const GLboolean *flag);
	void APIENTRY (*Scissor)(GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*ClipPlane)(GLenum plane, const GLdouble *equation);
	void APIENTRY (*GetClipPlane)(GLenum plane, GLdouble *equation);
	void APIENTRY (*DrawBuffer)(GLenum mode);
	void APIENTRY (*ReadBuffer)(GLenum mode);
	void APIENTRY (*Enable)(GLenum cap);
	void APIENTRY (*Disable)(GLenum cap);
	GLboolean APIENTRY (*IsEnabled)(GLenum cap);
	void APIENTRY (*EnableClientState)(GLenum array);
	void APIENTRY (*DisableClientState)(GLenum array);
	void APIENTRY (*GetBooleanv)(GLenum pname, GLboolean *params);
	void APIENTRY (*GetDoublev)(GLenum pname, GLdouble *params);
	void APIENTRY (*GetFloatv)(GLenum pname, GLfloat *params);
	void APIENTRY (*GetIntegerv)(GLenum pname, GLint *params);
	void APIENTRY (*PushAttrib)(GLbitfield mask);
	void APIENTRY (*PopAttrib)(void);
	void APIENTRY (*PushClientAttrib)(GLbitfield mask);
	void APIENTRY (*PopClientAttrib)(void);
	GLint APIENTRY (*RenderMode)(GLenum mode);
	GLenum APIENTRY (*GetError)(void);
	void APIENTRY (*Finish)(void);
	void APIENTRY (*Flush)(void);
	void APIENTRY (*Hint)(GLenum target, GLenum mode);
	void APIENTRY (*ClearDepth)(GLclampd depth);
	void APIENTRY (*DepthFunc)(GLenum func);
	void APIENTRY (*DepthMask)(GLboolean32 flag);
	void APIENTRY (*DepthRange)(GLclampd zNear, GLclampd zFar);
	void APIENTRY (*ClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void APIENTRY (*Accum)(GLenum op, GLfloat value);
	void APIENTRY (*MatrixMode)(GLenum mode);
	void APIENTRY (*Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void APIENTRY (*Frustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void APIENTRY (*Viewport)(GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*PushMatrix)(void);
	void APIENTRY (*PopMatrix)(void);
	void APIENTRY (*LoadIdentity)(void);
	void APIENTRY (*LoadMatrixd)(const GLdouble *m);
	void APIENTRY (*LoadMatrixf)(const GLfloat *m);
	void APIENTRY (*MultMatrixd)(const GLdouble *m);
	void APIENTRY (*MultMatrixf)(const GLfloat *m);
	void APIENTRY (*Rotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*Scaled)(GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*Scalef)(GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*Translated)(GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*Translatef)(GLfloat x, GLfloat y, GLfloat z);
	GLboolean APIENTRY (*IsList)(GLuint list);
	void APIENTRY (*DeleteLists)(GLuint list, GLsizei range);
	GLuint APIENTRY (*GenLists)(GLsizei range);
	void APIENTRY (*NewList)(GLuint list, GLenum mode);
	void APIENTRY (*EndList)(void);
	void APIENTRY (*CallList)(GLuint list);
	void APIENTRY (*CallLists)(GLsizei n, GLenum type, const GLvoid *lists);
	void APIENTRY (*ListBase)(GLuint base);
	void APIENTRY (*Begin)(GLenum mode);
	void APIENTRY (*End)(void);
	void APIENTRY (*Vertex2d)(GLdouble x, GLdouble y);
	void APIENTRY (*Vertex2f)(GLfloat x, GLfloat y);
	void APIENTRY (*Vertex2i)(GLint x, GLint y);
	void APIENTRY (*Vertex2s)(GLshort32 x, GLshort32 y);
	void APIENTRY (*Vertex3d)(GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*Vertex3f)(GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*Vertex3i)(GLint x, GLint y, GLint z);
	void APIENTRY (*Vertex3s)(GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*Vertex4i)(GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*Vertex4s)(GLshort32 x, GLshort32 y, GLshort32 z, GLshort32 w);
	void APIENTRY (*Vertex2dv)(const GLdouble *v);
	void APIENTRY (*Vertex2fv)(const GLfloat *v);
	void APIENTRY (*Vertex2iv)(const GLint *v);
	void APIENTRY (*Vertex2sv)(const GLshort *v);
	void APIENTRY (*Vertex3dv)(const GLdouble *v);
	void APIENTRY (*Vertex3fv)(const GLfloat *v);
	void APIENTRY (*Vertex3iv)(const GLint *v);
	void APIENTRY (*Vertex3sv)(const GLshort *v);
	void APIENTRY (*Vertex4dv)(const GLdouble *v);
	void APIENTRY (*Vertex4fv)(const GLfloat *v);
	void APIENTRY (*Vertex4iv)(const GLint *v);
	void APIENTRY (*Vertex4sv)(const GLshort *v);
	void APIENTRY (*Normal3b)(GLbyte32 nx, GLbyte32 ny, GLbyte32 nz);
	void APIENTRY (*Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
	void APIENTRY (*Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
	void APIENTRY (*Normal3i)(GLint nx, GLint ny, GLint nz);
	void APIENTRY (*Normal3s)(GLshort32 nx, GLshort32 ny, GLshort32 nz);
	void APIENTRY (*Normal3bv)(const GLbyte *v);
	void APIENTRY (*Normal3dv)(const GLdouble *v);
	void APIENTRY (*Normal3fv)(const GLfloat *v);
	void APIENTRY (*Normal3iv)(const GLint *v);
	void APIENTRY (*Normal3sv)(const GLshort *v);
	void APIENTRY (*Indexd)(GLdouble c);
	void APIENTRY (*Indexf)(GLfloat c);
	void APIENTRY (*Indexi)(GLint c);
	void APIENTRY (*Indexs)(GLshort32 c);
	void APIENTRY (*Indexub)(GLubyte32 c);
	void APIENTRY (*Indexdv)(const GLdouble *c);
	void APIENTRY (*Indexfv)(const GLfloat *c);
	void APIENTRY (*Indexiv)(const GLint *c);
	void APIENTRY (*Indexsv)(const GLshort *c);
	void APIENTRY (*Indexubv)(const GLubyte *c);
	void APIENTRY (*Color3b)(GLbyte32 red, GLbyte32 green, GLbyte32 blue);
	void APIENTRY (*Color3d)(GLdouble red, GLdouble green, GLdouble blue);
	void APIENTRY (*Color3f)(GLfloat red, GLfloat green, GLfloat blue);
	void APIENTRY (*Color3i)(GLint red, GLint green, GLint blue);
	void APIENTRY (*Color3s)(GLshort32 red, GLshort32 green, GLshort32 blue);
	void APIENTRY (*Color3ub)(GLubyte32 red, GLubyte32 green, GLubyte32 blue);
	void APIENTRY (*Color3ui)(GLuint red, GLuint green, GLuint blue);
	void APIENTRY (*Color3us)(GLushort32 red, GLushort32 green, GLushort32 blue);
	void APIENTRY (*Color4b)(GLbyte32 red, GLbyte32 green, GLbyte32 blue, GLbyte32 alpha);
	void APIENTRY (*Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
	void APIENTRY (*Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void APIENTRY (*Color4i)(GLint red, GLint green, GLint blue, GLint alpha);
	void APIENTRY (*Color4s)(GLshort32 red, GLshort32 green, GLshort32 blue, GLshort32 alpha);
	void APIENTRY (*Color4ub)(GLubyte32 red, GLubyte32 green, GLubyte32 blue, GLubyte32 alpha);
	void APIENTRY (*Color4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
	void APIENTRY (*Color4us)(GLushort32 red, GLushort32 green, GLushort32 blue, GLushort32 alpha);
	void APIENTRY (*Color3bv)(const GLbyte *v);
	void APIENTRY (*Color3dv)(const GLdouble *v);
	void APIENTRY (*Color3fv)(const GLfloat *v);
	void APIENTRY (*Color3iv)(const GLint *v);
	void APIENTRY (*Color3sv)(const GLshort *v);
	void APIENTRY (*Color3ubv)(const GLubyte *v);
	void APIENTRY (*Color3uiv)(const GLuint *v);
	void APIENTRY (*Color3usv)(const GLushort *v);
	void APIENTRY (*Color4bv)(const GLbyte *v);
	void APIENTRY (*Color4dv)(const GLdouble *v);
	void APIENTRY (*Color4fv)(const GLfloat *v);
	void APIENTRY (*Color4iv)(const GLint *v);
	void APIENTRY (*Color4sv)(const GLshort *v);
	void APIENTRY (*Color4ubv)(const GLubyte *v);
	void APIENTRY (*Color4uiv)(const GLuint *v);
	void APIENTRY (*Color4usv)(const GLushort *v);
	void APIENTRY (*TexCoord1d)(GLdouble s);
	void APIENTRY (*TexCoord1f)(GLfloat s);
	void APIENTRY (*TexCoord1i)(GLint s);
	void APIENTRY (*TexCoord1s)(GLshort32 s);
	void APIENTRY (*TexCoord2d)(GLdouble s, GLdouble t);
	void APIENTRY (*TexCoord2f)(GLfloat s, GLfloat t);
	void APIENTRY (*TexCoord2i)(GLint s, GLint t);
	void APIENTRY (*TexCoord2s)(GLshort32 s, GLshort32 t);
	void APIENTRY (*TexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
	void APIENTRY (*TexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
	void APIENTRY (*TexCoord3i)(GLint s, GLint t, GLint r);
	void APIENTRY (*TexCoord3s)(GLshort32 s, GLshort32 t, GLshort32 r);
	void APIENTRY (*TexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
	void APIENTRY (*TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	void APIENTRY (*TexCoord4i)(GLint s, GLint t, GLint r, GLint q);
	void APIENTRY (*TexCoord4s)(GLshort32 s, GLshort32 t, GLshort32 r, GLshort32 q);
	void APIENTRY (*TexCoord1dv)(const GLdouble *v);
	void APIENTRY (*TexCoord1fv)(const GLfloat *v);
	void APIENTRY (*TexCoord1iv)(const GLint *v);
	void APIENTRY (*TexCoord1sv)(const GLshort *v);
	void APIENTRY (*TexCoord2dv)(const GLdouble *v);
	void APIENTRY (*TexCoord2fv)(const GLfloat *v);
	void APIENTRY (*TexCoord2iv)(const GLint *v);
	void APIENTRY (*TexCoord2sv)(const GLshort *v);
	void APIENTRY (*TexCoord3dv)(const GLdouble *v);
	void APIENTRY (*TexCoord3fv)(const GLfloat *v);
	void APIENTRY (*TexCoord3iv)(const GLint *v);
	void APIENTRY (*TexCoord3sv)(const GLshort *v);
	void APIENTRY (*TexCoord4dv)(const GLdouble *v);
	void APIENTRY (*TexCoord4fv)(const GLfloat *v);
	void APIENTRY (*TexCoord4iv)(const GLint *v);
	void APIENTRY (*TexCoord4sv)(const GLshort *v);
	void APIENTRY (*RasterPos2d)(GLdouble x, GLdouble y);
	void APIENTRY (*RasterPos2f)(GLfloat x, GLfloat y);
	void APIENTRY (*RasterPos2i)(GLint x, GLint y);
	void APIENTRY (*RasterPos2s)(GLshort32 x, GLshort32 y);
	void APIENTRY (*RasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*RasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*RasterPos3i)(GLint x, GLint y, GLint z);
	void APIENTRY (*RasterPos3s)(GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*RasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*RasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*RasterPos4i)(GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*RasterPos4s)(GLshort32 x, GLshort32 y, GLshort32 z, GLshort32 w);
	void APIENTRY (*RasterPos2dv)(const GLdouble *v);
	void APIENTRY (*RasterPos2fv)(const GLfloat *v);
	void APIENTRY (*RasterPos2iv)(const GLint *v);
	void APIENTRY (*RasterPos2sv)(const GLshort *v);
	void APIENTRY (*RasterPos3dv)(const GLdouble *v);
	void APIENTRY (*RasterPos3fv)(const GLfloat *v);
	void APIENTRY (*RasterPos3iv)(const GLint *v);
	void APIENTRY (*RasterPos3sv)(const GLshort *v);
	void APIENTRY (*RasterPos4dv)(const GLdouble *v);
	void APIENTRY (*RasterPos4fv)(const GLfloat *v);
	void APIENTRY (*RasterPos4iv)(const GLint *v);
	void APIENTRY (*RasterPos4sv)(const GLshort *v);
	void APIENTRY (*Rectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
	void APIENTRY (*Rectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
	void APIENTRY (*Recti)(GLint x1, GLint y1, GLint x2, GLint y2);
	void APIENTRY (*Rects)(GLshort32 x1, GLshort32 y1, GLshort32 x2, GLshort32 y2);
	void APIENTRY (*Rectdv)(const GLdouble *v1, const GLdouble *v2);
	void APIENTRY (*Rectfv)(const GLfloat *v1, const GLfloat *v2);
	void APIENTRY (*Rectiv)(const GLint *v1, const GLint *v2);
	void APIENTRY (*Rectsv)(const GLshort *v1, const GLshort *v2);
	void APIENTRY (*VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*NormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*IndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*EdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*GetPointerv)(GLenum pname, GLvoid* *params);
	void APIENTRY (*ArrayElement)(GLint i);
	void APIENTRY (*DrawArrays)(GLenum mode, GLint first, GLsizei count);
	void APIENTRY (*DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
	void APIENTRY (*InterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*ShadeModel)(GLenum mode);
	void APIENTRY (*Lightf)(GLenum light, GLenum pname, GLfloat param);
	void APIENTRY (*Lighti)(GLenum light, GLenum pname, GLint param);
	void APIENTRY (*Lightfv)(GLenum light, GLenum pname, const GLfloat *params);
	void APIENTRY (*Lightiv)(GLenum light, GLenum pname, const GLint *params);
	void APIENTRY (*GetLightfv)(GLenum light, GLenum pname, GLfloat *params);
	void APIENTRY (*GetLightiv)(GLenum light, GLenum pname, GLint *params);
	void APIENTRY (*LightModelf)(GLenum pname, GLfloat param);
	void APIENTRY (*LightModeli)(GLenum pname, GLint param);
	void APIENTRY (*LightModelfv)(GLenum pname, const GLfloat *params);
	void APIENTRY (*LightModeliv)(GLenum pname, const GLint *params);
	void APIENTRY (*Materialf)(GLenum face, GLenum pname, GLfloat param);
	void APIENTRY (*Materiali)(GLenum face, GLenum pname, GLint param);
	void APIENTRY (*Materialfv)(GLenum face, GLenum pname, const GLfloat *params);
	void APIENTRY (*Materialiv)(GLenum face, GLenum pname, const GLint *params);
	void APIENTRY (*GetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
	void APIENTRY (*GetMaterialiv)(GLenum face, GLenum pname, GLint *params);
	void APIENTRY (*ColorMaterial)(GLenum face, GLenum mode);
	void APIENTRY (*PixelZoom)(GLfloat xfactor, GLfloat yfactor);
	void APIENTRY (*PixelStoref)(GLenum pname, GLfloat param);
	void APIENTRY (*PixelStorei)(GLenum pname, GLint param);
	void APIENTRY (*PixelTransferf)(GLenum pname, GLfloat param);
	void APIENTRY (*PixelTransferi)(GLenum pname, GLint param);
	void APIENTRY (*PixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values);
	void APIENTRY (*PixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values);
	void APIENTRY (*PixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values);
	void APIENTRY (*GetPixelMapfv)(GLenum map, GLfloat *values);
	void APIENTRY (*GetPixelMapuiv)(GLenum map, GLuint *values);
	void APIENTRY (*GetPixelMapusv)(GLenum map, GLushort *values);
	void APIENTRY (*Bitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
	void APIENTRY (*ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
	void APIENTRY (*DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void APIENTRY (*CopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
	void APIENTRY (*StencilFunc)(GLenum func, GLint ref, GLuint mask);
	void APIENTRY (*StencilMask)(GLuint mask);
	void APIENTRY (*StencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
	void APIENTRY (*ClearStencil)(GLint s);
	void APIENTRY (*TexGend)(GLenum coord, GLenum pname, GLdouble param);
	void APIENTRY (*TexGenf)(GLenum coord, GLenum pname, GLfloat param);
	void APIENTRY (*TexGeni)(GLenum coord, GLenum pname, GLint param);
	void APIENTRY (*TexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
	void APIENTRY (*TexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
	void APIENTRY (*TexGeniv)(GLenum coord, GLenum pname, const GLint *params);
	void APIENTRY (*GetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
	void APIENTRY (*GetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
	void APIENTRY (*GetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
	void APIENTRY (*TexEnvf)(GLenum target, GLenum pname, GLfloat param);
	void APIENTRY (*TexEnvi)(GLenum target, GLenum pname, GLint param);
	void APIENTRY (*TexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*TexEnviv)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*GetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetTexEnviv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*TexParameterf)(GLenum target, GLenum pname, GLfloat param);
	void APIENTRY (*TexParameteri)(GLenum target, GLenum pname, GLint param);
	void APIENTRY (*TexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*TexParameteriv)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*GetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
	void APIENTRY (*GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
	void APIENTRY (*TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void APIENTRY (*TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void APIENTRY (*GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
	void APIENTRY (*GenTextures)(GLsizei n, GLuint *textures);
	void APIENTRY (*DeleteTextures)(GLsizei n, const GLuint *textures);
	void APIENTRY (*BindTexture)(GLenum target, GLuint texture);
	void APIENTRY (*PrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
	GLboolean APIENTRY (*AreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences);
	GLboolean APIENTRY (*IsTexture)(GLuint texture);
	void APIENTRY (*TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
	void APIENTRY (*TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void APIENTRY (*CopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
	void APIENTRY (*CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void APIENTRY (*CopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	void APIENTRY (*CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*Map1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
	void APIENTRY (*Map1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
	void APIENTRY (*Map2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
	void APIENTRY (*Map2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
	void APIENTRY (*GetMapdv)(GLenum target, GLenum query, GLdouble *v);
	void APIENTRY (*GetMapfv)(GLenum target, GLenum query, GLfloat *v);
	void APIENTRY (*GetMapiv)(GLenum target, GLenum query, GLint *v);
	void APIENTRY (*EvalCoord1d)(GLdouble u);
	void APIENTRY (*EvalCoord1f)(GLfloat u);
	void APIENTRY (*EvalCoord1dv)(const GLdouble *u);
	void APIENTRY (*EvalCoord1fv)(const GLfloat *u);
	void APIENTRY (*EvalCoord2d)(GLdouble u, GLdouble v);
	void APIENTRY (*EvalCoord2f)(GLfloat u, GLfloat v);
	void APIENTRY (*EvalCoord2dv)(const GLdouble *u);
	void APIENTRY (*EvalCoord2fv)(const GLfloat *u);
	void APIENTRY (*MapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
	void APIENTRY (*MapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
	void APIENTRY (*MapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
	void APIENTRY (*MapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
	void APIENTRY (*EvalPoint1)(GLint i);
	void APIENTRY (*EvalPoint2)(GLint i, GLint j);
	void APIENTRY (*EvalMesh1)(GLenum mode, GLint i1, GLint i2);
	void APIENTRY (*EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
	void APIENTRY (*Fogf)(GLenum pname, GLfloat param);
	void APIENTRY (*Fogi)(GLenum pname, GLint param);
	void APIENTRY (*Fogfv)(GLenum pname, const GLfloat *params);
	void APIENTRY (*Fogiv)(GLenum pname, const GLint *params);
	void APIENTRY (*FeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
	void APIENTRY (*PassThrough)(GLfloat token);
	void APIENTRY (*SelectBuffer)(GLsizei size, GLuint *buffer);
	void APIENTRY (*InitNames)(void);
	void APIENTRY (*LoadName)(GLuint name);
	void APIENTRY (*PushName)(GLuint name);
	void APIENTRY (*PopName)(void);
	void APIENTRY (*EnableTraceMESA)(GLbitfield mask);
	void APIENTRY (*DisableTraceMESA)(GLbitfield mask);
	void APIENTRY (*NewTraceMESA)(GLbitfield mask, const GLubyte *traceName);
	void APIENTRY (*EndTraceMESA)(void);
	void APIENTRY (*TraceAssertAttribMESA)(GLbitfield attribMask);
	void APIENTRY (*TraceCommentMESA)(const GLubyte *comment);
	void APIENTRY (*TraceTextureMESA)(GLuint name, const GLubyte *comment);
	void APIENTRY (*TraceListMESA)(GLuint name, const GLubyte *comment);
	void APIENTRY (*TracePointerMESA)(GLvoid *pointer, const GLubyte *comment);
	void APIENTRY (*TracePointerRangeMESA)(const GLvoid *first, const GLvoid *last, const GLubyte *comment);
	void APIENTRY (*BlendEquationSeparateATI)(GLenum equationRGB, GLenum equationAlpha);
	void APIENTRY (*BlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void APIENTRY (*BlendEquation)(GLenum mode);
	void APIENTRY (*DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
	void APIENTRY (*ColorTable)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table);
	void APIENTRY (*ColorTableParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*ColorTableParameteriv)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*CopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
	void APIENTRY (*GetColorTable)(GLenum target, GLenum format, GLenum type, void *table);
	void APIENTRY (*GetColorTableParameterfv)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetColorTableParameteriv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*ColorSubTable)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data);
	void APIENTRY (*CopyColorSubTable)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
	void APIENTRY (*ConvolutionFilter1D)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image);
	void APIENTRY (*ConvolutionFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image);
	void APIENTRY (*ConvolutionParameterf)(GLenum target, GLenum pname, GLfloat params);
	void APIENTRY (*ConvolutionParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*ConvolutionParameteri)(GLenum target, GLenum pname, GLint params);
	void APIENTRY (*ConvolutionParameteriv)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*CopyConvolutionFilter1D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
	void APIENTRY (*CopyConvolutionFilter2D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*GetConvolutionFilter)(GLenum target, GLenum format, GLenum type, void *image);
	void APIENTRY (*GetConvolutionParameterfv)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetConvolutionParameteriv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetSeparableFilter)(GLenum target, GLenum format, GLenum type, void *row, void *column, void *span);
	void APIENTRY (*SeparableFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column);
	void APIENTRY (*GetHistogram)(GLenum target, GLboolean32 reset, GLenum format, GLenum type, void *values);
	void APIENTRY (*GetHistogramParameterfv)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetHistogramParameteriv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetMinmax)(GLenum target, GLboolean32 reset, GLenum format, GLenum type, void *values);
	void APIENTRY (*GetMinmaxParameterfv)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetMinmaxParameteriv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*Histogram)(GLenum target, GLsizei width, GLenum internalformat, GLboolean32 sink);
	void APIENTRY (*Minmax)(GLenum target, GLenum internalformat, GLboolean32 sink);
	void APIENTRY (*ResetHistogram)(GLenum target);
	void APIENTRY (*ResetMinmax)(GLenum target);
	void APIENTRY (*TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*CopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*ActiveTexture)(GLenum texture);
	void APIENTRY (*ClientActiveTexture)(GLenum texture);
	void APIENTRY (*MultiTexCoord1d)(GLenum target, GLdouble s);
	void APIENTRY (*MultiTexCoord1dv)(GLenum target, const GLdouble *v);
	void APIENTRY (*MultiTexCoord1f)(GLenum target, GLfloat s);
	void APIENTRY (*MultiTexCoord1fv)(GLenum target, const GLfloat *v);
	void APIENTRY (*MultiTexCoord1i)(GLenum target, GLint s);
	void APIENTRY (*MultiTexCoord1iv)(GLenum target, const GLint *v);
	void APIENTRY (*MultiTexCoord1s)(GLenum target, GLshort32 s);
	void APIENTRY (*MultiTexCoord1sv)(GLenum target, const GLshort *v);
	void APIENTRY (*MultiTexCoord2d)(GLenum target, GLdouble s, GLdouble t);
	void APIENTRY (*MultiTexCoord2dv)(GLenum target, const GLdouble *v);
	void APIENTRY (*MultiTexCoord2f)(GLenum target, GLfloat s, GLfloat t);
	void APIENTRY (*MultiTexCoord2fv)(GLenum target, const GLfloat *v);
	void APIENTRY (*MultiTexCoord2i)(GLenum target, GLint s, GLint t);
	void APIENTRY (*MultiTexCoord2iv)(GLenum target, const GLint *v);
	void APIENTRY (*MultiTexCoord2s)(GLenum target, GLshort32 s, GLshort32 t);
	void APIENTRY (*MultiTexCoord2sv)(GLenum target, const GLshort *v);
	void APIENTRY (*MultiTexCoord3d)(GLenum target, GLdouble s, GLdouble t, GLdouble r);
	void APIENTRY (*MultiTexCoord3dv)(GLenum target, const GLdouble *v);
	void APIENTRY (*MultiTexCoord3f)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
	void APIENTRY (*MultiTexCoord3fv)(GLenum target, const GLfloat *v);
	void APIENTRY (*MultiTexCoord3i)(GLenum target, GLint s, GLint t, GLint r);
	void APIENTRY (*MultiTexCoord3iv)(GLenum target, const GLint *v);
	void APIENTRY (*MultiTexCoord3s)(GLenum target, GLshort32 s, GLshort32 t, GLshort32 r);
	void APIENTRY (*MultiTexCoord3sv)(GLenum target, const GLshort *v);
	void APIENTRY (*MultiTexCoord4d)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
	void APIENTRY (*MultiTexCoord4dv)(GLenum target, const GLdouble *v);
	void APIENTRY (*MultiTexCoord4f)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	void APIENTRY (*MultiTexCoord4fv)(GLenum target, const GLfloat *v);
	void APIENTRY (*MultiTexCoord4i)(GLenum target, GLint s, GLint t, GLint r, GLint q);
	void APIENTRY (*MultiTexCoord4iv)(GLenum target, const GLint *v);
	void APIENTRY (*MultiTexCoord4s)(GLenum target, GLshort32 s, GLshort32 t, GLshort32 r, GLshort32 q);
	void APIENTRY (*MultiTexCoord4sv)(GLenum target, const GLshort *v);
	void APIENTRY (*LoadTransposeMatrixf)(const GLfloat *m);
	void APIENTRY (*LoadTransposeMatrixd)(const GLdouble *m);
	void APIENTRY (*MultTransposeMatrixf)(const GLfloat *m);
	void APIENTRY (*MultTransposeMatrixd)(const GLdouble *m);
	void APIENTRY (*SampleCoverage)(GLfloat value, GLboolean32 invert);
	void APIENTRY (*CompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*GetCompressedTexImage)(GLenum target, GLint level, void *img);
	void APIENTRY (*BlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
	void APIENTRY (*FogCoordf)(GLfloat coord);
	void APIENTRY (*FogCoordfv)(const GLfloat *coord);
	void APIENTRY (*FogCoordd)(GLdouble coord);
	void APIENTRY (*FogCoorddv)(const GLdouble *coord);
	void APIENTRY (*FogCoordPointer)(GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*MultiDrawArrays)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount);
	void APIENTRY (*MultiDrawElements)(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei drawcount);
	void APIENTRY (*PointParameterf)(GLenum pname, GLfloat param);
	void APIENTRY (*PointParameterfv)(GLenum pname, const GLfloat *params);
	void APIENTRY (*PointParameteri)(GLenum pname, GLint param);
	void APIENTRY (*PointParameteriv)(GLenum pname, const GLint *params);
	void APIENTRY (*SecondaryColor3b)(GLbyte32 red, GLbyte32 green, GLbyte32 blue);
	void APIENTRY (*SecondaryColor3bv)(const GLbyte *v);
	void APIENTRY (*SecondaryColor3d)(GLdouble red, GLdouble green, GLdouble blue);
	void APIENTRY (*SecondaryColor3dv)(const GLdouble *v);
	void APIENTRY (*SecondaryColor3f)(GLfloat red, GLfloat green, GLfloat blue);
	void APIENTRY (*SecondaryColor3fv)(const GLfloat *v);
	void APIENTRY (*SecondaryColor3i)(GLint red, GLint green, GLint blue);
	void APIENTRY (*SecondaryColor3iv)(const GLint *v);
	void APIENTRY (*SecondaryColor3s)(GLshort32 red, GLshort32 green, GLshort32 blue);
	void APIENTRY (*SecondaryColor3sv)(const GLshort *v);
	void APIENTRY (*SecondaryColor3ub)(GLubyte32 red, GLubyte32 green, GLubyte32 blue);
	void APIENTRY (*SecondaryColor3ubv)(const GLubyte *v);
	void APIENTRY (*SecondaryColor3ui)(GLuint red, GLuint green, GLuint blue);
	void APIENTRY (*SecondaryColor3uiv)(const GLuint *v);
	void APIENTRY (*SecondaryColor3us)(GLushort32 red, GLushort32 green, GLushort32 blue);
	void APIENTRY (*SecondaryColor3usv)(const GLushort *v);
	void APIENTRY (*SecondaryColorPointer)(GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*WindowPos2d)(GLdouble x, GLdouble y);
	void APIENTRY (*WindowPos2dv)(const GLdouble *v);
	void APIENTRY (*WindowPos2f)(GLfloat x, GLfloat y);
	void APIENTRY (*WindowPos2fv)(const GLfloat *v);
	void APIENTRY (*WindowPos2i)(GLint x, GLint y);
	void APIENTRY (*WindowPos2iv)(const GLint *v);
	void APIENTRY (*WindowPos2s)(GLshort32 x, GLshort32 y);
	void APIENTRY (*WindowPos2sv)(const GLshort *v);
	void APIENTRY (*WindowPos3d)(GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*WindowPos3dv)(const GLdouble *v);
	void APIENTRY (*WindowPos3f)(GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*WindowPos3fv)(const GLfloat *v);
	void APIENTRY (*WindowPos3i)(GLint x, GLint y, GLint z);
	void APIENTRY (*WindowPos3iv)(const GLint *v);
	void APIENTRY (*WindowPos3s)(GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*WindowPos3sv)(const GLshort *v);
	void APIENTRY (*GenQueries)(GLsizei n, GLuint *ids);
	void APIENTRY (*DeleteQueries)(GLsizei n, const GLuint *ids);
	GLboolean APIENTRY (*IsQuery)(GLuint id);
	void APIENTRY (*BeginQuery)(GLenum target, GLuint id);
	void APIENTRY (*EndQuery)(GLenum target);
	void APIENTRY (*GetQueryiv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetQueryObjectiv)(GLuint id, GLenum pname, GLint *params);
	void APIENTRY (*GetQueryObjectuiv)(GLuint id, GLenum pname, GLuint *params);
	void APIENTRY (*BindBuffer)(GLenum target, GLuint buffer);
	void APIENTRY (*DeleteBuffers)(GLsizei n, const GLuint *buffers);
	void APIENTRY (*GenBuffers)(GLsizei n, GLuint *buffers);
	GLboolean APIENTRY (*IsBuffer)(GLuint buffer);
	void APIENTRY (*BufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
	void APIENTRY (*BufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
	void APIENTRY (*GetBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, void *data);
	void * APIENTRY (*MapBuffer)(GLenum target, GLenum access);
	GLboolean APIENTRY (*UnmapBuffer)(GLenum target);
	void APIENTRY (*GetBufferParameteriv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetBufferPointerv)(GLenum target, GLenum pname, void * *params);
	void APIENTRY (*ActiveTextureARB)(GLenum texture);
	void APIENTRY (*ClientActiveTextureARB)(GLenum texture);
	void APIENTRY (*MultiTexCoord1dARB)(GLenum target, GLdouble s);
	void APIENTRY (*MultiTexCoord1dvARB)(GLenum target, const GLdouble *v);
	void APIENTRY (*MultiTexCoord1fARB)(GLenum target, GLfloat s);
	void APIENTRY (*MultiTexCoord1fvARB)(GLenum target, const GLfloat *v);
	void APIENTRY (*MultiTexCoord1iARB)(GLenum target, GLint s);
	void APIENTRY (*MultiTexCoord1ivARB)(GLenum target, const GLint *v);
	void APIENTRY (*MultiTexCoord1sARB)(GLenum target, GLshort32 s);
	void APIENTRY (*MultiTexCoord1svARB)(GLenum target, const GLshort *v);
	void APIENTRY (*MultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t);
	void APIENTRY (*MultiTexCoord2dvARB)(GLenum target, const GLdouble *v);
	void APIENTRY (*MultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t);
	void APIENTRY (*MultiTexCoord2fvARB)(GLenum target, const GLfloat *v);
	void APIENTRY (*MultiTexCoord2iARB)(GLenum target, GLint s, GLint t);
	void APIENTRY (*MultiTexCoord2ivARB)(GLenum target, const GLint *v);
	void APIENTRY (*MultiTexCoord2sARB)(GLenum target, GLshort32 s, GLshort32 t);
	void APIENTRY (*MultiTexCoord2svARB)(GLenum target, const GLshort *v);
	void APIENTRY (*MultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r);
	void APIENTRY (*MultiTexCoord3dvARB)(GLenum target, const GLdouble *v);
	void APIENTRY (*MultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
	void APIENTRY (*MultiTexCoord3fvARB)(GLenum target, const GLfloat *v);
	void APIENTRY (*MultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r);
	void APIENTRY (*MultiTexCoord3ivARB)(GLenum target, const GLint *v);
	void APIENTRY (*MultiTexCoord3sARB)(GLenum target, GLshort32 s, GLshort32 t, GLshort32 r);
	void APIENTRY (*MultiTexCoord3svARB)(GLenum target, const GLshort *v);
	void APIENTRY (*MultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
	void APIENTRY (*MultiTexCoord4dvARB)(GLenum target, const GLdouble *v);
	void APIENTRY (*MultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	void APIENTRY (*MultiTexCoord4fvARB)(GLenum target, const GLfloat *v);
	void APIENTRY (*MultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q);
	void APIENTRY (*MultiTexCoord4ivARB)(GLenum target, const GLint *v);
	void APIENTRY (*MultiTexCoord4sARB)(GLenum target, GLshort32 s, GLshort32 t, GLshort32 r, GLshort32 q);
	void APIENTRY (*MultiTexCoord4svARB)(GLenum target, const GLshort *v);
	void APIENTRY (*LoadTransposeMatrixfARB)(const GLfloat *m);
	void APIENTRY (*LoadTransposeMatrixdARB)(const GLdouble *m);
	void APIENTRY (*MultTransposeMatrixfARB)(const GLfloat *m);
	void APIENTRY (*MultTransposeMatrixdARB)(const GLdouble *m);
	void APIENTRY (*SampleCoverageARB)(GLfloat value, GLboolean32 invert);
	void APIENTRY (*CompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*GetCompressedTexImageARB)(GLenum target, GLint level, void *img);
	void APIENTRY (*PointParameterfARB)(GLenum pname, GLfloat param);
	void APIENTRY (*PointParameterfvARB)(GLenum pname, const GLfloat *params);
	void APIENTRY (*WeightbvARB)(GLint size, const GLbyte *weights);
	void APIENTRY (*WeightsvARB)(GLint size, const GLshort *weights);
	void APIENTRY (*WeightivARB)(GLint size, const GLint *weights);
	void APIENTRY (*WeightfvARB)(GLint size, const GLfloat *weights);
	void APIENTRY (*WeightdvARB)(GLint size, const GLdouble *weights);
	void APIENTRY (*WeightubvARB)(GLint size, const GLubyte *weights);
	void APIENTRY (*WeightusvARB)(GLint size, const GLushort *weights);
	void APIENTRY (*WeightuivARB)(GLint size, const GLuint *weights);
	void APIENTRY (*WeightPointerARB)(GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*VertexBlendARB)(GLint count);
	void APIENTRY (*CurrentPaletteMatrixARB)(GLint index);
	void APIENTRY (*MatrixIndexubvARB)(GLint size, const GLubyte *indices);
	void APIENTRY (*MatrixIndexusvARB)(GLint size, const GLushort *indices);
	void APIENTRY (*MatrixIndexuivARB)(GLint size, const GLuint *indices);
	void APIENTRY (*MatrixIndexPointerARB)(GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*WindowPos2dARB)(GLdouble x, GLdouble y);
	void APIENTRY (*WindowPos2dvARB)(const GLdouble *v);
	void APIENTRY (*WindowPos2fARB)(GLfloat x, GLfloat y);
	void APIENTRY (*WindowPos2fvARB)(const GLfloat *v);
	void APIENTRY (*WindowPos2iARB)(GLint x, GLint y);
	void APIENTRY (*WindowPos2ivARB)(const GLint *v);
	void APIENTRY (*WindowPos2sARB)(GLshort32 x, GLshort32 y);
	void APIENTRY (*WindowPos2svARB)(const GLshort *v);
	void APIENTRY (*WindowPos3dARB)(GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*WindowPos3dvARB)(const GLdouble *v);
	void APIENTRY (*WindowPos3fARB)(GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*WindowPos3fvARB)(const GLfloat *v);
	void APIENTRY (*WindowPos3iARB)(GLint x, GLint y, GLint z);
	void APIENTRY (*WindowPos3ivARB)(const GLint *v);
	void APIENTRY (*WindowPos3sARB)(GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*WindowPos3svARB)(const GLshort *v);
	void APIENTRY (*VertexAttrib1dARB)(GLuint index, GLdouble x);
	void APIENTRY (*VertexAttrib1dvARB)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib1fARB)(GLuint index, GLfloat x);
	void APIENTRY (*VertexAttrib1fvARB)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib1sARB)(GLuint index, GLshort32 x);
	void APIENTRY (*VertexAttrib1svARB)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib2dARB)(GLuint index, GLdouble x, GLdouble y);
	void APIENTRY (*VertexAttrib2dvARB)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib2fARB)(GLuint index, GLfloat x, GLfloat y);
	void APIENTRY (*VertexAttrib2fvARB)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib2sARB)(GLuint index, GLshort32 x, GLshort32 y);
	void APIENTRY (*VertexAttrib2svARB)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib3dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*VertexAttrib3dvARB)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib3fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*VertexAttrib3fvARB)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib3sARB)(GLuint index, GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*VertexAttrib3svARB)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib4NbvARB)(GLuint index, const GLbyte *v);
	void APIENTRY (*VertexAttrib4NivARB)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttrib4NsvARB)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib4NubARB)(GLuint index, GLubyte32 x, GLubyte32 y, GLubyte32 z, GLubyte32 w);
	void APIENTRY (*VertexAttrib4NubvARB)(GLuint index, const GLubyte *v);
	void APIENTRY (*VertexAttrib4NuivARB)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttrib4NusvARB)(GLuint index, const GLushort *v);
	void APIENTRY (*VertexAttrib4bvARB)(GLuint index, const GLbyte *v);
	void APIENTRY (*VertexAttrib4dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*VertexAttrib4dvARB)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib4fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*VertexAttrib4fvARB)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib4ivARB)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttrib4sARB)(GLuint index, GLshort32 x, GLshort32 y, GLshort32 z, GLshort32 w);
	void APIENTRY (*VertexAttrib4svARB)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib4ubvARB)(GLuint index, const GLubyte *v);
	void APIENTRY (*VertexAttrib4uivARB)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttrib4usvARB)(GLuint index, const GLushort *v);
	void APIENTRY (*VertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean32 normalized, GLsizei stride, const void *pointer);
	void APIENTRY (*EnableVertexAttribArrayARB)(GLuint index);
	void APIENTRY (*DisableVertexAttribArrayARB)(GLuint index);
	void APIENTRY (*ProgramStringARB)(GLenum target, GLenum format, GLsizei len, const void *string);
	void APIENTRY (*BindProgramARB)(GLenum target, GLuint program);
	void APIENTRY (*DeleteProgramsARB)(GLsizei n, const GLuint *programs);
	void APIENTRY (*GenProgramsARB)(GLsizei n, GLuint *programs);
	void APIENTRY (*ProgramEnvParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*ProgramEnvParameter4dvARB)(GLenum target, GLuint index, const GLdouble *params);
	void APIENTRY (*ProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*ProgramEnvParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params);
	void APIENTRY (*ProgramLocalParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*ProgramLocalParameter4dvARB)(GLenum target, GLuint index, const GLdouble *params);
	void APIENTRY (*ProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*ProgramLocalParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params);
	void APIENTRY (*GetProgramEnvParameterdvARB)(GLenum target, GLuint index, GLdouble *params);
	void APIENTRY (*GetProgramEnvParameterfvARB)(GLenum target, GLuint index, GLfloat *params);
	void APIENTRY (*GetProgramLocalParameterdvARB)(GLenum target, GLuint index, GLdouble *params);
	void APIENTRY (*GetProgramLocalParameterfvARB)(GLenum target, GLuint index, GLfloat *params);
	void APIENTRY (*GetProgramivARB)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetProgramStringARB)(GLenum target, GLenum pname, void *string);
	void APIENTRY (*GetVertexAttribdvARB)(GLuint index, GLenum pname, GLdouble *params);
	void APIENTRY (*GetVertexAttribfvARB)(GLuint index, GLenum pname, GLfloat *params);
	void APIENTRY (*GetVertexAttribivARB)(GLuint index, GLenum pname, GLint *params);
	void APIENTRY (*GetVertexAttribPointervARB)(GLuint index, GLenum pname, void * *pointer);
	GLboolean APIENTRY (*IsProgramARB)(GLuint program);
	void APIENTRY (*BindBufferARB)(GLenum target, GLuint buffer);
	void APIENTRY (*DeleteBuffersARB)(GLsizei n, const GLuint *buffers);
	void APIENTRY (*GenBuffersARB)(GLsizei n, GLuint *buffers);
	GLboolean APIENTRY (*IsBufferARB)(GLuint buffer);
	void APIENTRY (*BufferDataARB)(GLenum target, GLsizeiptrARB size, const void *data, GLenum usage);
	void APIENTRY (*BufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data);
	void APIENTRY (*GetBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, void *data);
	void * APIENTRY (*MapBufferARB)(GLenum target, GLenum access);
	GLboolean APIENTRY (*UnmapBufferARB)(GLenum target);
	void APIENTRY (*GetBufferParameterivARB)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetBufferPointervARB)(GLenum target, GLenum pname, void * *params);
	void APIENTRY (*GenQueriesARB)(GLsizei n, GLuint *ids);
	void APIENTRY (*DeleteQueriesARB)(GLsizei n, const GLuint *ids);
	GLboolean APIENTRY (*IsQueryARB)(GLuint id);
	void APIENTRY (*BeginQueryARB)(GLenum target, GLuint id);
	void APIENTRY (*EndQueryARB)(GLenum target);
	void APIENTRY (*GetQueryivARB)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetQueryObjectivARB)(GLuint id, GLenum pname, GLint *params);
	void APIENTRY (*GetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint *params);
	void APIENTRY (*DeleteObjectARB)(GLhandleARB obj);
	GLhandleARB APIENTRY (*GetHandleARB)(GLenum pname);
	void APIENTRY (*DetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj);
	GLhandleARB APIENTRY (*CreateShaderObjectARB)(GLenum shaderType);
	void APIENTRY (*ShaderSourceARB)(GLhandleARB shaderObj, GLsizei count, const GLcharARB * *string, const GLint *length);
	void APIENTRY (*CompileShaderARB)(GLhandleARB shaderObj);
	GLhandleARB APIENTRY (*CreateProgramObjectARB)(void);
	void APIENTRY (*AttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj);
	void APIENTRY (*LinkProgramARB)(GLhandleARB programObj);
	void APIENTRY (*UseProgramObjectARB)(GLhandleARB programObj);
	void APIENTRY (*ValidateProgramARB)(GLhandleARB programObj);
	void APIENTRY (*Uniform1fARB)(GLint location, GLfloat v0);
	void APIENTRY (*Uniform2fARB)(GLint location, GLfloat v0, GLfloat v1);
	void APIENTRY (*Uniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	void APIENTRY (*Uniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void APIENTRY (*Uniform1iARB)(GLint location, GLint v0);
	void APIENTRY (*Uniform2iARB)(GLint location, GLint v0, GLint v1);
	void APIENTRY (*Uniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2);
	void APIENTRY (*Uniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	void APIENTRY (*Uniform1fvARB)(GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*Uniform2fvARB)(GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*Uniform3fvARB)(GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*Uniform4fvARB)(GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*Uniform1ivARB)(GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*Uniform2ivARB)(GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*Uniform3ivARB)(GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*Uniform4ivARB)(GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*UniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*GetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat *params);
	void APIENTRY (*GetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint *params);
	void APIENTRY (*GetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
	void APIENTRY (*GetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
	GLint APIENTRY (*GetUniformLocationARB)(GLhandleARB programObj, const GLcharARB *name);
	void APIENTRY (*GetActiveUniformARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
	void APIENTRY (*GetUniformfvARB)(GLhandleARB programObj, GLint location, GLfloat *params);
	void APIENTRY (*GetUniformivARB)(GLhandleARB programObj, GLint location, GLint *params);
	void APIENTRY (*GetShaderSourceARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
	void APIENTRY (*BindAttribLocationARB)(GLhandleARB programObj, GLuint index, const GLcharARB *name);
	void APIENTRY (*GetActiveAttribARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
	GLint APIENTRY (*GetAttribLocationARB)(GLhandleARB programObj, const GLcharARB *name);
	void APIENTRY (*DrawBuffersARB)(GLsizei n, const GLenum *bufs);
	void APIENTRY (*BlendColorEXT)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void APIENTRY (*PolygonOffsetEXT)(GLfloat factor, GLfloat bias);
	void APIENTRY (*TexImage3DEXT)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*GetTexFilterFuncSGIS)(GLenum target, GLenum filter, GLfloat *weights);
	void APIENTRY (*TexFilterFuncSGIS)(GLenum target, GLenum filter, GLsizei n, const GLfloat *weights);
	void APIENTRY (*TexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*CopyTexImage1DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
	void APIENTRY (*CopyTexImage2DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void APIENTRY (*CopyTexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	void APIENTRY (*CopyTexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*CopyTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*GetHistogramEXT)(GLenum target, GLboolean32 reset, GLenum format, GLenum type, void *values);
	void APIENTRY (*GetHistogramParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetHistogramParameterivEXT)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetMinmaxEXT)(GLenum target, GLboolean32 reset, GLenum format, GLenum type, void *values);
	void APIENTRY (*GetMinmaxParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetMinmaxParameterivEXT)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*HistogramEXT)(GLenum target, GLsizei width, GLenum internalformat, GLboolean32 sink);
	void APIENTRY (*MinmaxEXT)(GLenum target, GLenum internalformat, GLboolean32 sink);
	void APIENTRY (*ResetHistogramEXT)(GLenum target);
	void APIENTRY (*ResetMinmaxEXT)(GLenum target);
	void APIENTRY (*ConvolutionFilter1DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image);
	void APIENTRY (*ConvolutionFilter2DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image);
	void APIENTRY (*ConvolutionParameterfEXT)(GLenum target, GLenum pname, GLfloat params);
	void APIENTRY (*ConvolutionParameterfvEXT)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*ConvolutionParameteriEXT)(GLenum target, GLenum pname, GLint params);
	void APIENTRY (*ConvolutionParameterivEXT)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*CopyConvolutionFilter1DEXT)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
	void APIENTRY (*CopyConvolutionFilter2DEXT)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*GetConvolutionFilterEXT)(GLenum target, GLenum format, GLenum type, void *image);
	void APIENTRY (*GetConvolutionParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetConvolutionParameterivEXT)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetSeparableFilterEXT)(GLenum target, GLenum format, GLenum type, void *row, void *column, void *span);
	void APIENTRY (*SeparableFilter2DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column);
	void APIENTRY (*ColorTableSGI)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table);
	void APIENTRY (*ColorTableParameterfvSGI)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*ColorTableParameterivSGI)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*CopyColorTableSGI)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
	void APIENTRY (*GetColorTableSGI)(GLenum target, GLenum format, GLenum type, void *table);
	void APIENTRY (*GetColorTableParameterfvSGI)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetColorTableParameterivSGI)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*PixelTexGenSGIX)(GLenum mode);
	void APIENTRY (*PixelTexGenParameteriSGIS)(GLenum pname, GLint param);
	void APIENTRY (*PixelTexGenParameterivSGIS)(GLenum pname, const GLint *params);
	void APIENTRY (*PixelTexGenParameterfSGIS)(GLenum pname, GLfloat param);
	void APIENTRY (*PixelTexGenParameterfvSGIS)(GLenum pname, const GLfloat *params);
	void APIENTRY (*GetPixelTexGenParameterivSGIS)(GLenum pname, GLint *params);
	void APIENTRY (*GetPixelTexGenParameterfvSGIS)(GLenum pname, GLfloat *params);
	void APIENTRY (*TexImage4DSGIS)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TexSubImage4DSGIS)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const void *pixels);
	GLboolean APIENTRY (*AreTexturesResidentEXT)(GLsizei n, const GLuint *textures, GLboolean *residences);
	void APIENTRY (*BindTextureEXT)(GLenum target, GLuint texture);
	void APIENTRY (*DeleteTexturesEXT)(GLsizei n, const GLuint *textures);
	void APIENTRY (*GenTexturesEXT)(GLsizei n, GLuint *textures);
	GLboolean APIENTRY (*IsTextureEXT)(GLuint texture);
	void APIENTRY (*PrioritizeTexturesEXT)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
	void APIENTRY (*DetailTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat *points);
	void APIENTRY (*GetDetailTexFuncSGIS)(GLenum target, GLfloat *points);
	void APIENTRY (*SharpenTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat *points);
	void APIENTRY (*GetSharpenTexFuncSGIS)(GLenum target, GLfloat *points);
	void APIENTRY (*SampleMaskSGIS)(GLclampf value, GLboolean32 invert);
	void APIENTRY (*SamplePatternSGIS)(GLenum pattern);
	void APIENTRY (*ArrayElementEXT)(GLint i);
	void APIENTRY (*ColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer);
	void APIENTRY (*DrawArraysEXT)(GLenum mode, GLint first, GLsizei count);
	void APIENTRY (*EdgeFlagPointerEXT)(GLsizei stride, GLsizei count, const GLboolean *pointer);
	void APIENTRY (*GetPointervEXT)(GLenum pname, void * *params);
	void APIENTRY (*IndexPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const void *pointer);
	void APIENTRY (*NormalPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const void *pointer);
	void APIENTRY (*TexCoordPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer);
	void APIENTRY (*VertexPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer);
	void APIENTRY (*BlendEquationEXT)(GLenum mode);
	void APIENTRY (*SpriteParameterfSGIX)(GLenum pname, GLfloat param);
	void APIENTRY (*SpriteParameterfvSGIX)(GLenum pname, const GLfloat *params);
	void APIENTRY (*SpriteParameteriSGIX)(GLenum pname, GLint param);
	void APIENTRY (*SpriteParameterivSGIX)(GLenum pname, const GLint *params);
	void APIENTRY (*PointParameterfEXT)(GLenum pname, GLfloat param);
	void APIENTRY (*PointParameterfvEXT)(GLenum pname, const GLfloat *params);
	void APIENTRY (*PointParameterfSGIS)(GLenum pname, GLfloat param);
	void APIENTRY (*PointParameterfvSGIS)(GLenum pname, const GLfloat *params);
	GLint APIENTRY (*GetInstrumentsSGIX)(void);
	void APIENTRY (*InstrumentsBufferSGIX)(GLsizei size, GLint *buffer);
	GLint APIENTRY (*PollInstrumentsSGIX)(GLint *marker_p);
	void APIENTRY (*ReadInstrumentsSGIX)(GLint marker);
	void APIENTRY (*StartInstrumentsSGIX)(void);
	void APIENTRY (*StopInstrumentsSGIX)(GLint marker);
	void APIENTRY (*FrameZoomSGIX)(GLint factor);
	void APIENTRY (*TagSampleBufferSGIX)(void);
	void APIENTRY (*DeformationMap3dSGIX)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points);
	void APIENTRY (*DeformationMap3fSGIX)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points);
	void APIENTRY (*DeformSGIX)(GLbitfield mask);
	void APIENTRY (*LoadIdentityDeformationMapSGIX)(GLbitfield mask);
	void APIENTRY (*ReferencePlaneSGIX)(const GLdouble *equation);
	void APIENTRY (*FlushRasterSGIX)(void);
	void APIENTRY (*FogFuncSGIS)(GLsizei n, const GLfloat *points);
	void APIENTRY (*GetFogFuncSGIS)(GLfloat *points);
	void APIENTRY (*ImageTransformParameteriHP)(GLenum target, GLenum pname, GLint param);
	void APIENTRY (*ImageTransformParameterfHP)(GLenum target, GLenum pname, GLfloat param);
	void APIENTRY (*ImageTransformParameterivHP)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*ImageTransformParameterfvHP)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*GetImageTransformParameterivHP)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetImageTransformParameterfvHP)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*ColorSubTableEXT)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data);
	void APIENTRY (*CopyColorSubTableEXT)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
	void APIENTRY (*HintPGI)(GLenum target, GLint mode);
	void APIENTRY (*ColorTableEXT)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void *table);
	void APIENTRY (*GetColorTableEXT)(GLenum target, GLenum format, GLenum type, void *data);
	void APIENTRY (*GetColorTableParameterivEXT)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetColorTableParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetListParameterfvSGIX)(GLuint list, GLenum pname, GLfloat *params);
	void APIENTRY (*GetListParameterivSGIX)(GLuint list, GLenum pname, GLint *params);
	void APIENTRY (*ListParameterfSGIX)(GLuint list, GLenum pname, GLfloat param);
	void APIENTRY (*ListParameterfvSGIX)(GLuint list, GLenum pname, const GLfloat *params);
	void APIENTRY (*ListParameteriSGIX)(GLuint list, GLenum pname, GLint param);
	void APIENTRY (*ListParameterivSGIX)(GLuint list, GLenum pname, const GLint *params);
	void APIENTRY (*IndexMaterialEXT)(GLenum face, GLenum mode);
	void APIENTRY (*IndexFuncEXT)(GLenum func, GLclampf ref);
	void APIENTRY (*LockArraysEXT)(GLint first, GLsizei count);
	void APIENTRY (*UnlockArraysEXT)(void);
	void APIENTRY (*CullParameterdvEXT)(GLenum pname, GLdouble *params);
	void APIENTRY (*CullParameterfvEXT)(GLenum pname, GLfloat *params);
	void APIENTRY (*FragmentColorMaterialSGIX)(GLenum face, GLenum mode);
	void APIENTRY (*FragmentLightfSGIX)(GLenum light, GLenum pname, GLfloat param);
	void APIENTRY (*FragmentLightfvSGIX)(GLenum light, GLenum pname, const GLfloat *params);
	void APIENTRY (*FragmentLightiSGIX)(GLenum light, GLenum pname, GLint param);
	void APIENTRY (*FragmentLightivSGIX)(GLenum light, GLenum pname, const GLint *params);
	void APIENTRY (*FragmentLightModelfSGIX)(GLenum pname, GLfloat param);
	void APIENTRY (*FragmentLightModelfvSGIX)(GLenum pname, const GLfloat *params);
	void APIENTRY (*FragmentLightModeliSGIX)(GLenum pname, GLint param);
	void APIENTRY (*FragmentLightModelivSGIX)(GLenum pname, const GLint *params);
	void APIENTRY (*FragmentMaterialfSGIX)(GLenum face, GLenum pname, GLfloat param);
	void APIENTRY (*FragmentMaterialfvSGIX)(GLenum face, GLenum pname, const GLfloat *params);
	void APIENTRY (*FragmentMaterialiSGIX)(GLenum face, GLenum pname, GLint param);
	void APIENTRY (*FragmentMaterialivSGIX)(GLenum face, GLenum pname, const GLint *params);
	void APIENTRY (*GetFragmentLightfvSGIX)(GLenum light, GLenum pname, GLfloat *params);
	void APIENTRY (*GetFragmentLightivSGIX)(GLenum light, GLenum pname, GLint *params);
	void APIENTRY (*GetFragmentMaterialfvSGIX)(GLenum face, GLenum pname, GLfloat *params);
	void APIENTRY (*GetFragmentMaterialivSGIX)(GLenum face, GLenum pname, GLint *params);
	void APIENTRY (*LightEnviSGIX)(GLenum pname, GLint param);
	void APIENTRY (*DrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
	void APIENTRY (*ApplyTextureEXT)(GLenum mode);
	void APIENTRY (*TextureLightEXT)(GLenum pname);
	void APIENTRY (*TextureMaterialEXT)(GLenum face, GLenum mode);
	void APIENTRY (*AsyncMarkerSGIX)(GLuint marker);
	GLint APIENTRY (*FinishAsyncSGIX)(GLuint *markerp);
	GLint APIENTRY (*PollAsyncSGIX)(GLuint *markerp);
	GLuint APIENTRY (*GenAsyncMarkersSGIX)(GLsizei range);
	void APIENTRY (*DeleteAsyncMarkersSGIX)(GLuint marker, GLsizei range);
	GLboolean APIENTRY (*IsAsyncMarkerSGIX)(GLuint marker);
	void APIENTRY (*VertexPointervINTEL)(GLint size, GLenum type, const void * *pointer);
	void APIENTRY (*NormalPointervINTEL)(GLenum type, const void * *pointer);
	void APIENTRY (*ColorPointervINTEL)(GLint size, GLenum type, const void * *pointer);
	void APIENTRY (*TexCoordPointervINTEL)(GLint size, GLenum type, const void * *pointer);
	void APIENTRY (*PixelTransformParameteriEXT)(GLenum target, GLenum pname, GLint param);
	void APIENTRY (*PixelTransformParameterfEXT)(GLenum target, GLenum pname, GLfloat param);
	void APIENTRY (*PixelTransformParameterivEXT)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*PixelTransformParameterfvEXT)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*SecondaryColor3bEXT)(GLbyte32 red, GLbyte32 green, GLbyte32 blue);
	void APIENTRY (*SecondaryColor3bvEXT)(const GLbyte *v);
	void APIENTRY (*SecondaryColor3dEXT)(GLdouble red, GLdouble green, GLdouble blue);
	void APIENTRY (*SecondaryColor3dvEXT)(const GLdouble *v);
	void APIENTRY (*SecondaryColor3fEXT)(GLfloat red, GLfloat green, GLfloat blue);
	void APIENTRY (*SecondaryColor3fvEXT)(const GLfloat *v);
	void APIENTRY (*SecondaryColor3iEXT)(GLint red, GLint green, GLint blue);
	void APIENTRY (*SecondaryColor3ivEXT)(const GLint *v);
	void APIENTRY (*SecondaryColor3sEXT)(GLshort32 red, GLshort32 green, GLshort32 blue);
	void APIENTRY (*SecondaryColor3svEXT)(const GLshort *v);
	void APIENTRY (*SecondaryColor3ubEXT)(GLubyte32 red, GLubyte32 green, GLubyte32 blue);
	void APIENTRY (*SecondaryColor3ubvEXT)(const GLubyte *v);
	void APIENTRY (*SecondaryColor3uiEXT)(GLuint red, GLuint green, GLuint blue);
	void APIENTRY (*SecondaryColor3uivEXT)(const GLuint *v);
	void APIENTRY (*SecondaryColor3usEXT)(GLushort32 red, GLushort32 green, GLushort32 blue);
	void APIENTRY (*SecondaryColor3usvEXT)(const GLushort *v);
	void APIENTRY (*SecondaryColorPointerEXT)(GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*TextureNormalEXT)(GLenum mode);
	void APIENTRY (*MultiDrawArraysEXT)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
	void APIENTRY (*MultiDrawElementsEXT)(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount);
	void APIENTRY (*FogCoordfEXT)(GLfloat coord);
	void APIENTRY (*FogCoordfvEXT)(const GLfloat *coord);
	void APIENTRY (*FogCoorddEXT)(GLdouble coord);
	void APIENTRY (*FogCoorddvEXT)(const GLdouble *coord);
	void APIENTRY (*FogCoordPointerEXT)(GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*Tangent3bEXT)(GLbyte32 tx, GLbyte32 ty, GLbyte32 tz);
	void APIENTRY (*Tangent3bvEXT)(const GLbyte *v);
	void APIENTRY (*Tangent3dEXT)(GLdouble tx, GLdouble ty, GLdouble tz);
	void APIENTRY (*Tangent3dvEXT)(const GLdouble *v);
	void APIENTRY (*Tangent3fEXT)(GLfloat tx, GLfloat ty, GLfloat tz);
	void APIENTRY (*Tangent3fvEXT)(const GLfloat *v);
	void APIENTRY (*Tangent3iEXT)(GLint tx, GLint ty, GLint tz);
	void APIENTRY (*Tangent3ivEXT)(const GLint *v);
	void APIENTRY (*Tangent3sEXT)(GLshort32 tx, GLshort32 ty, GLshort32 tz);
	void APIENTRY (*Tangent3svEXT)(const GLshort *v);
	void APIENTRY (*Binormal3bEXT)(GLbyte32 bx, GLbyte32 by, GLbyte32 bz);
	void APIENTRY (*Binormal3bvEXT)(const GLbyte *v);
	void APIENTRY (*Binormal3dEXT)(GLdouble bx, GLdouble by, GLdouble bz);
	void APIENTRY (*Binormal3dvEXT)(const GLdouble *v);
	void APIENTRY (*Binormal3fEXT)(GLfloat bx, GLfloat by, GLfloat bz);
	void APIENTRY (*Binormal3fvEXT)(const GLfloat *v);
	void APIENTRY (*Binormal3iEXT)(GLint bx, GLint by, GLint bz);
	void APIENTRY (*Binormal3ivEXT)(const GLint *v);
	void APIENTRY (*Binormal3sEXT)(GLshort32 bx, GLshort32 by, GLshort32 bz);
	void APIENTRY (*Binormal3svEXT)(const GLshort *v);
	void APIENTRY (*TangentPointerEXT)(GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*BinormalPointerEXT)(GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*FinishTextureSUNX)(void);
	void APIENTRY (*GlobalAlphaFactorbSUN)(GLbyte32 factor);
	void APIENTRY (*GlobalAlphaFactorsSUN)(GLshort32 factor);
	void APIENTRY (*GlobalAlphaFactoriSUN)(GLint factor);
	void APIENTRY (*GlobalAlphaFactorfSUN)(GLfloat factor);
	void APIENTRY (*GlobalAlphaFactordSUN)(GLdouble factor);
	void APIENTRY (*GlobalAlphaFactorubSUN)(GLubyte32 factor);
	void APIENTRY (*GlobalAlphaFactorusSUN)(GLushort32 factor);
	void APIENTRY (*GlobalAlphaFactoruiSUN)(GLuint factor);
	void APIENTRY (*ReplacementCodeuiSUN)(GLuint code);
	void APIENTRY (*ReplacementCodeusSUN)(GLushort32 code);
	void APIENTRY (*ReplacementCodeubSUN)(GLubyte32 code);
	void APIENTRY (*ReplacementCodeuivSUN)(const GLuint *code);
	void APIENTRY (*ReplacementCodeusvSUN)(const GLushort *code);
	void APIENTRY (*ReplacementCodeubvSUN)(const GLubyte *code);
	void APIENTRY (*ReplacementCodePointerSUN)(GLenum type, GLsizei stride, const void * *pointer);
	void APIENTRY (*Color4ubVertex2fSUN)(GLubyte32 r, GLubyte32 g, GLubyte32 b, GLubyte32 a, GLfloat x, GLfloat y);
	void APIENTRY (*Color4ubVertex2fvSUN)(const GLubyte *c, const GLfloat *v);
	void APIENTRY (*Color4ubVertex3fSUN)(GLubyte32 r, GLubyte32 g, GLubyte32 b, GLubyte32 a, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*Color4ubVertex3fvSUN)(const GLubyte *c, const GLfloat *v);
	void APIENTRY (*Color3fVertex3fSUN)(GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*Color3fVertex3fvSUN)(const GLfloat *c, const GLfloat *v);
	void APIENTRY (*Normal3fVertex3fSUN)(GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*Normal3fVertex3fvSUN)(const GLfloat *n, const GLfloat *v);
	void APIENTRY (*Color4fNormal3fVertex3fSUN)(GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*Color4fNormal3fVertex3fvSUN)(const GLfloat *c, const GLfloat *n, const GLfloat *v);
	void APIENTRY (*TexCoord2fVertex3fSUN)(GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*TexCoord2fVertex3fvSUN)(const GLfloat *tc, const GLfloat *v);
	void APIENTRY (*TexCoord4fVertex4fSUN)(GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*TexCoord4fVertex4fvSUN)(const GLfloat *tc, const GLfloat *v);
	void APIENTRY (*TexCoord2fColor4ubVertex3fSUN)(GLfloat s, GLfloat t, GLubyte32 r, GLubyte32 g, GLubyte32 b, GLubyte32 a, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*TexCoord2fColor4ubVertex3fvSUN)(const GLfloat *tc, const GLubyte *c, const GLfloat *v);
	void APIENTRY (*TexCoord2fColor3fVertex3fSUN)(GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*TexCoord2fColor3fVertex3fvSUN)(const GLfloat *tc, const GLfloat *c, const GLfloat *v);
	void APIENTRY (*TexCoord2fNormal3fVertex3fSUN)(GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*TexCoord2fNormal3fVertex3fvSUN)(const GLfloat *tc, const GLfloat *n, const GLfloat *v);
	void APIENTRY (*TexCoord2fColor4fNormal3fVertex3fSUN)(GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*TexCoord2fColor4fNormal3fVertex3fvSUN)(const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
	void APIENTRY (*TexCoord4fColor4fNormal3fVertex4fSUN)(GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*TexCoord4fColor4fNormal3fVertex4fvSUN)(const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
	void APIENTRY (*ReplacementCodeuiVertex3fSUN)(GLuint rc, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ReplacementCodeuiVertex3fvSUN)(const GLuint *rc, const GLfloat *v);
	void APIENTRY (*ReplacementCodeuiColor4ubVertex3fSUN)(GLuint rc, GLubyte32 r, GLubyte32 g, GLubyte32 b, GLubyte32 a, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ReplacementCodeuiColor4ubVertex3fvSUN)(const GLuint *rc, const GLubyte *c, const GLfloat *v);
	void APIENTRY (*ReplacementCodeuiColor3fVertex3fSUN)(GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ReplacementCodeuiColor3fVertex3fvSUN)(const GLuint *rc, const GLfloat *c, const GLfloat *v);
	void APIENTRY (*ReplacementCodeuiNormal3fVertex3fSUN)(GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ReplacementCodeuiNormal3fVertex3fvSUN)(const GLuint *rc, const GLfloat *n, const GLfloat *v);
	void APIENTRY (*ReplacementCodeuiColor4fNormal3fVertex3fSUN)(GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ReplacementCodeuiColor4fNormal3fVertex3fvSUN)(const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
	void APIENTRY (*ReplacementCodeuiTexCoord2fVertex3fSUN)(GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ReplacementCodeuiTexCoord2fVertex3fvSUN)(const GLuint *rc, const GLfloat *tc, const GLfloat *v);
	void APIENTRY (*ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)(GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)(const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v);
	void APIENTRY (*ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)(GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)(const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
	void APIENTRY (*BlendFuncSeparateEXT)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
	void APIENTRY (*BlendFuncSeparateINGR)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
	void APIENTRY (*VertexWeightfEXT)(GLfloat weight);
	void APIENTRY (*VertexWeightfvEXT)(const GLfloat *weight);
	void APIENTRY (*VertexWeightPointerEXT)(GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*FlushVertexArrayRangeNV)(void);
	void APIENTRY (*VertexArrayRangeNV)(GLsizei length, const void *pointer);
	void APIENTRY (*CombinerParameterfvNV)(GLenum pname, const GLfloat *params);
	void APIENTRY (*CombinerParameterfNV)(GLenum pname, GLfloat param);
	void APIENTRY (*CombinerParameterivNV)(GLenum pname, const GLint *params);
	void APIENTRY (*CombinerParameteriNV)(GLenum pname, GLint param);
	void APIENTRY (*CombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
	void APIENTRY (*CombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean32 abDotProduct, GLboolean32 cdDotProduct, GLboolean32 muxSum);
	void APIENTRY (*FinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
	void APIENTRY (*GetCombinerInputParameterfvNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params);
	void APIENTRY (*GetCombinerInputParameterivNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params);
	void APIENTRY (*GetCombinerOutputParameterfvNV)(GLenum stage, GLenum portion, GLenum pname, GLfloat *params);
	void APIENTRY (*GetCombinerOutputParameterivNV)(GLenum stage, GLenum portion, GLenum pname, GLint *params);
	void APIENTRY (*GetFinalCombinerInputParameterfvNV)(GLenum variable, GLenum pname, GLfloat *params);
	void APIENTRY (*GetFinalCombinerInputParameterivNV)(GLenum variable, GLenum pname, GLint *params);
	void APIENTRY (*ResizeBuffersMESA)(void);
	void APIENTRY (*WindowPos2dMESA)(GLdouble x, GLdouble y);
	void APIENTRY (*WindowPos2dvMESA)(const GLdouble *v);
	void APIENTRY (*WindowPos2fMESA)(GLfloat x, GLfloat y);
	void APIENTRY (*WindowPos2fvMESA)(const GLfloat *v);
	void APIENTRY (*WindowPos2iMESA)(GLint x, GLint y);
	void APIENTRY (*WindowPos2ivMESA)(const GLint *v);
	void APIENTRY (*WindowPos2sMESA)(GLshort32 x, GLshort32 y);
	void APIENTRY (*WindowPos2svMESA)(const GLshort *v);
	void APIENTRY (*WindowPos3dMESA)(GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*WindowPos3dvMESA)(const GLdouble *v);
	void APIENTRY (*WindowPos3fMESA)(GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*WindowPos3fvMESA)(const GLfloat *v);
	void APIENTRY (*WindowPos3iMESA)(GLint x, GLint y, GLint z);
	void APIENTRY (*WindowPos3ivMESA)(const GLint *v);
	void APIENTRY (*WindowPos3sMESA)(GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*WindowPos3svMESA)(const GLshort *v);
	void APIENTRY (*WindowPos4dMESA)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*WindowPos4dvMESA)(const GLdouble *v);
	void APIENTRY (*WindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*WindowPos4fvMESA)(const GLfloat *v);
	void APIENTRY (*WindowPos4iMESA)(GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*WindowPos4ivMESA)(const GLint *v);
	void APIENTRY (*WindowPos4sMESA)(GLshort32 x, GLshort32 y, GLshort32 z, GLshort32 w);
	void APIENTRY (*WindowPos4svMESA)(const GLshort *v);
	void APIENTRY (*MultiModeDrawArraysIBM)(const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride);
	void APIENTRY (*MultiModeDrawElementsIBM)(const GLenum *mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount, GLint modestride);
	void APIENTRY (*ColorPointerListIBM)(GLint size, GLenum type, GLint stride, const void * *pointer, GLint ptrstride);
	void APIENTRY (*SecondaryColorPointerListIBM)(GLint size, GLenum type, GLint stride, const void * *pointer, GLint ptrstride);
	void APIENTRY (*EdgeFlagPointerListIBM)(GLint stride, const GLboolean * *pointer, GLint ptrstride);
	void APIENTRY (*FogCoordPointerListIBM)(GLenum type, GLint stride, const void * *pointer, GLint ptrstride);
	void APIENTRY (*IndexPointerListIBM)(GLenum type, GLint stride, const void * *pointer, GLint ptrstride);
	void APIENTRY (*NormalPointerListIBM)(GLenum type, GLint stride, const void * *pointer, GLint ptrstride);
	void APIENTRY (*TexCoordPointerListIBM)(GLint size, GLenum type, GLint stride, const void * *pointer, GLint ptrstride);
	void APIENTRY (*VertexPointerListIBM)(GLint size, GLenum type, GLint stride, const void * *pointer, GLint ptrstride);
	void APIENTRY (*TbufferMask3DFX)(GLuint mask);
	void APIENTRY (*SampleMaskEXT)(GLclampf value, GLboolean32 invert);
	void APIENTRY (*SamplePatternEXT)(GLenum pattern);
	void APIENTRY (*TextureColorMaskSGIS)(GLboolean32 red, GLboolean32 green, GLboolean32 blue, GLboolean32 alpha);
	void APIENTRY (*IglooInterfaceSGIX)(GLenum pname, const void *params);
	void APIENTRY (*DeleteFencesNV)(GLsizei n, const GLuint *fences);
	void APIENTRY (*GenFencesNV)(GLsizei n, GLuint *fences);
	GLboolean APIENTRY (*IsFenceNV)(GLuint fence);
	GLboolean APIENTRY (*TestFenceNV)(GLuint fence);
	void APIENTRY (*GetFenceivNV)(GLuint fence, GLenum pname, GLint *params);
	void APIENTRY (*FinishFenceNV)(GLuint fence);
	void APIENTRY (*SetFenceNV)(GLuint fence, GLenum condition);
	void APIENTRY (*MapControlPointsNV)(GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean32 packed, const void *points);
	void APIENTRY (*MapParameterivNV)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*MapParameterfvNV)(GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*GetMapControlPointsNV)(GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean32 packed, void *points);
	void APIENTRY (*GetMapParameterivNV)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetMapParameterfvNV)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetMapAttribParameterivNV)(GLenum target, GLuint index, GLenum pname, GLint *params);
	void APIENTRY (*GetMapAttribParameterfvNV)(GLenum target, GLuint index, GLenum pname, GLfloat *params);
	void APIENTRY (*EvalMapsNV)(GLenum target, GLenum mode);
	void APIENTRY (*CombinerStageParameterfvNV)(GLenum stage, GLenum pname, const GLfloat *params);
	void APIENTRY (*GetCombinerStageParameterfvNV)(GLenum stage, GLenum pname, GLfloat *params);
	GLboolean APIENTRY (*AreProgramsResidentNV)(GLsizei n, const GLuint *programs, GLboolean *residences);
	void APIENTRY (*BindProgramNV)(GLenum target, GLuint id);
	void APIENTRY (*DeleteProgramsNV)(GLsizei n, const GLuint *programs);
	void APIENTRY (*ExecuteProgramNV)(GLenum target, GLuint id, const GLfloat *params);
	void APIENTRY (*GenProgramsNV)(GLsizei n, GLuint *programs);
	void APIENTRY (*GetProgramParameterdvNV)(GLenum target, GLuint index, GLenum pname, GLdouble *params);
	void APIENTRY (*GetProgramParameterfvNV)(GLenum target, GLuint index, GLenum pname, GLfloat *params);
	void APIENTRY (*GetProgramivNV)(GLuint id, GLenum pname, GLint *params);
	void APIENTRY (*GetProgramStringNV)(GLuint id, GLenum pname, GLubyte *program);
	void APIENTRY (*GetTrackMatrixivNV)(GLenum target, GLuint address, GLenum pname, GLint *params);
	void APIENTRY (*GetVertexAttribdvNV)(GLuint index, GLenum pname, GLdouble *params);
	void APIENTRY (*GetVertexAttribfvNV)(GLuint index, GLenum pname, GLfloat *params);
	void APIENTRY (*GetVertexAttribivNV)(GLuint index, GLenum pname, GLint *params);
	void APIENTRY (*GetVertexAttribPointervNV)(GLuint index, GLenum pname, void * *pointer);
	GLboolean APIENTRY (*IsProgramNV)(GLuint id);
	void APIENTRY (*LoadProgramNV)(GLenum target, GLuint id, GLsizei len, const GLubyte *program);
	void APIENTRY (*ProgramParameter4dNV)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*ProgramParameter4dvNV)(GLenum target, GLuint index, const GLdouble *v);
	void APIENTRY (*ProgramParameter4fNV)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*ProgramParameter4fvNV)(GLenum target, GLuint index, const GLfloat *v);
	void APIENTRY (*ProgramParameters4dvNV)(GLenum target, GLuint index, GLsizei count, const GLdouble *v);
	void APIENTRY (*ProgramParameters4fvNV)(GLenum target, GLuint index, GLsizei count, const GLfloat *v);
	void APIENTRY (*RequestResidentProgramsNV)(GLsizei n, const GLuint *programs);
	void APIENTRY (*TrackMatrixNV)(GLenum target, GLuint address, GLenum matrix, GLenum transform);
	void APIENTRY (*VertexAttribPointerNV)(GLuint index, GLint fsize, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*VertexAttrib1dNV)(GLuint index, GLdouble x);
	void APIENTRY (*VertexAttrib1dvNV)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib1fNV)(GLuint index, GLfloat x);
	void APIENTRY (*VertexAttrib1fvNV)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib1sNV)(GLuint index, GLshort32 x);
	void APIENTRY (*VertexAttrib1svNV)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib2dNV)(GLuint index, GLdouble x, GLdouble y);
	void APIENTRY (*VertexAttrib2dvNV)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib2fNV)(GLuint index, GLfloat x, GLfloat y);
	void APIENTRY (*VertexAttrib2fvNV)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib2sNV)(GLuint index, GLshort32 x, GLshort32 y);
	void APIENTRY (*VertexAttrib2svNV)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib3dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*VertexAttrib3dvNV)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib3fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*VertexAttrib3fvNV)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib3sNV)(GLuint index, GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*VertexAttrib3svNV)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib4dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*VertexAttrib4dvNV)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib4fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*VertexAttrib4fvNV)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib4sNV)(GLuint index, GLshort32 x, GLshort32 y, GLshort32 z, GLshort32 w);
	void APIENTRY (*VertexAttrib4svNV)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib4ubNV)(GLuint index, GLubyte32 x, GLubyte32 y, GLubyte32 z, GLubyte32 w);
	void APIENTRY (*VertexAttrib4ubvNV)(GLuint index, const GLubyte *v);
	void APIENTRY (*VertexAttribs1dvNV)(GLuint index, GLsizei count, const GLdouble *v);
	void APIENTRY (*VertexAttribs1fvNV)(GLuint index, GLsizei count, const GLfloat *v);
	void APIENTRY (*VertexAttribs1svNV)(GLuint index, GLsizei count, const GLshort *v);
	void APIENTRY (*VertexAttribs2dvNV)(GLuint index, GLsizei count, const GLdouble *v);
	void APIENTRY (*VertexAttribs2fvNV)(GLuint index, GLsizei count, const GLfloat *v);
	void APIENTRY (*VertexAttribs2svNV)(GLuint index, GLsizei count, const GLshort *v);
	void APIENTRY (*VertexAttribs3dvNV)(GLuint index, GLsizei count, const GLdouble *v);
	void APIENTRY (*VertexAttribs3fvNV)(GLuint index, GLsizei count, const GLfloat *v);
	void APIENTRY (*VertexAttribs3svNV)(GLuint index, GLsizei count, const GLshort *v);
	void APIENTRY (*VertexAttribs4dvNV)(GLuint index, GLsizei count, const GLdouble *v);
	void APIENTRY (*VertexAttribs4fvNV)(GLuint index, GLsizei count, const GLfloat *v);
	void APIENTRY (*VertexAttribs4svNV)(GLuint index, GLsizei count, const GLshort *v);
	void APIENTRY (*VertexAttribs4ubvNV)(GLuint index, GLsizei count, const GLubyte *v);
	void APIENTRY (*TexBumpParameterivATI)(GLenum pname, const GLint *param);
	void APIENTRY (*TexBumpParameterfvATI)(GLenum pname, const GLfloat *param);
	void APIENTRY (*GetTexBumpParameterivATI)(GLenum pname, GLint *param);
	void APIENTRY (*GetTexBumpParameterfvATI)(GLenum pname, GLfloat *param);
	GLuint APIENTRY (*GenFragmentShadersATI)(GLuint range);
	void APIENTRY (*BindFragmentShaderATI)(GLuint id);
	void APIENTRY (*DeleteFragmentShaderATI)(GLuint id);
	void APIENTRY (*BeginFragmentShaderATI)(void);
	void APIENTRY (*EndFragmentShaderATI)(void);
	void APIENTRY (*PassTexCoordATI)(GLuint dst, GLuint coord, GLenum swizzle);
	void APIENTRY (*SampleMapATI)(GLuint dst, GLuint interp, GLenum swizzle);
	void APIENTRY (*ColorFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
	void APIENTRY (*ColorFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
	void APIENTRY (*ColorFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
	void APIENTRY (*AlphaFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
	void APIENTRY (*AlphaFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
	void APIENTRY (*AlphaFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
	void APIENTRY (*SetFragmentShaderConstantATI)(GLuint dst, const GLfloat *value);
	void APIENTRY (*PNTrianglesiATI)(GLenum pname, GLint param);
	void APIENTRY (*PNTrianglesfATI)(GLenum pname, GLfloat param);
	GLuint APIENTRY (*NewObjectBufferATI)(GLsizei size, const void *pointer, GLenum usage);
	GLboolean APIENTRY (*IsObjectBufferATI)(GLuint buffer);
	void APIENTRY (*UpdateObjectBufferATI)(GLuint buffer, GLuint offset, GLsizei size, const void *pointer, GLenum preserve);
	void APIENTRY (*GetObjectBufferfvATI)(GLuint buffer, GLenum pname, GLfloat *params);
	void APIENTRY (*GetObjectBufferivATI)(GLuint buffer, GLenum pname, GLint *params);
	void APIENTRY (*FreeObjectBufferATI)(GLuint buffer);
	void APIENTRY (*ArrayObjectATI)(GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset);
	void APIENTRY (*GetArrayObjectfvATI)(GLenum array, GLenum pname, GLfloat *params);
	void APIENTRY (*GetArrayObjectivATI)(GLenum array, GLenum pname, GLint *params);
	void APIENTRY (*VariantArrayObjectATI)(GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset);
	void APIENTRY (*GetVariantArrayObjectfvATI)(GLuint id, GLenum pname, GLfloat *params);
	void APIENTRY (*GetVariantArrayObjectivATI)(GLuint id, GLenum pname, GLint *params);
	void APIENTRY (*BeginVertexShaderEXT)(void);
	void APIENTRY (*EndVertexShaderEXT)(void);
	void APIENTRY (*BindVertexShaderEXT)(GLuint id);
	GLuint APIENTRY (*GenVertexShadersEXT)(GLuint range);
	void APIENTRY (*DeleteVertexShaderEXT)(GLuint id);
	void APIENTRY (*ShaderOp1EXT)(GLenum op, GLuint res, GLuint arg1);
	void APIENTRY (*ShaderOp2EXT)(GLenum op, GLuint res, GLuint arg1, GLuint arg2);
	void APIENTRY (*ShaderOp3EXT)(GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3);
	void APIENTRY (*SwizzleEXT)(GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW);
	void APIENTRY (*WriteMaskEXT)(GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW);
	void APIENTRY (*InsertComponentEXT)(GLuint res, GLuint src, GLuint num);
	void APIENTRY (*ExtractComponentEXT)(GLuint res, GLuint src, GLuint num);
	GLuint APIENTRY (*GenSymbolsEXT)(GLenum datatype, GLenum storagetype, GLenum range, GLuint components);
	void APIENTRY (*SetInvariantEXT)(GLuint id, GLenum type, const void *addr);
	void APIENTRY (*SetLocalConstantEXT)(GLuint id, GLenum type, const void *addr);
	void APIENTRY (*VariantbvEXT)(GLuint id, const GLbyte *addr);
	void APIENTRY (*VariantsvEXT)(GLuint id, const GLshort *addr);
	void APIENTRY (*VariantivEXT)(GLuint id, const GLint *addr);
	void APIENTRY (*VariantfvEXT)(GLuint id, const GLfloat *addr);
	void APIENTRY (*VariantdvEXT)(GLuint id, const GLdouble *addr);
	void APIENTRY (*VariantubvEXT)(GLuint id, const GLubyte *addr);
	void APIENTRY (*VariantusvEXT)(GLuint id, const GLushort *addr);
	void APIENTRY (*VariantuivEXT)(GLuint id, const GLuint *addr);
	void APIENTRY (*VariantPointerEXT)(GLuint id, GLenum type, GLuint stride, const void *addr);
	void APIENTRY (*EnableVariantClientStateEXT)(GLuint id);
	void APIENTRY (*DisableVariantClientStateEXT)(GLuint id);
	GLuint APIENTRY (*BindLightParameterEXT)(GLenum light, GLenum value);
	GLuint APIENTRY (*BindMaterialParameterEXT)(GLenum face, GLenum value);
	GLuint APIENTRY (*BindTexGenParameterEXT)(GLenum unit, GLenum coord, GLenum value);
	GLuint APIENTRY (*BindTextureUnitParameterEXT)(GLenum unit, GLenum value);
	GLuint APIENTRY (*BindParameterEXT)(GLenum value);
	GLboolean APIENTRY (*IsVariantEnabledEXT)(GLuint id, GLenum cap);
	void APIENTRY (*GetVariantBooleanvEXT)(GLuint id, GLenum value, GLboolean *data);
	void APIENTRY (*GetVariantIntegervEXT)(GLuint id, GLenum value, GLint *data);
	void APIENTRY (*GetVariantFloatvEXT)(GLuint id, GLenum value, GLfloat *data);
	void APIENTRY (*GetVariantPointervEXT)(GLuint id, GLenum value, void * *data);
	void APIENTRY (*GetInvariantBooleanvEXT)(GLuint id, GLenum value, GLboolean *data);
	void APIENTRY (*GetInvariantIntegervEXT)(GLuint id, GLenum value, GLint *data);
	void APIENTRY (*GetInvariantFloatvEXT)(GLuint id, GLenum value, GLfloat *data);
	void APIENTRY (*GetLocalConstantBooleanvEXT)(GLuint id, GLenum value, GLboolean *data);
	void APIENTRY (*GetLocalConstantIntegervEXT)(GLuint id, GLenum value, GLint *data);
	void APIENTRY (*GetLocalConstantFloatvEXT)(GLuint id, GLenum value, GLfloat *data);
	void APIENTRY (*VertexStream1sATI)(GLenum stream, GLshort32 x);
	void APIENTRY (*VertexStream1svATI)(GLenum stream, const GLshort *coords);
	void APIENTRY (*VertexStream1iATI)(GLenum stream, GLint x);
	void APIENTRY (*VertexStream1ivATI)(GLenum stream, const GLint *coords);
	void APIENTRY (*VertexStream1fATI)(GLenum stream, GLfloat x);
	void APIENTRY (*VertexStream1fvATI)(GLenum stream, const GLfloat *coords);
	void APIENTRY (*VertexStream1dATI)(GLenum stream, GLdouble x);
	void APIENTRY (*VertexStream1dvATI)(GLenum stream, const GLdouble *coords);
	void APIENTRY (*VertexStream2sATI)(GLenum stream, GLshort32 x, GLshort32 y);
	void APIENTRY (*VertexStream2svATI)(GLenum stream, const GLshort *coords);
	void APIENTRY (*VertexStream2iATI)(GLenum stream, GLint x, GLint y);
	void APIENTRY (*VertexStream2ivATI)(GLenum stream, const GLint *coords);
	void APIENTRY (*VertexStream2fATI)(GLenum stream, GLfloat x, GLfloat y);
	void APIENTRY (*VertexStream2fvATI)(GLenum stream, const GLfloat *coords);
	void APIENTRY (*VertexStream2dATI)(GLenum stream, GLdouble x, GLdouble y);
	void APIENTRY (*VertexStream2dvATI)(GLenum stream, const GLdouble *coords);
	void APIENTRY (*VertexStream3sATI)(GLenum stream, GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*VertexStream3svATI)(GLenum stream, const GLshort *coords);
	void APIENTRY (*VertexStream3iATI)(GLenum stream, GLint x, GLint y, GLint z);
	void APIENTRY (*VertexStream3ivATI)(GLenum stream, const GLint *coords);
	void APIENTRY (*VertexStream3fATI)(GLenum stream, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*VertexStream3fvATI)(GLenum stream, const GLfloat *coords);
	void APIENTRY (*VertexStream3dATI)(GLenum stream, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*VertexStream3dvATI)(GLenum stream, const GLdouble *coords);
	void APIENTRY (*VertexStream4sATI)(GLenum stream, GLshort32 x, GLshort32 y, GLshort32 z, GLshort32 w);
	void APIENTRY (*VertexStream4svATI)(GLenum stream, const GLshort *coords);
	void APIENTRY (*VertexStream4iATI)(GLenum stream, GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*VertexStream4ivATI)(GLenum stream, const GLint *coords);
	void APIENTRY (*VertexStream4fATI)(GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*VertexStream4fvATI)(GLenum stream, const GLfloat *coords);
	void APIENTRY (*VertexStream4dATI)(GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*VertexStream4dvATI)(GLenum stream, const GLdouble *coords);
	void APIENTRY (*NormalStream3bATI)(GLenum stream, GLbyte32 nx, GLbyte32 ny, GLbyte32 nz);
	void APIENTRY (*NormalStream3bvATI)(GLenum stream, const GLbyte *coords);
	void APIENTRY (*NormalStream3sATI)(GLenum stream, GLshort32 nx, GLshort32 ny, GLshort32 nz);
	void APIENTRY (*NormalStream3svATI)(GLenum stream, const GLshort *coords);
	void APIENTRY (*NormalStream3iATI)(GLenum stream, GLint nx, GLint ny, GLint nz);
	void APIENTRY (*NormalStream3ivATI)(GLenum stream, const GLint *coords);
	void APIENTRY (*NormalStream3fATI)(GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz);
	void APIENTRY (*NormalStream3fvATI)(GLenum stream, const GLfloat *coords);
	void APIENTRY (*NormalStream3dATI)(GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz);
	void APIENTRY (*NormalStream3dvATI)(GLenum stream, const GLdouble *coords);
	void APIENTRY (*ClientActiveVertexStreamATI)(GLenum stream);
	void APIENTRY (*VertexBlendEnviATI)(GLenum pname, GLint param);
	void APIENTRY (*VertexBlendEnvfATI)(GLenum pname, GLfloat param);
	void APIENTRY (*ElementPointerATI)(GLenum type, const void *pointer);
	void APIENTRY (*DrawElementArrayATI)(GLenum mode, GLsizei count);
	void APIENTRY (*DrawRangeElementArrayATI)(GLenum mode, GLuint start, GLuint end, GLsizei count);
	void APIENTRY (*DrawMeshArraysSUN)(GLenum mode, GLint first, GLsizei count, GLsizei width);
	void APIENTRY (*GenOcclusionQueriesNV)(GLsizei n, GLuint *ids);
	void APIENTRY (*DeleteOcclusionQueriesNV)(GLsizei n, const GLuint *ids);
	GLboolean APIENTRY (*IsOcclusionQueryNV)(GLuint id);
	void APIENTRY (*BeginOcclusionQueryNV)(GLuint id);
	void APIENTRY (*EndOcclusionQueryNV)(void);
	void APIENTRY (*GetOcclusionQueryivNV)(GLuint id, GLenum pname, GLint *params);
	void APIENTRY (*GetOcclusionQueryuivNV)(GLuint id, GLenum pname, GLuint *params);
	void APIENTRY (*PointParameteriNV)(GLenum pname, GLint param);
	void APIENTRY (*PointParameterivNV)(GLenum pname, const GLint *params);
	void APIENTRY (*ActiveStencilFaceEXT)(GLenum face);
	void APIENTRY (*ElementPointerAPPLE)(GLenum type, const void *pointer);
	void APIENTRY (*DrawElementArrayAPPLE)(GLenum mode, GLint first, GLsizei count);
	void APIENTRY (*DrawRangeElementArrayAPPLE)(GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count);
	void APIENTRY (*MultiDrawElementArrayAPPLE)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
	void APIENTRY (*MultiDrawRangeElementArrayAPPLE)(GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount);
	void APIENTRY (*GenFencesAPPLE)(GLsizei n, GLuint *fences);
	void APIENTRY (*DeleteFencesAPPLE)(GLsizei n, const GLuint *fences);
	void APIENTRY (*SetFenceAPPLE)(GLuint fence);
	GLboolean APIENTRY (*IsFenceAPPLE)(GLuint fence);
	GLboolean APIENTRY (*TestFenceAPPLE)(GLuint fence);
	void APIENTRY (*FinishFenceAPPLE)(GLuint fence);
	GLboolean APIENTRY (*TestObjectAPPLE)(GLenum object, GLuint name);
	void APIENTRY (*FinishObjectAPPLE)(GLenum object, GLuint name);
	void APIENTRY (*BindVertexArrayAPPLE)(GLuint array);
	void APIENTRY (*DeleteVertexArraysAPPLE)(GLsizei n, const GLuint *arrays);
	void APIENTRY (*GenVertexArraysAPPLE)(GLsizei n, GLuint *arrays);
	GLboolean APIENTRY (*IsVertexArrayAPPLE)(GLuint array);
	void APIENTRY (*VertexArrayRangeAPPLE)(GLsizei length, void *pointer);
	void APIENTRY (*FlushVertexArrayRangeAPPLE)(GLsizei length, void *pointer);
	void APIENTRY (*VertexArrayParameteriAPPLE)(GLenum pname, GLint param);
	void APIENTRY (*DrawBuffersATI)(GLsizei n, const GLenum *bufs);
	void APIENTRY (*ProgramNamedParameter4fNV)(GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*ProgramNamedParameter4dNV)(GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*ProgramNamedParameter4fvNV)(GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v);
	void APIENTRY (*ProgramNamedParameter4dvNV)(GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v);
	void APIENTRY (*GetProgramNamedParameterfvNV)(GLuint id, GLsizei len, const GLubyte *name, GLfloat *params);
	void APIENTRY (*GetProgramNamedParameterdvNV)(GLuint id, GLsizei len, const GLubyte *name, GLdouble *params);
	void APIENTRY (*Vertex2hNV)(GLhalfNV32 x, GLhalfNV32 y);
	void APIENTRY (*Vertex2hvNV)(const GLhalfNV *v);
	void APIENTRY (*Vertex3hNV)(GLhalfNV32 x, GLhalfNV32 y, GLhalfNV32 z);
	void APIENTRY (*Vertex3hvNV)(const GLhalfNV *v);
	void APIENTRY (*Vertex4hNV)(GLhalfNV32 x, GLhalfNV32 y, GLhalfNV32 z, GLhalfNV32 w);
	void APIENTRY (*Vertex4hvNV)(const GLhalfNV *v);
	void APIENTRY (*Normal3hNV)(GLhalfNV32 nx, GLhalfNV32 ny, GLhalfNV32 nz);
	void APIENTRY (*Normal3hvNV)(const GLhalfNV *v);
	void APIENTRY (*Color3hNV)(GLhalfNV32 red, GLhalfNV32 green, GLhalfNV32 blue);
	void APIENTRY (*Color3hvNV)(const GLhalfNV *v);
	void APIENTRY (*Color4hNV)(GLhalfNV32 red, GLhalfNV32 green, GLhalfNV32 blue, GLhalfNV32 alpha);
	void APIENTRY (*Color4hvNV)(const GLhalfNV *v);
	void APIENTRY (*TexCoord1hNV)(GLhalfNV32 s);
	void APIENTRY (*TexCoord1hvNV)(const GLhalfNV *v);
	void APIENTRY (*TexCoord2hNV)(GLhalfNV32 s, GLhalfNV32 t);
	void APIENTRY (*TexCoord2hvNV)(const GLhalfNV *v);
	void APIENTRY (*TexCoord3hNV)(GLhalfNV32 s, GLhalfNV32 t, GLhalfNV32 r);
	void APIENTRY (*TexCoord3hvNV)(const GLhalfNV *v);
	void APIENTRY (*TexCoord4hNV)(GLhalfNV32 s, GLhalfNV32 t, GLhalfNV32 r, GLhalfNV32 q);
	void APIENTRY (*TexCoord4hvNV)(const GLhalfNV *v);
	void APIENTRY (*MultiTexCoord1hNV)(GLenum target, GLhalfNV32 s);
	void APIENTRY (*MultiTexCoord1hvNV)(GLenum target, const GLhalfNV *v);
	void APIENTRY (*MultiTexCoord2hNV)(GLenum target, GLhalfNV32 s, GLhalfNV32 t);
	void APIENTRY (*MultiTexCoord2hvNV)(GLenum target, const GLhalfNV *v);
	void APIENTRY (*MultiTexCoord3hNV)(GLenum target, GLhalfNV32 s, GLhalfNV32 t, GLhalfNV32 r);
	void APIENTRY (*MultiTexCoord3hvNV)(GLenum target, const GLhalfNV *v);
	void APIENTRY (*MultiTexCoord4hNV)(GLenum target, GLhalfNV32 s, GLhalfNV32 t, GLhalfNV32 r, GLhalfNV32 q);
	void APIENTRY (*MultiTexCoord4hvNV)(GLenum target, const GLhalfNV *v);
	void APIENTRY (*FogCoordhNV)(GLhalfNV32 fog);
	void APIENTRY (*FogCoordhvNV)(const GLhalfNV *fog);
	void APIENTRY (*SecondaryColor3hNV)(GLhalfNV32 red, GLhalfNV32 green, GLhalfNV32 blue);
	void APIENTRY (*SecondaryColor3hvNV)(const GLhalfNV *v);
	void APIENTRY (*VertexWeighthNV)(GLhalfNV32 weight);
	void APIENTRY (*VertexWeighthvNV)(const GLhalfNV *weight);
	void APIENTRY (*VertexAttrib1hNV)(GLuint index, GLhalfNV32 x);
	void APIENTRY (*VertexAttrib1hvNV)(GLuint index, const GLhalfNV *v);
	void APIENTRY (*VertexAttrib2hNV)(GLuint index, GLhalfNV32 x, GLhalfNV32 y);
	void APIENTRY (*VertexAttrib2hvNV)(GLuint index, const GLhalfNV *v);
	void APIENTRY (*VertexAttrib3hNV)(GLuint index, GLhalfNV32 x, GLhalfNV32 y, GLhalfNV32 z);
	void APIENTRY (*VertexAttrib3hvNV)(GLuint index, const GLhalfNV *v);
	void APIENTRY (*VertexAttrib4hNV)(GLuint index, GLhalfNV32 x, GLhalfNV32 y, GLhalfNV32 z, GLhalfNV32 w);
	void APIENTRY (*VertexAttrib4hvNV)(GLuint index, const GLhalfNV *v);
	void APIENTRY (*VertexAttribs1hvNV)(GLuint index, GLsizei n, const GLhalfNV *v);
	void APIENTRY (*VertexAttribs2hvNV)(GLuint index, GLsizei n, const GLhalfNV *v);
	void APIENTRY (*VertexAttribs3hvNV)(GLuint index, GLsizei n, const GLhalfNV *v);
	void APIENTRY (*VertexAttribs4hvNV)(GLuint index, GLsizei n, const GLhalfNV *v);
	void APIENTRY (*PixelDataRangeNV)(GLenum target, GLsizei length, const void *pointer);
	void APIENTRY (*FlushPixelDataRangeNV)(GLenum target);
	void APIENTRY (*PrimitiveRestartNV)(void);
	void APIENTRY (*PrimitiveRestartIndexNV)(GLuint index);
	void * APIENTRY (*MapObjectBufferATI)(GLuint buffer);
	void APIENTRY (*UnmapObjectBufferATI)(GLuint buffer);
	void APIENTRY (*StencilOpSeparateATI)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
	void APIENTRY (*StencilFuncSeparateATI)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
	void APIENTRY (*VertexAttribArrayObjectATI)(GLuint index, GLint size, GLenum type, GLboolean32 normalized, GLsizei stride, GLuint buffer, GLuint offset);
	void APIENTRY (*GetVertexAttribArrayObjectfvATI)(GLuint index, GLenum pname, GLfloat *params);
	void APIENTRY (*GetVertexAttribArrayObjectivATI)(GLuint index, GLenum pname, GLint *params);
	void APIENTRY (*DepthBoundsEXT)(GLclampd zmin, GLclampd zmax);
	void APIENTRY (*BlendEquationSeparateEXT)(GLenum modeRGB, GLenum modeAlpha);
	void APIENTRY (*OSMesaColorClamp)(GLboolean32 enable);
	void APIENTRY (*OSMesaPostprocess)(OSMesaContext osmesa, const char *filter, GLuint enable_value);
	void * APIENTRY (*OSMesaCreateLDG)(GLenum format, GLenum type, GLint width, GLint height);
	void APIENTRY (*OSMesaDestroyLDG)(void);
	GLsizei APIENTRY (*max_width)(void);
	GLsizei APIENTRY (*max_height)(void);
	void APIENTRY (*information)(void);
	void APIENTRY (*exception_error)(void (CALLBACK *exception)(GLenum param) );
	void APIENTRY (*gluLookAtf)(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ);
	void APIENTRY (*Frustumf)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
	void APIENTRY (*Orthof)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val);
	void APIENTRY (*swapbuffer)(void *buffer);
	void APIENTRY (*gluLookAt)(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ);
	void APIENTRY (*AccumxOES)(GLenum op, GLfixed value);
	void APIENTRY (*ActiveProgramEXT)(GLuint program);
	void APIENTRY (*ActiveShaderProgram)(GLuint pipeline, GLuint program);
	void APIENTRY (*ActiveVaryingNV)(GLuint program, const GLchar *name);
	void APIENTRY (*AddSwapHintRectWIN)(GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*AlphaFuncxOES)(GLenum func, GLfixed ref);
	void APIENTRY (*AttachShader)(GLuint program, GLuint shader);
	void APIENTRY (*BeginConditionalRender)(GLuint id, GLenum mode);
	void APIENTRY (*BeginConditionalRenderNV)(GLuint id, GLenum mode);
	void APIENTRY (*BeginConditionalRenderNVX)(GLuint id);
	void APIENTRY (*BeginPerfMonitorAMD)(GLuint monitor);
	void APIENTRY (*BeginPerfQueryINTEL)(GLuint queryHandle);
	void APIENTRY (*BeginQueryIndexed)(GLenum target, GLuint index, GLuint id);
	void APIENTRY (*BeginTransformFeedback)(GLenum primitiveMode);
	void APIENTRY (*BeginTransformFeedbackEXT)(GLenum primitiveMode);
	void APIENTRY (*BeginTransformFeedbackNV)(GLenum primitiveMode);
	void APIENTRY (*BeginVideoCaptureNV)(GLuint video_capture_slot);
	void APIENTRY (*BindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
	void APIENTRY (*BindBufferBase)(GLenum target, GLuint index, GLuint buffer);
	void APIENTRY (*BindBufferBaseEXT)(GLenum target, GLuint index, GLuint buffer);
	void APIENTRY (*BindBufferBaseNV)(GLenum target, GLuint index, GLuint buffer);
	void APIENTRY (*BindBufferOffsetEXT)(GLenum target, GLuint index, GLuint buffer, GLintptr offset);
	void APIENTRY (*BindBufferOffsetNV)(GLenum target, GLuint index, GLuint buffer, GLintptr offset);
	void APIENTRY (*BindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	void APIENTRY (*BindBufferRangeEXT)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	void APIENTRY (*BindBufferRangeNV)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	void APIENTRY (*BindBuffersBase)(GLenum target, GLuint first, GLsizei count, const GLuint *buffers);
	void APIENTRY (*BindBuffersRange)(GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes);
	void APIENTRY (*BindFragDataLocation)(GLuint program, GLuint color, const GLchar *name);
	void APIENTRY (*BindFragDataLocationEXT)(GLuint program, GLuint color, const GLchar *name);
	void APIENTRY (*BindFragDataLocationIndexed)(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name);
	void APIENTRY (*BindFramebuffer)(GLenum target, GLuint framebuffer);
	void APIENTRY (*BindFramebufferEXT)(GLenum target, GLuint framebuffer);
	void APIENTRY (*BindImageTexture)(GLuint unit, GLuint texture, GLint level, GLboolean32 layered, GLint layer, GLenum access, GLenum format);
	void APIENTRY (*BindImageTextureEXT)(GLuint index, GLuint texture, GLint level, GLboolean32 layered, GLint layer, GLenum access, GLint format);
	void APIENTRY (*BindImageTextures)(GLuint first, GLsizei count, const GLuint *textures);
	void APIENTRY (*BindMultiTextureEXT)(GLenum texunit, GLenum target, GLuint texture);
	void APIENTRY (*BindProgramPipeline)(GLuint pipeline);
	void APIENTRY (*BindRenderbuffer)(GLenum target, GLuint renderbuffer);
	void APIENTRY (*BindRenderbufferEXT)(GLenum target, GLuint renderbuffer);
	void APIENTRY (*BindSampler)(GLuint unit, GLuint sampler);
	void APIENTRY (*BindSamplers)(GLuint first, GLsizei count, const GLuint *samplers);
	void APIENTRY (*BindTextures)(GLuint first, GLsizei count, const GLuint *textures);
	void APIENTRY (*BindTransformFeedback)(GLenum target, GLuint id);
	void APIENTRY (*BindTransformFeedbackNV)(GLenum target, GLuint id);
	void APIENTRY (*BindVertexArray)(GLuint array);
	void APIENTRY (*BindVertexBuffer)(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
	void APIENTRY (*BindVertexBuffers)(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides);
	void APIENTRY (*BindVideoCaptureStreamBufferNV)(GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLintptrARB offset);
	void APIENTRY (*BindVideoCaptureStreamTextureNV)(GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture);
	void APIENTRY (*BitmapxOES)(GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig, GLfixed xmove, GLfixed ymove, const GLubyte *bitmap);
	void APIENTRY (*BlendBarrierNV)(void);
	void APIENTRY (*BlendColorxOES)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
	void APIENTRY (*BlendEquationIndexedAMD)(GLuint buf, GLenum mode);
	void APIENTRY (*BlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
	void APIENTRY (*BlendEquationSeparateIndexedAMD)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
	void APIENTRY (*BlendEquationSeparatei)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
	void APIENTRY (*BlendEquationSeparateiARB)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
	void APIENTRY (*BlendEquationi)(GLuint buf, GLenum mode);
	void APIENTRY (*BlendEquationiARB)(GLuint buf, GLenum mode);
	void APIENTRY (*BlendFuncIndexedAMD)(GLuint buf, GLenum src, GLenum dst);
	void APIENTRY (*BlendFuncSeparateIndexedAMD)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	void APIENTRY (*BlendFuncSeparatei)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	void APIENTRY (*BlendFuncSeparateiARB)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	void APIENTRY (*BlendFunci)(GLuint buf, GLenum src, GLenum dst);
	void APIENTRY (*BlendFunciARB)(GLuint buf, GLenum src, GLenum dst);
	void APIENTRY (*BlendParameteriNV)(GLenum pname, GLint value);
	void APIENTRY (*BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	void APIENTRY (*BlitFramebufferEXT)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	void APIENTRY (*BufferAddressRangeNV)(GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length);
	void APIENTRY (*BufferParameteriAPPLE)(GLenum target, GLenum pname, GLint param);
	void APIENTRY (*BufferStorage)(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
	GLenum APIENTRY (*CheckFramebufferStatus)(GLenum target);
	GLenum APIENTRY (*CheckFramebufferStatusEXT)(GLenum target);
	GLenum APIENTRY (*CheckNamedFramebufferStatusEXT)(GLuint framebuffer, GLenum target);
	void APIENTRY (*ClampColor)(GLenum target, GLenum clamp);
	void APIENTRY (*ClampColorARB)(GLenum target, GLenum clamp);
	void APIENTRY (*ClearAccumxOES)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
	void APIENTRY (*ClearBufferData)(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data);
	void APIENTRY (*ClearBufferSubData)(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
	void APIENTRY (*ClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
	void APIENTRY (*ClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat *value);
	void APIENTRY (*ClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint *value);
	void APIENTRY (*ClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint *value);
	void APIENTRY (*ClearColorIiEXT)(GLint red, GLint green, GLint blue, GLint alpha);
	void APIENTRY (*ClearColorIuiEXT)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
	void APIENTRY (*ClearColorxOES)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
	void APIENTRY (*ClearDebugLogMESA)(GLhandleARB obj, GLenum logType, GLenum shaderType);
	void APIENTRY (*ClearDepthdNV)(GLdouble depth);
	void APIENTRY (*ClearDepthf)(GLfloat d);
	void APIENTRY (*ClearDepthfOES)(GLclampf depth);
	void APIENTRY (*ClearDepthxOES)(GLfixed depth);
	void APIENTRY (*ClearNamedBufferDataEXT)(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data);
	void APIENTRY (*ClearNamedBufferSubDataEXT)(GLuint buffer, GLenum internalformat, GLsizeiptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
	void APIENTRY (*ClearTexImage)(GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
	void APIENTRY (*ClearTexSubImage)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data);
	void APIENTRY (*ClientAttribDefaultEXT)(GLbitfield mask);
	GLenum APIENTRY (*ClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
	void APIENTRY (*ClipPlanefOES)(GLenum plane, const GLfloat *equation);
	void APIENTRY (*ClipPlanexOES)(GLenum plane, const GLfixed *equation);
	void APIENTRY (*Color3xOES)(GLfixed red, GLfixed green, GLfixed blue);
	void APIENTRY (*Color3xvOES)(const GLfixed *components);
	void APIENTRY (*Color4xOES)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
	void APIENTRY (*Color4xvOES)(const GLfixed *components);
	void APIENTRY (*ColorFormatNV)(GLint size, GLenum type, GLsizei stride);
	void APIENTRY (*ColorMaskIndexedEXT)(GLuint index, GLboolean32 r, GLboolean32 g, GLboolean32 b, GLboolean32 a);
	void APIENTRY (*ColorMaski)(GLuint index, GLboolean32 r, GLboolean32 g, GLboolean32 b, GLboolean32 a);
	void APIENTRY (*ColorP3ui)(GLenum type, GLuint color);
	void APIENTRY (*ColorP3uiv)(GLenum type, const GLuint *color);
	void APIENTRY (*ColorP4ui)(GLenum type, GLuint color);
	void APIENTRY (*ColorP4uiv)(GLenum type, const GLuint *color);
	void APIENTRY (*CompileShader)(GLuint shader);
	void APIENTRY (*CompileShaderIncludeARB)(GLuint shader, GLsizei count, const GLchar *const *path, const GLint *length);
	void APIENTRY (*CompressedMultiTexImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedMultiTexImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedMultiTexImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedMultiTexSubImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedMultiTexSubImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedMultiTexSubImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedTextureImage1DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedTextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedTextureImage3DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedTextureSubImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedTextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits);
	void APIENTRY (*CompressedTextureSubImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits);
	void APIENTRY (*ConvolutionParameterxOES)(GLenum target, GLenum pname, GLfixed param);
	void APIENTRY (*ConvolutionParameterxvOES)(GLenum target, GLenum pname, const GLfixed *params);
	void APIENTRY (*CopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
	void APIENTRY (*CopyImageSubData)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);
	void APIENTRY (*CopyImageSubDataNV)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth);
	void APIENTRY (*CopyMultiTexImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
	void APIENTRY (*CopyMultiTexImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void APIENTRY (*CopyMultiTexSubImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	void APIENTRY (*CopyMultiTexSubImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*CopyMultiTexSubImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*CopyPathNV)(GLuint resultPath, GLuint srcPath);
	void APIENTRY (*CopyTextureImage1DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
	void APIENTRY (*CopyTextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void APIENTRY (*CopyTextureSubImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	void APIENTRY (*CopyTextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*CopyTextureSubImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*CoverFillPathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
	void APIENTRY (*CoverFillPathNV)(GLuint path, GLenum coverMode);
	void APIENTRY (*CoverStrokePathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
	void APIENTRY (*CoverStrokePathNV)(GLuint path, GLenum coverMode);
	GLhandleARB APIENTRY (*CreateDebugObjectMESA)(void);
	void APIENTRY (*CreatePerfQueryINTEL)(GLuint queryId, GLuint *queryHandle);
	GLuint APIENTRY (*CreateProgram)(void);
	GLuint APIENTRY (*CreateShader)(GLenum type);
	GLuint APIENTRY (*CreateShaderProgramEXT)(GLenum type, const GLchar *string);
	GLuint APIENTRY (*CreateShaderProgramv)(GLenum type, GLsizei count, const GLchar *const *strings);
	GLsync APIENTRY (*CreateSyncFromCLeventARB)(struct _cl_context *context, struct _cl_event *event, GLbitfield flags);
	void APIENTRY (*DebugMessageCallback)(GLDEBUGPROC callback, const void *userParam);
	void APIENTRY (*DebugMessageCallbackAMD)(GLDEBUGPROCAMD callback, void *userParam);
	void APIENTRY (*DebugMessageCallbackARB)(GLDEBUGPROCARB callback, const void *userParam);
	void APIENTRY (*DebugMessageControl)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean32 enabled);
	void APIENTRY (*DebugMessageControlARB)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean32 enabled);
	void APIENTRY (*DebugMessageEnableAMD)(GLenum category, GLenum severity, GLsizei count, const GLuint *ids, GLboolean32 enabled);
	void APIENTRY (*DebugMessageInsert)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf);
	void APIENTRY (*DebugMessageInsertAMD)(GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar *buf);
	void APIENTRY (*DebugMessageInsertARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf);
	void APIENTRY (*DeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
	void APIENTRY (*DeleteFramebuffersEXT)(GLsizei n, const GLuint *framebuffers);
	void APIENTRY (*DeleteNamedStringARB)(GLint namelen, const GLchar *name);
	void APIENTRY (*DeleteNamesAMD)(GLenum identifier, GLuint num, const GLuint *names);
	void APIENTRY (*DeleteObjectBufferATI)(GLuint buffer);
	void APIENTRY (*DeletePathsNV)(GLuint path, GLsizei range);
	void APIENTRY (*DeletePerfMonitorsAMD)(GLsizei n, GLuint *monitors);
	void APIENTRY (*DeletePerfQueryINTEL)(GLuint queryHandle);
	void APIENTRY (*DeleteProgram)(GLuint program);
	void APIENTRY (*DeleteProgramPipelines)(GLsizei n, const GLuint *pipelines);
	void APIENTRY (*DeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
	void APIENTRY (*DeleteRenderbuffersEXT)(GLsizei n, const GLuint *renderbuffers);
	void APIENTRY (*DeleteSamplers)(GLsizei count, const GLuint *samplers);
	void APIENTRY (*DeleteShader)(GLuint shader);
	void APIENTRY (*DeleteSync)(GLsync sync);
	void APIENTRY (*DeleteTransformFeedbacks)(GLsizei n, const GLuint *ids);
	void APIENTRY (*DeleteTransformFeedbacksNV)(GLsizei n, const GLuint *ids);
	void APIENTRY (*DeleteVertexArrays)(GLsizei n, const GLuint *arrays);
	void APIENTRY (*DepthBoundsdNV)(GLdouble zmin, GLdouble zmax);
	void APIENTRY (*DepthRangeArrayv)(GLuint first, GLsizei count, const GLdouble *v);
	void APIENTRY (*DepthRangeIndexed)(GLuint index, GLdouble n, GLdouble f);
	void APIENTRY (*DepthRangedNV)(GLdouble zNear, GLdouble zFar);
	void APIENTRY (*DepthRangef)(GLfloat n, GLfloat f);
	void APIENTRY (*DepthRangefOES)(GLclampf n, GLclampf f);
	void APIENTRY (*DepthRangexOES)(GLfixed n, GLfixed f);
	void APIENTRY (*DetachShader)(GLuint program, GLuint shader);
	void APIENTRY (*DisableClientStateIndexedEXT)(GLenum array, GLuint index);
	void APIENTRY (*DisableClientStateiEXT)(GLenum array, GLuint index);
	void APIENTRY (*DisableIndexedEXT)(GLenum target, GLuint index);
	void APIENTRY (*DisableVertexArrayAttribEXT)(GLuint vaobj, GLuint index);
	void APIENTRY (*DisableVertexArrayEXT)(GLuint vaobj, GLenum array);
	void APIENTRY (*DisableVertexAttribAPPLE)(GLuint index, GLenum pname);
	void APIENTRY (*DisableVertexAttribArray)(GLuint index);
	void APIENTRY (*Disablei)(GLenum target, GLuint index);
	void APIENTRY (*DispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
	void APIENTRY (*DispatchComputeGroupSizeARB)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z, GLuint group_size_x, GLuint group_size_y, GLuint group_size_z);
	void APIENTRY (*DispatchComputeIndirect)(GLintptr indirect);
	void APIENTRY (*DrawArraysIndirect)(GLenum mode, const void *indirect);
	void APIENTRY (*DrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
	void APIENTRY (*DrawArraysInstancedARB)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
	void APIENTRY (*DrawArraysInstancedBaseInstance)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
	void APIENTRY (*DrawArraysInstancedEXT)(GLenum mode, GLint start, GLsizei count, GLsizei primcount);
	void APIENTRY (*DrawBuffers)(GLsizei n, const GLenum *bufs);
	void APIENTRY (*DrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
	void APIENTRY (*DrawElementsIndirect)(GLenum mode, GLenum type, const void *indirect);
	void APIENTRY (*DrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
	void APIENTRY (*DrawElementsInstancedARB)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
	void APIENTRY (*DrawElementsInstancedBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
	void APIENTRY (*DrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
	void APIENTRY (*DrawElementsInstancedBaseVertexBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);
	void APIENTRY (*DrawElementsInstancedEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
	void APIENTRY (*DrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex);
	void APIENTRY (*DrawTextureNV)(GLuint texture, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);
	void APIENTRY (*DrawTransformFeedback)(GLenum mode, GLuint id);
	void APIENTRY (*DrawTransformFeedbackInstanced)(GLenum mode, GLuint id, GLsizei instancecount);
	void APIENTRY (*DrawTransformFeedbackNV)(GLenum mode, GLuint id);
	void APIENTRY (*DrawTransformFeedbackStream)(GLenum mode, GLuint id, GLuint stream);
	void APIENTRY (*DrawTransformFeedbackStreamInstanced)(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount);
	void APIENTRY (*EdgeFlagFormatNV)(GLsizei stride);
	void APIENTRY (*EnableClientStateIndexedEXT)(GLenum array, GLuint index);
	void APIENTRY (*EnableClientStateiEXT)(GLenum array, GLuint index);
	void APIENTRY (*EnableIndexedEXT)(GLenum target, GLuint index);
	void APIENTRY (*EnableVertexArrayAttribEXT)(GLuint vaobj, GLuint index);
	void APIENTRY (*EnableVertexArrayEXT)(GLuint vaobj, GLenum array);
	void APIENTRY (*EnableVertexAttribAPPLE)(GLuint index, GLenum pname);
	void APIENTRY (*EnableVertexAttribArray)(GLuint index);
	void APIENTRY (*Enablei)(GLenum target, GLuint index);
	void APIENTRY (*EndConditionalRender)(void);
	void APIENTRY (*EndConditionalRenderNV)(void);
	void APIENTRY (*EndConditionalRenderNVX)(void);
	void APIENTRY (*EndPerfMonitorAMD)(GLuint monitor);
	void APIENTRY (*EndPerfQueryINTEL)(GLuint queryHandle);
	void APIENTRY (*EndQueryIndexed)(GLenum target, GLuint index);
	void APIENTRY (*EndTransformFeedback)(void);
	void APIENTRY (*EndTransformFeedbackEXT)(void);
	void APIENTRY (*EndTransformFeedbackNV)(void);
	void APIENTRY (*EndVideoCaptureNV)(GLuint video_capture_slot);
	void APIENTRY (*EvalCoord1xOES)(GLfixed u);
	void APIENTRY (*EvalCoord1xvOES)(const GLfixed *coords);
	void APIENTRY (*EvalCoord2xOES)(GLfixed u, GLfixed v);
	void APIENTRY (*EvalCoord2xvOES)(const GLfixed *coords);
	void APIENTRY (*FeedbackBufferxOES)(GLsizei n, GLenum type, const GLfixed *buffer);
	GLsync APIENTRY (*FenceSync)(GLenum condition, GLbitfield flags);
	void APIENTRY (*FinishRenderAPPLE)(void);
	void APIENTRY (*FlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
	void APIENTRY (*FlushMappedBufferRangeAPPLE)(GLenum target, GLintptr offset, GLsizeiptr size);
	void APIENTRY (*FlushMappedNamedBufferRangeEXT)(GLuint buffer, GLintptr offset, GLsizeiptr length);
	void APIENTRY (*FlushRenderAPPLE)(void);
	void APIENTRY (*FlushStaticDataIBM)(GLenum target);
	void APIENTRY (*FogCoordFormatNV)(GLenum type, GLsizei stride);
	void APIENTRY (*FogxOES)(GLenum pname, GLfixed param);
	void APIENTRY (*FogxvOES)(GLenum pname, const GLfixed *param);
	void APIENTRY (*FrameTerminatorGREMEDY)(void);
	void APIENTRY (*FramebufferDrawBufferEXT)(GLuint framebuffer, GLenum mode);
	void APIENTRY (*FramebufferDrawBuffersEXT)(GLuint framebuffer, GLsizei n, const GLenum *bufs);
	void APIENTRY (*FramebufferParameteri)(GLenum target, GLenum pname, GLint param);
	void APIENTRY (*FramebufferReadBufferEXT)(GLuint framebuffer, GLenum mode);
	void APIENTRY (*FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	void APIENTRY (*FramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	void APIENTRY (*FramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
	void APIENTRY (*FramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void APIENTRY (*FramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void APIENTRY (*FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void APIENTRY (*FramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void APIENTRY (*FramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
	void APIENTRY (*FramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
	void APIENTRY (*FramebufferTextureARB)(GLenum target, GLenum attachment, GLuint texture, GLint level);
	void APIENTRY (*FramebufferTextureEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level);
	void APIENTRY (*FramebufferTextureFaceARB)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face);
	void APIENTRY (*FramebufferTextureFaceEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face);
	void APIENTRY (*FramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
	void APIENTRY (*FramebufferTextureLayerARB)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
	void APIENTRY (*FramebufferTextureLayerEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
	void APIENTRY (*FrustumfOES)(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	void APIENTRY (*FrustumxOES)(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f);
	void APIENTRY (*GenFramebuffers)(GLsizei n, GLuint *framebuffers);
	void APIENTRY (*GenFramebuffersEXT)(GLsizei n, GLuint *framebuffers);
	void APIENTRY (*GenNamesAMD)(GLenum identifier, GLuint num, GLuint *names);
	GLuint APIENTRY (*GenPathsNV)(GLsizei range);
	void APIENTRY (*GenPerfMonitorsAMD)(GLsizei n, GLuint *monitors);
	void APIENTRY (*GenProgramPipelines)(GLsizei n, GLuint *pipelines);
	void APIENTRY (*GenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
	void APIENTRY (*GenRenderbuffersEXT)(GLsizei n, GLuint *renderbuffers);
	void APIENTRY (*GenSamplers)(GLsizei count, GLuint *samplers);
	void APIENTRY (*GenTransformFeedbacks)(GLsizei n, GLuint *ids);
	void APIENTRY (*GenTransformFeedbacksNV)(GLsizei n, GLuint *ids);
	void APIENTRY (*GenVertexArrays)(GLsizei n, GLuint *arrays);
	void APIENTRY (*GenerateMipmap)(GLenum target);
	void APIENTRY (*GenerateMipmapEXT)(GLenum target);
	void APIENTRY (*GenerateMultiTexMipmapEXT)(GLenum texunit, GLenum target);
	void APIENTRY (*GenerateTextureMipmapEXT)(GLuint texture, GLenum target);
	void APIENTRY (*GetActiveAtomicCounterBufferiv)(GLuint program, GLuint bufferIndex, GLenum pname, GLint *params);
	void APIENTRY (*GetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
	void APIENTRY (*GetActiveSubroutineName)(GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name);
	void APIENTRY (*GetActiveSubroutineUniformName)(GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name);
	void APIENTRY (*GetActiveSubroutineUniformiv)(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values);
	void APIENTRY (*GetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
	GLuint APIENTRY (*GetActiveUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName);
	void APIENTRY (*GetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
	void APIENTRY (*GetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
	void APIENTRY (*GetActiveUniformName)(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName);
	void APIENTRY (*GetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
	void APIENTRY (*GetActiveVaryingNV)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
	void APIENTRY (*GetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
	GLint APIENTRY (*GetAttribLocation)(GLuint program, const GLchar *name);
	void APIENTRY (*GetBooleanIndexedvEXT)(GLenum target, GLuint index, GLboolean *data);
	void APIENTRY (*GetBooleani_v)(GLenum target, GLuint index, GLboolean *data);
	void APIENTRY (*GetBufferParameteri64v)(GLenum target, GLenum pname, GLint64 *params);
	void APIENTRY (*GetBufferParameterui64vNV)(GLenum target, GLenum pname, GLuint64EXT *params);
	void APIENTRY (*GetClipPlanefOES)(GLenum plane, GLfloat *equation);
	void APIENTRY (*GetClipPlanexOES)(GLenum plane, GLfixed *equation);
	void APIENTRY (*GetCompressedMultiTexImageEXT)(GLenum texunit, GLenum target, GLint lod, void *img);
	void APIENTRY (*GetCompressedTextureImageEXT)(GLuint texture, GLenum target, GLint lod, void *img);
	void APIENTRY (*GetConvolutionParameterxvOES)(GLenum target, GLenum pname, GLfixed *params);
	GLsizei APIENTRY (*GetDebugLogLengthMESA)(GLhandleARB obj, GLenum logType, GLenum shaderType);
	void APIENTRY (*GetDebugLogMESA)(GLhandleARB obj, GLenum logType, GLenum shaderType, GLsizei maxLength, GLsizei *length, GLcharARB *debugLog);
	GLuint APIENTRY (*GetDebugMessageLog)(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);
	GLuint APIENTRY (*GetDebugMessageLogAMD)(GLuint count, GLsizei bufsize, GLenum *categories, GLuint *severities, GLuint *ids, GLsizei *lengths, GLchar *message);
	GLuint APIENTRY (*GetDebugMessageLogARB)(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);
	void APIENTRY (*GetDoubleIndexedvEXT)(GLenum target, GLuint index, GLdouble *data);
	void APIENTRY (*GetDoublei_v)(GLenum target, GLuint index, GLdouble *data);
	void APIENTRY (*GetDoublei_vEXT)(GLenum pname, GLuint index, GLdouble *params);
	void APIENTRY (*GetFirstPerfQueryIdINTEL)(GLuint *queryId);
	void APIENTRY (*GetFixedvOES)(GLenum pname, GLfixed *params);
	void APIENTRY (*GetFloatIndexedvEXT)(GLenum target, GLuint index, GLfloat *data);
	void APIENTRY (*GetFloati_v)(GLenum target, GLuint index, GLfloat *data);
	void APIENTRY (*GetFloati_vEXT)(GLenum pname, GLuint index, GLfloat *params);
	GLint APIENTRY (*GetFragDataIndex)(GLuint program, const GLchar *name);
	GLint APIENTRY (*GetFragDataLocation)(GLuint program, const GLchar *name);
	GLint APIENTRY (*GetFragDataLocationEXT)(GLuint program, const GLchar *name);
	void APIENTRY (*GetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
	void APIENTRY (*GetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
	void APIENTRY (*GetFramebufferParameteriv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetFramebufferParameterivEXT)(GLuint framebuffer, GLenum pname, GLint *params);
	GLenum APIENTRY (*GetGraphicsResetStatusARB)(void);
	void APIENTRY (*GetHistogramParameterxvOES)(GLenum target, GLenum pname, GLfixed *params);
	GLuint64 APIENTRY (*GetImageHandleARB)(GLuint texture, GLint level, GLboolean32 layered, GLint layer, GLenum format);
	GLuint64 APIENTRY (*GetImageHandleNV)(GLuint texture, GLint level, GLboolean32 layered, GLint layer, GLenum format);
	void APIENTRY (*GetInteger64i_v)(GLenum target, GLuint index, GLint64 *data);
	void APIENTRY (*GetInteger64v)(GLenum pname, GLint64 *data);
	void APIENTRY (*GetIntegerIndexedvEXT)(GLenum target, GLuint index, GLint *data);
	void APIENTRY (*GetIntegeri_v)(GLenum target, GLuint index, GLint *data);
	void APIENTRY (*GetIntegerui64i_vNV)(GLenum value, GLuint index, GLuint64EXT *result);
	void APIENTRY (*GetIntegerui64vNV)(GLenum value, GLuint64EXT *result);
	void APIENTRY (*GetInternalformati64v)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint64 *params);
	void APIENTRY (*GetInternalformativ)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);
	void APIENTRY (*GetLightxOES)(GLenum light, GLenum pname, GLfixed *params);
	void APIENTRY (*GetMapxvOES)(GLenum target, GLenum query, GLfixed *v);
	void APIENTRY (*GetMaterialxOES)(GLenum face, GLenum pname, GLfixed param);
	void APIENTRY (*GetMultiTexEnvfvEXT)(GLenum texunit, GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetMultiTexEnvivEXT)(GLenum texunit, GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetMultiTexGendvEXT)(GLenum texunit, GLenum coord, GLenum pname, GLdouble *params);
	void APIENTRY (*GetMultiTexGenfvEXT)(GLenum texunit, GLenum coord, GLenum pname, GLfloat *params);
	void APIENTRY (*GetMultiTexGenivEXT)(GLenum texunit, GLenum coord, GLenum pname, GLint *params);
	void APIENTRY (*GetMultiTexImageEXT)(GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void *pixels);
	void APIENTRY (*GetMultiTexLevelParameterfvEXT)(GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat *params);
	void APIENTRY (*GetMultiTexLevelParameterivEXT)(GLenum texunit, GLenum target, GLint level, GLenum pname, GLint *params);
	void APIENTRY (*GetMultiTexParameterIivEXT)(GLenum texunit, GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetMultiTexParameterIuivEXT)(GLenum texunit, GLenum target, GLenum pname, GLuint *params);
	void APIENTRY (*GetMultiTexParameterfvEXT)(GLenum texunit, GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetMultiTexParameterivEXT)(GLenum texunit, GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetMultisamplefv)(GLenum pname, GLuint index, GLfloat *val);
	void APIENTRY (*GetMultisamplefvNV)(GLenum pname, GLuint index, GLfloat *val);
	void APIENTRY (*GetNamedBufferParameterivEXT)(GLuint buffer, GLenum pname, GLint *params);
	void APIENTRY (*GetNamedBufferParameterui64vNV)(GLuint buffer, GLenum pname, GLuint64EXT *params);
	void APIENTRY (*GetNamedBufferPointervEXT)(GLuint buffer, GLenum pname, void * *params);
	void APIENTRY (*GetNamedBufferSubDataEXT)(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data);
	void APIENTRY (*GetNamedFramebufferAttachmentParameterivEXT)(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params);
	void APIENTRY (*GetNamedFramebufferParameterivEXT)(GLuint framebuffer, GLenum pname, GLint *params);
	void APIENTRY (*GetNamedProgramLocalParameterIivEXT)(GLuint program, GLenum target, GLuint index, GLint *params);
	void APIENTRY (*GetNamedProgramLocalParameterIuivEXT)(GLuint program, GLenum target, GLuint index, GLuint *params);
	void APIENTRY (*GetNamedProgramLocalParameterdvEXT)(GLuint program, GLenum target, GLuint index, GLdouble *params);
	void APIENTRY (*GetNamedProgramLocalParameterfvEXT)(GLuint program, GLenum target, GLuint index, GLfloat *params);
	void APIENTRY (*GetNamedProgramStringEXT)(GLuint program, GLenum target, GLenum pname, void *string);
	void APIENTRY (*GetNamedProgramivEXT)(GLuint program, GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetNamedRenderbufferParameterivEXT)(GLuint renderbuffer, GLenum pname, GLint *params);
	void APIENTRY (*GetNamedStringARB)(GLint namelen, const GLchar *name, GLsizei bufSize, GLint *stringlen, GLchar *string);
	void APIENTRY (*GetNamedStringivARB)(GLint namelen, const GLchar *name, GLenum pname, GLint *params);
	void APIENTRY (*GetNextPerfQueryIdINTEL)(GLuint queryId, GLuint *nextQueryId);
	void APIENTRY (*GetObjectLabel)(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label);
	void APIENTRY (*GetObjectLabelEXT)(GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label);
	void APIENTRY (*GetObjectParameterivAPPLE)(GLenum objectType, GLuint name, GLenum pname, GLint *params);
	void APIENTRY (*GetObjectPtrLabel)(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label);
	void APIENTRY (*GetPathColorGenfvNV)(GLenum color, GLenum pname, GLfloat *value);
	void APIENTRY (*GetPathColorGenivNV)(GLenum color, GLenum pname, GLint *value);
	void APIENTRY (*GetPathCommandsNV)(GLuint path, GLubyte *commands);
	void APIENTRY (*GetPathCoordsNV)(GLuint path, GLfloat *coords);
	void APIENTRY (*GetPathDashArrayNV)(GLuint path, GLfloat *dashArray);
	GLfloat APIENTRY (*GetPathLengthNV)(GLuint path, GLsizei startSegment, GLsizei numSegments);
	void APIENTRY (*GetPathMetricRangeNV)(GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics);
	void APIENTRY (*GetPathMetricsNV)(GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics);
	void APIENTRY (*GetPathParameterfvNV)(GLuint path, GLenum pname, GLfloat *value);
	void APIENTRY (*GetPathParameterivNV)(GLuint path, GLenum pname, GLint *value);
	void APIENTRY (*GetPathSpacingNV)(GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing);
	void APIENTRY (*GetPathTexGenfvNV)(GLenum texCoordSet, GLenum pname, GLfloat *value);
	void APIENTRY (*GetPathTexGenivNV)(GLenum texCoordSet, GLenum pname, GLint *value);
	void APIENTRY (*GetPerfCounterInfoINTEL)(GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue);
	void APIENTRY (*GetPerfMonitorCounterDataAMD)(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten);
	void APIENTRY (*GetPerfMonitorCounterInfoAMD)(GLuint group, GLuint counter, GLenum pname, void *data);
	void APIENTRY (*GetPerfMonitorCounterStringAMD)(GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString);
	void APIENTRY (*GetPerfMonitorCountersAMD)(GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters);
	void APIENTRY (*GetPerfMonitorGroupStringAMD)(GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString);
	void APIENTRY (*GetPerfMonitorGroupsAMD)(GLint *numGroups, GLsizei groupsSize, GLuint *groups);
	void APIENTRY (*GetPerfQueryDataINTEL)(GLuint queryHandle, GLuint flags, GLsizei dataSize, GLvoid *data, GLuint *bytesWritten);
	void APIENTRY (*GetPerfQueryIdByNameINTEL)(GLchar *queryName, GLuint *queryId);
	void APIENTRY (*GetPerfQueryInfoINTEL)(GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask);
	void APIENTRY (*GetPixelMapxv)(GLenum map, GLint size, GLfixed *values);
	void APIENTRY (*GetPixelTransformParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetPixelTransformParameterivEXT)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetPointerIndexedvEXT)(GLenum target, GLuint index, void * *data);
	void APIENTRY (*GetPointeri_vEXT)(GLenum pname, GLuint index, void * *params);
	void APIENTRY (*GetProgramBinary)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
	void APIENTRY (*GetProgramEnvParameterIivNV)(GLenum target, GLuint index, GLint *params);
	void APIENTRY (*GetProgramEnvParameterIuivNV)(GLenum target, GLuint index, GLuint *params);
	void APIENTRY (*GetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
	void APIENTRY (*GetProgramInterfaceiv)(GLuint program, GLenum programInterface, GLenum pname, GLint *params);
	void APIENTRY (*GetProgramLocalParameterIivNV)(GLenum target, GLuint index, GLint *params);
	void APIENTRY (*GetProgramLocalParameterIuivNV)(GLenum target, GLuint index, GLuint *params);
	void APIENTRY (*GetProgramPipelineInfoLog)(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
	void APIENTRY (*GetProgramPipelineiv)(GLuint pipeline, GLenum pname, GLint *params);
	void APIENTRY (*GetProgramRegisterfvMESA)(GLenum target, GLsizei len, const GLubyte *name, GLfloat *v);
	GLuint APIENTRY (*GetProgramResourceIndex)(GLuint program, GLenum programInterface, const GLchar *name);
	GLint APIENTRY (*GetProgramResourceLocation)(GLuint program, GLenum programInterface, const GLchar *name);
	GLint APIENTRY (*GetProgramResourceLocationIndex)(GLuint program, GLenum programInterface, const GLchar *name);
	void APIENTRY (*GetProgramResourceName)(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name);
	void APIENTRY (*GetProgramResourceiv)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params);
	void APIENTRY (*GetProgramStageiv)(GLuint program, GLenum shadertype, GLenum pname, GLint *values);
	void APIENTRY (*GetProgramSubroutineParameteruivNV)(GLenum target, GLuint index, GLuint *param);
	void APIENTRY (*GetProgramiv)(GLuint program, GLenum pname, GLint *params);
	void APIENTRY (*GetQueryIndexediv)(GLenum target, GLuint index, GLenum pname, GLint *params);
	void APIENTRY (*GetQueryObjecti64v)(GLuint id, GLenum pname, GLint64 *params);
	void APIENTRY (*GetQueryObjecti64vEXT)(GLuint id, GLenum pname, GLint64 *params);
	void APIENTRY (*GetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64 *params);
	void APIENTRY (*GetQueryObjectui64vEXT)(GLuint id, GLenum pname, GLuint64 *params);
	void APIENTRY (*GetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetSamplerParameterIiv)(GLuint sampler, GLenum pname, GLint *params);
	void APIENTRY (*GetSamplerParameterIuiv)(GLuint sampler, GLenum pname, GLuint *params);
	void APIENTRY (*GetSamplerParameterfv)(GLuint sampler, GLenum pname, GLfloat *params);
	void APIENTRY (*GetSamplerParameteriv)(GLuint sampler, GLenum pname, GLint *params);
	void APIENTRY (*GetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
	void APIENTRY (*GetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision);
	void APIENTRY (*GetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
	void APIENTRY (*GetShaderiv)(GLuint shader, GLenum pname, GLint *params);
	const GLubyte * APIENTRY (*GetStringi)(GLenum name, GLuint index);
	GLuint APIENTRY (*GetSubroutineIndex)(GLuint program, GLenum shadertype, const GLchar *name);
	GLint APIENTRY (*GetSubroutineUniformLocation)(GLuint program, GLenum shadertype, const GLchar *name);
	void APIENTRY (*GetSynciv)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
	void APIENTRY (*GetTexEnvxvOES)(GLenum target, GLenum pname, GLfixed *params);
	void APIENTRY (*GetTexGenxvOES)(GLenum coord, GLenum pname, GLfixed *params);
	void APIENTRY (*GetTexLevelParameterxvOES)(GLenum target, GLint level, GLenum pname, GLfixed *params);
	void APIENTRY (*GetTexParameterIiv)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetTexParameterIivEXT)(GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetTexParameterIuiv)(GLenum target, GLenum pname, GLuint *params);
	void APIENTRY (*GetTexParameterIuivEXT)(GLenum target, GLenum pname, GLuint *params);
	void APIENTRY (*GetTexParameterPointervAPPLE)(GLenum target, GLenum pname, void * *params);
	void APIENTRY (*GetTexParameterxvOES)(GLenum target, GLenum pname, GLfixed *params);
	GLuint64 APIENTRY (*GetTextureHandleARB)(GLuint texture);
	GLuint64 APIENTRY (*GetTextureHandleNV)(GLuint texture);
	void APIENTRY (*GetTextureImageEXT)(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels);
	void APIENTRY (*GetTextureLevelParameterfvEXT)(GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat *params);
	void APIENTRY (*GetTextureLevelParameterivEXT)(GLuint texture, GLenum target, GLint level, GLenum pname, GLint *params);
	void APIENTRY (*GetTextureParameterIivEXT)(GLuint texture, GLenum target, GLenum pname, GLint *params);
	void APIENTRY (*GetTextureParameterIuivEXT)(GLuint texture, GLenum target, GLenum pname, GLuint *params);
	void APIENTRY (*GetTextureParameterfvEXT)(GLuint texture, GLenum target, GLenum pname, GLfloat *params);
	void APIENTRY (*GetTextureParameterivEXT)(GLuint texture, GLenum target, GLenum pname, GLint *params);
	GLuint64 APIENTRY (*GetTextureSamplerHandleARB)(GLuint texture, GLuint sampler);
	GLuint64 APIENTRY (*GetTextureSamplerHandleNV)(GLuint texture, GLuint sampler);
	void APIENTRY (*GetTransformFeedbackVarying)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
	void APIENTRY (*GetTransformFeedbackVaryingEXT)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
	void APIENTRY (*GetTransformFeedbackVaryingNV)(GLuint program, GLuint index, GLint *location);
	GLuint APIENTRY (*GetUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName);
	GLint APIENTRY (*GetUniformBufferSizeEXT)(GLuint program, GLint location);
	void APIENTRY (*GetUniformIndices)(GLuint program, GLsizei uniformCount, const GLchar *const *uniformNames, GLuint *uniformIndices);
	GLint APIENTRY (*GetUniformLocation)(GLuint program, const GLchar *name);
	GLintptr APIENTRY (*GetUniformOffsetEXT)(GLuint program, GLint location);
	void APIENTRY (*GetUniformSubroutineuiv)(GLenum shadertype, GLint location, GLuint *params);
	void APIENTRY (*GetUniformdv)(GLuint program, GLint location, GLdouble *params);
	void APIENTRY (*GetUniformfv)(GLuint program, GLint location, GLfloat *params);
	void APIENTRY (*GetUniformi64vNV)(GLuint program, GLint location, GLint64EXT *params);
	void APIENTRY (*GetUniformiv)(GLuint program, GLint location, GLint *params);
	void APIENTRY (*GetUniformui64vNV)(GLuint program, GLint location, GLuint64EXT *params);
	void APIENTRY (*GetUniformuiv)(GLuint program, GLint location, GLuint *params);
	void APIENTRY (*GetUniformuivEXT)(GLuint program, GLint location, GLuint *params);
	GLint APIENTRY (*GetVaryingLocationNV)(GLuint program, const GLchar *name);
	void APIENTRY (*GetVertexArrayIntegeri_vEXT)(GLuint vaobj, GLuint index, GLenum pname, GLint *param);
	void APIENTRY (*GetVertexArrayIntegervEXT)(GLuint vaobj, GLenum pname, GLint *param);
	void APIENTRY (*GetVertexArrayPointeri_vEXT)(GLuint vaobj, GLuint index, GLenum pname, void * *param);
	void APIENTRY (*GetVertexArrayPointervEXT)(GLuint vaobj, GLenum pname, void * *param);
	void APIENTRY (*GetVertexAttribIiv)(GLuint index, GLenum pname, GLint *params);
	void APIENTRY (*GetVertexAttribIivEXT)(GLuint index, GLenum pname, GLint *params);
	void APIENTRY (*GetVertexAttribIuiv)(GLuint index, GLenum pname, GLuint *params);
	void APIENTRY (*GetVertexAttribIuivEXT)(GLuint index, GLenum pname, GLuint *params);
	void APIENTRY (*GetVertexAttribLdv)(GLuint index, GLenum pname, GLdouble *params);
	void APIENTRY (*GetVertexAttribLdvEXT)(GLuint index, GLenum pname, GLdouble *params);
	void APIENTRY (*GetVertexAttribLi64vNV)(GLuint index, GLenum pname, GLint64EXT *params);
	void APIENTRY (*GetVertexAttribLui64vARB)(GLuint index, GLenum pname, GLuint64EXT *params);
	void APIENTRY (*GetVertexAttribLui64vNV)(GLuint index, GLenum pname, GLuint64EXT *params);
	void APIENTRY (*GetVertexAttribPointerv)(GLuint index, GLenum pname, void * *pointer);
	void APIENTRY (*GetVertexAttribdv)(GLuint index, GLenum pname, GLdouble *params);
	void APIENTRY (*GetVertexAttribfv)(GLuint index, GLenum pname, GLfloat *params);
	void APIENTRY (*GetVertexAttribiv)(GLuint index, GLenum pname, GLint *params);
	void APIENTRY (*GetVideoCaptureStreamdvNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble *params);
	void APIENTRY (*GetVideoCaptureStreamfvNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat *params);
	void APIENTRY (*GetVideoCaptureStreamivNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, GLint *params);
	void APIENTRY (*GetVideoCaptureivNV)(GLuint video_capture_slot, GLenum pname, GLint *params);
	void APIENTRY (*GetVideoi64vNV)(GLuint video_slot, GLenum pname, GLint64EXT *params);
	void APIENTRY (*GetVideoivNV)(GLuint video_slot, GLenum pname, GLint *params);
	void APIENTRY (*GetVideoui64vNV)(GLuint video_slot, GLenum pname, GLuint64EXT *params);
	void APIENTRY (*GetVideouivNV)(GLuint video_slot, GLenum pname, GLuint *params);
	void APIENTRY (*GetnColorTableARB)(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table);
	void APIENTRY (*GetnCompressedTexImageARB)(GLenum target, GLint lod, GLsizei bufSize, void *img);
	void APIENTRY (*GetnConvolutionFilterARB)(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image);
	void APIENTRY (*GetnHistogramARB)(GLenum target, GLboolean32 reset, GLenum format, GLenum type, GLsizei bufSize, void *values);
	void APIENTRY (*GetnMapdvARB)(GLenum target, GLenum query, GLsizei bufSize, GLdouble *v);
	void APIENTRY (*GetnMapfvARB)(GLenum target, GLenum query, GLsizei bufSize, GLfloat *v);
	void APIENTRY (*GetnMapivARB)(GLenum target, GLenum query, GLsizei bufSize, GLint *v);
	void APIENTRY (*GetnMinmaxARB)(GLenum target, GLboolean32 reset, GLenum format, GLenum type, GLsizei bufSize, void *values);
	void APIENTRY (*GetnPixelMapfvARB)(GLenum map, GLsizei bufSize, GLfloat *values);
	void APIENTRY (*GetnPixelMapuivARB)(GLenum map, GLsizei bufSize, GLuint *values);
	void APIENTRY (*GetnPixelMapusvARB)(GLenum map, GLsizei bufSize, GLushort *values);
	void APIENTRY (*GetnPolygonStippleARB)(GLsizei bufSize, GLubyte *pattern);
	void APIENTRY (*GetnSeparableFilterARB)(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span);
	void APIENTRY (*GetnTexImageARB)(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *img);
	void APIENTRY (*GetnUniformdvARB)(GLuint program, GLint location, GLsizei bufSize, GLdouble *params);
	void APIENTRY (*GetnUniformfvARB)(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
	void APIENTRY (*GetnUniformivARB)(GLuint program, GLint location, GLsizei bufSize, GLint *params);
	void APIENTRY (*GetnUniformuivARB)(GLuint program, GLint location, GLsizei bufSize, GLuint *params);
	GLsync APIENTRY (*ImportSyncEXT)(GLenum external_sync_type, GLintptr external_sync, GLbitfield flags);
	void APIENTRY (*IndexFormatNV)(GLenum type, GLsizei stride);
	void APIENTRY (*IndexxOES)(GLfixed component);
	void APIENTRY (*IndexxvOES)(const GLfixed *component);
	void APIENTRY (*InsertEventMarkerEXT)(GLsizei length, const GLchar *marker);
	void APIENTRY (*InterpolatePathsNV)(GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight);
	void APIENTRY (*InvalidateBufferData)(GLuint buffer);
	void APIENTRY (*InvalidateBufferSubData)(GLuint buffer, GLintptr offset, GLsizeiptr length);
	void APIENTRY (*InvalidateFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments);
	void APIENTRY (*InvalidateSubFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*InvalidateTexImage)(GLuint texture, GLint level);
	void APIENTRY (*InvalidateTexSubImage)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth);
	GLboolean APIENTRY (*IsBufferResidentNV)(GLenum target);
	GLboolean APIENTRY (*IsEnabledIndexedEXT)(GLenum target, GLuint index);
	GLboolean APIENTRY (*IsEnabledi)(GLenum target, GLuint index);
	GLboolean APIENTRY (*IsFramebuffer)(GLuint framebuffer);
	GLboolean APIENTRY (*IsFramebufferEXT)(GLuint framebuffer);
	GLboolean APIENTRY (*IsImageHandleResidentARB)(GLuint64 handle);
	GLboolean APIENTRY (*IsImageHandleResidentNV)(GLuint64 handle);
	GLboolean APIENTRY (*IsNameAMD)(GLenum identifier, GLuint name);
	GLboolean APIENTRY (*IsNamedBufferResidentNV)(GLuint buffer);
	GLboolean APIENTRY (*IsNamedStringARB)(GLint namelen, const GLchar *name);
	GLboolean APIENTRY (*IsPathNV)(GLuint path);
	GLboolean APIENTRY (*IsPointInFillPathNV)(GLuint path, GLuint mask, GLfloat x, GLfloat y);
	GLboolean APIENTRY (*IsPointInStrokePathNV)(GLuint path, GLfloat x, GLfloat y);
	GLboolean APIENTRY (*IsProgram)(GLuint program);
	GLboolean APIENTRY (*IsProgramPipeline)(GLuint pipeline);
	GLboolean APIENTRY (*IsRenderbuffer)(GLuint renderbuffer);
	GLboolean APIENTRY (*IsRenderbufferEXT)(GLuint renderbuffer);
	GLboolean APIENTRY (*IsSampler)(GLuint sampler);
	GLboolean APIENTRY (*IsShader)(GLuint shader);
	GLboolean APIENTRY (*IsSync)(GLsync sync);
	GLboolean APIENTRY (*IsTextureHandleResidentARB)(GLuint64 handle);
	GLboolean APIENTRY (*IsTextureHandleResidentNV)(GLuint64 handle);
	GLboolean APIENTRY (*IsTransformFeedback)(GLuint id);
	GLboolean APIENTRY (*IsTransformFeedbackNV)(GLuint id);
	GLboolean APIENTRY (*IsVertexArray)(GLuint array);
	GLboolean APIENTRY (*IsVertexAttribEnabledAPPLE)(GLuint index, GLenum pname);
	void APIENTRY (*LabelObjectEXT)(GLenum type, GLuint object, GLsizei length, const GLchar *label);
	void APIENTRY (*LightModelxOES)(GLenum pname, GLfixed param);
	void APIENTRY (*LightModelxvOES)(GLenum pname, const GLfixed *param);
	void APIENTRY (*LightxOES)(GLenum light, GLenum pname, GLfixed param);
	void APIENTRY (*LightxvOES)(GLenum light, GLenum pname, const GLfixed *params);
	void APIENTRY (*LineWidthxOES)(GLfixed width);
	void APIENTRY (*LinkProgram)(GLuint program);
	void APIENTRY (*LoadMatrixxOES)(const GLfixed *m);
	void APIENTRY (*LoadTransposeMatrixxOES)(const GLfixed *m);
	void APIENTRY (*MakeBufferNonResidentNV)(GLenum target);
	void APIENTRY (*MakeBufferResidentNV)(GLenum target, GLenum access);
	void APIENTRY (*MakeImageHandleNonResidentARB)(GLuint64 handle);
	void APIENTRY (*MakeImageHandleNonResidentNV)(GLuint64 handle);
	void APIENTRY (*MakeImageHandleResidentARB)(GLuint64 handle, GLenum access);
	void APIENTRY (*MakeImageHandleResidentNV)(GLuint64 handle, GLenum access);
	void APIENTRY (*MakeNamedBufferNonResidentNV)(GLuint buffer);
	void APIENTRY (*MakeNamedBufferResidentNV)(GLuint buffer, GLenum access);
	void APIENTRY (*MakeTextureHandleNonResidentARB)(GLuint64 handle);
	void APIENTRY (*MakeTextureHandleNonResidentNV)(GLuint64 handle);
	void APIENTRY (*MakeTextureHandleResidentARB)(GLuint64 handle);
	void APIENTRY (*MakeTextureHandleResidentNV)(GLuint64 handle);
	void APIENTRY (*Map1xOES)(GLenum target, GLfixed u1, GLfixed u2, GLint stride, GLint order, GLfixed points);
	void APIENTRY (*Map2xOES)(GLenum target, GLfixed u1, GLfixed u2, GLint ustride, GLint uorder, GLfixed v1, GLfixed v2, GLint vstride, GLint vorder, GLfixed points);
	void * APIENTRY (*MapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
	void APIENTRY (*MapGrid1xOES)(GLint n, GLfixed u1, GLfixed u2);
	void APIENTRY (*MapGrid2xOES)(GLint n, GLfixed u1, GLfixed u2, GLfixed v1, GLfixed v2);
	void * APIENTRY (*MapNamedBufferEXT)(GLuint buffer, GLenum access);
	void * APIENTRY (*MapNamedBufferRangeEXT)(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
	void * APIENTRY (*MapTexture2DINTEL)(GLuint texture, GLint level, GLbitfield access, const GLint *stride, const GLenum *layout);
	void APIENTRY (*MapVertexAttrib1dAPPLE)(GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
	void APIENTRY (*MapVertexAttrib1fAPPLE)(GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
	void APIENTRY (*MapVertexAttrib2dAPPLE)(GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
	void APIENTRY (*MapVertexAttrib2fAPPLE)(GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
	void APIENTRY (*MaterialxOES)(GLenum face, GLenum pname, GLfixed param);
	void APIENTRY (*MaterialxvOES)(GLenum face, GLenum pname, const GLfixed *param);
	void APIENTRY (*MatrixFrustumEXT)(GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void APIENTRY (*MatrixLoadIdentityEXT)(GLenum mode);
	void APIENTRY (*MatrixLoadTransposedEXT)(GLenum mode, const GLdouble *m);
	void APIENTRY (*MatrixLoadTransposefEXT)(GLenum mode, const GLfloat *m);
	void APIENTRY (*MatrixLoaddEXT)(GLenum mode, const GLdouble *m);
	void APIENTRY (*MatrixLoadfEXT)(GLenum mode, const GLfloat *m);
	void APIENTRY (*MatrixMultTransposedEXT)(GLenum mode, const GLdouble *m);
	void APIENTRY (*MatrixMultTransposefEXT)(GLenum mode, const GLfloat *m);
	void APIENTRY (*MatrixMultdEXT)(GLenum mode, const GLdouble *m);
	void APIENTRY (*MatrixMultfEXT)(GLenum mode, const GLfloat *m);
	void APIENTRY (*MatrixOrthoEXT)(GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void APIENTRY (*MatrixPopEXT)(GLenum mode);
	void APIENTRY (*MatrixPushEXT)(GLenum mode);
	void APIENTRY (*MatrixRotatedEXT)(GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*MatrixRotatefEXT)(GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*MatrixScaledEXT)(GLenum mode, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*MatrixScalefEXT)(GLenum mode, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*MatrixTranslatedEXT)(GLenum mode, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*MatrixTranslatefEXT)(GLenum mode, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*MemoryBarrier)(GLbitfield barriers);
	void APIENTRY (*MemoryBarrierEXT)(GLbitfield barriers);
	void APIENTRY (*MinSampleShading)(GLfloat value);
	void APIENTRY (*MinSampleShadingARB)(GLfloat value);
	void APIENTRY (*MultMatrixxOES)(const GLfixed *m);
	void APIENTRY (*MultTransposeMatrixxOES)(const GLfixed *m);
	void APIENTRY (*MultiDrawArraysIndirect)(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride);
	void APIENTRY (*MultiDrawArraysIndirectAMD)(GLenum mode, const void *indirect, GLsizei primcount, GLsizei stride);
	void APIENTRY (*MultiDrawArraysIndirectBindlessNV)(GLenum mode, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount);
	void APIENTRY (*MultiDrawArraysIndirectCountARB)(GLenum mode, GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride);
	void APIENTRY (*MultiDrawElementsBaseVertex)(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei drawcount, const GLint *basevertex);
	void APIENTRY (*MultiDrawElementsIndirect)(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);
	void APIENTRY (*MultiDrawElementsIndirectAMD)(GLenum mode, GLenum type, const void *indirect, GLsizei primcount, GLsizei stride);
	void APIENTRY (*MultiDrawElementsIndirectBindlessNV)(GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount);
	void APIENTRY (*MultiDrawElementsIndirectCountARB)(GLenum mode, GLenum type, GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride);
	void APIENTRY (*MultiTexBufferEXT)(GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer);
	void APIENTRY (*MultiTexCoord1bOES)(GLenum texture, GLbyte32 s);
	void APIENTRY (*MultiTexCoord1bvOES)(GLenum texture, const GLbyte *coords);
	void APIENTRY (*MultiTexCoord1xOES)(GLenum texture, GLfixed s);
	void APIENTRY (*MultiTexCoord1xvOES)(GLenum texture, const GLfixed *coords);
	void APIENTRY (*MultiTexCoord2bOES)(GLenum texture, GLbyte32 s, GLbyte32 t);
	void APIENTRY (*MultiTexCoord2bvOES)(GLenum texture, const GLbyte *coords);
	void APIENTRY (*MultiTexCoord2xOES)(GLenum texture, GLfixed s, GLfixed t);
	void APIENTRY (*MultiTexCoord2xvOES)(GLenum texture, const GLfixed *coords);
	void APIENTRY (*MultiTexCoord3bOES)(GLenum texture, GLbyte32 s, GLbyte32 t, GLbyte32 r);
	void APIENTRY (*MultiTexCoord3bvOES)(GLenum texture, const GLbyte *coords);
	void APIENTRY (*MultiTexCoord3xOES)(GLenum texture, GLfixed s, GLfixed t, GLfixed r);
	void APIENTRY (*MultiTexCoord3xvOES)(GLenum texture, const GLfixed *coords);
	void APIENTRY (*MultiTexCoord4bOES)(GLenum texture, GLbyte32 s, GLbyte32 t, GLbyte32 r, GLbyte32 q);
	void APIENTRY (*MultiTexCoord4bvOES)(GLenum texture, const GLbyte *coords);
	void APIENTRY (*MultiTexCoord4xOES)(GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
	void APIENTRY (*MultiTexCoord4xvOES)(GLenum texture, const GLfixed *coords);
	void APIENTRY (*MultiTexCoordP1ui)(GLenum texture, GLenum type, GLuint coords);
	void APIENTRY (*MultiTexCoordP1uiv)(GLenum texture, GLenum type, const GLuint *coords);
	void APIENTRY (*MultiTexCoordP2ui)(GLenum texture, GLenum type, GLuint coords);
	void APIENTRY (*MultiTexCoordP2uiv)(GLenum texture, GLenum type, const GLuint *coords);
	void APIENTRY (*MultiTexCoordP3ui)(GLenum texture, GLenum type, GLuint coords);
	void APIENTRY (*MultiTexCoordP3uiv)(GLenum texture, GLenum type, const GLuint *coords);
	void APIENTRY (*MultiTexCoordP4ui)(GLenum texture, GLenum type, GLuint coords);
	void APIENTRY (*MultiTexCoordP4uiv)(GLenum texture, GLenum type, const GLuint *coords);
	void APIENTRY (*MultiTexCoordPointerEXT)(GLenum texunit, GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*MultiTexEnvfEXT)(GLenum texunit, GLenum target, GLenum pname, GLfloat param);
	void APIENTRY (*MultiTexEnvfvEXT)(GLenum texunit, GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*MultiTexEnviEXT)(GLenum texunit, GLenum target, GLenum pname, GLint param);
	void APIENTRY (*MultiTexEnvivEXT)(GLenum texunit, GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*MultiTexGendEXT)(GLenum texunit, GLenum coord, GLenum pname, GLdouble param);
	void APIENTRY (*MultiTexGendvEXT)(GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params);
	void APIENTRY (*MultiTexGenfEXT)(GLenum texunit, GLenum coord, GLenum pname, GLfloat param);
	void APIENTRY (*MultiTexGenfvEXT)(GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params);
	void APIENTRY (*MultiTexGeniEXT)(GLenum texunit, GLenum coord, GLenum pname, GLint param);
	void APIENTRY (*MultiTexGenivEXT)(GLenum texunit, GLenum coord, GLenum pname, const GLint *params);
	void APIENTRY (*MultiTexImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*MultiTexImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*MultiTexImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*MultiTexParameterIivEXT)(GLenum texunit, GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*MultiTexParameterIuivEXT)(GLenum texunit, GLenum target, GLenum pname, const GLuint *params);
	void APIENTRY (*MultiTexParameterfEXT)(GLenum texunit, GLenum target, GLenum pname, GLfloat param);
	void APIENTRY (*MultiTexParameterfvEXT)(GLenum texunit, GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*MultiTexParameteriEXT)(GLenum texunit, GLenum target, GLenum pname, GLint param);
	void APIENTRY (*MultiTexParameterivEXT)(GLenum texunit, GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*MultiTexRenderbufferEXT)(GLenum texunit, GLenum target, GLuint renderbuffer);
	void APIENTRY (*MultiTexSubImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*MultiTexSubImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*MultiTexSubImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*NamedBufferDataEXT)(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
	void APIENTRY (*NamedBufferStorageEXT)(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);
	void APIENTRY (*NamedBufferSubDataEXT)(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
	void APIENTRY (*NamedCopyBufferSubDataEXT)(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
	void APIENTRY (*NamedFramebufferParameteriEXT)(GLuint framebuffer, GLenum pname, GLint param);
	void APIENTRY (*NamedFramebufferRenderbufferEXT)(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	void APIENTRY (*NamedFramebufferTexture1DEXT)(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void APIENTRY (*NamedFramebufferTexture2DEXT)(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void APIENTRY (*NamedFramebufferTexture3DEXT)(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
	void APIENTRY (*NamedFramebufferTextureEXT)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
	void APIENTRY (*NamedFramebufferTextureFaceEXT)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face);
	void APIENTRY (*NamedFramebufferTextureLayerEXT)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);
	void APIENTRY (*NamedProgramLocalParameter4dEXT)(GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*NamedProgramLocalParameter4dvEXT)(GLuint program, GLenum target, GLuint index, const GLdouble *params);
	void APIENTRY (*NamedProgramLocalParameter4fEXT)(GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*NamedProgramLocalParameter4fvEXT)(GLuint program, GLenum target, GLuint index, const GLfloat *params);
	void APIENTRY (*NamedProgramLocalParameterI4iEXT)(GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*NamedProgramLocalParameterI4ivEXT)(GLuint program, GLenum target, GLuint index, const GLint *params);
	void APIENTRY (*NamedProgramLocalParameterI4uiEXT)(GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
	void APIENTRY (*NamedProgramLocalParameterI4uivEXT)(GLuint program, GLenum target, GLuint index, const GLuint *params);
	void APIENTRY (*NamedProgramLocalParameters4fvEXT)(GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat *params);
	void APIENTRY (*NamedProgramLocalParametersI4ivEXT)(GLuint program, GLenum target, GLuint index, GLsizei count, const GLint *params);
	void APIENTRY (*NamedProgramLocalParametersI4uivEXT)(GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint *params);
	void APIENTRY (*NamedProgramStringEXT)(GLuint program, GLenum target, GLenum format, GLsizei len, const void *string);
	void APIENTRY (*NamedRenderbufferStorageEXT)(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*NamedRenderbufferStorageMultisampleCoverageEXT)(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*NamedRenderbufferStorageMultisampleEXT)(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*NamedStringARB)(GLenum type, GLint namelen, const GLchar *name, GLint stringlen, const GLchar *string);
	void APIENTRY (*Normal3xOES)(GLfixed nx, GLfixed ny, GLfixed nz);
	void APIENTRY (*Normal3xvOES)(const GLfixed *coords);
	void APIENTRY (*NormalFormatNV)(GLenum type, GLsizei stride);
	void APIENTRY (*NormalP3ui)(GLenum type, GLuint coords);
	void APIENTRY (*NormalP3uiv)(GLenum type, const GLuint *coords);
	void APIENTRY (*ObjectLabel)(GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
	void APIENTRY (*ObjectPtrLabel)(const void *ptr, GLsizei length, const GLchar *label);
	GLenum APIENTRY (*ObjectPurgeableAPPLE)(GLenum objectType, GLuint name, GLenum option);
	GLenum APIENTRY (*ObjectUnpurgeableAPPLE)(GLenum objectType, GLuint name, GLenum option);
	void APIENTRY (*OrthofOES)(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	void APIENTRY (*OrthoxOES)(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f);
	void APIENTRY (*PassThroughxOES)(GLfixed token);
	void APIENTRY (*PatchParameterfv)(GLenum pname, const GLfloat *values);
	void APIENTRY (*PatchParameteri)(GLenum pname, GLint value);
	void APIENTRY (*PathColorGenNV)(GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs);
	void APIENTRY (*PathCommandsNV)(GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords);
	void APIENTRY (*PathCoordsNV)(GLuint path, GLsizei numCoords, GLenum coordType, const void *coords);
	void APIENTRY (*PathCoverDepthFuncNV)(GLenum func);
	void APIENTRY (*PathDashArrayNV)(GLuint path, GLsizei dashCount, const GLfloat *dashArray);
	void APIENTRY (*PathFogGenNV)(GLenum genMode);
	void APIENTRY (*PathGlyphRangeNV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
	void APIENTRY (*PathGlyphsNV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
	void APIENTRY (*PathParameterfNV)(GLuint path, GLenum pname, GLfloat value);
	void APIENTRY (*PathParameterfvNV)(GLuint path, GLenum pname, const GLfloat *value);
	void APIENTRY (*PathParameteriNV)(GLuint path, GLenum pname, GLint value);
	void APIENTRY (*PathParameterivNV)(GLuint path, GLenum pname, const GLint *value);
	void APIENTRY (*PathStencilDepthOffsetNV)(GLfloat factor, GLfloat units);
	void APIENTRY (*PathStencilFuncNV)(GLenum func, GLint ref, GLuint mask);
	void APIENTRY (*PathStringNV)(GLuint path, GLenum format, GLsizei length, const void *pathString);
	void APIENTRY (*PathSubCommandsNV)(GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords);
	void APIENTRY (*PathSubCoordsNV)(GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords);
	void APIENTRY (*PathTexGenNV)(GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat *coeffs);
	void APIENTRY (*PauseTransformFeedback)(void);
	void APIENTRY (*PauseTransformFeedbackNV)(void);
	void APIENTRY (*PixelMapx)(GLenum map, GLint size, const GLfixed *values);
	void APIENTRY (*PixelStorex)(GLenum pname, GLfixed param);
	void APIENTRY (*PixelTransferxOES)(GLenum pname, GLfixed param);
	void APIENTRY (*PixelZoomxOES)(GLfixed xfactor, GLfixed yfactor);
	GLboolean APIENTRY (*PointAlongPathNV)(GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY);
	void APIENTRY (*PointParameterxvOES)(GLenum pname, const GLfixed *params);
	void APIENTRY (*PointSizePointerAPPLE)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void APIENTRY (*PointSizexOES)(GLfixed size);
	void APIENTRY (*PolygonOffsetxOES)(GLfixed factor, GLfixed units);
	void APIENTRY (*PopDebugGroup)(void);
	void APIENTRY (*PopGroupMarkerEXT)(void);
	void APIENTRY (*PresentFrameDualFillNV)(GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3);
	void APIENTRY (*PresentFrameKeyedNV)(GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1);
	void APIENTRY (*PrimitiveRestartIndex)(GLuint index);
	void APIENTRY (*PrioritizeTexturesxOES)(GLsizei n, const GLuint *textures, const GLfixed *priorities);
	void APIENTRY (*ProgramBinary)(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
	void APIENTRY (*ProgramBufferParametersIivNV)(GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLint *params);
	void APIENTRY (*ProgramBufferParametersIuivNV)(GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLuint *params);
	void APIENTRY (*ProgramBufferParametersfvNV)(GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLfloat *params);
	void APIENTRY (*ProgramCallbackMESA)(GLenum target, GLprogramcallbackMESA callback, GLvoid *data);
	void APIENTRY (*ProgramEnvParameterI4iNV)(GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*ProgramEnvParameterI4ivNV)(GLenum target, GLuint index, const GLint *params);
	void APIENTRY (*ProgramEnvParameterI4uiNV)(GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
	void APIENTRY (*ProgramEnvParameterI4uivNV)(GLenum target, GLuint index, const GLuint *params);
	void APIENTRY (*ProgramEnvParameters4fvEXT)(GLenum target, GLuint index, GLsizei count, const GLfloat *params);
	void APIENTRY (*ProgramEnvParametersI4ivNV)(GLenum target, GLuint index, GLsizei count, const GLint *params);
	void APIENTRY (*ProgramEnvParametersI4uivNV)(GLenum target, GLuint index, GLsizei count, const GLuint *params);
	void APIENTRY (*ProgramLocalParameterI4iNV)(GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*ProgramLocalParameterI4ivNV)(GLenum target, GLuint index, const GLint *params);
	void APIENTRY (*ProgramLocalParameterI4uiNV)(GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
	void APIENTRY (*ProgramLocalParameterI4uivNV)(GLenum target, GLuint index, const GLuint *params);
	void APIENTRY (*ProgramLocalParameters4fvEXT)(GLenum target, GLuint index, GLsizei count, const GLfloat *params);
	void APIENTRY (*ProgramLocalParametersI4ivNV)(GLenum target, GLuint index, GLsizei count, const GLint *params);
	void APIENTRY (*ProgramLocalParametersI4uivNV)(GLenum target, GLuint index, GLsizei count, const GLuint *params);
	void APIENTRY (*ProgramParameteri)(GLuint program, GLenum pname, GLint value);
	void APIENTRY (*ProgramParameteriARB)(GLuint program, GLenum pname, GLint value);
	void APIENTRY (*ProgramParameteriEXT)(GLuint program, GLenum pname, GLint value);
	void APIENTRY (*ProgramSubroutineParametersuivNV)(GLenum target, GLsizei count, const GLuint *params);
	void APIENTRY (*ProgramUniform1d)(GLuint program, GLint location, GLdouble v0);
	void APIENTRY (*ProgramUniform1dEXT)(GLuint program, GLint location, GLdouble x);
	void APIENTRY (*ProgramUniform1dv)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*ProgramUniform1dvEXT)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*ProgramUniform1f)(GLuint program, GLint location, GLfloat v0);
	void APIENTRY (*ProgramUniform1fEXT)(GLuint program, GLint location, GLfloat v0);
	void APIENTRY (*ProgramUniform1fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*ProgramUniform1fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*ProgramUniform1i)(GLuint program, GLint location, GLint v0);
	void APIENTRY (*ProgramUniform1i64NV)(GLuint program, GLint location, GLint64EXT x);
	void APIENTRY (*ProgramUniform1i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
	void APIENTRY (*ProgramUniform1iEXT)(GLuint program, GLint location, GLint v0);
	void APIENTRY (*ProgramUniform1iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*ProgramUniform1ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*ProgramUniform1ui)(GLuint program, GLint location, GLuint v0);
	void APIENTRY (*ProgramUniform1ui64NV)(GLuint program, GLint location, GLuint64EXT x);
	void APIENTRY (*ProgramUniform1ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*ProgramUniform1uiEXT)(GLuint program, GLint location, GLuint v0);
	void APIENTRY (*ProgramUniform1uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*ProgramUniform1uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*ProgramUniform2d)(GLuint program, GLint location, GLdouble v0, GLdouble v1);
	void APIENTRY (*ProgramUniform2dEXT)(GLuint program, GLint location, GLdouble x, GLdouble y);
	void APIENTRY (*ProgramUniform2dv)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*ProgramUniform2dvEXT)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*ProgramUniform2f)(GLuint program, GLint location, GLfloat v0, GLfloat v1);
	void APIENTRY (*ProgramUniform2fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1);
	void APIENTRY (*ProgramUniform2fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*ProgramUniform2fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*ProgramUniform2i)(GLuint program, GLint location, GLint v0, GLint v1);
	void APIENTRY (*ProgramUniform2i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y);
	void APIENTRY (*ProgramUniform2i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
	void APIENTRY (*ProgramUniform2iEXT)(GLuint program, GLint location, GLint v0, GLint v1);
	void APIENTRY (*ProgramUniform2iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*ProgramUniform2ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*ProgramUniform2ui)(GLuint program, GLint location, GLuint v0, GLuint v1);
	void APIENTRY (*ProgramUniform2ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y);
	void APIENTRY (*ProgramUniform2ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*ProgramUniform2uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1);
	void APIENTRY (*ProgramUniform2uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*ProgramUniform2uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*ProgramUniform3d)(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2);
	void APIENTRY (*ProgramUniform3dEXT)(GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*ProgramUniform3dv)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*ProgramUniform3dvEXT)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*ProgramUniform3f)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	void APIENTRY (*ProgramUniform3fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	void APIENTRY (*ProgramUniform3fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*ProgramUniform3fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*ProgramUniform3i)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
	void APIENTRY (*ProgramUniform3i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z);
	void APIENTRY (*ProgramUniform3i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
	void APIENTRY (*ProgramUniform3iEXT)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
	void APIENTRY (*ProgramUniform3iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*ProgramUniform3ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*ProgramUniform3ui)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
	void APIENTRY (*ProgramUniform3ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
	void APIENTRY (*ProgramUniform3ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*ProgramUniform3uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
	void APIENTRY (*ProgramUniform3uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*ProgramUniform3uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*ProgramUniform4d)(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
	void APIENTRY (*ProgramUniform4dEXT)(GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*ProgramUniform4dv)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*ProgramUniform4dvEXT)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*ProgramUniform4f)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void APIENTRY (*ProgramUniform4fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void APIENTRY (*ProgramUniform4fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*ProgramUniform4fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*ProgramUniform4i)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	void APIENTRY (*ProgramUniform4i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
	void APIENTRY (*ProgramUniform4i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
	void APIENTRY (*ProgramUniform4iEXT)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	void APIENTRY (*ProgramUniform4iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*ProgramUniform4ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*ProgramUniform4ui)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
	void APIENTRY (*ProgramUniform4ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
	void APIENTRY (*ProgramUniform4ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*ProgramUniform4uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
	void APIENTRY (*ProgramUniform4uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*ProgramUniform4uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*ProgramUniformHandleui64ARB)(GLuint program, GLint location, GLuint64 value);
	void APIENTRY (*ProgramUniformHandleui64NV)(GLuint program, GLint location, GLuint64 value);
	void APIENTRY (*ProgramUniformHandleui64vARB)(GLuint program, GLint location, GLsizei count, const GLuint64 *values);
	void APIENTRY (*ProgramUniformHandleui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64 *values);
	void APIENTRY (*ProgramUniformMatrix2dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix2dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix2fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix2x3dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix2x3dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix2x3fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix2x3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix2x4dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix2x4dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix2x4fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix2x4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix3dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix3dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix3fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix3x2dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix3x2dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix3x2fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix3x2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix3x4dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix3x4dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix3x4fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix3x4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix4dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix4dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix4fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix4x2dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix4x2dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix4x2fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix4x2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix4x3dv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix4x3dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*ProgramUniformMatrix4x3fv)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformMatrix4x3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*ProgramUniformui64NV)(GLuint program, GLint location, GLuint64EXT value);
	void APIENTRY (*ProgramUniformui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*ProgramVertexLimitNV)(GLenum target, GLint limit);
	void APIENTRY (*ProvokingVertex)(GLenum mode);
	void APIENTRY (*ProvokingVertexEXT)(GLenum mode);
	void APIENTRY (*PushClientAttribDefaultEXT)(GLbitfield mask);
	void APIENTRY (*PushDebugGroup)(GLenum source, GLuint id, GLsizei length, const GLchar *message);
	void APIENTRY (*PushGroupMarkerEXT)(GLsizei length, const GLchar *marker);
	void APIENTRY (*QueryCounter)(GLuint id, GLenum target);
	GLbitfield APIENTRY (*QueryMatrixxOES)(GLfixed *mantissa, GLint *exponent);
	void APIENTRY (*QueryObjectParameteruiAMD)(GLenum target, GLuint id, GLenum pname, GLuint param);
	void APIENTRY (*RasterPos2xOES)(GLfixed x, GLfixed y);
	void APIENTRY (*RasterPos2xvOES)(const GLfixed *coords);
	void APIENTRY (*RasterPos3xOES)(GLfixed x, GLfixed y, GLfixed z);
	void APIENTRY (*RasterPos3xvOES)(const GLfixed *coords);
	void APIENTRY (*RasterPos4xOES)(GLfixed x, GLfixed y, GLfixed z, GLfixed w);
	void APIENTRY (*RasterPos4xvOES)(const GLfixed *coords);
	void APIENTRY (*ReadnPixelsARB)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
	void APIENTRY (*RectxOES)(GLfixed x1, GLfixed y1, GLfixed x2, GLfixed y2);
	void APIENTRY (*RectxvOES)(const GLfixed *v1, const GLfixed *v2);
	void APIENTRY (*ReleaseShaderCompiler)(void);
	void APIENTRY (*RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*RenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*RenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*RenderbufferStorageMultisampleCoverageNV)(GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*RenderbufferStorageMultisampleEXT)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*ResumeTransformFeedback)(void);
	void APIENTRY (*ResumeTransformFeedbackNV)(void);
	void APIENTRY (*RotatexOES)(GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
	void APIENTRY (*SampleCoverageOES)(GLfixed value, GLboolean32 invert);
	void APIENTRY (*SampleMaskIndexedNV)(GLuint index, GLbitfield mask);
	void APIENTRY (*SampleMaski)(GLuint maskNumber, GLbitfield mask);
	void APIENTRY (*SamplePass)(GLenum mode);
	void APIENTRY (*SamplePassARB)(GLenum mode);
	void APIENTRY (*SamplerParameterIiv)(GLuint sampler, GLenum pname, const GLint *param);
	void APIENTRY (*SamplerParameterIuiv)(GLuint sampler, GLenum pname, const GLuint *param);
	void APIENTRY (*SamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param);
	void APIENTRY (*SamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat *param);
	void APIENTRY (*SamplerParameteri)(GLuint sampler, GLenum pname, GLint param);
	void APIENTRY (*SamplerParameteriv)(GLuint sampler, GLenum pname, const GLint *param);
	void APIENTRY (*ScalexOES)(GLfixed x, GLfixed y, GLfixed z);
	void APIENTRY (*ScissorArrayv)(GLuint first, GLsizei count, const GLint *v);
	void APIENTRY (*ScissorIndexed)(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
	void APIENTRY (*ScissorIndexedv)(GLuint index, const GLint *v);
	void APIENTRY (*SecondaryColorFormatNV)(GLint size, GLenum type, GLsizei stride);
	void APIENTRY (*SecondaryColorP3ui)(GLenum type, GLuint color);
	void APIENTRY (*SecondaryColorP3uiv)(GLenum type, const GLuint *color);
	void APIENTRY (*SelectPerfMonitorCountersAMD)(GLuint monitor, GLboolean32 enable, GLuint group, GLint numCounters, GLuint *counterList);
	void APIENTRY (*SetMultisamplefvAMD)(GLenum pname, GLuint index, const GLfloat *val);
	void APIENTRY (*ShaderBinary)(GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length);
	void APIENTRY (*ShaderSource)(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length);
	void APIENTRY (*ShaderStorageBlockBinding)(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding);
	void APIENTRY (*StencilClearTagEXT)(GLsizei stencilTagBits, GLuint stencilClearTag);
	void APIENTRY (*StencilFillPathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues);
	void APIENTRY (*StencilFillPathNV)(GLuint path, GLenum fillMode, GLuint mask);
	void APIENTRY (*StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
	void APIENTRY (*StencilMaskSeparate)(GLenum face, GLuint mask);
	void APIENTRY (*StencilOpSeparate)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
	void APIENTRY (*StencilOpValueAMD)(GLenum face, GLuint value);
	void APIENTRY (*StencilStrokePathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues);
	void APIENTRY (*StencilStrokePathNV)(GLuint path, GLint reference, GLuint mask);
	void APIENTRY (*StringMarkerGREMEDY)(GLsizei len, const void *string);
	void APIENTRY (*SwapAPPLE)(void);
	void APIENTRY (*SyncTextureINTEL)(GLuint texture);
	void APIENTRY (*TessellationFactorAMD)(GLfloat factor);
	void APIENTRY (*TessellationModeAMD)(GLenum mode);
	void APIENTRY (*TexBuffer)(GLenum target, GLenum internalformat, GLuint buffer);
	void APIENTRY (*TexBufferARB)(GLenum target, GLenum internalformat, GLuint buffer);
	void APIENTRY (*TexBufferEXT)(GLenum target, GLenum internalformat, GLuint buffer);
	void APIENTRY (*TexBufferRange)(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
	void APIENTRY (*TexCoord1bOES)(GLbyte32 s);
	void APIENTRY (*TexCoord1bvOES)(const GLbyte *coords);
	void APIENTRY (*TexCoord1xOES)(GLfixed s);
	void APIENTRY (*TexCoord1xvOES)(const GLfixed *coords);
	void APIENTRY (*TexCoord2bOES)(GLbyte32 s, GLbyte32 t);
	void APIENTRY (*TexCoord2bvOES)(const GLbyte *coords);
	void APIENTRY (*TexCoord2xOES)(GLfixed s, GLfixed t);
	void APIENTRY (*TexCoord2xvOES)(const GLfixed *coords);
	void APIENTRY (*TexCoord3bOES)(GLbyte32 s, GLbyte32 t, GLbyte32 r);
	void APIENTRY (*TexCoord3bvOES)(const GLbyte *coords);
	void APIENTRY (*TexCoord3xOES)(GLfixed s, GLfixed t, GLfixed r);
	void APIENTRY (*TexCoord3xvOES)(const GLfixed *coords);
	void APIENTRY (*TexCoord4bOES)(GLbyte32 s, GLbyte32 t, GLbyte32 r, GLbyte32 q);
	void APIENTRY (*TexCoord4bvOES)(const GLbyte *coords);
	void APIENTRY (*TexCoord4xOES)(GLfixed s, GLfixed t, GLfixed r, GLfixed q);
	void APIENTRY (*TexCoord4xvOES)(const GLfixed *coords);
	void APIENTRY (*TexCoordFormatNV)(GLint size, GLenum type, GLsizei stride);
	void APIENTRY (*TexCoordP1ui)(GLenum type, GLuint coords);
	void APIENTRY (*TexCoordP1uiv)(GLenum type, const GLuint *coords);
	void APIENTRY (*TexCoordP2ui)(GLenum type, GLuint coords);
	void APIENTRY (*TexCoordP2uiv)(GLenum type, const GLuint *coords);
	void APIENTRY (*TexCoordP3ui)(GLenum type, GLuint coords);
	void APIENTRY (*TexCoordP3uiv)(GLenum type, const GLuint *coords);
	void APIENTRY (*TexCoordP4ui)(GLenum type, GLuint coords);
	void APIENTRY (*TexCoordP4uiv)(GLenum type, const GLuint *coords);
	void APIENTRY (*TexEnvxOES)(GLenum target, GLenum pname, GLfixed param);
	void APIENTRY (*TexEnvxvOES)(GLenum target, GLenum pname, const GLfixed *params);
	void APIENTRY (*TexGenxOES)(GLenum coord, GLenum pname, GLfixed param);
	void APIENTRY (*TexGenxvOES)(GLenum coord, GLenum pname, const GLfixed *params);
	void APIENTRY (*TexImage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean32 fixedsamplelocations);
	void APIENTRY (*TexImage2DMultisampleCoverageNV)(GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean32 fixedSampleLocations);
	void APIENTRY (*TexImage3DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 fixedsamplelocations);
	void APIENTRY (*TexImage3DMultisampleCoverageNV)(GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 fixedSampleLocations);
	void APIENTRY (*TexPageCommitmentARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 commit);
	void APIENTRY (*TexParameterIiv)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*TexParameterIivEXT)(GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*TexParameterIuiv)(GLenum target, GLenum pname, const GLuint *params);
	void APIENTRY (*TexParameterIuivEXT)(GLenum target, GLenum pname, const GLuint *params);
	void APIENTRY (*TexParameterxOES)(GLenum target, GLenum pname, GLfixed param);
	void APIENTRY (*TexParameterxvOES)(GLenum target, GLenum pname, const GLfixed *params);
	void APIENTRY (*TexRenderbufferNV)(GLenum target, GLuint renderbuffer);
	void APIENTRY (*TexScissorFuncINTEL)(GLenum target, GLenum lfunc, GLenum hfunc);
	void APIENTRY (*TexScissorINTEL)(GLenum target, GLclampf tlow, GLclampf thigh);
	void APIENTRY (*TexStorage1D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
	void APIENTRY (*TexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*TexStorage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean32 fixedsamplelocations);
	void APIENTRY (*TexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	void APIENTRY (*TexStorage3DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 fixedsamplelocations);
	void APIENTRY (*TexStorageSparseAMD)(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags);
	void APIENTRY (*TextureBarrierNV)(void);
	void APIENTRY (*TextureBufferEXT)(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer);
	void APIENTRY (*TextureBufferRangeEXT)(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
	void APIENTRY (*TextureFogSGIX)(GLenum pname);
	void APIENTRY (*TextureImage1DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TextureImage2DMultisampleCoverageNV)(GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean32 fixedSampleLocations);
	void APIENTRY (*TextureImage2DMultisampleNV)(GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean32 fixedSampleLocations);
	void APIENTRY (*TextureImage3DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TextureImage3DMultisampleCoverageNV)(GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 fixedSampleLocations);
	void APIENTRY (*TextureImage3DMultisampleNV)(GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 fixedSampleLocations);
	void APIENTRY (*TexturePageCommitmentEXT)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 commit);
	void APIENTRY (*TextureParameterIivEXT)(GLuint texture, GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*TextureParameterIuivEXT)(GLuint texture, GLenum target, GLenum pname, const GLuint *params);
	void APIENTRY (*TextureParameterfEXT)(GLuint texture, GLenum target, GLenum pname, GLfloat param);
	void APIENTRY (*TextureParameterfvEXT)(GLuint texture, GLenum target, GLenum pname, const GLfloat *params);
	void APIENTRY (*TextureParameteriEXT)(GLuint texture, GLenum target, GLenum pname, GLint param);
	void APIENTRY (*TextureParameterivEXT)(GLuint texture, GLenum target, GLenum pname, const GLint *params);
	void APIENTRY (*TextureRangeAPPLE)(GLenum target, GLsizei length, const void *pointer);
	void APIENTRY (*TextureRenderbufferEXT)(GLuint texture, GLenum target, GLuint renderbuffer);
	void APIENTRY (*TextureStorage1DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
	void APIENTRY (*TextureStorage2DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*TextureStorage2DMultisampleEXT)(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean32 fixedsamplelocations);
	void APIENTRY (*TextureStorage3DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	void APIENTRY (*TextureStorage3DMultisampleEXT)(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 fixedsamplelocations);
	void APIENTRY (*TextureStorageSparseAMD)(GLuint texture, GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags);
	void APIENTRY (*TextureSubImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TextureSubImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TextureView)(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);
	void APIENTRY (*TransformFeedbackAttribsNV)(GLsizei count, const GLint *attribs, GLenum bufferMode);
	void APIENTRY (*TransformFeedbackStreamAttribsNV)(GLsizei count, const GLint *attribs, GLsizei nbuffers, const GLint *bufstreams, GLenum bufferMode);
	void APIENTRY (*TransformFeedbackVaryings)(GLuint program, GLsizei count, const GLchar *const *varyings, GLenum bufferMode);
	void APIENTRY (*TransformFeedbackVaryingsEXT)(GLuint program, GLsizei count, const GLchar *const *varyings, GLenum bufferMode);
	void APIENTRY (*TransformFeedbackVaryingsNV)(GLuint program, GLsizei count, const GLint *locations, GLenum bufferMode);
	void APIENTRY (*TransformPathNV)(GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues);
	void APIENTRY (*TranslatexOES)(GLfixed x, GLfixed y, GLfixed z);
	void APIENTRY (*Uniform1d)(GLint location, GLdouble x);
	void APIENTRY (*Uniform1dv)(GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*Uniform1f)(GLint location, GLfloat v0);
	void APIENTRY (*Uniform1fv)(GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*Uniform1i)(GLint location, GLint v0);
	void APIENTRY (*Uniform1i64NV)(GLint location, GLint64EXT x);
	void APIENTRY (*Uniform1i64vNV)(GLint location, GLsizei count, const GLint64EXT *value);
	void APIENTRY (*Uniform1iv)(GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*Uniform1ui)(GLint location, GLuint v0);
	void APIENTRY (*Uniform1ui64NV)(GLint location, GLuint64EXT x);
	void APIENTRY (*Uniform1ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*Uniform1uiEXT)(GLint location, GLuint v0);
	void APIENTRY (*Uniform1uiv)(GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*Uniform1uivEXT)(GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*Uniform2d)(GLint location, GLdouble x, GLdouble y);
	void APIENTRY (*Uniform2dv)(GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*Uniform2f)(GLint location, GLfloat v0, GLfloat v1);
	void APIENTRY (*Uniform2fv)(GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*Uniform2i)(GLint location, GLint v0, GLint v1);
	void APIENTRY (*Uniform2i64NV)(GLint location, GLint64EXT x, GLint64EXT y);
	void APIENTRY (*Uniform2i64vNV)(GLint location, GLsizei count, const GLint64EXT *value);
	void APIENTRY (*Uniform2iv)(GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*Uniform2ui)(GLint location, GLuint v0, GLuint v1);
	void APIENTRY (*Uniform2ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y);
	void APIENTRY (*Uniform2ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*Uniform2uiEXT)(GLint location, GLuint v0, GLuint v1);
	void APIENTRY (*Uniform2uiv)(GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*Uniform2uivEXT)(GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*Uniform3d)(GLint location, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*Uniform3dv)(GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*Uniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	void APIENTRY (*Uniform3fv)(GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*Uniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
	void APIENTRY (*Uniform3i64NV)(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z);
	void APIENTRY (*Uniform3i64vNV)(GLint location, GLsizei count, const GLint64EXT *value);
	void APIENTRY (*Uniform3iv)(GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*Uniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
	void APIENTRY (*Uniform3ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
	void APIENTRY (*Uniform3ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*Uniform3uiEXT)(GLint location, GLuint v0, GLuint v1, GLuint v2);
	void APIENTRY (*Uniform3uiv)(GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*Uniform3uivEXT)(GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*Uniform4d)(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*Uniform4dv)(GLint location, GLsizei count, const GLdouble *value);
	void APIENTRY (*Uniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void APIENTRY (*Uniform4fv)(GLint location, GLsizei count, const GLfloat *value);
	void APIENTRY (*Uniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	void APIENTRY (*Uniform4i64NV)(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
	void APIENTRY (*Uniform4i64vNV)(GLint location, GLsizei count, const GLint64EXT *value);
	void APIENTRY (*Uniform4iv)(GLint location, GLsizei count, const GLint *value);
	void APIENTRY (*Uniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
	void APIENTRY (*Uniform4ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
	void APIENTRY (*Uniform4ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
	void APIENTRY (*Uniform4uiEXT)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
	void APIENTRY (*Uniform4uiv)(GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*Uniform4uivEXT)(GLint location, GLsizei count, const GLuint *value);
	void APIENTRY (*UniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
	void APIENTRY (*UniformBufferEXT)(GLuint program, GLint location, GLuint buffer);
	void APIENTRY (*UniformHandleui64ARB)(GLint location, GLuint64 value);
	void APIENTRY (*UniformHandleui64NV)(GLint location, GLuint64 value);
	void APIENTRY (*UniformHandleui64vARB)(GLint location, GLsizei count, const GLuint64 *value);
	void APIENTRY (*UniformHandleui64vNV)(GLint location, GLsizei count, const GLuint64 *value);
	void APIENTRY (*UniformMatrix2dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix2fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix2x3dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix2x4dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix3dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix3fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix3x2dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix3x4dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix4dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix4fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix4x2dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformMatrix4x3dv)(GLint location, GLsizei count, GLboolean32 transpose, const GLdouble *value);
	void APIENTRY (*UniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean32 transpose, const GLfloat *value);
	void APIENTRY (*UniformSubroutinesuiv)(GLenum shadertype, GLsizei count, const GLuint *indices);
	void APIENTRY (*Uniformui64NV)(GLint location, GLuint64EXT value);
	void APIENTRY (*Uniformui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
	GLboolean APIENTRY (*UnmapNamedBufferEXT)(GLuint buffer);
	void APIENTRY (*UnmapTexture2DINTEL)(GLuint texture, GLint level);
	void APIENTRY (*UseProgram)(GLuint program);
	void APIENTRY (*UseProgramStages)(GLuint pipeline, GLbitfield stages, GLuint program);
	void APIENTRY (*UseShaderProgramEXT)(GLenum type, GLuint program);
	void APIENTRY (*VDPAUFiniNV)(void);
	void APIENTRY (*VDPAUGetSurfaceivNV)(GLvdpauSurfaceNV surface, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
	void APIENTRY (*VDPAUInitNV)(const void *vdpDevice, const void *getProcAddress);
	GLboolean APIENTRY (*VDPAUIsSurfaceNV)(GLvdpauSurfaceNV surface);
	void APIENTRY (*VDPAUMapSurfacesNV)(GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces);
	GLvdpauSurfaceNV APIENTRY (*VDPAURegisterOutputSurfaceNV)(const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
	GLvdpauSurfaceNV APIENTRY (*VDPAURegisterVideoSurfaceNV)(const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
	void APIENTRY (*VDPAUSurfaceAccessNV)(GLvdpauSurfaceNV surface, GLenum access);
	void APIENTRY (*VDPAUUnmapSurfacesNV)(GLsizei numSurface, const GLvdpauSurfaceNV *surfaces);
	void APIENTRY (*VDPAUUnregisterSurfaceNV)(GLvdpauSurfaceNV surface);
	void APIENTRY (*ValidateProgram)(GLuint program);
	void APIENTRY (*ValidateProgramPipeline)(GLuint pipeline);
	void APIENTRY (*Vertex2bOES)(GLbyte32 x, GLbyte32 y);
	void APIENTRY (*Vertex2bvOES)(const GLbyte *coords);
	void APIENTRY (*Vertex2xOES)(GLfixed x);
	void APIENTRY (*Vertex2xvOES)(const GLfixed *coords);
	void APIENTRY (*Vertex3bOES)(GLbyte32 x, GLbyte32 y, GLbyte32 z);
	void APIENTRY (*Vertex3bvOES)(const GLbyte *coords);
	void APIENTRY (*Vertex3xOES)(GLfixed x, GLfixed y);
	void APIENTRY (*Vertex3xvOES)(const GLfixed *coords);
	void APIENTRY (*Vertex4bOES)(GLbyte32 x, GLbyte32 y, GLbyte32 z, GLbyte32 w);
	void APIENTRY (*Vertex4bvOES)(const GLbyte *coords);
	void APIENTRY (*Vertex4xOES)(GLfixed x, GLfixed y, GLfixed z);
	void APIENTRY (*Vertex4xvOES)(const GLfixed *coords);
	void APIENTRY (*VertexArrayBindVertexBufferEXT)(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
	void APIENTRY (*VertexArrayColorOffsetEXT)(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayEdgeFlagOffsetEXT)(GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayFogCoordOffsetEXT)(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayIndexOffsetEXT)(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayMultiTexCoordOffsetEXT)(GLuint vaobj, GLuint buffer, GLenum texunit, GLint size, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayNormalOffsetEXT)(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArraySecondaryColorOffsetEXT)(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayTexCoordOffsetEXT)(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayVertexAttribBindingEXT)(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
	void APIENTRY (*VertexArrayVertexAttribDivisorEXT)(GLuint vaobj, GLuint index, GLuint divisor);
	void APIENTRY (*VertexArrayVertexAttribFormatEXT)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean32 normalized, GLuint relativeoffset);
	void APIENTRY (*VertexArrayVertexAttribIFormatEXT)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	void APIENTRY (*VertexArrayVertexAttribIOffsetEXT)(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayVertexAttribLFormatEXT)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	void APIENTRY (*VertexArrayVertexAttribLOffsetEXT)(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayVertexAttribOffsetEXT)(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean32 normalized, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexArrayVertexBindingDivisorEXT)(GLuint vaobj, GLuint bindingindex, GLuint divisor);
	void APIENTRY (*VertexArrayVertexOffsetEXT)(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset);
	void APIENTRY (*VertexAttrib1d)(GLuint index, GLdouble x);
	void APIENTRY (*VertexAttrib1dv)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib1f)(GLuint index, GLfloat x);
	void APIENTRY (*VertexAttrib1fv)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib1s)(GLuint index, GLshort32 x);
	void APIENTRY (*VertexAttrib1sv)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib2d)(GLuint index, GLdouble x, GLdouble y);
	void APIENTRY (*VertexAttrib2dv)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib2f)(GLuint index, GLfloat x, GLfloat y);
	void APIENTRY (*VertexAttrib2fv)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib2s)(GLuint index, GLshort32 x, GLshort32 y);
	void APIENTRY (*VertexAttrib2sv)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*VertexAttrib3dv)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
	void APIENTRY (*VertexAttrib3fv)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib3s)(GLuint index, GLshort32 x, GLshort32 y, GLshort32 z);
	void APIENTRY (*VertexAttrib3sv)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib4Nbv)(GLuint index, const GLbyte *v);
	void APIENTRY (*VertexAttrib4Niv)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttrib4Nsv)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib4Nub)(GLuint index, GLubyte32 x, GLubyte32 y, GLubyte32 z, GLubyte32 w);
	void APIENTRY (*VertexAttrib4Nubv)(GLuint index, const GLubyte *v);
	void APIENTRY (*VertexAttrib4Nuiv)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttrib4Nusv)(GLuint index, const GLushort *v);
	void APIENTRY (*VertexAttrib4bv)(GLuint index, const GLbyte *v);
	void APIENTRY (*VertexAttrib4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*VertexAttrib4dv)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void APIENTRY (*VertexAttrib4fv)(GLuint index, const GLfloat *v);
	void APIENTRY (*VertexAttrib4iv)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttrib4s)(GLuint index, GLshort32 x, GLshort32 y, GLshort32 z, GLshort32 w);
	void APIENTRY (*VertexAttrib4sv)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttrib4ubv)(GLuint index, const GLubyte *v);
	void APIENTRY (*VertexAttrib4uiv)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttrib4usv)(GLuint index, const GLushort *v);
	void APIENTRY (*VertexAttribBinding)(GLuint attribindex, GLuint bindingindex);
	void APIENTRY (*VertexAttribDivisor)(GLuint index, GLuint divisor);
	void APIENTRY (*VertexAttribDivisorARB)(GLuint index, GLuint divisor);
	void APIENTRY (*VertexAttribFormat)(GLuint attribindex, GLint size, GLenum type, GLboolean32 normalized, GLuint relativeoffset);
	void APIENTRY (*VertexAttribFormatNV)(GLuint index, GLint size, GLenum type, GLboolean32 normalized, GLsizei stride);
	void APIENTRY (*VertexAttribI1i)(GLuint index, GLint x);
	void APIENTRY (*VertexAttribI1iEXT)(GLuint index, GLint x);
	void APIENTRY (*VertexAttribI1iv)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttribI1ivEXT)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttribI1ui)(GLuint index, GLuint x);
	void APIENTRY (*VertexAttribI1uiEXT)(GLuint index, GLuint x);
	void APIENTRY (*VertexAttribI1uiv)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttribI1uivEXT)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttribI2i)(GLuint index, GLint x, GLint y);
	void APIENTRY (*VertexAttribI2iEXT)(GLuint index, GLint x, GLint y);
	void APIENTRY (*VertexAttribI2iv)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttribI2ivEXT)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttribI2ui)(GLuint index, GLuint x, GLuint y);
	void APIENTRY (*VertexAttribI2uiEXT)(GLuint index, GLuint x, GLuint y);
	void APIENTRY (*VertexAttribI2uiv)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttribI2uivEXT)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttribI3i)(GLuint index, GLint x, GLint y, GLint z);
	void APIENTRY (*VertexAttribI3iEXT)(GLuint index, GLint x, GLint y, GLint z);
	void APIENTRY (*VertexAttribI3iv)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttribI3ivEXT)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttribI3ui)(GLuint index, GLuint x, GLuint y, GLuint z);
	void APIENTRY (*VertexAttribI3uiEXT)(GLuint index, GLuint x, GLuint y, GLuint z);
	void APIENTRY (*VertexAttribI3uiv)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttribI3uivEXT)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttribI4bv)(GLuint index, const GLbyte *v);
	void APIENTRY (*VertexAttribI4bvEXT)(GLuint index, const GLbyte *v);
	void APIENTRY (*VertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*VertexAttribI4iEXT)(GLuint index, GLint x, GLint y, GLint z, GLint w);
	void APIENTRY (*VertexAttribI4iv)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttribI4ivEXT)(GLuint index, const GLint *v);
	void APIENTRY (*VertexAttribI4sv)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttribI4svEXT)(GLuint index, const GLshort *v);
	void APIENTRY (*VertexAttribI4ubv)(GLuint index, const GLubyte *v);
	void APIENTRY (*VertexAttribI4ubvEXT)(GLuint index, const GLubyte *v);
	void APIENTRY (*VertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
	void APIENTRY (*VertexAttribI4uiEXT)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
	void APIENTRY (*VertexAttribI4uiv)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttribI4uivEXT)(GLuint index, const GLuint *v);
	void APIENTRY (*VertexAttribI4usv)(GLuint index, const GLushort *v);
	void APIENTRY (*VertexAttribI4usvEXT)(GLuint index, const GLushort *v);
	void APIENTRY (*VertexAttribIFormat)(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	void APIENTRY (*VertexAttribIFormatNV)(GLuint index, GLint size, GLenum type, GLsizei stride);
	void APIENTRY (*VertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*VertexAttribIPointerEXT)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*VertexAttribL1d)(GLuint index, GLdouble x);
	void APIENTRY (*VertexAttribL1dEXT)(GLuint index, GLdouble x);
	void APIENTRY (*VertexAttribL1dv)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttribL1dvEXT)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttribL1i64NV)(GLuint index, GLint64EXT x);
	void APIENTRY (*VertexAttribL1i64vNV)(GLuint index, const GLint64EXT *v);
	void APIENTRY (*VertexAttribL1ui64ARB)(GLuint index, GLuint64EXT x);
	void APIENTRY (*VertexAttribL1ui64NV)(GLuint index, GLuint64EXT x);
	void APIENTRY (*VertexAttribL1ui64vARB)(GLuint index, const GLuint64EXT *v);
	void APIENTRY (*VertexAttribL1ui64vNV)(GLuint index, const GLuint64EXT *v);
	void APIENTRY (*VertexAttribL2d)(GLuint index, GLdouble x, GLdouble y);
	void APIENTRY (*VertexAttribL2dEXT)(GLuint index, GLdouble x, GLdouble y);
	void APIENTRY (*VertexAttribL2dv)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttribL2dvEXT)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttribL2i64NV)(GLuint index, GLint64EXT x, GLint64EXT y);
	void APIENTRY (*VertexAttribL2i64vNV)(GLuint index, const GLint64EXT *v);
	void APIENTRY (*VertexAttribL2ui64NV)(GLuint index, GLuint64EXT x, GLuint64EXT y);
	void APIENTRY (*VertexAttribL2ui64vNV)(GLuint index, const GLuint64EXT *v);
	void APIENTRY (*VertexAttribL3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*VertexAttribL3dEXT)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	void APIENTRY (*VertexAttribL3dv)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttribL3dvEXT)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttribL3i64NV)(GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z);
	void APIENTRY (*VertexAttribL3i64vNV)(GLuint index, const GLint64EXT *v);
	void APIENTRY (*VertexAttribL3ui64NV)(GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
	void APIENTRY (*VertexAttribL3ui64vNV)(GLuint index, const GLuint64EXT *v);
	void APIENTRY (*VertexAttribL4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*VertexAttribL4dEXT)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void APIENTRY (*VertexAttribL4dv)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttribL4dvEXT)(GLuint index, const GLdouble *v);
	void APIENTRY (*VertexAttribL4i64NV)(GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
	void APIENTRY (*VertexAttribL4i64vNV)(GLuint index, const GLint64EXT *v);
	void APIENTRY (*VertexAttribL4ui64NV)(GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
	void APIENTRY (*VertexAttribL4ui64vNV)(GLuint index, const GLuint64EXT *v);
	void APIENTRY (*VertexAttribLFormat)(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	void APIENTRY (*VertexAttribLFormatNV)(GLuint index, GLint size, GLenum type, GLsizei stride);
	void APIENTRY (*VertexAttribLPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*VertexAttribLPointerEXT)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
	void APIENTRY (*VertexAttribP1ui)(GLuint index, GLenum type, GLboolean32 normalized, GLuint value);
	void APIENTRY (*VertexAttribP1uiv)(GLuint index, GLenum type, GLboolean32 normalized, const GLuint *value);
	void APIENTRY (*VertexAttribP2ui)(GLuint index, GLenum type, GLboolean32 normalized, GLuint value);
	void APIENTRY (*VertexAttribP2uiv)(GLuint index, GLenum type, GLboolean32 normalized, const GLuint *value);
	void APIENTRY (*VertexAttribP3ui)(GLuint index, GLenum type, GLboolean32 normalized, GLuint value);
	void APIENTRY (*VertexAttribP3uiv)(GLuint index, GLenum type, GLboolean32 normalized, const GLuint *value);
	void APIENTRY (*VertexAttribP4ui)(GLuint index, GLenum type, GLboolean32 normalized, GLuint value);
	void APIENTRY (*VertexAttribP4uiv)(GLuint index, GLenum type, GLboolean32 normalized, const GLuint *value);
	void APIENTRY (*VertexAttribParameteriAMD)(GLuint index, GLenum pname, GLint param);
	void APIENTRY (*VertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean32 normalized, GLsizei stride, const void *pointer);
	void APIENTRY (*VertexBindingDivisor)(GLuint bindingindex, GLuint divisor);
	void APIENTRY (*VertexFormatNV)(GLint size, GLenum type, GLsizei stride);
	void APIENTRY (*VertexP2ui)(GLenum type, GLuint value);
	void APIENTRY (*VertexP2uiv)(GLenum type, const GLuint *value);
	void APIENTRY (*VertexP3ui)(GLenum type, GLuint value);
	void APIENTRY (*VertexP3uiv)(GLenum type, const GLuint *value);
	void APIENTRY (*VertexP4ui)(GLenum type, GLuint value);
	void APIENTRY (*VertexP4uiv)(GLenum type, const GLuint *value);
	void APIENTRY (*VertexPointSizefAPPLE)(GLfloat size);
	GLenum APIENTRY (*VideoCaptureNV)(GLuint video_capture_slot, GLuint *sequence_num, GLuint64EXT *capture_time);
	void APIENTRY (*VideoCaptureStreamParameterdvNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble *params);
	void APIENTRY (*VideoCaptureStreamParameterfvNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat *params);
	void APIENTRY (*VideoCaptureStreamParameterivNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint *params);
	void APIENTRY (*ViewportArrayv)(GLuint first, GLsizei count, const GLfloat *v);
	void APIENTRY (*ViewportIndexedf)(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
	void APIENTRY (*ViewportIndexedfv)(GLuint index, const GLfloat *v);
	void APIENTRY (*WaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
	void APIENTRY (*WeightPathsNV)(GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights);
	void APIENTRY (*BindTextureUnit)(GLuint unit, GLuint texture);
	void APIENTRY (*BlendBarrierKHR)(void);
	void APIENTRY (*BlitNamedFramebuffer)(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	void APIENTRY (*BufferPageCommitmentARB)(GLenum target, GLintptr offset, GLsizeiptr size, GLboolean32 commit);
	void APIENTRY (*CallCommandListNV)(GLuint list);
	GLenum APIENTRY (*CheckNamedFramebufferStatus)(GLuint framebuffer, GLenum target);
	void APIENTRY (*ClearNamedBufferData)(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data);
	void APIENTRY (*ClearNamedBufferSubData)(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
	void APIENTRY (*ClearNamedFramebufferfi)(GLuint framebuffer, GLenum buffer, GLfloat depth, GLint stencil);
	void APIENTRY (*ClearNamedFramebufferfv)(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value);
	void APIENTRY (*ClearNamedFramebufferiv)(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value);
	void APIENTRY (*ClearNamedFramebufferuiv)(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value);
	void APIENTRY (*ClipControl)(GLenum origin, GLenum depth);
	void APIENTRY (*CommandListSegmentsNV)(GLuint list, GLuint segments);
	void APIENTRY (*CompileCommandListNV)(GLuint list);
	void APIENTRY (*CompressedTextureSubImage1D)(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTextureSubImage2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*CompressedTextureSubImage3D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
	void APIENTRY (*CopyNamedBufferSubData)(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
	void APIENTRY (*CopyTextureSubImage1D)(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	void APIENTRY (*CopyTextureSubImage2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*CopyTextureSubImage3D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void APIENTRY (*CoverageModulationNV)(GLenum components);
	void APIENTRY (*CoverageModulationTableNV)(GLsizei n, const GLfloat *v);
	void APIENTRY (*CreateBuffers)(GLsizei n, GLuint *buffers);
	void APIENTRY (*CreateCommandListsNV)(GLsizei n, GLuint *lists);
	void APIENTRY (*CreateFramebuffers)(GLsizei n, GLuint *framebuffers);
	void APIENTRY (*CreateProgramPipelines)(GLsizei n, GLuint *pipelines);
	void APIENTRY (*CreateQueries)(GLenum target, GLsizei n, GLuint *ids);
	void APIENTRY (*CreateRenderbuffers)(GLsizei n, GLuint *renderbuffers);
	void APIENTRY (*CreateSamplers)(GLsizei n, GLuint *samplers);
	void APIENTRY (*CreateStatesNV)(GLsizei n, GLuint *states);
	void APIENTRY (*CreateTextures)(GLenum target, GLsizei n, GLuint *textures);
	void APIENTRY (*CreateTransformFeedbacks)(GLsizei n, GLuint *ids);
	void APIENTRY (*CreateVertexArrays)(GLsizei n, GLuint *arrays);
	void APIENTRY (*DeleteCommandListsNV)(GLsizei n, const GLuint *lists);
	void APIENTRY (*DeleteStatesNV)(GLsizei n, const GLuint *states);
	void APIENTRY (*DisableVertexArrayAttrib)(GLuint vaobj, GLuint index);
	void APIENTRY (*DrawCommandsAddressNV)(GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count);
	void APIENTRY (*DrawCommandsNV)(GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count);
	void APIENTRY (*DrawCommandsStatesAddressNV)(const GLuint64 *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
	void APIENTRY (*DrawCommandsStatesNV)(GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
	void APIENTRY (*EnableVertexArrayAttrib)(GLuint vaobj, GLuint index);
	void APIENTRY (*FlushMappedNamedBufferRange)(GLuint buffer, GLintptr offset, GLsizeiptr length);
	void APIENTRY (*FragmentCoverageColorNV)(GLuint color);
	void APIENTRY (*FramebufferSampleLocationsfvNV)(GLenum target, GLuint start, GLsizei count, const GLfloat *v);
	void APIENTRY (*FramebufferTextureMultiviewOVR)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews);
	void APIENTRY (*GenerateTextureMipmap)(GLuint texture);
	GLuint APIENTRY (*GetCommandHeaderNV)(GLenum tokenID, GLuint size);
	void APIENTRY (*GetCompressedTextureImage)(GLuint texture, GLint level, GLsizei bufSize, void *pixels);
	void APIENTRY (*GetCompressedTextureSubImage)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels);
	void APIENTRY (*GetCoverageModulationTableNV)(GLsizei bufsize, GLfloat *v);
	GLenum APIENTRY (*GetGraphicsResetStatus)(void);
	void APIENTRY (*GetInternalformatSampleivNV)(GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei bufSize, GLint *params);
	void APIENTRY (*GetNamedBufferParameteri64v)(GLuint buffer, GLenum pname, GLint64 *params);
	void APIENTRY (*GetNamedBufferParameteriv)(GLuint buffer, GLenum pname, GLint *params);
	void APIENTRY (*GetNamedBufferPointerv)(GLuint buffer, GLenum pname, void * *params);
	void APIENTRY (*GetNamedBufferSubData)(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data);
	void APIENTRY (*GetNamedFramebufferAttachmentParameteriv)(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params);
	void APIENTRY (*GetNamedFramebufferParameteriv)(GLuint framebuffer, GLenum pname, GLint *param);
	void APIENTRY (*GetNamedRenderbufferParameteriv)(GLuint renderbuffer, GLenum pname, GLint *params);
	void APIENTRY (*GetProgramResourcefvNV)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLfloat *params);
	void APIENTRY (*GetQueryBufferObjecti64v)(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	void APIENTRY (*GetQueryBufferObjectiv)(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	void APIENTRY (*GetQueryBufferObjectui64v)(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	void APIENTRY (*GetQueryBufferObjectuiv)(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	GLushort APIENTRY (*GetStageIndexNV)(GLenum shadertype);
	void APIENTRY (*GetTextureImage)(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
	void APIENTRY (*GetTextureLevelParameterfv)(GLuint texture, GLint level, GLenum pname, GLfloat *params);
	void APIENTRY (*GetTextureLevelParameteriv)(GLuint texture, GLint level, GLenum pname, GLint *params);
	void APIENTRY (*GetTextureParameterIiv)(GLuint texture, GLenum pname, GLint *params);
	void APIENTRY (*GetTextureParameterIuiv)(GLuint texture, GLenum pname, GLuint *params);
	void APIENTRY (*GetTextureParameterfv)(GLuint texture, GLenum pname, GLfloat *params);
	void APIENTRY (*GetTextureParameteriv)(GLuint texture, GLenum pname, GLint *params);
	void APIENTRY (*GetTextureSubImage)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
	void APIENTRY (*GetTransformFeedbacki64_v)(GLuint xfb, GLenum pname, GLuint index, GLint64 *param);
	void APIENTRY (*GetTransformFeedbacki_v)(GLuint xfb, GLenum pname, GLuint index, GLint *param);
	void APIENTRY (*GetTransformFeedbackiv)(GLuint xfb, GLenum pname, GLint *param);
	void APIENTRY (*GetVertexArrayIndexed64iv)(GLuint vaobj, GLuint index, GLenum pname, GLint64 *param);
	void APIENTRY (*GetVertexArrayIndexediv)(GLuint vaobj, GLuint index, GLenum pname, GLint *param);
	void APIENTRY (*GetVertexArrayiv)(GLuint vaobj, GLenum pname, GLint *param);
	void APIENTRY (*GetnColorTable)(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table);
	void APIENTRY (*GetnCompressedTexImage)(GLenum target, GLint lod, GLsizei bufSize, void *pixels);
	void APIENTRY (*GetnConvolutionFilter)(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image);
	void APIENTRY (*GetnHistogram)(GLenum target, GLboolean32 reset, GLenum format, GLenum type, GLsizei bufSize, void *values);
	void APIENTRY (*GetnMapdv)(GLenum target, GLenum query, GLsizei bufSize, GLdouble *v);
	void APIENTRY (*GetnMapfv)(GLenum target, GLenum query, GLsizei bufSize, GLfloat *v);
	void APIENTRY (*GetnMapiv)(GLenum target, GLenum query, GLsizei bufSize, GLint *v);
	void APIENTRY (*GetnMinmax)(GLenum target, GLboolean32 reset, GLenum format, GLenum type, GLsizei bufSize, void *values);
	void APIENTRY (*GetnPixelMapfv)(GLenum map, GLsizei bufSize, GLfloat *values);
	void APIENTRY (*GetnPixelMapuiv)(GLenum map, GLsizei bufSize, GLuint *values);
	void APIENTRY (*GetnPixelMapusv)(GLenum map, GLsizei bufSize, GLushort *values);
	void APIENTRY (*GetnPolygonStipple)(GLsizei bufSize, GLubyte *pattern);
	void APIENTRY (*GetnSeparableFilter)(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span);
	void APIENTRY (*GetnTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
	void APIENTRY (*GetnUniformdv)(GLuint program, GLint location, GLsizei bufSize, GLdouble *params);
	void APIENTRY (*GetnUniformfv)(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
	void APIENTRY (*GetnUniformiv)(GLuint program, GLint location, GLsizei bufSize, GLint *params);
	void APIENTRY (*GetnUniformuiv)(GLuint program, GLint location, GLsizei bufSize, GLuint *params);
	void APIENTRY (*InvalidateNamedFramebufferData)(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments);
	void APIENTRY (*InvalidateNamedFramebufferSubData)(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
	GLboolean APIENTRY (*IsCommandListNV)(GLuint list);
	GLboolean APIENTRY (*IsStateNV)(GLuint state);
	void APIENTRY (*ListDrawCommandsStatesClientNV)(GLuint list, GLuint segment, const void * *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
	void * APIENTRY (*MapNamedBuffer)(GLuint buffer, GLenum access);
	void * APIENTRY (*MapNamedBufferRange)(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
	void APIENTRY (*MatrixLoad3x2fNV)(GLenum matrixMode, const GLfloat *m);
	void APIENTRY (*MatrixLoad3x3fNV)(GLenum matrixMode, const GLfloat *m);
	void APIENTRY (*MatrixLoadTranspose3x3fNV)(GLenum matrixMode, const GLfloat *m);
	void APIENTRY (*MatrixMult3x2fNV)(GLenum matrixMode, const GLfloat *m);
	void APIENTRY (*MatrixMult3x3fNV)(GLenum matrixMode, const GLfloat *m);
	void APIENTRY (*MatrixMultTranspose3x3fNV)(GLenum matrixMode, const GLfloat *m);
	void APIENTRY (*MemoryBarrierByRegion)(GLbitfield barriers);
	void APIENTRY (*MultiDrawArraysIndirectBindlessCountNV)(GLenum mode, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount);
	void APIENTRY (*MultiDrawElementsIndirectBindlessCountNV)(GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount);
	void APIENTRY (*NamedBufferData)(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
	void APIENTRY (*NamedBufferPageCommitmentARB)(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean32 commit);
	void APIENTRY (*NamedBufferPageCommitmentEXT)(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean32 commit);
	void APIENTRY (*NamedBufferStorage)(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);
	void APIENTRY (*NamedBufferSubData)(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
	void APIENTRY (*NamedFramebufferDrawBuffer)(GLuint framebuffer, GLenum buf);
	void APIENTRY (*NamedFramebufferDrawBuffers)(GLuint framebuffer, GLsizei n, const GLenum *bufs);
	void APIENTRY (*NamedFramebufferParameteri)(GLuint framebuffer, GLenum pname, GLint param);
	void APIENTRY (*NamedFramebufferReadBuffer)(GLuint framebuffer, GLenum src);
	void APIENTRY (*NamedFramebufferRenderbuffer)(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	void APIENTRY (*NamedFramebufferSampleLocationsfvNV)(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v);
	void APIENTRY (*NamedFramebufferTexture)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
	void APIENTRY (*NamedFramebufferTextureLayer)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);
	void APIENTRY (*NamedRenderbufferStorage)(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*NamedRenderbufferStorageMultisample)(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	GLenum APIENTRY (*PathGlyphIndexArrayNV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
	GLenum APIENTRY (*PathGlyphIndexRangeNV)(GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint *baseAndCount);
	GLenum APIENTRY (*PathMemoryGlyphIndexArrayNV)(GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
	void APIENTRY (*PolygonOffsetClampEXT)(GLfloat factor, GLfloat units, GLfloat clamp);
	void APIENTRY (*ProgramPathFragmentInputGenNV)(GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs);
	void APIENTRY (*RasterSamplesEXT)(GLuint samples, GLboolean32 fixedsamplelocations);
	void APIENTRY (*ReadnPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
	void APIENTRY (*ResolveDepthValuesNV)(void);
	void APIENTRY (*StateCaptureNV)(GLuint state, GLenum mode);
	void APIENTRY (*StencilThenCoverFillPathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
	void APIENTRY (*StencilThenCoverFillPathNV)(GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode);
	void APIENTRY (*StencilThenCoverStrokePathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
	void APIENTRY (*StencilThenCoverStrokePathNV)(GLuint path, GLint reference, GLuint mask, GLenum coverMode);
	void APIENTRY (*SubpixelPrecisionBiasNV)(GLuint xbits, GLuint ybits);
	void APIENTRY (*TextureBarrier)(void);
	void APIENTRY (*TextureBuffer)(GLuint texture, GLenum internalformat, GLuint buffer);
	void APIENTRY (*TextureBufferRange)(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
	void APIENTRY (*TextureParameterIiv)(GLuint texture, GLenum pname, const GLint *params);
	void APIENTRY (*TextureParameterIuiv)(GLuint texture, GLenum pname, const GLuint *params);
	void APIENTRY (*TextureParameterf)(GLuint texture, GLenum pname, GLfloat param);
	void APIENTRY (*TextureParameterfv)(GLuint texture, GLenum pname, const GLfloat *param);
	void APIENTRY (*TextureParameteri)(GLuint texture, GLenum pname, GLint param);
	void APIENTRY (*TextureParameteriv)(GLuint texture, GLenum pname, const GLint *param);
	void APIENTRY (*TextureStorage1D)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width);
	void APIENTRY (*TextureStorage2D)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	void APIENTRY (*TextureStorage2DMultisample)(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean32 fixedsamplelocations);
	void APIENTRY (*TextureStorage3D)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	void APIENTRY (*TextureStorage3DMultisample)(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean32 fixedsamplelocations);
	void APIENTRY (*TextureSubImage1D)(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TextureSubImage2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TextureSubImage3D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	void APIENTRY (*TransformFeedbackBufferBase)(GLuint xfb, GLuint index, GLuint buffer);
	void APIENTRY (*TransformFeedbackBufferRange)(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	GLboolean APIENTRY (*UnmapNamedBuffer)(GLuint buffer);
	void APIENTRY (*VertexArrayAttribBinding)(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
	void APIENTRY (*VertexArrayAttribFormat)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean32 normalized, GLuint relativeoffset);
	void APIENTRY (*VertexArrayAttribIFormat)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	void APIENTRY (*VertexArrayAttribLFormat)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	void APIENTRY (*VertexArrayBindingDivisor)(GLuint vaobj, GLuint bindingindex, GLuint divisor);
	void APIENTRY (*VertexArrayElementBuffer)(GLuint vaobj, GLuint buffer);
	void APIENTRY (*VertexArrayVertexBuffer)(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
	void APIENTRY (*VertexArrayVertexBuffers)(GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides);

};

extern struct _gl_osmesa gl;

#ifdef __cplusplus
}
#endif


#ifndef NFOSMESA_NO_MANGLE
#undef glGetString
#define glGetString (gl.GetString)
#undef OSMesaCreateContext
#define OSMesaCreateContext (gl.OSMesaCreateContext)
#undef OSMesaCreateContextExt
#define OSMesaCreateContextExt (gl.OSMesaCreateContextExt)
#undef OSMesaDestroyContext
#define OSMesaDestroyContext (gl.OSMesaDestroyContext)
#undef OSMesaMakeCurrent
#define OSMesaMakeCurrent (gl.OSMesaMakeCurrent)
#undef OSMesaGetCurrentContext
#define OSMesaGetCurrentContext (gl.OSMesaGetCurrentContext)
#undef OSMesaPixelStore
#define OSMesaPixelStore (gl.OSMesaPixelStore)
#undef OSMesaGetIntegerv
#define OSMesaGetIntegerv (gl.OSMesaGetIntegerv)
#undef OSMesaGetDepthBuffer
#define OSMesaGetDepthBuffer (gl.OSMesaGetDepthBuffer)
#undef OSMesaGetColorBuffer
#define OSMesaGetColorBuffer (gl.OSMesaGetColorBuffer)
#undef OSMesaGetProcAddress
#define OSMesaGetProcAddress (gl.OSMesaGetProcAddress)
#undef glClearIndex
#define glClearIndex (gl.ClearIndex)
#undef glClearColor
#define glClearColor (gl.ClearColor)
#undef glClear
#define glClear (gl.Clear)
#undef glIndexMask
#define glIndexMask (gl.IndexMask)
#undef glColorMask
#define glColorMask (gl.ColorMask)
#undef glAlphaFunc
#define glAlphaFunc (gl.AlphaFunc)
#undef glBlendFunc
#define glBlendFunc (gl.BlendFunc)
#undef glLogicOp
#define glLogicOp (gl.LogicOp)
#undef glCullFace
#define glCullFace (gl.CullFace)
#undef glFrontFace
#define glFrontFace (gl.FrontFace)
#undef glPointSize
#define glPointSize (gl.PointSize)
#undef glLineWidth
#define glLineWidth (gl.LineWidth)
#undef glLineStipple
#define glLineStipple (gl.LineStipple)
#undef glPolygonMode
#define glPolygonMode (gl.PolygonMode)
#undef glPolygonOffset
#define glPolygonOffset (gl.PolygonOffset)
#undef glPolygonStipple
#define glPolygonStipple (gl.PolygonStipple)
#undef glGetPolygonStipple
#define glGetPolygonStipple (gl.GetPolygonStipple)
#undef glEdgeFlag
#define glEdgeFlag (gl.EdgeFlag)
#undef glEdgeFlagv
#define glEdgeFlagv (gl.EdgeFlagv)
#undef glScissor
#define glScissor (gl.Scissor)
#undef glClipPlane
#define glClipPlane (gl.ClipPlane)
#undef glGetClipPlane
#define glGetClipPlane (gl.GetClipPlane)
#undef glDrawBuffer
#define glDrawBuffer (gl.DrawBuffer)
#undef glReadBuffer
#define glReadBuffer (gl.ReadBuffer)
#undef glEnable
#define glEnable (gl.Enable)
#undef glDisable
#define glDisable (gl.Disable)
#undef glIsEnabled
#define glIsEnabled (gl.IsEnabled)
#undef glEnableClientState
#define glEnableClientState (gl.EnableClientState)
#undef glDisableClientState
#define glDisableClientState (gl.DisableClientState)
#undef glGetBooleanv
#define glGetBooleanv (gl.GetBooleanv)
#undef glGetDoublev
#define glGetDoublev (gl.GetDoublev)
#undef glGetFloatv
#define glGetFloatv (gl.GetFloatv)
#undef glGetIntegerv
#define glGetIntegerv (gl.GetIntegerv)
#undef glPushAttrib
#define glPushAttrib (gl.PushAttrib)
#undef glPopAttrib
#define glPopAttrib (gl.PopAttrib)
#undef glPushClientAttrib
#define glPushClientAttrib (gl.PushClientAttrib)
#undef glPopClientAttrib
#define glPopClientAttrib (gl.PopClientAttrib)
#undef glRenderMode
#define glRenderMode (gl.RenderMode)
#undef glGetError
#define glGetError (gl.GetError)
#undef glFinish
#define glFinish (gl.Finish)
#undef glFlush
#define glFlush (gl.Flush)
#undef glHint
#define glHint (gl.Hint)
#undef glClearDepth
#define glClearDepth (gl.ClearDepth)
#undef glDepthFunc
#define glDepthFunc (gl.DepthFunc)
#undef glDepthMask
#define glDepthMask (gl.DepthMask)
#undef glDepthRange
#define glDepthRange (gl.DepthRange)
#undef glClearAccum
#define glClearAccum (gl.ClearAccum)
#undef glAccum
#define glAccum (gl.Accum)
#undef glMatrixMode
#define glMatrixMode (gl.MatrixMode)
#undef glOrtho
#define glOrtho (gl.Ortho)
#undef glFrustum
#define glFrustum (gl.Frustum)
#undef glViewport
#define glViewport (gl.Viewport)
#undef glPushMatrix
#define glPushMatrix (gl.PushMatrix)
#undef glPopMatrix
#define glPopMatrix (gl.PopMatrix)
#undef glLoadIdentity
#define glLoadIdentity (gl.LoadIdentity)
#undef glLoadMatrixd
#define glLoadMatrixd (gl.LoadMatrixd)
#undef glLoadMatrixf
#define glLoadMatrixf (gl.LoadMatrixf)
#undef glMultMatrixd
#define glMultMatrixd (gl.MultMatrixd)
#undef glMultMatrixf
#define glMultMatrixf (gl.MultMatrixf)
#undef glRotated
#define glRotated (gl.Rotated)
#undef glRotatef
#define glRotatef (gl.Rotatef)
#undef glScaled
#define glScaled (gl.Scaled)
#undef glScalef
#define glScalef (gl.Scalef)
#undef glTranslated
#define glTranslated (gl.Translated)
#undef glTranslatef
#define glTranslatef (gl.Translatef)
#undef glIsList
#define glIsList (gl.IsList)
#undef glDeleteLists
#define glDeleteLists (gl.DeleteLists)
#undef glGenLists
#define glGenLists (gl.GenLists)
#undef glNewList
#define glNewList (gl.NewList)
#undef glEndList
#define glEndList (gl.EndList)
#undef glCallList
#define glCallList (gl.CallList)
#undef glCallLists
#define glCallLists (gl.CallLists)
#undef glListBase
#define glListBase (gl.ListBase)
#undef glBegin
#define glBegin (gl.Begin)
#undef glEnd
#define glEnd (gl.End)
#undef glVertex2d
#define glVertex2d (gl.Vertex2d)
#undef glVertex2f
#define glVertex2f (gl.Vertex2f)
#undef glVertex2i
#define glVertex2i (gl.Vertex2i)
#undef glVertex2s
#define glVertex2s (gl.Vertex2s)
#undef glVertex3d
#define glVertex3d (gl.Vertex3d)
#undef glVertex3f
#define glVertex3f (gl.Vertex3f)
#undef glVertex3i
#define glVertex3i (gl.Vertex3i)
#undef glVertex3s
#define glVertex3s (gl.Vertex3s)
#undef glVertex4d
#define glVertex4d (gl.Vertex4d)
#undef glVertex4f
#define glVertex4f (gl.Vertex4f)
#undef glVertex4i
#define glVertex4i (gl.Vertex4i)
#undef glVertex4s
#define glVertex4s (gl.Vertex4s)
#undef glVertex2dv
#define glVertex2dv (gl.Vertex2dv)
#undef glVertex2fv
#define glVertex2fv (gl.Vertex2fv)
#undef glVertex2iv
#define glVertex2iv (gl.Vertex2iv)
#undef glVertex2sv
#define glVertex2sv (gl.Vertex2sv)
#undef glVertex3dv
#define glVertex3dv (gl.Vertex3dv)
#undef glVertex3fv
#define glVertex3fv (gl.Vertex3fv)
#undef glVertex3iv
#define glVertex3iv (gl.Vertex3iv)
#undef glVertex3sv
#define glVertex3sv (gl.Vertex3sv)
#undef glVertex4dv
#define glVertex4dv (gl.Vertex4dv)
#undef glVertex4fv
#define glVertex4fv (gl.Vertex4fv)
#undef glVertex4iv
#define glVertex4iv (gl.Vertex4iv)
#undef glVertex4sv
#define glVertex4sv (gl.Vertex4sv)
#undef glNormal3b
#define glNormal3b (gl.Normal3b)
#undef glNormal3d
#define glNormal3d (gl.Normal3d)
#undef glNormal3f
#define glNormal3f (gl.Normal3f)
#undef glNormal3i
#define glNormal3i (gl.Normal3i)
#undef glNormal3s
#define glNormal3s (gl.Normal3s)
#undef glNormal3bv
#define glNormal3bv (gl.Normal3bv)
#undef glNormal3dv
#define glNormal3dv (gl.Normal3dv)
#undef glNormal3fv
#define glNormal3fv (gl.Normal3fv)
#undef glNormal3iv
#define glNormal3iv (gl.Normal3iv)
#undef glNormal3sv
#define glNormal3sv (gl.Normal3sv)
#undef glIndexd
#define glIndexd (gl.Indexd)
#undef glIndexf
#define glIndexf (gl.Indexf)
#undef glIndexi
#define glIndexi (gl.Indexi)
#undef glIndexs
#define glIndexs (gl.Indexs)
#undef glIndexub
#define glIndexub (gl.Indexub)
#undef glIndexdv
#define glIndexdv (gl.Indexdv)
#undef glIndexfv
#define glIndexfv (gl.Indexfv)
#undef glIndexiv
#define glIndexiv (gl.Indexiv)
#undef glIndexsv
#define glIndexsv (gl.Indexsv)
#undef glIndexubv
#define glIndexubv (gl.Indexubv)
#undef glColor3b
#define glColor3b (gl.Color3b)
#undef glColor3d
#define glColor3d (gl.Color3d)
#undef glColor3f
#define glColor3f (gl.Color3f)
#undef glColor3i
#define glColor3i (gl.Color3i)
#undef glColor3s
#define glColor3s (gl.Color3s)
#undef glColor3ub
#define glColor3ub (gl.Color3ub)
#undef glColor3ui
#define glColor3ui (gl.Color3ui)
#undef glColor3us
#define glColor3us (gl.Color3us)
#undef glColor4b
#define glColor4b (gl.Color4b)
#undef glColor4d
#define glColor4d (gl.Color4d)
#undef glColor4f
#define glColor4f (gl.Color4f)
#undef glColor4i
#define glColor4i (gl.Color4i)
#undef glColor4s
#define glColor4s (gl.Color4s)
#undef glColor4ub
#define glColor4ub (gl.Color4ub)
#undef glColor4ui
#define glColor4ui (gl.Color4ui)
#undef glColor4us
#define glColor4us (gl.Color4us)
#undef glColor3bv
#define glColor3bv (gl.Color3bv)
#undef glColor3dv
#define glColor3dv (gl.Color3dv)
#undef glColor3fv
#define glColor3fv (gl.Color3fv)
#undef glColor3iv
#define glColor3iv (gl.Color3iv)
#undef glColor3sv
#define glColor3sv (gl.Color3sv)
#undef glColor3ubv
#define glColor3ubv (gl.Color3ubv)
#undef glColor3uiv
#define glColor3uiv (gl.Color3uiv)
#undef glColor3usv
#define glColor3usv (gl.Color3usv)
#undef glColor4bv
#define glColor4bv (gl.Color4bv)
#undef glColor4dv
#define glColor4dv (gl.Color4dv)
#undef glColor4fv
#define glColor4fv (gl.Color4fv)
#undef glColor4iv
#define glColor4iv (gl.Color4iv)
#undef glColor4sv
#define glColor4sv (gl.Color4sv)
#undef glColor4ubv
#define glColor4ubv (gl.Color4ubv)
#undef glColor4uiv
#define glColor4uiv (gl.Color4uiv)
#undef glColor4usv
#define glColor4usv (gl.Color4usv)
#undef glTexCoord1d
#define glTexCoord1d (gl.TexCoord1d)
#undef glTexCoord1f
#define glTexCoord1f (gl.TexCoord1f)
#undef glTexCoord1i
#define glTexCoord1i (gl.TexCoord1i)
#undef glTexCoord1s
#define glTexCoord1s (gl.TexCoord1s)
#undef glTexCoord2d
#define glTexCoord2d (gl.TexCoord2d)
#undef glTexCoord2f
#define glTexCoord2f (gl.TexCoord2f)
#undef glTexCoord2i
#define glTexCoord2i (gl.TexCoord2i)
#undef glTexCoord2s
#define glTexCoord2s (gl.TexCoord2s)
#undef glTexCoord3d
#define glTexCoord3d (gl.TexCoord3d)
#undef glTexCoord3f
#define glTexCoord3f (gl.TexCoord3f)
#undef glTexCoord3i
#define glTexCoord3i (gl.TexCoord3i)
#undef glTexCoord3s
#define glTexCoord3s (gl.TexCoord3s)
#undef glTexCoord4d
#define glTexCoord4d (gl.TexCoord4d)
#undef glTexCoord4f
#define glTexCoord4f (gl.TexCoord4f)
#undef glTexCoord4i
#define glTexCoord4i (gl.TexCoord4i)
#undef glTexCoord4s
#define glTexCoord4s (gl.TexCoord4s)
#undef glTexCoord1dv
#define glTexCoord1dv (gl.TexCoord1dv)
#undef glTexCoord1fv
#define glTexCoord1fv (gl.TexCoord1fv)
#undef glTexCoord1iv
#define glTexCoord1iv (gl.TexCoord1iv)
#undef glTexCoord1sv
#define glTexCoord1sv (gl.TexCoord1sv)
#undef glTexCoord2dv
#define glTexCoord2dv (gl.TexCoord2dv)
#undef glTexCoord2fv
#define glTexCoord2fv (gl.TexCoord2fv)
#undef glTexCoord2iv
#define glTexCoord2iv (gl.TexCoord2iv)
#undef glTexCoord2sv
#define glTexCoord2sv (gl.TexCoord2sv)
#undef glTexCoord3dv
#define glTexCoord3dv (gl.TexCoord3dv)
#undef glTexCoord3fv
#define glTexCoord3fv (gl.TexCoord3fv)
#undef glTexCoord3iv
#define glTexCoord3iv (gl.TexCoord3iv)
#undef glTexCoord3sv
#define glTexCoord3sv (gl.TexCoord3sv)
#undef glTexCoord4dv
#define glTexCoord4dv (gl.TexCoord4dv)
#undef glTexCoord4fv
#define glTexCoord4fv (gl.TexCoord4fv)
#undef glTexCoord4iv
#define glTexCoord4iv (gl.TexCoord4iv)
#undef glTexCoord4sv
#define glTexCoord4sv (gl.TexCoord4sv)
#undef glRasterPos2d
#define glRasterPos2d (gl.RasterPos2d)
#undef glRasterPos2f
#define glRasterPos2f (gl.RasterPos2f)
#undef glRasterPos2i
#define glRasterPos2i (gl.RasterPos2i)
#undef glRasterPos2s
#define glRasterPos2s (gl.RasterPos2s)
#undef glRasterPos3d
#define glRasterPos3d (gl.RasterPos3d)
#undef glRasterPos3f
#define glRasterPos3f (gl.RasterPos3f)
#undef glRasterPos3i
#define glRasterPos3i (gl.RasterPos3i)
#undef glRasterPos3s
#define glRasterPos3s (gl.RasterPos3s)
#undef glRasterPos4d
#define glRasterPos4d (gl.RasterPos4d)
#undef glRasterPos4f
#define glRasterPos4f (gl.RasterPos4f)
#undef glRasterPos4i
#define glRasterPos4i (gl.RasterPos4i)
#undef glRasterPos4s
#define glRasterPos4s (gl.RasterPos4s)
#undef glRasterPos2dv
#define glRasterPos2dv (gl.RasterPos2dv)
#undef glRasterPos2fv
#define glRasterPos2fv (gl.RasterPos2fv)
#undef glRasterPos2iv
#define glRasterPos2iv (gl.RasterPos2iv)
#undef glRasterPos2sv
#define glRasterPos2sv (gl.RasterPos2sv)
#undef glRasterPos3dv
#define glRasterPos3dv (gl.RasterPos3dv)
#undef glRasterPos3fv
#define glRasterPos3fv (gl.RasterPos3fv)
#undef glRasterPos3iv
#define glRasterPos3iv (gl.RasterPos3iv)
#undef glRasterPos3sv
#define glRasterPos3sv (gl.RasterPos3sv)
#undef glRasterPos4dv
#define glRasterPos4dv (gl.RasterPos4dv)
#undef glRasterPos4fv
#define glRasterPos4fv (gl.RasterPos4fv)
#undef glRasterPos4iv
#define glRasterPos4iv (gl.RasterPos4iv)
#undef glRasterPos4sv
#define glRasterPos4sv (gl.RasterPos4sv)
#undef glRectd
#define glRectd (gl.Rectd)
#undef glRectf
#define glRectf (gl.Rectf)
#undef glRecti
#define glRecti (gl.Recti)
#undef glRects
#define glRects (gl.Rects)
#undef glRectdv
#define glRectdv (gl.Rectdv)
#undef glRectfv
#define glRectfv (gl.Rectfv)
#undef glRectiv
#define glRectiv (gl.Rectiv)
#undef glRectsv
#define glRectsv (gl.Rectsv)
#undef glVertexPointer
#define glVertexPointer (gl.VertexPointer)
#undef glNormalPointer
#define glNormalPointer (gl.NormalPointer)
#undef glColorPointer
#define glColorPointer (gl.ColorPointer)
#undef glIndexPointer
#define glIndexPointer (gl.IndexPointer)
#undef glTexCoordPointer
#define glTexCoordPointer (gl.TexCoordPointer)
#undef glEdgeFlagPointer
#define glEdgeFlagPointer (gl.EdgeFlagPointer)
#undef glGetPointerv
#define glGetPointerv (gl.GetPointerv)
#undef glArrayElement
#define glArrayElement (gl.ArrayElement)
#undef glDrawArrays
#define glDrawArrays (gl.DrawArrays)
#undef glDrawElements
#define glDrawElements (gl.DrawElements)
#undef glInterleavedArrays
#define glInterleavedArrays (gl.InterleavedArrays)
#undef glShadeModel
#define glShadeModel (gl.ShadeModel)
#undef glLightf
#define glLightf (gl.Lightf)
#undef glLighti
#define glLighti (gl.Lighti)
#undef glLightfv
#define glLightfv (gl.Lightfv)
#undef glLightiv
#define glLightiv (gl.Lightiv)
#undef glGetLightfv
#define glGetLightfv (gl.GetLightfv)
#undef glGetLightiv
#define glGetLightiv (gl.GetLightiv)
#undef glLightModelf
#define glLightModelf (gl.LightModelf)
#undef glLightModeli
#define glLightModeli (gl.LightModeli)
#undef glLightModelfv
#define glLightModelfv (gl.LightModelfv)
#undef glLightModeliv
#define glLightModeliv (gl.LightModeliv)
#undef glMaterialf
#define glMaterialf (gl.Materialf)
#undef glMateriali
#define glMateriali (gl.Materiali)
#undef glMaterialfv
#define glMaterialfv (gl.Materialfv)
#undef glMaterialiv
#define glMaterialiv (gl.Materialiv)
#undef glGetMaterialfv
#define glGetMaterialfv (gl.GetMaterialfv)
#undef glGetMaterialiv
#define glGetMaterialiv (gl.GetMaterialiv)
#undef glColorMaterial
#define glColorMaterial (gl.ColorMaterial)
#undef glPixelZoom
#define glPixelZoom (gl.PixelZoom)
#undef glPixelStoref
#define glPixelStoref (gl.PixelStoref)
#undef glPixelStorei
#define glPixelStorei (gl.PixelStorei)
#undef glPixelTransferf
#define glPixelTransferf (gl.PixelTransferf)
#undef glPixelTransferi
#define glPixelTransferi (gl.PixelTransferi)
#undef glPixelMapfv
#define glPixelMapfv (gl.PixelMapfv)
#undef glPixelMapuiv
#define glPixelMapuiv (gl.PixelMapuiv)
#undef glPixelMapusv
#define glPixelMapusv (gl.PixelMapusv)
#undef glGetPixelMapfv
#define glGetPixelMapfv (gl.GetPixelMapfv)
#undef glGetPixelMapuiv
#define glGetPixelMapuiv (gl.GetPixelMapuiv)
#undef glGetPixelMapusv
#define glGetPixelMapusv (gl.GetPixelMapusv)
#undef glBitmap
#define glBitmap (gl.Bitmap)
#undef glReadPixels
#define glReadPixels (gl.ReadPixels)
#undef glDrawPixels
#define glDrawPixels (gl.DrawPixels)
#undef glCopyPixels
#define glCopyPixels (gl.CopyPixels)
#undef glStencilFunc
#define glStencilFunc (gl.StencilFunc)
#undef glStencilMask
#define glStencilMask (gl.StencilMask)
#undef glStencilOp
#define glStencilOp (gl.StencilOp)
#undef glClearStencil
#define glClearStencil (gl.ClearStencil)
#undef glTexGend
#define glTexGend (gl.TexGend)
#undef glTexGenf
#define glTexGenf (gl.TexGenf)
#undef glTexGeni
#define glTexGeni (gl.TexGeni)
#undef glTexGendv
#define glTexGendv (gl.TexGendv)
#undef glTexGenfv
#define glTexGenfv (gl.TexGenfv)
#undef glTexGeniv
#define glTexGeniv (gl.TexGeniv)
#undef glGetTexGendv
#define glGetTexGendv (gl.GetTexGendv)
#undef glGetTexGenfv
#define glGetTexGenfv (gl.GetTexGenfv)
#undef glGetTexGeniv
#define glGetTexGeniv (gl.GetTexGeniv)
#undef glTexEnvf
#define glTexEnvf (gl.TexEnvf)
#undef glTexEnvi
#define glTexEnvi (gl.TexEnvi)
#undef glTexEnvfv
#define glTexEnvfv (gl.TexEnvfv)
#undef glTexEnviv
#define glTexEnviv (gl.TexEnviv)
#undef glGetTexEnvfv
#define glGetTexEnvfv (gl.GetTexEnvfv)
#undef glGetTexEnviv
#define glGetTexEnviv (gl.GetTexEnviv)
#undef glTexParameterf
#define glTexParameterf (gl.TexParameterf)
#undef glTexParameteri
#define glTexParameteri (gl.TexParameteri)
#undef glTexParameterfv
#define glTexParameterfv (gl.TexParameterfv)
#undef glTexParameteriv
#define glTexParameteriv (gl.TexParameteriv)
#undef glGetTexParameterfv
#define glGetTexParameterfv (gl.GetTexParameterfv)
#undef glGetTexParameteriv
#define glGetTexParameteriv (gl.GetTexParameteriv)
#undef glGetTexLevelParameterfv
#define glGetTexLevelParameterfv (gl.GetTexLevelParameterfv)
#undef glGetTexLevelParameteriv
#define glGetTexLevelParameteriv (gl.GetTexLevelParameteriv)
#undef glTexImage1D
#define glTexImage1D (gl.TexImage1D)
#undef glTexImage2D
#define glTexImage2D (gl.TexImage2D)
#undef glGetTexImage
#define glGetTexImage (gl.GetTexImage)
#undef glGenTextures
#define glGenTextures (gl.GenTextures)
#undef glDeleteTextures
#define glDeleteTextures (gl.DeleteTextures)
#undef glBindTexture
#define glBindTexture (gl.BindTexture)
#undef glPrioritizeTextures
#define glPrioritizeTextures (gl.PrioritizeTextures)
#undef glAreTexturesResident
#define glAreTexturesResident (gl.AreTexturesResident)
#undef glIsTexture
#define glIsTexture (gl.IsTexture)
#undef glTexSubImage1D
#define glTexSubImage1D (gl.TexSubImage1D)
#undef glTexSubImage2D
#define glTexSubImage2D (gl.TexSubImage2D)
#undef glCopyTexImage1D
#define glCopyTexImage1D (gl.CopyTexImage1D)
#undef glCopyTexImage2D
#define glCopyTexImage2D (gl.CopyTexImage2D)
#undef glCopyTexSubImage1D
#define glCopyTexSubImage1D (gl.CopyTexSubImage1D)
#undef glCopyTexSubImage2D
#define glCopyTexSubImage2D (gl.CopyTexSubImage2D)
#undef glMap1d
#define glMap1d (gl.Map1d)
#undef glMap1f
#define glMap1f (gl.Map1f)
#undef glMap2d
#define glMap2d (gl.Map2d)
#undef glMap2f
#define glMap2f (gl.Map2f)
#undef glGetMapdv
#define glGetMapdv (gl.GetMapdv)
#undef glGetMapfv
#define glGetMapfv (gl.GetMapfv)
#undef glGetMapiv
#define glGetMapiv (gl.GetMapiv)
#undef glEvalCoord1d
#define glEvalCoord1d (gl.EvalCoord1d)
#undef glEvalCoord1f
#define glEvalCoord1f (gl.EvalCoord1f)
#undef glEvalCoord1dv
#define glEvalCoord1dv (gl.EvalCoord1dv)
#undef glEvalCoord1fv
#define glEvalCoord1fv (gl.EvalCoord1fv)
#undef glEvalCoord2d
#define glEvalCoord2d (gl.EvalCoord2d)
#undef glEvalCoord2f
#define glEvalCoord2f (gl.EvalCoord2f)
#undef glEvalCoord2dv
#define glEvalCoord2dv (gl.EvalCoord2dv)
#undef glEvalCoord2fv
#define glEvalCoord2fv (gl.EvalCoord2fv)
#undef glMapGrid1d
#define glMapGrid1d (gl.MapGrid1d)
#undef glMapGrid1f
#define glMapGrid1f (gl.MapGrid1f)
#undef glMapGrid2d
#define glMapGrid2d (gl.MapGrid2d)
#undef glMapGrid2f
#define glMapGrid2f (gl.MapGrid2f)
#undef glEvalPoint1
#define glEvalPoint1 (gl.EvalPoint1)
#undef glEvalPoint2
#define glEvalPoint2 (gl.EvalPoint2)
#undef glEvalMesh1
#define glEvalMesh1 (gl.EvalMesh1)
#undef glEvalMesh2
#define glEvalMesh2 (gl.EvalMesh2)
#undef glFogf
#define glFogf (gl.Fogf)
#undef glFogi
#define glFogi (gl.Fogi)
#undef glFogfv
#define glFogfv (gl.Fogfv)
#undef glFogiv
#define glFogiv (gl.Fogiv)
#undef glFeedbackBuffer
#define glFeedbackBuffer (gl.FeedbackBuffer)
#undef glPassThrough
#define glPassThrough (gl.PassThrough)
#undef glSelectBuffer
#define glSelectBuffer (gl.SelectBuffer)
#undef glInitNames
#define glInitNames (gl.InitNames)
#undef glLoadName
#define glLoadName (gl.LoadName)
#undef glPushName
#define glPushName (gl.PushName)
#undef glPopName
#define glPopName (gl.PopName)
#undef glEnableTraceMESA
#define glEnableTraceMESA (gl.EnableTraceMESA)
#undef glDisableTraceMESA
#define glDisableTraceMESA (gl.DisableTraceMESA)
#undef glNewTraceMESA
#define glNewTraceMESA (gl.NewTraceMESA)
#undef glEndTraceMESA
#define glEndTraceMESA (gl.EndTraceMESA)
#undef glTraceAssertAttribMESA
#define glTraceAssertAttribMESA (gl.TraceAssertAttribMESA)
#undef glTraceCommentMESA
#define glTraceCommentMESA (gl.TraceCommentMESA)
#undef glTraceTextureMESA
#define glTraceTextureMESA (gl.TraceTextureMESA)
#undef glTraceListMESA
#define glTraceListMESA (gl.TraceListMESA)
#undef glTracePointerMESA
#define glTracePointerMESA (gl.TracePointerMESA)
#undef glTracePointerRangeMESA
#define glTracePointerRangeMESA (gl.TracePointerRangeMESA)
#undef glBlendEquationSeparateATI
#define glBlendEquationSeparateATI (gl.BlendEquationSeparateATI)
#undef glBlendColor
#define glBlendColor (gl.BlendColor)
#undef glBlendEquation
#define glBlendEquation (gl.BlendEquation)
#undef glDrawRangeElements
#define glDrawRangeElements (gl.DrawRangeElements)
#undef glColorTable
#define glColorTable (gl.ColorTable)
#undef glColorTableParameterfv
#define glColorTableParameterfv (gl.ColorTableParameterfv)
#undef glColorTableParameteriv
#define glColorTableParameteriv (gl.ColorTableParameteriv)
#undef glCopyColorTable
#define glCopyColorTable (gl.CopyColorTable)
#undef glGetColorTable
#define glGetColorTable (gl.GetColorTable)
#undef glGetColorTableParameterfv
#define glGetColorTableParameterfv (gl.GetColorTableParameterfv)
#undef glGetColorTableParameteriv
#define glGetColorTableParameteriv (gl.GetColorTableParameteriv)
#undef glColorSubTable
#define glColorSubTable (gl.ColorSubTable)
#undef glCopyColorSubTable
#define glCopyColorSubTable (gl.CopyColorSubTable)
#undef glConvolutionFilter1D
#define glConvolutionFilter1D (gl.ConvolutionFilter1D)
#undef glConvolutionFilter2D
#define glConvolutionFilter2D (gl.ConvolutionFilter2D)
#undef glConvolutionParameterf
#define glConvolutionParameterf (gl.ConvolutionParameterf)
#undef glConvolutionParameterfv
#define glConvolutionParameterfv (gl.ConvolutionParameterfv)
#undef glConvolutionParameteri
#define glConvolutionParameteri (gl.ConvolutionParameteri)
#undef glConvolutionParameteriv
#define glConvolutionParameteriv (gl.ConvolutionParameteriv)
#undef glCopyConvolutionFilter1D
#define glCopyConvolutionFilter1D (gl.CopyConvolutionFilter1D)
#undef glCopyConvolutionFilter2D
#define glCopyConvolutionFilter2D (gl.CopyConvolutionFilter2D)
#undef glGetConvolutionFilter
#define glGetConvolutionFilter (gl.GetConvolutionFilter)
#undef glGetConvolutionParameterfv
#define glGetConvolutionParameterfv (gl.GetConvolutionParameterfv)
#undef glGetConvolutionParameteriv
#define glGetConvolutionParameteriv (gl.GetConvolutionParameteriv)
#undef glGetSeparableFilter
#define glGetSeparableFilter (gl.GetSeparableFilter)
#undef glSeparableFilter2D
#define glSeparableFilter2D (gl.SeparableFilter2D)
#undef glGetHistogram
#define glGetHistogram (gl.GetHistogram)
#undef glGetHistogramParameterfv
#define glGetHistogramParameterfv (gl.GetHistogramParameterfv)
#undef glGetHistogramParameteriv
#define glGetHistogramParameteriv (gl.GetHistogramParameteriv)
#undef glGetMinmax
#define glGetMinmax (gl.GetMinmax)
#undef glGetMinmaxParameterfv
#define glGetMinmaxParameterfv (gl.GetMinmaxParameterfv)
#undef glGetMinmaxParameteriv
#define glGetMinmaxParameteriv (gl.GetMinmaxParameteriv)
#undef glHistogram
#define glHistogram (gl.Histogram)
#undef glMinmax
#define glMinmax (gl.Minmax)
#undef glResetHistogram
#define glResetHistogram (gl.ResetHistogram)
#undef glResetMinmax
#define glResetMinmax (gl.ResetMinmax)
#undef glTexImage3D
#define glTexImage3D (gl.TexImage3D)
#undef glTexSubImage3D
#define glTexSubImage3D (gl.TexSubImage3D)
#undef glCopyTexSubImage3D
#define glCopyTexSubImage3D (gl.CopyTexSubImage3D)
#undef glActiveTexture
#define glActiveTexture (gl.ActiveTexture)
#undef glClientActiveTexture
#define glClientActiveTexture (gl.ClientActiveTexture)
#undef glMultiTexCoord1d
#define glMultiTexCoord1d (gl.MultiTexCoord1d)
#undef glMultiTexCoord1dv
#define glMultiTexCoord1dv (gl.MultiTexCoord1dv)
#undef glMultiTexCoord1f
#define glMultiTexCoord1f (gl.MultiTexCoord1f)
#undef glMultiTexCoord1fv
#define glMultiTexCoord1fv (gl.MultiTexCoord1fv)
#undef glMultiTexCoord1i
#define glMultiTexCoord1i (gl.MultiTexCoord1i)
#undef glMultiTexCoord1iv
#define glMultiTexCoord1iv (gl.MultiTexCoord1iv)
#undef glMultiTexCoord1s
#define glMultiTexCoord1s (gl.MultiTexCoord1s)
#undef glMultiTexCoord1sv
#define glMultiTexCoord1sv (gl.MultiTexCoord1sv)
#undef glMultiTexCoord2d
#define glMultiTexCoord2d (gl.MultiTexCoord2d)
#undef glMultiTexCoord2dv
#define glMultiTexCoord2dv (gl.MultiTexCoord2dv)
#undef glMultiTexCoord2f
#define glMultiTexCoord2f (gl.MultiTexCoord2f)
#undef glMultiTexCoord2fv
#define glMultiTexCoord2fv (gl.MultiTexCoord2fv)
#undef glMultiTexCoord2i
#define glMultiTexCoord2i (gl.MultiTexCoord2i)
#undef glMultiTexCoord2iv
#define glMultiTexCoord2iv (gl.MultiTexCoord2iv)
#undef glMultiTexCoord2s
#define glMultiTexCoord2s (gl.MultiTexCoord2s)
#undef glMultiTexCoord2sv
#define glMultiTexCoord2sv (gl.MultiTexCoord2sv)
#undef glMultiTexCoord3d
#define glMultiTexCoord3d (gl.MultiTexCoord3d)
#undef glMultiTexCoord3dv
#define glMultiTexCoord3dv (gl.MultiTexCoord3dv)
#undef glMultiTexCoord3f
#define glMultiTexCoord3f (gl.MultiTexCoord3f)
#undef glMultiTexCoord3fv
#define glMultiTexCoord3fv (gl.MultiTexCoord3fv)
#undef glMultiTexCoord3i
#define glMultiTexCoord3i (gl.MultiTexCoord3i)
#undef glMultiTexCoord3iv
#define glMultiTexCoord3iv (gl.MultiTexCoord3iv)
#undef glMultiTexCoord3s
#define glMultiTexCoord3s (gl.MultiTexCoord3s)
#undef glMultiTexCoord3sv
#define glMultiTexCoord3sv (gl.MultiTexCoord3sv)
#undef glMultiTexCoord4d
#define glMultiTexCoord4d (gl.MultiTexCoord4d)
#undef glMultiTexCoord4dv
#define glMultiTexCoord4dv (gl.MultiTexCoord4dv)
#undef glMultiTexCoord4f
#define glMultiTexCoord4f (gl.MultiTexCoord4f)
#undef glMultiTexCoord4fv
#define glMultiTexCoord4fv (gl.MultiTexCoord4fv)
#undef glMultiTexCoord4i
#define glMultiTexCoord4i (gl.MultiTexCoord4i)
#undef glMultiTexCoord4iv
#define glMultiTexCoord4iv (gl.MultiTexCoord4iv)
#undef glMultiTexCoord4s
#define glMultiTexCoord4s (gl.MultiTexCoord4s)
#undef glMultiTexCoord4sv
#define glMultiTexCoord4sv (gl.MultiTexCoord4sv)
#undef glLoadTransposeMatrixf
#define glLoadTransposeMatrixf (gl.LoadTransposeMatrixf)
#undef glLoadTransposeMatrixd
#define glLoadTransposeMatrixd (gl.LoadTransposeMatrixd)
#undef glMultTransposeMatrixf
#define glMultTransposeMatrixf (gl.MultTransposeMatrixf)
#undef glMultTransposeMatrixd
#define glMultTransposeMatrixd (gl.MultTransposeMatrixd)
#undef glSampleCoverage
#define glSampleCoverage (gl.SampleCoverage)
#undef glCompressedTexImage3D
#define glCompressedTexImage3D (gl.CompressedTexImage3D)
#undef glCompressedTexImage2D
#define glCompressedTexImage2D (gl.CompressedTexImage2D)
#undef glCompressedTexImage1D
#define glCompressedTexImage1D (gl.CompressedTexImage1D)
#undef glCompressedTexSubImage3D
#define glCompressedTexSubImage3D (gl.CompressedTexSubImage3D)
#undef glCompressedTexSubImage2D
#define glCompressedTexSubImage2D (gl.CompressedTexSubImage2D)
#undef glCompressedTexSubImage1D
#define glCompressedTexSubImage1D (gl.CompressedTexSubImage1D)
#undef glGetCompressedTexImage
#define glGetCompressedTexImage (gl.GetCompressedTexImage)
#undef glBlendFuncSeparate
#define glBlendFuncSeparate (gl.BlendFuncSeparate)
#undef glFogCoordf
#define glFogCoordf (gl.FogCoordf)
#undef glFogCoordfv
#define glFogCoordfv (gl.FogCoordfv)
#undef glFogCoordd
#define glFogCoordd (gl.FogCoordd)
#undef glFogCoorddv
#define glFogCoorddv (gl.FogCoorddv)
#undef glFogCoordPointer
#define glFogCoordPointer (gl.FogCoordPointer)
#undef glMultiDrawArrays
#define glMultiDrawArrays (gl.MultiDrawArrays)
#undef glMultiDrawElements
#define glMultiDrawElements (gl.MultiDrawElements)
#undef glPointParameterf
#define glPointParameterf (gl.PointParameterf)
#undef glPointParameterfv
#define glPointParameterfv (gl.PointParameterfv)
#undef glPointParameteri
#define glPointParameteri (gl.PointParameteri)
#undef glPointParameteriv
#define glPointParameteriv (gl.PointParameteriv)
#undef glSecondaryColor3b
#define glSecondaryColor3b (gl.SecondaryColor3b)
#undef glSecondaryColor3bv
#define glSecondaryColor3bv (gl.SecondaryColor3bv)
#undef glSecondaryColor3d
#define glSecondaryColor3d (gl.SecondaryColor3d)
#undef glSecondaryColor3dv
#define glSecondaryColor3dv (gl.SecondaryColor3dv)
#undef glSecondaryColor3f
#define glSecondaryColor3f (gl.SecondaryColor3f)
#undef glSecondaryColor3fv
#define glSecondaryColor3fv (gl.SecondaryColor3fv)
#undef glSecondaryColor3i
#define glSecondaryColor3i (gl.SecondaryColor3i)
#undef glSecondaryColor3iv
#define glSecondaryColor3iv (gl.SecondaryColor3iv)
#undef glSecondaryColor3s
#define glSecondaryColor3s (gl.SecondaryColor3s)
#undef glSecondaryColor3sv
#define glSecondaryColor3sv (gl.SecondaryColor3sv)
#undef glSecondaryColor3ub
#define glSecondaryColor3ub (gl.SecondaryColor3ub)
#undef glSecondaryColor3ubv
#define glSecondaryColor3ubv (gl.SecondaryColor3ubv)
#undef glSecondaryColor3ui
#define glSecondaryColor3ui (gl.SecondaryColor3ui)
#undef glSecondaryColor3uiv
#define glSecondaryColor3uiv (gl.SecondaryColor3uiv)
#undef glSecondaryColor3us
#define glSecondaryColor3us (gl.SecondaryColor3us)
#undef glSecondaryColor3usv
#define glSecondaryColor3usv (gl.SecondaryColor3usv)
#undef glSecondaryColorPointer
#define glSecondaryColorPointer (gl.SecondaryColorPointer)
#undef glWindowPos2d
#define glWindowPos2d (gl.WindowPos2d)
#undef glWindowPos2dv
#define glWindowPos2dv (gl.WindowPos2dv)
#undef glWindowPos2f
#define glWindowPos2f (gl.WindowPos2f)
#undef glWindowPos2fv
#define glWindowPos2fv (gl.WindowPos2fv)
#undef glWindowPos2i
#define glWindowPos2i (gl.WindowPos2i)
#undef glWindowPos2iv
#define glWindowPos2iv (gl.WindowPos2iv)
#undef glWindowPos2s
#define glWindowPos2s (gl.WindowPos2s)
#undef glWindowPos2sv
#define glWindowPos2sv (gl.WindowPos2sv)
#undef glWindowPos3d
#define glWindowPos3d (gl.WindowPos3d)
#undef glWindowPos3dv
#define glWindowPos3dv (gl.WindowPos3dv)
#undef glWindowPos3f
#define glWindowPos3f (gl.WindowPos3f)
#undef glWindowPos3fv
#define glWindowPos3fv (gl.WindowPos3fv)
#undef glWindowPos3i
#define glWindowPos3i (gl.WindowPos3i)
#undef glWindowPos3iv
#define glWindowPos3iv (gl.WindowPos3iv)
#undef glWindowPos3s
#define glWindowPos3s (gl.WindowPos3s)
#undef glWindowPos3sv
#define glWindowPos3sv (gl.WindowPos3sv)
#undef glGenQueries
#define glGenQueries (gl.GenQueries)
#undef glDeleteQueries
#define glDeleteQueries (gl.DeleteQueries)
#undef glIsQuery
#define glIsQuery (gl.IsQuery)
#undef glBeginQuery
#define glBeginQuery (gl.BeginQuery)
#undef glEndQuery
#define glEndQuery (gl.EndQuery)
#undef glGetQueryiv
#define glGetQueryiv (gl.GetQueryiv)
#undef glGetQueryObjectiv
#define glGetQueryObjectiv (gl.GetQueryObjectiv)
#undef glGetQueryObjectuiv
#define glGetQueryObjectuiv (gl.GetQueryObjectuiv)
#undef glBindBuffer
#define glBindBuffer (gl.BindBuffer)
#undef glDeleteBuffers
#define glDeleteBuffers (gl.DeleteBuffers)
#undef glGenBuffers
#define glGenBuffers (gl.GenBuffers)
#undef glIsBuffer
#define glIsBuffer (gl.IsBuffer)
#undef glBufferData
#define glBufferData (gl.BufferData)
#undef glBufferSubData
#define glBufferSubData (gl.BufferSubData)
#undef glGetBufferSubData
#define glGetBufferSubData (gl.GetBufferSubData)
#undef glMapBuffer
#define glMapBuffer (gl.MapBuffer)
#undef glUnmapBuffer
#define glUnmapBuffer (gl.UnmapBuffer)
#undef glGetBufferParameteriv
#define glGetBufferParameteriv (gl.GetBufferParameteriv)
#undef glGetBufferPointerv
#define glGetBufferPointerv (gl.GetBufferPointerv)
#undef glActiveTextureARB
#define glActiveTextureARB (gl.ActiveTextureARB)
#undef glClientActiveTextureARB
#define glClientActiveTextureARB (gl.ClientActiveTextureARB)
#undef glMultiTexCoord1dARB
#define glMultiTexCoord1dARB (gl.MultiTexCoord1dARB)
#undef glMultiTexCoord1dvARB
#define glMultiTexCoord1dvARB (gl.MultiTexCoord1dvARB)
#undef glMultiTexCoord1fARB
#define glMultiTexCoord1fARB (gl.MultiTexCoord1fARB)
#undef glMultiTexCoord1fvARB
#define glMultiTexCoord1fvARB (gl.MultiTexCoord1fvARB)
#undef glMultiTexCoord1iARB
#define glMultiTexCoord1iARB (gl.MultiTexCoord1iARB)
#undef glMultiTexCoord1ivARB
#define glMultiTexCoord1ivARB (gl.MultiTexCoord1ivARB)
#undef glMultiTexCoord1sARB
#define glMultiTexCoord1sARB (gl.MultiTexCoord1sARB)
#undef glMultiTexCoord1svARB
#define glMultiTexCoord1svARB (gl.MultiTexCoord1svARB)
#undef glMultiTexCoord2dARB
#define glMultiTexCoord2dARB (gl.MultiTexCoord2dARB)
#undef glMultiTexCoord2dvARB
#define glMultiTexCoord2dvARB (gl.MultiTexCoord2dvARB)
#undef glMultiTexCoord2fARB
#define glMultiTexCoord2fARB (gl.MultiTexCoord2fARB)
#undef glMultiTexCoord2fvARB
#define glMultiTexCoord2fvARB (gl.MultiTexCoord2fvARB)
#undef glMultiTexCoord2iARB
#define glMultiTexCoord2iARB (gl.MultiTexCoord2iARB)
#undef glMultiTexCoord2ivARB
#define glMultiTexCoord2ivARB (gl.MultiTexCoord2ivARB)
#undef glMultiTexCoord2sARB
#define glMultiTexCoord2sARB (gl.MultiTexCoord2sARB)
#undef glMultiTexCoord2svARB
#define glMultiTexCoord2svARB (gl.MultiTexCoord2svARB)
#undef glMultiTexCoord3dARB
#define glMultiTexCoord3dARB (gl.MultiTexCoord3dARB)
#undef glMultiTexCoord3dvARB
#define glMultiTexCoord3dvARB (gl.MultiTexCoord3dvARB)
#undef glMultiTexCoord3fARB
#define glMultiTexCoord3fARB (gl.MultiTexCoord3fARB)
#undef glMultiTexCoord3fvARB
#define glMultiTexCoord3fvARB (gl.MultiTexCoord3fvARB)
#undef glMultiTexCoord3iARB
#define glMultiTexCoord3iARB (gl.MultiTexCoord3iARB)
#undef glMultiTexCoord3ivARB
#define glMultiTexCoord3ivARB (gl.MultiTexCoord3ivARB)
#undef glMultiTexCoord3sARB
#define glMultiTexCoord3sARB (gl.MultiTexCoord3sARB)
#undef glMultiTexCoord3svARB
#define glMultiTexCoord3svARB (gl.MultiTexCoord3svARB)
#undef glMultiTexCoord4dARB
#define glMultiTexCoord4dARB (gl.MultiTexCoord4dARB)
#undef glMultiTexCoord4dvARB
#define glMultiTexCoord4dvARB (gl.MultiTexCoord4dvARB)
#undef glMultiTexCoord4fARB
#define glMultiTexCoord4fARB (gl.MultiTexCoord4fARB)
#undef glMultiTexCoord4fvARB
#define glMultiTexCoord4fvARB (gl.MultiTexCoord4fvARB)
#undef glMultiTexCoord4iARB
#define glMultiTexCoord4iARB (gl.MultiTexCoord4iARB)
#undef glMultiTexCoord4ivARB
#define glMultiTexCoord4ivARB (gl.MultiTexCoord4ivARB)
#undef glMultiTexCoord4sARB
#define glMultiTexCoord4sARB (gl.MultiTexCoord4sARB)
#undef glMultiTexCoord4svARB
#define glMultiTexCoord4svARB (gl.MultiTexCoord4svARB)
#undef glLoadTransposeMatrixfARB
#define glLoadTransposeMatrixfARB (gl.LoadTransposeMatrixfARB)
#undef glLoadTransposeMatrixdARB
#define glLoadTransposeMatrixdARB (gl.LoadTransposeMatrixdARB)
#undef glMultTransposeMatrixfARB
#define glMultTransposeMatrixfARB (gl.MultTransposeMatrixfARB)
#undef glMultTransposeMatrixdARB
#define glMultTransposeMatrixdARB (gl.MultTransposeMatrixdARB)
#undef glSampleCoverageARB
#define glSampleCoverageARB (gl.SampleCoverageARB)
#undef glCompressedTexImage3DARB
#define glCompressedTexImage3DARB (gl.CompressedTexImage3DARB)
#undef glCompressedTexImage2DARB
#define glCompressedTexImage2DARB (gl.CompressedTexImage2DARB)
#undef glCompressedTexImage1DARB
#define glCompressedTexImage1DARB (gl.CompressedTexImage1DARB)
#undef glCompressedTexSubImage3DARB
#define glCompressedTexSubImage3DARB (gl.CompressedTexSubImage3DARB)
#undef glCompressedTexSubImage2DARB
#define glCompressedTexSubImage2DARB (gl.CompressedTexSubImage2DARB)
#undef glCompressedTexSubImage1DARB
#define glCompressedTexSubImage1DARB (gl.CompressedTexSubImage1DARB)
#undef glGetCompressedTexImageARB
#define glGetCompressedTexImageARB (gl.GetCompressedTexImageARB)
#undef glPointParameterfARB
#define glPointParameterfARB (gl.PointParameterfARB)
#undef glPointParameterfvARB
#define glPointParameterfvARB (gl.PointParameterfvARB)
#undef glWeightbvARB
#define glWeightbvARB (gl.WeightbvARB)
#undef glWeightsvARB
#define glWeightsvARB (gl.WeightsvARB)
#undef glWeightivARB
#define glWeightivARB (gl.WeightivARB)
#undef glWeightfvARB
#define glWeightfvARB (gl.WeightfvARB)
#undef glWeightdvARB
#define glWeightdvARB (gl.WeightdvARB)
#undef glWeightubvARB
#define glWeightubvARB (gl.WeightubvARB)
#undef glWeightusvARB
#define glWeightusvARB (gl.WeightusvARB)
#undef glWeightuivARB
#define glWeightuivARB (gl.WeightuivARB)
#undef glWeightPointerARB
#define glWeightPointerARB (gl.WeightPointerARB)
#undef glVertexBlendARB
#define glVertexBlendARB (gl.VertexBlendARB)
#undef glCurrentPaletteMatrixARB
#define glCurrentPaletteMatrixARB (gl.CurrentPaletteMatrixARB)
#undef glMatrixIndexubvARB
#define glMatrixIndexubvARB (gl.MatrixIndexubvARB)
#undef glMatrixIndexusvARB
#define glMatrixIndexusvARB (gl.MatrixIndexusvARB)
#undef glMatrixIndexuivARB
#define glMatrixIndexuivARB (gl.MatrixIndexuivARB)
#undef glMatrixIndexPointerARB
#define glMatrixIndexPointerARB (gl.MatrixIndexPointerARB)
#undef glWindowPos2dARB
#define glWindowPos2dARB (gl.WindowPos2dARB)
#undef glWindowPos2dvARB
#define glWindowPos2dvARB (gl.WindowPos2dvARB)
#undef glWindowPos2fARB
#define glWindowPos2fARB (gl.WindowPos2fARB)
#undef glWindowPos2fvARB
#define glWindowPos2fvARB (gl.WindowPos2fvARB)
#undef glWindowPos2iARB
#define glWindowPos2iARB (gl.WindowPos2iARB)
#undef glWindowPos2ivARB
#define glWindowPos2ivARB (gl.WindowPos2ivARB)
#undef glWindowPos2sARB
#define glWindowPos2sARB (gl.WindowPos2sARB)
#undef glWindowPos2svARB
#define glWindowPos2svARB (gl.WindowPos2svARB)
#undef glWindowPos3dARB
#define glWindowPos3dARB (gl.WindowPos3dARB)
#undef glWindowPos3dvARB
#define glWindowPos3dvARB (gl.WindowPos3dvARB)
#undef glWindowPos3fARB
#define glWindowPos3fARB (gl.WindowPos3fARB)
#undef glWindowPos3fvARB
#define glWindowPos3fvARB (gl.WindowPos3fvARB)
#undef glWindowPos3iARB
#define glWindowPos3iARB (gl.WindowPos3iARB)
#undef glWindowPos3ivARB
#define glWindowPos3ivARB (gl.WindowPos3ivARB)
#undef glWindowPos3sARB
#define glWindowPos3sARB (gl.WindowPos3sARB)
#undef glWindowPos3svARB
#define glWindowPos3svARB (gl.WindowPos3svARB)
#undef glVertexAttrib1dARB
#define glVertexAttrib1dARB (gl.VertexAttrib1dARB)
#undef glVertexAttrib1dvARB
#define glVertexAttrib1dvARB (gl.VertexAttrib1dvARB)
#undef glVertexAttrib1fARB
#define glVertexAttrib1fARB (gl.VertexAttrib1fARB)
#undef glVertexAttrib1fvARB
#define glVertexAttrib1fvARB (gl.VertexAttrib1fvARB)
#undef glVertexAttrib1sARB
#define glVertexAttrib1sARB (gl.VertexAttrib1sARB)
#undef glVertexAttrib1svARB
#define glVertexAttrib1svARB (gl.VertexAttrib1svARB)
#undef glVertexAttrib2dARB
#define glVertexAttrib2dARB (gl.VertexAttrib2dARB)
#undef glVertexAttrib2dvARB
#define glVertexAttrib2dvARB (gl.VertexAttrib2dvARB)
#undef glVertexAttrib2fARB
#define glVertexAttrib2fARB (gl.VertexAttrib2fARB)
#undef glVertexAttrib2fvARB
#define glVertexAttrib2fvARB (gl.VertexAttrib2fvARB)
#undef glVertexAttrib2sARB
#define glVertexAttrib2sARB (gl.VertexAttrib2sARB)
#undef glVertexAttrib2svARB
#define glVertexAttrib2svARB (gl.VertexAttrib2svARB)
#undef glVertexAttrib3dARB
#define glVertexAttrib3dARB (gl.VertexAttrib3dARB)
#undef glVertexAttrib3dvARB
#define glVertexAttrib3dvARB (gl.VertexAttrib3dvARB)
#undef glVertexAttrib3fARB
#define glVertexAttrib3fARB (gl.VertexAttrib3fARB)
#undef glVertexAttrib3fvARB
#define glVertexAttrib3fvARB (gl.VertexAttrib3fvARB)
#undef glVertexAttrib3sARB
#define glVertexAttrib3sARB (gl.VertexAttrib3sARB)
#undef glVertexAttrib3svARB
#define glVertexAttrib3svARB (gl.VertexAttrib3svARB)
#undef glVertexAttrib4NbvARB
#define glVertexAttrib4NbvARB (gl.VertexAttrib4NbvARB)
#undef glVertexAttrib4NivARB
#define glVertexAttrib4NivARB (gl.VertexAttrib4NivARB)
#undef glVertexAttrib4NsvARB
#define glVertexAttrib4NsvARB (gl.VertexAttrib4NsvARB)
#undef glVertexAttrib4NubARB
#define glVertexAttrib4NubARB (gl.VertexAttrib4NubARB)
#undef glVertexAttrib4NubvARB
#define glVertexAttrib4NubvARB (gl.VertexAttrib4NubvARB)
#undef glVertexAttrib4NuivARB
#define glVertexAttrib4NuivARB (gl.VertexAttrib4NuivARB)
#undef glVertexAttrib4NusvARB
#define glVertexAttrib4NusvARB (gl.VertexAttrib4NusvARB)
#undef glVertexAttrib4bvARB
#define glVertexAttrib4bvARB (gl.VertexAttrib4bvARB)
#undef glVertexAttrib4dARB
#define glVertexAttrib4dARB (gl.VertexAttrib4dARB)
#undef glVertexAttrib4dvARB
#define glVertexAttrib4dvARB (gl.VertexAttrib4dvARB)
#undef glVertexAttrib4fARB
#define glVertexAttrib4fARB (gl.VertexAttrib4fARB)
#undef glVertexAttrib4fvARB
#define glVertexAttrib4fvARB (gl.VertexAttrib4fvARB)
#undef glVertexAttrib4ivARB
#define glVertexAttrib4ivARB (gl.VertexAttrib4ivARB)
#undef glVertexAttrib4sARB
#define glVertexAttrib4sARB (gl.VertexAttrib4sARB)
#undef glVertexAttrib4svARB
#define glVertexAttrib4svARB (gl.VertexAttrib4svARB)
#undef glVertexAttrib4ubvARB
#define glVertexAttrib4ubvARB (gl.VertexAttrib4ubvARB)
#undef glVertexAttrib4uivARB
#define glVertexAttrib4uivARB (gl.VertexAttrib4uivARB)
#undef glVertexAttrib4usvARB
#define glVertexAttrib4usvARB (gl.VertexAttrib4usvARB)
#undef glVertexAttribPointerARB
#define glVertexAttribPointerARB (gl.VertexAttribPointerARB)
#undef glEnableVertexAttribArrayARB
#define glEnableVertexAttribArrayARB (gl.EnableVertexAttribArrayARB)
#undef glDisableVertexAttribArrayARB
#define glDisableVertexAttribArrayARB (gl.DisableVertexAttribArrayARB)
#undef glProgramStringARB
#define glProgramStringARB (gl.ProgramStringARB)
#undef glBindProgramARB
#define glBindProgramARB (gl.BindProgramARB)
#undef glDeleteProgramsARB
#define glDeleteProgramsARB (gl.DeleteProgramsARB)
#undef glGenProgramsARB
#define glGenProgramsARB (gl.GenProgramsARB)
#undef glProgramEnvParameter4dARB
#define glProgramEnvParameter4dARB (gl.ProgramEnvParameter4dARB)
#undef glProgramEnvParameter4dvARB
#define glProgramEnvParameter4dvARB (gl.ProgramEnvParameter4dvARB)
#undef glProgramEnvParameter4fARB
#define glProgramEnvParameter4fARB (gl.ProgramEnvParameter4fARB)
#undef glProgramEnvParameter4fvARB
#define glProgramEnvParameter4fvARB (gl.ProgramEnvParameter4fvARB)
#undef glProgramLocalParameter4dARB
#define glProgramLocalParameter4dARB (gl.ProgramLocalParameter4dARB)
#undef glProgramLocalParameter4dvARB
#define glProgramLocalParameter4dvARB (gl.ProgramLocalParameter4dvARB)
#undef glProgramLocalParameter4fARB
#define glProgramLocalParameter4fARB (gl.ProgramLocalParameter4fARB)
#undef glProgramLocalParameter4fvARB
#define glProgramLocalParameter4fvARB (gl.ProgramLocalParameter4fvARB)
#undef glGetProgramEnvParameterdvARB
#define glGetProgramEnvParameterdvARB (gl.GetProgramEnvParameterdvARB)
#undef glGetProgramEnvParameterfvARB
#define glGetProgramEnvParameterfvARB (gl.GetProgramEnvParameterfvARB)
#undef glGetProgramLocalParameterdvARB
#define glGetProgramLocalParameterdvARB (gl.GetProgramLocalParameterdvARB)
#undef glGetProgramLocalParameterfvARB
#define glGetProgramLocalParameterfvARB (gl.GetProgramLocalParameterfvARB)
#undef glGetProgramivARB
#define glGetProgramivARB (gl.GetProgramivARB)
#undef glGetProgramStringARB
#define glGetProgramStringARB (gl.GetProgramStringARB)
#undef glGetVertexAttribdvARB
#define glGetVertexAttribdvARB (gl.GetVertexAttribdvARB)
#undef glGetVertexAttribfvARB
#define glGetVertexAttribfvARB (gl.GetVertexAttribfvARB)
#undef glGetVertexAttribivARB
#define glGetVertexAttribivARB (gl.GetVertexAttribivARB)
#undef glGetVertexAttribPointervARB
#define glGetVertexAttribPointervARB (gl.GetVertexAttribPointervARB)
#undef glIsProgramARB
#define glIsProgramARB (gl.IsProgramARB)
#undef glBindBufferARB
#define glBindBufferARB (gl.BindBufferARB)
#undef glDeleteBuffersARB
#define glDeleteBuffersARB (gl.DeleteBuffersARB)
#undef glGenBuffersARB
#define glGenBuffersARB (gl.GenBuffersARB)
#undef glIsBufferARB
#define glIsBufferARB (gl.IsBufferARB)
#undef glBufferDataARB
#define glBufferDataARB (gl.BufferDataARB)
#undef glBufferSubDataARB
#define glBufferSubDataARB (gl.BufferSubDataARB)
#undef glGetBufferSubDataARB
#define glGetBufferSubDataARB (gl.GetBufferSubDataARB)
#undef glMapBufferARB
#define glMapBufferARB (gl.MapBufferARB)
#undef glUnmapBufferARB
#define glUnmapBufferARB (gl.UnmapBufferARB)
#undef glGetBufferParameterivARB
#define glGetBufferParameterivARB (gl.GetBufferParameterivARB)
#undef glGetBufferPointervARB
#define glGetBufferPointervARB (gl.GetBufferPointervARB)
#undef glGenQueriesARB
#define glGenQueriesARB (gl.GenQueriesARB)
#undef glDeleteQueriesARB
#define glDeleteQueriesARB (gl.DeleteQueriesARB)
#undef glIsQueryARB
#define glIsQueryARB (gl.IsQueryARB)
#undef glBeginQueryARB
#define glBeginQueryARB (gl.BeginQueryARB)
#undef glEndQueryARB
#define glEndQueryARB (gl.EndQueryARB)
#undef glGetQueryivARB
#define glGetQueryivARB (gl.GetQueryivARB)
#undef glGetQueryObjectivARB
#define glGetQueryObjectivARB (gl.GetQueryObjectivARB)
#undef glGetQueryObjectuivARB
#define glGetQueryObjectuivARB (gl.GetQueryObjectuivARB)
#undef glDeleteObjectARB
#define glDeleteObjectARB (gl.DeleteObjectARB)
#undef glGetHandleARB
#define glGetHandleARB (gl.GetHandleARB)
#undef glDetachObjectARB
#define glDetachObjectARB (gl.DetachObjectARB)
#undef glCreateShaderObjectARB
#define glCreateShaderObjectARB (gl.CreateShaderObjectARB)
#undef glShaderSourceARB
#define glShaderSourceARB (gl.ShaderSourceARB)
#undef glCompileShaderARB
#define glCompileShaderARB (gl.CompileShaderARB)
#undef glCreateProgramObjectARB
#define glCreateProgramObjectARB (gl.CreateProgramObjectARB)
#undef glAttachObjectARB
#define glAttachObjectARB (gl.AttachObjectARB)
#undef glLinkProgramARB
#define glLinkProgramARB (gl.LinkProgramARB)
#undef glUseProgramObjectARB
#define glUseProgramObjectARB (gl.UseProgramObjectARB)
#undef glValidateProgramARB
#define glValidateProgramARB (gl.ValidateProgramARB)
#undef glUniform1fARB
#define glUniform1fARB (gl.Uniform1fARB)
#undef glUniform2fARB
#define glUniform2fARB (gl.Uniform2fARB)
#undef glUniform3fARB
#define glUniform3fARB (gl.Uniform3fARB)
#undef glUniform4fARB
#define glUniform4fARB (gl.Uniform4fARB)
#undef glUniform1iARB
#define glUniform1iARB (gl.Uniform1iARB)
#undef glUniform2iARB
#define glUniform2iARB (gl.Uniform2iARB)
#undef glUniform3iARB
#define glUniform3iARB (gl.Uniform3iARB)
#undef glUniform4iARB
#define glUniform4iARB (gl.Uniform4iARB)
#undef glUniform1fvARB
#define glUniform1fvARB (gl.Uniform1fvARB)
#undef glUniform2fvARB
#define glUniform2fvARB (gl.Uniform2fvARB)
#undef glUniform3fvARB
#define glUniform3fvARB (gl.Uniform3fvARB)
#undef glUniform4fvARB
#define glUniform4fvARB (gl.Uniform4fvARB)
#undef glUniform1ivARB
#define glUniform1ivARB (gl.Uniform1ivARB)
#undef glUniform2ivARB
#define glUniform2ivARB (gl.Uniform2ivARB)
#undef glUniform3ivARB
#define glUniform3ivARB (gl.Uniform3ivARB)
#undef glUniform4ivARB
#define glUniform4ivARB (gl.Uniform4ivARB)
#undef glUniformMatrix2fvARB
#define glUniformMatrix2fvARB (gl.UniformMatrix2fvARB)
#undef glUniformMatrix3fvARB
#define glUniformMatrix3fvARB (gl.UniformMatrix3fvARB)
#undef glUniformMatrix4fvARB
#define glUniformMatrix4fvARB (gl.UniformMatrix4fvARB)
#undef glGetObjectParameterfvARB
#define glGetObjectParameterfvARB (gl.GetObjectParameterfvARB)
#undef glGetObjectParameterivARB
#define glGetObjectParameterivARB (gl.GetObjectParameterivARB)
#undef glGetInfoLogARB
#define glGetInfoLogARB (gl.GetInfoLogARB)
#undef glGetAttachedObjectsARB
#define glGetAttachedObjectsARB (gl.GetAttachedObjectsARB)
#undef glGetUniformLocationARB
#define glGetUniformLocationARB (gl.GetUniformLocationARB)
#undef glGetActiveUniformARB
#define glGetActiveUniformARB (gl.GetActiveUniformARB)
#undef glGetUniformfvARB
#define glGetUniformfvARB (gl.GetUniformfvARB)
#undef glGetUniformivARB
#define glGetUniformivARB (gl.GetUniformivARB)
#undef glGetShaderSourceARB
#define glGetShaderSourceARB (gl.GetShaderSourceARB)
#undef glBindAttribLocationARB
#define glBindAttribLocationARB (gl.BindAttribLocationARB)
#undef glGetActiveAttribARB
#define glGetActiveAttribARB (gl.GetActiveAttribARB)
#undef glGetAttribLocationARB
#define glGetAttribLocationARB (gl.GetAttribLocationARB)
#undef glDrawBuffersARB
#define glDrawBuffersARB (gl.DrawBuffersARB)
#undef glBlendColorEXT
#define glBlendColorEXT (gl.BlendColorEXT)
#undef glPolygonOffsetEXT
#define glPolygonOffsetEXT (gl.PolygonOffsetEXT)
#undef glTexImage3DEXT
#define glTexImage3DEXT (gl.TexImage3DEXT)
#undef glTexSubImage3DEXT
#define glTexSubImage3DEXT (gl.TexSubImage3DEXT)
#undef glGetTexFilterFuncSGIS
#define glGetTexFilterFuncSGIS (gl.GetTexFilterFuncSGIS)
#undef glTexFilterFuncSGIS
#define glTexFilterFuncSGIS (gl.TexFilterFuncSGIS)
#undef glTexSubImage1DEXT
#define glTexSubImage1DEXT (gl.TexSubImage1DEXT)
#undef glTexSubImage2DEXT
#define glTexSubImage2DEXT (gl.TexSubImage2DEXT)
#undef glCopyTexImage1DEXT
#define glCopyTexImage1DEXT (gl.CopyTexImage1DEXT)
#undef glCopyTexImage2DEXT
#define glCopyTexImage2DEXT (gl.CopyTexImage2DEXT)
#undef glCopyTexSubImage1DEXT
#define glCopyTexSubImage1DEXT (gl.CopyTexSubImage1DEXT)
#undef glCopyTexSubImage2DEXT
#define glCopyTexSubImage2DEXT (gl.CopyTexSubImage2DEXT)
#undef glCopyTexSubImage3DEXT
#define glCopyTexSubImage3DEXT (gl.CopyTexSubImage3DEXT)
#undef glGetHistogramEXT
#define glGetHistogramEXT (gl.GetHistogramEXT)
#undef glGetHistogramParameterfvEXT
#define glGetHistogramParameterfvEXT (gl.GetHistogramParameterfvEXT)
#undef glGetHistogramParameterivEXT
#define glGetHistogramParameterivEXT (gl.GetHistogramParameterivEXT)
#undef glGetMinmaxEXT
#define glGetMinmaxEXT (gl.GetMinmaxEXT)
#undef glGetMinmaxParameterfvEXT
#define glGetMinmaxParameterfvEXT (gl.GetMinmaxParameterfvEXT)
#undef glGetMinmaxParameterivEXT
#define glGetMinmaxParameterivEXT (gl.GetMinmaxParameterivEXT)
#undef glHistogramEXT
#define glHistogramEXT (gl.HistogramEXT)
#undef glMinmaxEXT
#define glMinmaxEXT (gl.MinmaxEXT)
#undef glResetHistogramEXT
#define glResetHistogramEXT (gl.ResetHistogramEXT)
#undef glResetMinmaxEXT
#define glResetMinmaxEXT (gl.ResetMinmaxEXT)
#undef glConvolutionFilter1DEXT
#define glConvolutionFilter1DEXT (gl.ConvolutionFilter1DEXT)
#undef glConvolutionFilter2DEXT
#define glConvolutionFilter2DEXT (gl.ConvolutionFilter2DEXT)
#undef glConvolutionParameterfEXT
#define glConvolutionParameterfEXT (gl.ConvolutionParameterfEXT)
#undef glConvolutionParameterfvEXT
#define glConvolutionParameterfvEXT (gl.ConvolutionParameterfvEXT)
#undef glConvolutionParameteriEXT
#define glConvolutionParameteriEXT (gl.ConvolutionParameteriEXT)
#undef glConvolutionParameterivEXT
#define glConvolutionParameterivEXT (gl.ConvolutionParameterivEXT)
#undef glCopyConvolutionFilter1DEXT
#define glCopyConvolutionFilter1DEXT (gl.CopyConvolutionFilter1DEXT)
#undef glCopyConvolutionFilter2DEXT
#define glCopyConvolutionFilter2DEXT (gl.CopyConvolutionFilter2DEXT)
#undef glGetConvolutionFilterEXT
#define glGetConvolutionFilterEXT (gl.GetConvolutionFilterEXT)
#undef glGetConvolutionParameterfvEXT
#define glGetConvolutionParameterfvEXT (gl.GetConvolutionParameterfvEXT)
#undef glGetConvolutionParameterivEXT
#define glGetConvolutionParameterivEXT (gl.GetConvolutionParameterivEXT)
#undef glGetSeparableFilterEXT
#define glGetSeparableFilterEXT (gl.GetSeparableFilterEXT)
#undef glSeparableFilter2DEXT
#define glSeparableFilter2DEXT (gl.SeparableFilter2DEXT)
#undef glColorTableSGI
#define glColorTableSGI (gl.ColorTableSGI)
#undef glColorTableParameterfvSGI
#define glColorTableParameterfvSGI (gl.ColorTableParameterfvSGI)
#undef glColorTableParameterivSGI
#define glColorTableParameterivSGI (gl.ColorTableParameterivSGI)
#undef glCopyColorTableSGI
#define glCopyColorTableSGI (gl.CopyColorTableSGI)
#undef glGetColorTableSGI
#define glGetColorTableSGI (gl.GetColorTableSGI)
#undef glGetColorTableParameterfvSGI
#define glGetColorTableParameterfvSGI (gl.GetColorTableParameterfvSGI)
#undef glGetColorTableParameterivSGI
#define glGetColorTableParameterivSGI (gl.GetColorTableParameterivSGI)
#undef glPixelTexGenSGIX
#define glPixelTexGenSGIX (gl.PixelTexGenSGIX)
#undef glPixelTexGenParameteriSGIS
#define glPixelTexGenParameteriSGIS (gl.PixelTexGenParameteriSGIS)
#undef glPixelTexGenParameterivSGIS
#define glPixelTexGenParameterivSGIS (gl.PixelTexGenParameterivSGIS)
#undef glPixelTexGenParameterfSGIS
#define glPixelTexGenParameterfSGIS (gl.PixelTexGenParameterfSGIS)
#undef glPixelTexGenParameterfvSGIS
#define glPixelTexGenParameterfvSGIS (gl.PixelTexGenParameterfvSGIS)
#undef glGetPixelTexGenParameterivSGIS
#define glGetPixelTexGenParameterivSGIS (gl.GetPixelTexGenParameterivSGIS)
#undef glGetPixelTexGenParameterfvSGIS
#define glGetPixelTexGenParameterfvSGIS (gl.GetPixelTexGenParameterfvSGIS)
#undef glTexImage4DSGIS
#define glTexImage4DSGIS (gl.TexImage4DSGIS)
#undef glTexSubImage4DSGIS
#define glTexSubImage4DSGIS (gl.TexSubImage4DSGIS)
#undef glAreTexturesResidentEXT
#define glAreTexturesResidentEXT (gl.AreTexturesResidentEXT)
#undef glBindTextureEXT
#define glBindTextureEXT (gl.BindTextureEXT)
#undef glDeleteTexturesEXT
#define glDeleteTexturesEXT (gl.DeleteTexturesEXT)
#undef glGenTexturesEXT
#define glGenTexturesEXT (gl.GenTexturesEXT)
#undef glIsTextureEXT
#define glIsTextureEXT (gl.IsTextureEXT)
#undef glPrioritizeTexturesEXT
#define glPrioritizeTexturesEXT (gl.PrioritizeTexturesEXT)
#undef glDetailTexFuncSGIS
#define glDetailTexFuncSGIS (gl.DetailTexFuncSGIS)
#undef glGetDetailTexFuncSGIS
#define glGetDetailTexFuncSGIS (gl.GetDetailTexFuncSGIS)
#undef glSharpenTexFuncSGIS
#define glSharpenTexFuncSGIS (gl.SharpenTexFuncSGIS)
#undef glGetSharpenTexFuncSGIS
#define glGetSharpenTexFuncSGIS (gl.GetSharpenTexFuncSGIS)
#undef glSampleMaskSGIS
#define glSampleMaskSGIS (gl.SampleMaskSGIS)
#undef glSamplePatternSGIS
#define glSamplePatternSGIS (gl.SamplePatternSGIS)
#undef glArrayElementEXT
#define glArrayElementEXT (gl.ArrayElementEXT)
#undef glColorPointerEXT
#define glColorPointerEXT (gl.ColorPointerEXT)
#undef glDrawArraysEXT
#define glDrawArraysEXT (gl.DrawArraysEXT)
#undef glEdgeFlagPointerEXT
#define glEdgeFlagPointerEXT (gl.EdgeFlagPointerEXT)
#undef glGetPointervEXT
#define glGetPointervEXT (gl.GetPointervEXT)
#undef glIndexPointerEXT
#define glIndexPointerEXT (gl.IndexPointerEXT)
#undef glNormalPointerEXT
#define glNormalPointerEXT (gl.NormalPointerEXT)
#undef glTexCoordPointerEXT
#define glTexCoordPointerEXT (gl.TexCoordPointerEXT)
#undef glVertexPointerEXT
#define glVertexPointerEXT (gl.VertexPointerEXT)
#undef glBlendEquationEXT
#define glBlendEquationEXT (gl.BlendEquationEXT)
#undef glSpriteParameterfSGIX
#define glSpriteParameterfSGIX (gl.SpriteParameterfSGIX)
#undef glSpriteParameterfvSGIX
#define glSpriteParameterfvSGIX (gl.SpriteParameterfvSGIX)
#undef glSpriteParameteriSGIX
#define glSpriteParameteriSGIX (gl.SpriteParameteriSGIX)
#undef glSpriteParameterivSGIX
#define glSpriteParameterivSGIX (gl.SpriteParameterivSGIX)
#undef glPointParameterfEXT
#define glPointParameterfEXT (gl.PointParameterfEXT)
#undef glPointParameterfvEXT
#define glPointParameterfvEXT (gl.PointParameterfvEXT)
#undef glPointParameterfSGIS
#define glPointParameterfSGIS (gl.PointParameterfSGIS)
#undef glPointParameterfvSGIS
#define glPointParameterfvSGIS (gl.PointParameterfvSGIS)
#undef glGetInstrumentsSGIX
#define glGetInstrumentsSGIX (gl.GetInstrumentsSGIX)
#undef glInstrumentsBufferSGIX
#define glInstrumentsBufferSGIX (gl.InstrumentsBufferSGIX)
#undef glPollInstrumentsSGIX
#define glPollInstrumentsSGIX (gl.PollInstrumentsSGIX)
#undef glReadInstrumentsSGIX
#define glReadInstrumentsSGIX (gl.ReadInstrumentsSGIX)
#undef glStartInstrumentsSGIX
#define glStartInstrumentsSGIX (gl.StartInstrumentsSGIX)
#undef glStopInstrumentsSGIX
#define glStopInstrumentsSGIX (gl.StopInstrumentsSGIX)
#undef glFrameZoomSGIX
#define glFrameZoomSGIX (gl.FrameZoomSGIX)
#undef glTagSampleBufferSGIX
#define glTagSampleBufferSGIX (gl.TagSampleBufferSGIX)
#undef glDeformationMap3dSGIX
#define glDeformationMap3dSGIX (gl.DeformationMap3dSGIX)
#undef glDeformationMap3fSGIX
#define glDeformationMap3fSGIX (gl.DeformationMap3fSGIX)
#undef glDeformSGIX
#define glDeformSGIX (gl.DeformSGIX)
#undef glLoadIdentityDeformationMapSGIX
#define glLoadIdentityDeformationMapSGIX (gl.LoadIdentityDeformationMapSGIX)
#undef glReferencePlaneSGIX
#define glReferencePlaneSGIX (gl.ReferencePlaneSGIX)
#undef glFlushRasterSGIX
#define glFlushRasterSGIX (gl.FlushRasterSGIX)
#undef glFogFuncSGIS
#define glFogFuncSGIS (gl.FogFuncSGIS)
#undef glGetFogFuncSGIS
#define glGetFogFuncSGIS (gl.GetFogFuncSGIS)
#undef glImageTransformParameteriHP
#define glImageTransformParameteriHP (gl.ImageTransformParameteriHP)
#undef glImageTransformParameterfHP
#define glImageTransformParameterfHP (gl.ImageTransformParameterfHP)
#undef glImageTransformParameterivHP
#define glImageTransformParameterivHP (gl.ImageTransformParameterivHP)
#undef glImageTransformParameterfvHP
#define glImageTransformParameterfvHP (gl.ImageTransformParameterfvHP)
#undef glGetImageTransformParameterivHP
#define glGetImageTransformParameterivHP (gl.GetImageTransformParameterivHP)
#undef glGetImageTransformParameterfvHP
#define glGetImageTransformParameterfvHP (gl.GetImageTransformParameterfvHP)
#undef glColorSubTableEXT
#define glColorSubTableEXT (gl.ColorSubTableEXT)
#undef glCopyColorSubTableEXT
#define glCopyColorSubTableEXT (gl.CopyColorSubTableEXT)
#undef glHintPGI
#define glHintPGI (gl.HintPGI)
#undef glColorTableEXT
#define glColorTableEXT (gl.ColorTableEXT)
#undef glGetColorTableEXT
#define glGetColorTableEXT (gl.GetColorTableEXT)
#undef glGetColorTableParameterivEXT
#define glGetColorTableParameterivEXT (gl.GetColorTableParameterivEXT)
#undef glGetColorTableParameterfvEXT
#define glGetColorTableParameterfvEXT (gl.GetColorTableParameterfvEXT)
#undef glGetListParameterfvSGIX
#define glGetListParameterfvSGIX (gl.GetListParameterfvSGIX)
#undef glGetListParameterivSGIX
#define glGetListParameterivSGIX (gl.GetListParameterivSGIX)
#undef glListParameterfSGIX
#define glListParameterfSGIX (gl.ListParameterfSGIX)
#undef glListParameterfvSGIX
#define glListParameterfvSGIX (gl.ListParameterfvSGIX)
#undef glListParameteriSGIX
#define glListParameteriSGIX (gl.ListParameteriSGIX)
#undef glListParameterivSGIX
#define glListParameterivSGIX (gl.ListParameterivSGIX)
#undef glIndexMaterialEXT
#define glIndexMaterialEXT (gl.IndexMaterialEXT)
#undef glIndexFuncEXT
#define glIndexFuncEXT (gl.IndexFuncEXT)
#undef glLockArraysEXT
#define glLockArraysEXT (gl.LockArraysEXT)
#undef glUnlockArraysEXT
#define glUnlockArraysEXT (gl.UnlockArraysEXT)
#undef glCullParameterdvEXT
#define glCullParameterdvEXT (gl.CullParameterdvEXT)
#undef glCullParameterfvEXT
#define glCullParameterfvEXT (gl.CullParameterfvEXT)
#undef glFragmentColorMaterialSGIX
#define glFragmentColorMaterialSGIX (gl.FragmentColorMaterialSGIX)
#undef glFragmentLightfSGIX
#define glFragmentLightfSGIX (gl.FragmentLightfSGIX)
#undef glFragmentLightfvSGIX
#define glFragmentLightfvSGIX (gl.FragmentLightfvSGIX)
#undef glFragmentLightiSGIX
#define glFragmentLightiSGIX (gl.FragmentLightiSGIX)
#undef glFragmentLightivSGIX
#define glFragmentLightivSGIX (gl.FragmentLightivSGIX)
#undef glFragmentLightModelfSGIX
#define glFragmentLightModelfSGIX (gl.FragmentLightModelfSGIX)
#undef glFragmentLightModelfvSGIX
#define glFragmentLightModelfvSGIX (gl.FragmentLightModelfvSGIX)
#undef glFragmentLightModeliSGIX
#define glFragmentLightModeliSGIX (gl.FragmentLightModeliSGIX)
#undef glFragmentLightModelivSGIX
#define glFragmentLightModelivSGIX (gl.FragmentLightModelivSGIX)
#undef glFragmentMaterialfSGIX
#define glFragmentMaterialfSGIX (gl.FragmentMaterialfSGIX)
#undef glFragmentMaterialfvSGIX
#define glFragmentMaterialfvSGIX (gl.FragmentMaterialfvSGIX)
#undef glFragmentMaterialiSGIX
#define glFragmentMaterialiSGIX (gl.FragmentMaterialiSGIX)
#undef glFragmentMaterialivSGIX
#define glFragmentMaterialivSGIX (gl.FragmentMaterialivSGIX)
#undef glGetFragmentLightfvSGIX
#define glGetFragmentLightfvSGIX (gl.GetFragmentLightfvSGIX)
#undef glGetFragmentLightivSGIX
#define glGetFragmentLightivSGIX (gl.GetFragmentLightivSGIX)
#undef glGetFragmentMaterialfvSGIX
#define glGetFragmentMaterialfvSGIX (gl.GetFragmentMaterialfvSGIX)
#undef glGetFragmentMaterialivSGIX
#define glGetFragmentMaterialivSGIX (gl.GetFragmentMaterialivSGIX)
#undef glLightEnviSGIX
#define glLightEnviSGIX (gl.LightEnviSGIX)
#undef glDrawRangeElementsEXT
#define glDrawRangeElementsEXT (gl.DrawRangeElementsEXT)
#undef glApplyTextureEXT
#define glApplyTextureEXT (gl.ApplyTextureEXT)
#undef glTextureLightEXT
#define glTextureLightEXT (gl.TextureLightEXT)
#undef glTextureMaterialEXT
#define glTextureMaterialEXT (gl.TextureMaterialEXT)
#undef glAsyncMarkerSGIX
#define glAsyncMarkerSGIX (gl.AsyncMarkerSGIX)
#undef glFinishAsyncSGIX
#define glFinishAsyncSGIX (gl.FinishAsyncSGIX)
#undef glPollAsyncSGIX
#define glPollAsyncSGIX (gl.PollAsyncSGIX)
#undef glGenAsyncMarkersSGIX
#define glGenAsyncMarkersSGIX (gl.GenAsyncMarkersSGIX)
#undef glDeleteAsyncMarkersSGIX
#define glDeleteAsyncMarkersSGIX (gl.DeleteAsyncMarkersSGIX)
#undef glIsAsyncMarkerSGIX
#define glIsAsyncMarkerSGIX (gl.IsAsyncMarkerSGIX)
#undef glVertexPointervINTEL
#define glVertexPointervINTEL (gl.VertexPointervINTEL)
#undef glNormalPointervINTEL
#define glNormalPointervINTEL (gl.NormalPointervINTEL)
#undef glColorPointervINTEL
#define glColorPointervINTEL (gl.ColorPointervINTEL)
#undef glTexCoordPointervINTEL
#define glTexCoordPointervINTEL (gl.TexCoordPointervINTEL)
#undef glPixelTransformParameteriEXT
#define glPixelTransformParameteriEXT (gl.PixelTransformParameteriEXT)
#undef glPixelTransformParameterfEXT
#define glPixelTransformParameterfEXT (gl.PixelTransformParameterfEXT)
#undef glPixelTransformParameterivEXT
#define glPixelTransformParameterivEXT (gl.PixelTransformParameterivEXT)
#undef glPixelTransformParameterfvEXT
#define glPixelTransformParameterfvEXT (gl.PixelTransformParameterfvEXT)
#undef glSecondaryColor3bEXT
#define glSecondaryColor3bEXT (gl.SecondaryColor3bEXT)
#undef glSecondaryColor3bvEXT
#define glSecondaryColor3bvEXT (gl.SecondaryColor3bvEXT)
#undef glSecondaryColor3dEXT
#define glSecondaryColor3dEXT (gl.SecondaryColor3dEXT)
#undef glSecondaryColor3dvEXT
#define glSecondaryColor3dvEXT (gl.SecondaryColor3dvEXT)
#undef glSecondaryColor3fEXT
#define glSecondaryColor3fEXT (gl.SecondaryColor3fEXT)
#undef glSecondaryColor3fvEXT
#define glSecondaryColor3fvEXT (gl.SecondaryColor3fvEXT)
#undef glSecondaryColor3iEXT
#define glSecondaryColor3iEXT (gl.SecondaryColor3iEXT)
#undef glSecondaryColor3ivEXT
#define glSecondaryColor3ivEXT (gl.SecondaryColor3ivEXT)
#undef glSecondaryColor3sEXT
#define glSecondaryColor3sEXT (gl.SecondaryColor3sEXT)
#undef glSecondaryColor3svEXT
#define glSecondaryColor3svEXT (gl.SecondaryColor3svEXT)
#undef glSecondaryColor3ubEXT
#define glSecondaryColor3ubEXT (gl.SecondaryColor3ubEXT)
#undef glSecondaryColor3ubvEXT
#define glSecondaryColor3ubvEXT (gl.SecondaryColor3ubvEXT)
#undef glSecondaryColor3uiEXT
#define glSecondaryColor3uiEXT (gl.SecondaryColor3uiEXT)
#undef glSecondaryColor3uivEXT
#define glSecondaryColor3uivEXT (gl.SecondaryColor3uivEXT)
#undef glSecondaryColor3usEXT
#define glSecondaryColor3usEXT (gl.SecondaryColor3usEXT)
#undef glSecondaryColor3usvEXT
#define glSecondaryColor3usvEXT (gl.SecondaryColor3usvEXT)
#undef glSecondaryColorPointerEXT
#define glSecondaryColorPointerEXT (gl.SecondaryColorPointerEXT)
#undef glTextureNormalEXT
#define glTextureNormalEXT (gl.TextureNormalEXT)
#undef glMultiDrawArraysEXT
#define glMultiDrawArraysEXT (gl.MultiDrawArraysEXT)
#undef glMultiDrawElementsEXT
#define glMultiDrawElementsEXT (gl.MultiDrawElementsEXT)
#undef glFogCoordfEXT
#define glFogCoordfEXT (gl.FogCoordfEXT)
#undef glFogCoordfvEXT
#define glFogCoordfvEXT (gl.FogCoordfvEXT)
#undef glFogCoorddEXT
#define glFogCoorddEXT (gl.FogCoorddEXT)
#undef glFogCoorddvEXT
#define glFogCoorddvEXT (gl.FogCoorddvEXT)
#undef glFogCoordPointerEXT
#define glFogCoordPointerEXT (gl.FogCoordPointerEXT)
#undef glTangent3bEXT
#define glTangent3bEXT (gl.Tangent3bEXT)
#undef glTangent3bvEXT
#define glTangent3bvEXT (gl.Tangent3bvEXT)
#undef glTangent3dEXT
#define glTangent3dEXT (gl.Tangent3dEXT)
#undef glTangent3dvEXT
#define glTangent3dvEXT (gl.Tangent3dvEXT)
#undef glTangent3fEXT
#define glTangent3fEXT (gl.Tangent3fEXT)
#undef glTangent3fvEXT
#define glTangent3fvEXT (gl.Tangent3fvEXT)
#undef glTangent3iEXT
#define glTangent3iEXT (gl.Tangent3iEXT)
#undef glTangent3ivEXT
#define glTangent3ivEXT (gl.Tangent3ivEXT)
#undef glTangent3sEXT
#define glTangent3sEXT (gl.Tangent3sEXT)
#undef glTangent3svEXT
#define glTangent3svEXT (gl.Tangent3svEXT)
#undef glBinormal3bEXT
#define glBinormal3bEXT (gl.Binormal3bEXT)
#undef glBinormal3bvEXT
#define glBinormal3bvEXT (gl.Binormal3bvEXT)
#undef glBinormal3dEXT
#define glBinormal3dEXT (gl.Binormal3dEXT)
#undef glBinormal3dvEXT
#define glBinormal3dvEXT (gl.Binormal3dvEXT)
#undef glBinormal3fEXT
#define glBinormal3fEXT (gl.Binormal3fEXT)
#undef glBinormal3fvEXT
#define glBinormal3fvEXT (gl.Binormal3fvEXT)
#undef glBinormal3iEXT
#define glBinormal3iEXT (gl.Binormal3iEXT)
#undef glBinormal3ivEXT
#define glBinormal3ivEXT (gl.Binormal3ivEXT)
#undef glBinormal3sEXT
#define glBinormal3sEXT (gl.Binormal3sEXT)
#undef glBinormal3svEXT
#define glBinormal3svEXT (gl.Binormal3svEXT)
#undef glTangentPointerEXT
#define glTangentPointerEXT (gl.TangentPointerEXT)
#undef glBinormalPointerEXT
#define glBinormalPointerEXT (gl.BinormalPointerEXT)
#undef glFinishTextureSUNX
#define glFinishTextureSUNX (gl.FinishTextureSUNX)
#undef glGlobalAlphaFactorbSUN
#define glGlobalAlphaFactorbSUN (gl.GlobalAlphaFactorbSUN)
#undef glGlobalAlphaFactorsSUN
#define glGlobalAlphaFactorsSUN (gl.GlobalAlphaFactorsSUN)
#undef glGlobalAlphaFactoriSUN
#define glGlobalAlphaFactoriSUN (gl.GlobalAlphaFactoriSUN)
#undef glGlobalAlphaFactorfSUN
#define glGlobalAlphaFactorfSUN (gl.GlobalAlphaFactorfSUN)
#undef glGlobalAlphaFactordSUN
#define glGlobalAlphaFactordSUN (gl.GlobalAlphaFactordSUN)
#undef glGlobalAlphaFactorubSUN
#define glGlobalAlphaFactorubSUN (gl.GlobalAlphaFactorubSUN)
#undef glGlobalAlphaFactorusSUN
#define glGlobalAlphaFactorusSUN (gl.GlobalAlphaFactorusSUN)
#undef glGlobalAlphaFactoruiSUN
#define glGlobalAlphaFactoruiSUN (gl.GlobalAlphaFactoruiSUN)
#undef glReplacementCodeuiSUN
#define glReplacementCodeuiSUN (gl.ReplacementCodeuiSUN)
#undef glReplacementCodeusSUN
#define glReplacementCodeusSUN (gl.ReplacementCodeusSUN)
#undef glReplacementCodeubSUN
#define glReplacementCodeubSUN (gl.ReplacementCodeubSUN)
#undef glReplacementCodeuivSUN
#define glReplacementCodeuivSUN (gl.ReplacementCodeuivSUN)
#undef glReplacementCodeusvSUN
#define glReplacementCodeusvSUN (gl.ReplacementCodeusvSUN)
#undef glReplacementCodeubvSUN
#define glReplacementCodeubvSUN (gl.ReplacementCodeubvSUN)
#undef glReplacementCodePointerSUN
#define glReplacementCodePointerSUN (gl.ReplacementCodePointerSUN)
#undef glColor4ubVertex2fSUN
#define glColor4ubVertex2fSUN (gl.Color4ubVertex2fSUN)
#undef glColor4ubVertex2fvSUN
#define glColor4ubVertex2fvSUN (gl.Color4ubVertex2fvSUN)
#undef glColor4ubVertex3fSUN
#define glColor4ubVertex3fSUN (gl.Color4ubVertex3fSUN)
#undef glColor4ubVertex3fvSUN
#define glColor4ubVertex3fvSUN (gl.Color4ubVertex3fvSUN)
#undef glColor3fVertex3fSUN
#define glColor3fVertex3fSUN (gl.Color3fVertex3fSUN)
#undef glColor3fVertex3fvSUN
#define glColor3fVertex3fvSUN (gl.Color3fVertex3fvSUN)
#undef glNormal3fVertex3fSUN
#define glNormal3fVertex3fSUN (gl.Normal3fVertex3fSUN)
#undef glNormal3fVertex3fvSUN
#define glNormal3fVertex3fvSUN (gl.Normal3fVertex3fvSUN)
#undef glColor4fNormal3fVertex3fSUN
#define glColor4fNormal3fVertex3fSUN (gl.Color4fNormal3fVertex3fSUN)
#undef glColor4fNormal3fVertex3fvSUN
#define glColor4fNormal3fVertex3fvSUN (gl.Color4fNormal3fVertex3fvSUN)
#undef glTexCoord2fVertex3fSUN
#define glTexCoord2fVertex3fSUN (gl.TexCoord2fVertex3fSUN)
#undef glTexCoord2fVertex3fvSUN
#define glTexCoord2fVertex3fvSUN (gl.TexCoord2fVertex3fvSUN)
#undef glTexCoord4fVertex4fSUN
#define glTexCoord4fVertex4fSUN (gl.TexCoord4fVertex4fSUN)
#undef glTexCoord4fVertex4fvSUN
#define glTexCoord4fVertex4fvSUN (gl.TexCoord4fVertex4fvSUN)
#undef glTexCoord2fColor4ubVertex3fSUN
#define glTexCoord2fColor4ubVertex3fSUN (gl.TexCoord2fColor4ubVertex3fSUN)
#undef glTexCoord2fColor4ubVertex3fvSUN
#define glTexCoord2fColor4ubVertex3fvSUN (gl.TexCoord2fColor4ubVertex3fvSUN)
#undef glTexCoord2fColor3fVertex3fSUN
#define glTexCoord2fColor3fVertex3fSUN (gl.TexCoord2fColor3fVertex3fSUN)
#undef glTexCoord2fColor3fVertex3fvSUN
#define glTexCoord2fColor3fVertex3fvSUN (gl.TexCoord2fColor3fVertex3fvSUN)
#undef glTexCoord2fNormal3fVertex3fSUN
#define glTexCoord2fNormal3fVertex3fSUN (gl.TexCoord2fNormal3fVertex3fSUN)
#undef glTexCoord2fNormal3fVertex3fvSUN
#define glTexCoord2fNormal3fVertex3fvSUN (gl.TexCoord2fNormal3fVertex3fvSUN)
#undef glTexCoord2fColor4fNormal3fVertex3fSUN
#define glTexCoord2fColor4fNormal3fVertex3fSUN (gl.TexCoord2fColor4fNormal3fVertex3fSUN)
#undef glTexCoord2fColor4fNormal3fVertex3fvSUN
#define glTexCoord2fColor4fNormal3fVertex3fvSUN (gl.TexCoord2fColor4fNormal3fVertex3fvSUN)
#undef glTexCoord4fColor4fNormal3fVertex4fSUN
#define glTexCoord4fColor4fNormal3fVertex4fSUN (gl.TexCoord4fColor4fNormal3fVertex4fSUN)
#undef glTexCoord4fColor4fNormal3fVertex4fvSUN
#define glTexCoord4fColor4fNormal3fVertex4fvSUN (gl.TexCoord4fColor4fNormal3fVertex4fvSUN)
#undef glReplacementCodeuiVertex3fSUN
#define glReplacementCodeuiVertex3fSUN (gl.ReplacementCodeuiVertex3fSUN)
#undef glReplacementCodeuiVertex3fvSUN
#define glReplacementCodeuiVertex3fvSUN (gl.ReplacementCodeuiVertex3fvSUN)
#undef glReplacementCodeuiColor4ubVertex3fSUN
#define glReplacementCodeuiColor4ubVertex3fSUN (gl.ReplacementCodeuiColor4ubVertex3fSUN)
#undef glReplacementCodeuiColor4ubVertex3fvSUN
#define glReplacementCodeuiColor4ubVertex3fvSUN (gl.ReplacementCodeuiColor4ubVertex3fvSUN)
#undef glReplacementCodeuiColor3fVertex3fSUN
#define glReplacementCodeuiColor3fVertex3fSUN (gl.ReplacementCodeuiColor3fVertex3fSUN)
#undef glReplacementCodeuiColor3fVertex3fvSUN
#define glReplacementCodeuiColor3fVertex3fvSUN (gl.ReplacementCodeuiColor3fVertex3fvSUN)
#undef glReplacementCodeuiNormal3fVertex3fSUN
#define glReplacementCodeuiNormal3fVertex3fSUN (gl.ReplacementCodeuiNormal3fVertex3fSUN)
#undef glReplacementCodeuiNormal3fVertex3fvSUN
#define glReplacementCodeuiNormal3fVertex3fvSUN (gl.ReplacementCodeuiNormal3fVertex3fvSUN)
#undef glReplacementCodeuiColor4fNormal3fVertex3fSUN
#define glReplacementCodeuiColor4fNormal3fVertex3fSUN (gl.ReplacementCodeuiColor4fNormal3fVertex3fSUN)
#undef glReplacementCodeuiColor4fNormal3fVertex3fvSUN
#define glReplacementCodeuiColor4fNormal3fVertex3fvSUN (gl.ReplacementCodeuiColor4fNormal3fVertex3fvSUN)
#undef glReplacementCodeuiTexCoord2fVertex3fSUN
#define glReplacementCodeuiTexCoord2fVertex3fSUN (gl.ReplacementCodeuiTexCoord2fVertex3fSUN)
#undef glReplacementCodeuiTexCoord2fVertex3fvSUN
#define glReplacementCodeuiTexCoord2fVertex3fvSUN (gl.ReplacementCodeuiTexCoord2fVertex3fvSUN)
#undef glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN
#define glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (gl.ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)
#undef glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN
#define glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (gl.ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)
#undef glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN
#define glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (gl.ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)
#undef glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN
#define glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (gl.ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)
#undef glBlendFuncSeparateEXT
#define glBlendFuncSeparateEXT (gl.BlendFuncSeparateEXT)
#undef glBlendFuncSeparateINGR
#define glBlendFuncSeparateINGR (gl.BlendFuncSeparateINGR)
#undef glVertexWeightfEXT
#define glVertexWeightfEXT (gl.VertexWeightfEXT)
#undef glVertexWeightfvEXT
#define glVertexWeightfvEXT (gl.VertexWeightfvEXT)
#undef glVertexWeightPointerEXT
#define glVertexWeightPointerEXT (gl.VertexWeightPointerEXT)
#undef glFlushVertexArrayRangeNV
#define glFlushVertexArrayRangeNV (gl.FlushVertexArrayRangeNV)
#undef glVertexArrayRangeNV
#define glVertexArrayRangeNV (gl.VertexArrayRangeNV)
#undef glCombinerParameterfvNV
#define glCombinerParameterfvNV (gl.CombinerParameterfvNV)
#undef glCombinerParameterfNV
#define glCombinerParameterfNV (gl.CombinerParameterfNV)
#undef glCombinerParameterivNV
#define glCombinerParameterivNV (gl.CombinerParameterivNV)
#undef glCombinerParameteriNV
#define glCombinerParameteriNV (gl.CombinerParameteriNV)
#undef glCombinerInputNV
#define glCombinerInputNV (gl.CombinerInputNV)
#undef glCombinerOutputNV
#define glCombinerOutputNV (gl.CombinerOutputNV)
#undef glFinalCombinerInputNV
#define glFinalCombinerInputNV (gl.FinalCombinerInputNV)
#undef glGetCombinerInputParameterfvNV
#define glGetCombinerInputParameterfvNV (gl.GetCombinerInputParameterfvNV)
#undef glGetCombinerInputParameterivNV
#define glGetCombinerInputParameterivNV (gl.GetCombinerInputParameterivNV)
#undef glGetCombinerOutputParameterfvNV
#define glGetCombinerOutputParameterfvNV (gl.GetCombinerOutputParameterfvNV)
#undef glGetCombinerOutputParameterivNV
#define glGetCombinerOutputParameterivNV (gl.GetCombinerOutputParameterivNV)
#undef glGetFinalCombinerInputParameterfvNV
#define glGetFinalCombinerInputParameterfvNV (gl.GetFinalCombinerInputParameterfvNV)
#undef glGetFinalCombinerInputParameterivNV
#define glGetFinalCombinerInputParameterivNV (gl.GetFinalCombinerInputParameterivNV)
#undef glResizeBuffersMESA
#define glResizeBuffersMESA (gl.ResizeBuffersMESA)
#undef glWindowPos2dMESA
#define glWindowPos2dMESA (gl.WindowPos2dMESA)
#undef glWindowPos2dvMESA
#define glWindowPos2dvMESA (gl.WindowPos2dvMESA)
#undef glWindowPos2fMESA
#define glWindowPos2fMESA (gl.WindowPos2fMESA)
#undef glWindowPos2fvMESA
#define glWindowPos2fvMESA (gl.WindowPos2fvMESA)
#undef glWindowPos2iMESA
#define glWindowPos2iMESA (gl.WindowPos2iMESA)
#undef glWindowPos2ivMESA
#define glWindowPos2ivMESA (gl.WindowPos2ivMESA)
#undef glWindowPos2sMESA
#define glWindowPos2sMESA (gl.WindowPos2sMESA)
#undef glWindowPos2svMESA
#define glWindowPos2svMESA (gl.WindowPos2svMESA)
#undef glWindowPos3dMESA
#define glWindowPos3dMESA (gl.WindowPos3dMESA)
#undef glWindowPos3dvMESA
#define glWindowPos3dvMESA (gl.WindowPos3dvMESA)
#undef glWindowPos3fMESA
#define glWindowPos3fMESA (gl.WindowPos3fMESA)
#undef glWindowPos3fvMESA
#define glWindowPos3fvMESA (gl.WindowPos3fvMESA)
#undef glWindowPos3iMESA
#define glWindowPos3iMESA (gl.WindowPos3iMESA)
#undef glWindowPos3ivMESA
#define glWindowPos3ivMESA (gl.WindowPos3ivMESA)
#undef glWindowPos3sMESA
#define glWindowPos3sMESA (gl.WindowPos3sMESA)
#undef glWindowPos3svMESA
#define glWindowPos3svMESA (gl.WindowPos3svMESA)
#undef glWindowPos4dMESA
#define glWindowPos4dMESA (gl.WindowPos4dMESA)
#undef glWindowPos4dvMESA
#define glWindowPos4dvMESA (gl.WindowPos4dvMESA)
#undef glWindowPos4fMESA
#define glWindowPos4fMESA (gl.WindowPos4fMESA)
#undef glWindowPos4fvMESA
#define glWindowPos4fvMESA (gl.WindowPos4fvMESA)
#undef glWindowPos4iMESA
#define glWindowPos4iMESA (gl.WindowPos4iMESA)
#undef glWindowPos4ivMESA
#define glWindowPos4ivMESA (gl.WindowPos4ivMESA)
#undef glWindowPos4sMESA
#define glWindowPos4sMESA (gl.WindowPos4sMESA)
#undef glWindowPos4svMESA
#define glWindowPos4svMESA (gl.WindowPos4svMESA)
#undef glMultiModeDrawArraysIBM
#define glMultiModeDrawArraysIBM (gl.MultiModeDrawArraysIBM)
#undef glMultiModeDrawElementsIBM
#define glMultiModeDrawElementsIBM (gl.MultiModeDrawElementsIBM)
#undef glColorPointerListIBM
#define glColorPointerListIBM (gl.ColorPointerListIBM)
#undef glSecondaryColorPointerListIBM
#define glSecondaryColorPointerListIBM (gl.SecondaryColorPointerListIBM)
#undef glEdgeFlagPointerListIBM
#define glEdgeFlagPointerListIBM (gl.EdgeFlagPointerListIBM)
#undef glFogCoordPointerListIBM
#define glFogCoordPointerListIBM (gl.FogCoordPointerListIBM)
#undef glIndexPointerListIBM
#define glIndexPointerListIBM (gl.IndexPointerListIBM)
#undef glNormalPointerListIBM
#define glNormalPointerListIBM (gl.NormalPointerListIBM)
#undef glTexCoordPointerListIBM
#define glTexCoordPointerListIBM (gl.TexCoordPointerListIBM)
#undef glVertexPointerListIBM
#define glVertexPointerListIBM (gl.VertexPointerListIBM)
#undef glTbufferMask3DFX
#define glTbufferMask3DFX (gl.TbufferMask3DFX)
#undef glSampleMaskEXT
#define glSampleMaskEXT (gl.SampleMaskEXT)
#undef glSamplePatternEXT
#define glSamplePatternEXT (gl.SamplePatternEXT)
#undef glTextureColorMaskSGIS
#define glTextureColorMaskSGIS (gl.TextureColorMaskSGIS)
#undef glIglooInterfaceSGIX
#define glIglooInterfaceSGIX (gl.IglooInterfaceSGIX)
#undef glDeleteFencesNV
#define glDeleteFencesNV (gl.DeleteFencesNV)
#undef glGenFencesNV
#define glGenFencesNV (gl.GenFencesNV)
#undef glIsFenceNV
#define glIsFenceNV (gl.IsFenceNV)
#undef glTestFenceNV
#define glTestFenceNV (gl.TestFenceNV)
#undef glGetFenceivNV
#define glGetFenceivNV (gl.GetFenceivNV)
#undef glFinishFenceNV
#define glFinishFenceNV (gl.FinishFenceNV)
#undef glSetFenceNV
#define glSetFenceNV (gl.SetFenceNV)
#undef glMapControlPointsNV
#define glMapControlPointsNV (gl.MapControlPointsNV)
#undef glMapParameterivNV
#define glMapParameterivNV (gl.MapParameterivNV)
#undef glMapParameterfvNV
#define glMapParameterfvNV (gl.MapParameterfvNV)
#undef glGetMapControlPointsNV
#define glGetMapControlPointsNV (gl.GetMapControlPointsNV)
#undef glGetMapParameterivNV
#define glGetMapParameterivNV (gl.GetMapParameterivNV)
#undef glGetMapParameterfvNV
#define glGetMapParameterfvNV (gl.GetMapParameterfvNV)
#undef glGetMapAttribParameterivNV
#define glGetMapAttribParameterivNV (gl.GetMapAttribParameterivNV)
#undef glGetMapAttribParameterfvNV
#define glGetMapAttribParameterfvNV (gl.GetMapAttribParameterfvNV)
#undef glEvalMapsNV
#define glEvalMapsNV (gl.EvalMapsNV)
#undef glCombinerStageParameterfvNV
#define glCombinerStageParameterfvNV (gl.CombinerStageParameterfvNV)
#undef glGetCombinerStageParameterfvNV
#define glGetCombinerStageParameterfvNV (gl.GetCombinerStageParameterfvNV)
#undef glAreProgramsResidentNV
#define glAreProgramsResidentNV (gl.AreProgramsResidentNV)
#undef glBindProgramNV
#define glBindProgramNV (gl.BindProgramNV)
#undef glDeleteProgramsNV
#define glDeleteProgramsNV (gl.DeleteProgramsNV)
#undef glExecuteProgramNV
#define glExecuteProgramNV (gl.ExecuteProgramNV)
#undef glGenProgramsNV
#define glGenProgramsNV (gl.GenProgramsNV)
#undef glGetProgramParameterdvNV
#define glGetProgramParameterdvNV (gl.GetProgramParameterdvNV)
#undef glGetProgramParameterfvNV
#define glGetProgramParameterfvNV (gl.GetProgramParameterfvNV)
#undef glGetProgramivNV
#define glGetProgramivNV (gl.GetProgramivNV)
#undef glGetProgramStringNV
#define glGetProgramStringNV (gl.GetProgramStringNV)
#undef glGetTrackMatrixivNV
#define glGetTrackMatrixivNV (gl.GetTrackMatrixivNV)
#undef glGetVertexAttribdvNV
#define glGetVertexAttribdvNV (gl.GetVertexAttribdvNV)
#undef glGetVertexAttribfvNV
#define glGetVertexAttribfvNV (gl.GetVertexAttribfvNV)
#undef glGetVertexAttribivNV
#define glGetVertexAttribivNV (gl.GetVertexAttribivNV)
#undef glGetVertexAttribPointervNV
#define glGetVertexAttribPointervNV (gl.GetVertexAttribPointervNV)
#undef glIsProgramNV
#define glIsProgramNV (gl.IsProgramNV)
#undef glLoadProgramNV
#define glLoadProgramNV (gl.LoadProgramNV)
#undef glProgramParameter4dNV
#define glProgramParameter4dNV (gl.ProgramParameter4dNV)
#undef glProgramParameter4dvNV
#define glProgramParameter4dvNV (gl.ProgramParameter4dvNV)
#undef glProgramParameter4fNV
#define glProgramParameter4fNV (gl.ProgramParameter4fNV)
#undef glProgramParameter4fvNV
#define glProgramParameter4fvNV (gl.ProgramParameter4fvNV)
#undef glProgramParameters4dvNV
#define glProgramParameters4dvNV (gl.ProgramParameters4dvNV)
#undef glProgramParameters4fvNV
#define glProgramParameters4fvNV (gl.ProgramParameters4fvNV)
#undef glRequestResidentProgramsNV
#define glRequestResidentProgramsNV (gl.RequestResidentProgramsNV)
#undef glTrackMatrixNV
#define glTrackMatrixNV (gl.TrackMatrixNV)
#undef glVertexAttribPointerNV
#define glVertexAttribPointerNV (gl.VertexAttribPointerNV)
#undef glVertexAttrib1dNV
#define glVertexAttrib1dNV (gl.VertexAttrib1dNV)
#undef glVertexAttrib1dvNV
#define glVertexAttrib1dvNV (gl.VertexAttrib1dvNV)
#undef glVertexAttrib1fNV
#define glVertexAttrib1fNV (gl.VertexAttrib1fNV)
#undef glVertexAttrib1fvNV
#define glVertexAttrib1fvNV (gl.VertexAttrib1fvNV)
#undef glVertexAttrib1sNV
#define glVertexAttrib1sNV (gl.VertexAttrib1sNV)
#undef glVertexAttrib1svNV
#define glVertexAttrib1svNV (gl.VertexAttrib1svNV)
#undef glVertexAttrib2dNV
#define glVertexAttrib2dNV (gl.VertexAttrib2dNV)
#undef glVertexAttrib2dvNV
#define glVertexAttrib2dvNV (gl.VertexAttrib2dvNV)
#undef glVertexAttrib2fNV
#define glVertexAttrib2fNV (gl.VertexAttrib2fNV)
#undef glVertexAttrib2fvNV
#define glVertexAttrib2fvNV (gl.VertexAttrib2fvNV)
#undef glVertexAttrib2sNV
#define glVertexAttrib2sNV (gl.VertexAttrib2sNV)
#undef glVertexAttrib2svNV
#define glVertexAttrib2svNV (gl.VertexAttrib2svNV)
#undef glVertexAttrib3dNV
#define glVertexAttrib3dNV (gl.VertexAttrib3dNV)
#undef glVertexAttrib3dvNV
#define glVertexAttrib3dvNV (gl.VertexAttrib3dvNV)
#undef glVertexAttrib3fNV
#define glVertexAttrib3fNV (gl.VertexAttrib3fNV)
#undef glVertexAttrib3fvNV
#define glVertexAttrib3fvNV (gl.VertexAttrib3fvNV)
#undef glVertexAttrib3sNV
#define glVertexAttrib3sNV (gl.VertexAttrib3sNV)
#undef glVertexAttrib3svNV
#define glVertexAttrib3svNV (gl.VertexAttrib3svNV)
#undef glVertexAttrib4dNV
#define glVertexAttrib4dNV (gl.VertexAttrib4dNV)
#undef glVertexAttrib4dvNV
#define glVertexAttrib4dvNV (gl.VertexAttrib4dvNV)
#undef glVertexAttrib4fNV
#define glVertexAttrib4fNV (gl.VertexAttrib4fNV)
#undef glVertexAttrib4fvNV
#define glVertexAttrib4fvNV (gl.VertexAttrib4fvNV)
#undef glVertexAttrib4sNV
#define glVertexAttrib4sNV (gl.VertexAttrib4sNV)
#undef glVertexAttrib4svNV
#define glVertexAttrib4svNV (gl.VertexAttrib4svNV)
#undef glVertexAttrib4ubNV
#define glVertexAttrib4ubNV (gl.VertexAttrib4ubNV)
#undef glVertexAttrib4ubvNV
#define glVertexAttrib4ubvNV (gl.VertexAttrib4ubvNV)
#undef glVertexAttribs1dvNV
#define glVertexAttribs1dvNV (gl.VertexAttribs1dvNV)
#undef glVertexAttribs1fvNV
#define glVertexAttribs1fvNV (gl.VertexAttribs1fvNV)
#undef glVertexAttribs1svNV
#define glVertexAttribs1svNV (gl.VertexAttribs1svNV)
#undef glVertexAttribs2dvNV
#define glVertexAttribs2dvNV (gl.VertexAttribs2dvNV)
#undef glVertexAttribs2fvNV
#define glVertexAttribs2fvNV (gl.VertexAttribs2fvNV)
#undef glVertexAttribs2svNV
#define glVertexAttribs2svNV (gl.VertexAttribs2svNV)
#undef glVertexAttribs3dvNV
#define glVertexAttribs3dvNV (gl.VertexAttribs3dvNV)
#undef glVertexAttribs3fvNV
#define glVertexAttribs3fvNV (gl.VertexAttribs3fvNV)
#undef glVertexAttribs3svNV
#define glVertexAttribs3svNV (gl.VertexAttribs3svNV)
#undef glVertexAttribs4dvNV
#define glVertexAttribs4dvNV (gl.VertexAttribs4dvNV)
#undef glVertexAttribs4fvNV
#define glVertexAttribs4fvNV (gl.VertexAttribs4fvNV)
#undef glVertexAttribs4svNV
#define glVertexAttribs4svNV (gl.VertexAttribs4svNV)
#undef glVertexAttribs4ubvNV
#define glVertexAttribs4ubvNV (gl.VertexAttribs4ubvNV)
#undef glTexBumpParameterivATI
#define glTexBumpParameterivATI (gl.TexBumpParameterivATI)
#undef glTexBumpParameterfvATI
#define glTexBumpParameterfvATI (gl.TexBumpParameterfvATI)
#undef glGetTexBumpParameterivATI
#define glGetTexBumpParameterivATI (gl.GetTexBumpParameterivATI)
#undef glGetTexBumpParameterfvATI
#define glGetTexBumpParameterfvATI (gl.GetTexBumpParameterfvATI)
#undef glGenFragmentShadersATI
#define glGenFragmentShadersATI (gl.GenFragmentShadersATI)
#undef glBindFragmentShaderATI
#define glBindFragmentShaderATI (gl.BindFragmentShaderATI)
#undef glDeleteFragmentShaderATI
#define glDeleteFragmentShaderATI (gl.DeleteFragmentShaderATI)
#undef glBeginFragmentShaderATI
#define glBeginFragmentShaderATI (gl.BeginFragmentShaderATI)
#undef glEndFragmentShaderATI
#define glEndFragmentShaderATI (gl.EndFragmentShaderATI)
#undef glPassTexCoordATI
#define glPassTexCoordATI (gl.PassTexCoordATI)
#undef glSampleMapATI
#define glSampleMapATI (gl.SampleMapATI)
#undef glColorFragmentOp1ATI
#define glColorFragmentOp1ATI (gl.ColorFragmentOp1ATI)
#undef glColorFragmentOp2ATI
#define glColorFragmentOp2ATI (gl.ColorFragmentOp2ATI)
#undef glColorFragmentOp3ATI
#define glColorFragmentOp3ATI (gl.ColorFragmentOp3ATI)
#undef glAlphaFragmentOp1ATI
#define glAlphaFragmentOp1ATI (gl.AlphaFragmentOp1ATI)
#undef glAlphaFragmentOp2ATI
#define glAlphaFragmentOp2ATI (gl.AlphaFragmentOp2ATI)
#undef glAlphaFragmentOp3ATI
#define glAlphaFragmentOp3ATI (gl.AlphaFragmentOp3ATI)
#undef glSetFragmentShaderConstantATI
#define glSetFragmentShaderConstantATI (gl.SetFragmentShaderConstantATI)
#undef glPNTrianglesiATI
#define glPNTrianglesiATI (gl.PNTrianglesiATI)
#undef glPNTrianglesfATI
#define glPNTrianglesfATI (gl.PNTrianglesfATI)
#undef glNewObjectBufferATI
#define glNewObjectBufferATI (gl.NewObjectBufferATI)
#undef glIsObjectBufferATI
#define glIsObjectBufferATI (gl.IsObjectBufferATI)
#undef glUpdateObjectBufferATI
#define glUpdateObjectBufferATI (gl.UpdateObjectBufferATI)
#undef glGetObjectBufferfvATI
#define glGetObjectBufferfvATI (gl.GetObjectBufferfvATI)
#undef glGetObjectBufferivATI
#define glGetObjectBufferivATI (gl.GetObjectBufferivATI)
#undef glFreeObjectBufferATI
#define glFreeObjectBufferATI (gl.FreeObjectBufferATI)
#undef glArrayObjectATI
#define glArrayObjectATI (gl.ArrayObjectATI)
#undef glGetArrayObjectfvATI
#define glGetArrayObjectfvATI (gl.GetArrayObjectfvATI)
#undef glGetArrayObjectivATI
#define glGetArrayObjectivATI (gl.GetArrayObjectivATI)
#undef glVariantArrayObjectATI
#define glVariantArrayObjectATI (gl.VariantArrayObjectATI)
#undef glGetVariantArrayObjectfvATI
#define glGetVariantArrayObjectfvATI (gl.GetVariantArrayObjectfvATI)
#undef glGetVariantArrayObjectivATI
#define glGetVariantArrayObjectivATI (gl.GetVariantArrayObjectivATI)
#undef glBeginVertexShaderEXT
#define glBeginVertexShaderEXT (gl.BeginVertexShaderEXT)
#undef glEndVertexShaderEXT
#define glEndVertexShaderEXT (gl.EndVertexShaderEXT)
#undef glBindVertexShaderEXT
#define glBindVertexShaderEXT (gl.BindVertexShaderEXT)
#undef glGenVertexShadersEXT
#define glGenVertexShadersEXT (gl.GenVertexShadersEXT)
#undef glDeleteVertexShaderEXT
#define glDeleteVertexShaderEXT (gl.DeleteVertexShaderEXT)
#undef glShaderOp1EXT
#define glShaderOp1EXT (gl.ShaderOp1EXT)
#undef glShaderOp2EXT
#define glShaderOp2EXT (gl.ShaderOp2EXT)
#undef glShaderOp3EXT
#define glShaderOp3EXT (gl.ShaderOp3EXT)
#undef glSwizzleEXT
#define glSwizzleEXT (gl.SwizzleEXT)
#undef glWriteMaskEXT
#define glWriteMaskEXT (gl.WriteMaskEXT)
#undef glInsertComponentEXT
#define glInsertComponentEXT (gl.InsertComponentEXT)
#undef glExtractComponentEXT
#define glExtractComponentEXT (gl.ExtractComponentEXT)
#undef glGenSymbolsEXT
#define glGenSymbolsEXT (gl.GenSymbolsEXT)
#undef glSetInvariantEXT
#define glSetInvariantEXT (gl.SetInvariantEXT)
#undef glSetLocalConstantEXT
#define glSetLocalConstantEXT (gl.SetLocalConstantEXT)
#undef glVariantbvEXT
#define glVariantbvEXT (gl.VariantbvEXT)
#undef glVariantsvEXT
#define glVariantsvEXT (gl.VariantsvEXT)
#undef glVariantivEXT
#define glVariantivEXT (gl.VariantivEXT)
#undef glVariantfvEXT
#define glVariantfvEXT (gl.VariantfvEXT)
#undef glVariantdvEXT
#define glVariantdvEXT (gl.VariantdvEXT)
#undef glVariantubvEXT
#define glVariantubvEXT (gl.VariantubvEXT)
#undef glVariantusvEXT
#define glVariantusvEXT (gl.VariantusvEXT)
#undef glVariantuivEXT
#define glVariantuivEXT (gl.VariantuivEXT)
#undef glVariantPointerEXT
#define glVariantPointerEXT (gl.VariantPointerEXT)
#undef glEnableVariantClientStateEXT
#define glEnableVariantClientStateEXT (gl.EnableVariantClientStateEXT)
#undef glDisableVariantClientStateEXT
#define glDisableVariantClientStateEXT (gl.DisableVariantClientStateEXT)
#undef glBindLightParameterEXT
#define glBindLightParameterEXT (gl.BindLightParameterEXT)
#undef glBindMaterialParameterEXT
#define glBindMaterialParameterEXT (gl.BindMaterialParameterEXT)
#undef glBindTexGenParameterEXT
#define glBindTexGenParameterEXT (gl.BindTexGenParameterEXT)
#undef glBindTextureUnitParameterEXT
#define glBindTextureUnitParameterEXT (gl.BindTextureUnitParameterEXT)
#undef glBindParameterEXT
#define glBindParameterEXT (gl.BindParameterEXT)
#undef glIsVariantEnabledEXT
#define glIsVariantEnabledEXT (gl.IsVariantEnabledEXT)
#undef glGetVariantBooleanvEXT
#define glGetVariantBooleanvEXT (gl.GetVariantBooleanvEXT)
#undef glGetVariantIntegervEXT
#define glGetVariantIntegervEXT (gl.GetVariantIntegervEXT)
#undef glGetVariantFloatvEXT
#define glGetVariantFloatvEXT (gl.GetVariantFloatvEXT)
#undef glGetVariantPointervEXT
#define glGetVariantPointervEXT (gl.GetVariantPointervEXT)
#undef glGetInvariantBooleanvEXT
#define glGetInvariantBooleanvEXT (gl.GetInvariantBooleanvEXT)
#undef glGetInvariantIntegervEXT
#define glGetInvariantIntegervEXT (gl.GetInvariantIntegervEXT)
#undef glGetInvariantFloatvEXT
#define glGetInvariantFloatvEXT (gl.GetInvariantFloatvEXT)
#undef glGetLocalConstantBooleanvEXT
#define glGetLocalConstantBooleanvEXT (gl.GetLocalConstantBooleanvEXT)
#undef glGetLocalConstantIntegervEXT
#define glGetLocalConstantIntegervEXT (gl.GetLocalConstantIntegervEXT)
#undef glGetLocalConstantFloatvEXT
#define glGetLocalConstantFloatvEXT (gl.GetLocalConstantFloatvEXT)
#undef glVertexStream1sATI
#define glVertexStream1sATI (gl.VertexStream1sATI)
#undef glVertexStream1svATI
#define glVertexStream1svATI (gl.VertexStream1svATI)
#undef glVertexStream1iATI
#define glVertexStream1iATI (gl.VertexStream1iATI)
#undef glVertexStream1ivATI
#define glVertexStream1ivATI (gl.VertexStream1ivATI)
#undef glVertexStream1fATI
#define glVertexStream1fATI (gl.VertexStream1fATI)
#undef glVertexStream1fvATI
#define glVertexStream1fvATI (gl.VertexStream1fvATI)
#undef glVertexStream1dATI
#define glVertexStream1dATI (gl.VertexStream1dATI)
#undef glVertexStream1dvATI
#define glVertexStream1dvATI (gl.VertexStream1dvATI)
#undef glVertexStream2sATI
#define glVertexStream2sATI (gl.VertexStream2sATI)
#undef glVertexStream2svATI
#define glVertexStream2svATI (gl.VertexStream2svATI)
#undef glVertexStream2iATI
#define glVertexStream2iATI (gl.VertexStream2iATI)
#undef glVertexStream2ivATI
#define glVertexStream2ivATI (gl.VertexStream2ivATI)
#undef glVertexStream2fATI
#define glVertexStream2fATI (gl.VertexStream2fATI)
#undef glVertexStream2fvATI
#define glVertexStream2fvATI (gl.VertexStream2fvATI)
#undef glVertexStream2dATI
#define glVertexStream2dATI (gl.VertexStream2dATI)
#undef glVertexStream2dvATI
#define glVertexStream2dvATI (gl.VertexStream2dvATI)
#undef glVertexStream3sATI
#define glVertexStream3sATI (gl.VertexStream3sATI)
#undef glVertexStream3svATI
#define glVertexStream3svATI (gl.VertexStream3svATI)
#undef glVertexStream3iATI
#define glVertexStream3iATI (gl.VertexStream3iATI)
#undef glVertexStream3ivATI
#define glVertexStream3ivATI (gl.VertexStream3ivATI)
#undef glVertexStream3fATI
#define glVertexStream3fATI (gl.VertexStream3fATI)
#undef glVertexStream3fvATI
#define glVertexStream3fvATI (gl.VertexStream3fvATI)
#undef glVertexStream3dATI
#define glVertexStream3dATI (gl.VertexStream3dATI)
#undef glVertexStream3dvATI
#define glVertexStream3dvATI (gl.VertexStream3dvATI)
#undef glVertexStream4sATI
#define glVertexStream4sATI (gl.VertexStream4sATI)
#undef glVertexStream4svATI
#define glVertexStream4svATI (gl.VertexStream4svATI)
#undef glVertexStream4iATI
#define glVertexStream4iATI (gl.VertexStream4iATI)
#undef glVertexStream4ivATI
#define glVertexStream4ivATI (gl.VertexStream4ivATI)
#undef glVertexStream4fATI
#define glVertexStream4fATI (gl.VertexStream4fATI)
#undef glVertexStream4fvATI
#define glVertexStream4fvATI (gl.VertexStream4fvATI)
#undef glVertexStream4dATI
#define glVertexStream4dATI (gl.VertexStream4dATI)
#undef glVertexStream4dvATI
#define glVertexStream4dvATI (gl.VertexStream4dvATI)
#undef glNormalStream3bATI
#define glNormalStream3bATI (gl.NormalStream3bATI)
#undef glNormalStream3bvATI
#define glNormalStream3bvATI (gl.NormalStream3bvATI)
#undef glNormalStream3sATI
#define glNormalStream3sATI (gl.NormalStream3sATI)
#undef glNormalStream3svATI
#define glNormalStream3svATI (gl.NormalStream3svATI)
#undef glNormalStream3iATI
#define glNormalStream3iATI (gl.NormalStream3iATI)
#undef glNormalStream3ivATI
#define glNormalStream3ivATI (gl.NormalStream3ivATI)
#undef glNormalStream3fATI
#define glNormalStream3fATI (gl.NormalStream3fATI)
#undef glNormalStream3fvATI
#define glNormalStream3fvATI (gl.NormalStream3fvATI)
#undef glNormalStream3dATI
#define glNormalStream3dATI (gl.NormalStream3dATI)
#undef glNormalStream3dvATI
#define glNormalStream3dvATI (gl.NormalStream3dvATI)
#undef glClientActiveVertexStreamATI
#define glClientActiveVertexStreamATI (gl.ClientActiveVertexStreamATI)
#undef glVertexBlendEnviATI
#define glVertexBlendEnviATI (gl.VertexBlendEnviATI)
#undef glVertexBlendEnvfATI
#define glVertexBlendEnvfATI (gl.VertexBlendEnvfATI)
#undef glElementPointerATI
#define glElementPointerATI (gl.ElementPointerATI)
#undef glDrawElementArrayATI
#define glDrawElementArrayATI (gl.DrawElementArrayATI)
#undef glDrawRangeElementArrayATI
#define glDrawRangeElementArrayATI (gl.DrawRangeElementArrayATI)
#undef glDrawMeshArraysSUN
#define glDrawMeshArraysSUN (gl.DrawMeshArraysSUN)
#undef glGenOcclusionQueriesNV
#define glGenOcclusionQueriesNV (gl.GenOcclusionQueriesNV)
#undef glDeleteOcclusionQueriesNV
#define glDeleteOcclusionQueriesNV (gl.DeleteOcclusionQueriesNV)
#undef glIsOcclusionQueryNV
#define glIsOcclusionQueryNV (gl.IsOcclusionQueryNV)
#undef glBeginOcclusionQueryNV
#define glBeginOcclusionQueryNV (gl.BeginOcclusionQueryNV)
#undef glEndOcclusionQueryNV
#define glEndOcclusionQueryNV (gl.EndOcclusionQueryNV)
#undef glGetOcclusionQueryivNV
#define glGetOcclusionQueryivNV (gl.GetOcclusionQueryivNV)
#undef glGetOcclusionQueryuivNV
#define glGetOcclusionQueryuivNV (gl.GetOcclusionQueryuivNV)
#undef glPointParameteriNV
#define glPointParameteriNV (gl.PointParameteriNV)
#undef glPointParameterivNV
#define glPointParameterivNV (gl.PointParameterivNV)
#undef glActiveStencilFaceEXT
#define glActiveStencilFaceEXT (gl.ActiveStencilFaceEXT)
#undef glElementPointerAPPLE
#define glElementPointerAPPLE (gl.ElementPointerAPPLE)
#undef glDrawElementArrayAPPLE
#define glDrawElementArrayAPPLE (gl.DrawElementArrayAPPLE)
#undef glDrawRangeElementArrayAPPLE
#define glDrawRangeElementArrayAPPLE (gl.DrawRangeElementArrayAPPLE)
#undef glMultiDrawElementArrayAPPLE
#define glMultiDrawElementArrayAPPLE (gl.MultiDrawElementArrayAPPLE)
#undef glMultiDrawRangeElementArrayAPPLE
#define glMultiDrawRangeElementArrayAPPLE (gl.MultiDrawRangeElementArrayAPPLE)
#undef glGenFencesAPPLE
#define glGenFencesAPPLE (gl.GenFencesAPPLE)
#undef glDeleteFencesAPPLE
#define glDeleteFencesAPPLE (gl.DeleteFencesAPPLE)
#undef glSetFenceAPPLE
#define glSetFenceAPPLE (gl.SetFenceAPPLE)
#undef glIsFenceAPPLE
#define glIsFenceAPPLE (gl.IsFenceAPPLE)
#undef glTestFenceAPPLE
#define glTestFenceAPPLE (gl.TestFenceAPPLE)
#undef glFinishFenceAPPLE
#define glFinishFenceAPPLE (gl.FinishFenceAPPLE)
#undef glTestObjectAPPLE
#define glTestObjectAPPLE (gl.TestObjectAPPLE)
#undef glFinishObjectAPPLE
#define glFinishObjectAPPLE (gl.FinishObjectAPPLE)
#undef glBindVertexArrayAPPLE
#define glBindVertexArrayAPPLE (gl.BindVertexArrayAPPLE)
#undef glDeleteVertexArraysAPPLE
#define glDeleteVertexArraysAPPLE (gl.DeleteVertexArraysAPPLE)
#undef glGenVertexArraysAPPLE
#define glGenVertexArraysAPPLE (gl.GenVertexArraysAPPLE)
#undef glIsVertexArrayAPPLE
#define glIsVertexArrayAPPLE (gl.IsVertexArrayAPPLE)
#undef glVertexArrayRangeAPPLE
#define glVertexArrayRangeAPPLE (gl.VertexArrayRangeAPPLE)
#undef glFlushVertexArrayRangeAPPLE
#define glFlushVertexArrayRangeAPPLE (gl.FlushVertexArrayRangeAPPLE)
#undef glVertexArrayParameteriAPPLE
#define glVertexArrayParameteriAPPLE (gl.VertexArrayParameteriAPPLE)
#undef glDrawBuffersATI
#define glDrawBuffersATI (gl.DrawBuffersATI)
#undef glProgramNamedParameter4fNV
#define glProgramNamedParameter4fNV (gl.ProgramNamedParameter4fNV)
#undef glProgramNamedParameter4dNV
#define glProgramNamedParameter4dNV (gl.ProgramNamedParameter4dNV)
#undef glProgramNamedParameter4fvNV
#define glProgramNamedParameter4fvNV (gl.ProgramNamedParameter4fvNV)
#undef glProgramNamedParameter4dvNV
#define glProgramNamedParameter4dvNV (gl.ProgramNamedParameter4dvNV)
#undef glGetProgramNamedParameterfvNV
#define glGetProgramNamedParameterfvNV (gl.GetProgramNamedParameterfvNV)
#undef glGetProgramNamedParameterdvNV
#define glGetProgramNamedParameterdvNV (gl.GetProgramNamedParameterdvNV)
#undef glVertex2hNV
#define glVertex2hNV (gl.Vertex2hNV)
#undef glVertex2hvNV
#define glVertex2hvNV (gl.Vertex2hvNV)
#undef glVertex3hNV
#define glVertex3hNV (gl.Vertex3hNV)
#undef glVertex3hvNV
#define glVertex3hvNV (gl.Vertex3hvNV)
#undef glVertex4hNV
#define glVertex4hNV (gl.Vertex4hNV)
#undef glVertex4hvNV
#define glVertex4hvNV (gl.Vertex4hvNV)
#undef glNormal3hNV
#define glNormal3hNV (gl.Normal3hNV)
#undef glNormal3hvNV
#define glNormal3hvNV (gl.Normal3hvNV)
#undef glColor3hNV
#define glColor3hNV (gl.Color3hNV)
#undef glColor3hvNV
#define glColor3hvNV (gl.Color3hvNV)
#undef glColor4hNV
#define glColor4hNV (gl.Color4hNV)
#undef glColor4hvNV
#define glColor4hvNV (gl.Color4hvNV)
#undef glTexCoord1hNV
#define glTexCoord1hNV (gl.TexCoord1hNV)
#undef glTexCoord1hvNV
#define glTexCoord1hvNV (gl.TexCoord1hvNV)
#undef glTexCoord2hNV
#define glTexCoord2hNV (gl.TexCoord2hNV)
#undef glTexCoord2hvNV
#define glTexCoord2hvNV (gl.TexCoord2hvNV)
#undef glTexCoord3hNV
#define glTexCoord3hNV (gl.TexCoord3hNV)
#undef glTexCoord3hvNV
#define glTexCoord3hvNV (gl.TexCoord3hvNV)
#undef glTexCoord4hNV
#define glTexCoord4hNV (gl.TexCoord4hNV)
#undef glTexCoord4hvNV
#define glTexCoord4hvNV (gl.TexCoord4hvNV)
#undef glMultiTexCoord1hNV
#define glMultiTexCoord1hNV (gl.MultiTexCoord1hNV)
#undef glMultiTexCoord1hvNV
#define glMultiTexCoord1hvNV (gl.MultiTexCoord1hvNV)
#undef glMultiTexCoord2hNV
#define glMultiTexCoord2hNV (gl.MultiTexCoord2hNV)
#undef glMultiTexCoord2hvNV
#define glMultiTexCoord2hvNV (gl.MultiTexCoord2hvNV)
#undef glMultiTexCoord3hNV
#define glMultiTexCoord3hNV (gl.MultiTexCoord3hNV)
#undef glMultiTexCoord3hvNV
#define glMultiTexCoord3hvNV (gl.MultiTexCoord3hvNV)
#undef glMultiTexCoord4hNV
#define glMultiTexCoord4hNV (gl.MultiTexCoord4hNV)
#undef glMultiTexCoord4hvNV
#define glMultiTexCoord4hvNV (gl.MultiTexCoord4hvNV)
#undef glFogCoordhNV
#define glFogCoordhNV (gl.FogCoordhNV)
#undef glFogCoordhvNV
#define glFogCoordhvNV (gl.FogCoordhvNV)
#undef glSecondaryColor3hNV
#define glSecondaryColor3hNV (gl.SecondaryColor3hNV)
#undef glSecondaryColor3hvNV
#define glSecondaryColor3hvNV (gl.SecondaryColor3hvNV)
#undef glVertexWeighthNV
#define glVertexWeighthNV (gl.VertexWeighthNV)
#undef glVertexWeighthvNV
#define glVertexWeighthvNV (gl.VertexWeighthvNV)
#undef glVertexAttrib1hNV
#define glVertexAttrib1hNV (gl.VertexAttrib1hNV)
#undef glVertexAttrib1hvNV
#define glVertexAttrib1hvNV (gl.VertexAttrib1hvNV)
#undef glVertexAttrib2hNV
#define glVertexAttrib2hNV (gl.VertexAttrib2hNV)
#undef glVertexAttrib2hvNV
#define glVertexAttrib2hvNV (gl.VertexAttrib2hvNV)
#undef glVertexAttrib3hNV
#define glVertexAttrib3hNV (gl.VertexAttrib3hNV)
#undef glVertexAttrib3hvNV
#define glVertexAttrib3hvNV (gl.VertexAttrib3hvNV)
#undef glVertexAttrib4hNV
#define glVertexAttrib4hNV (gl.VertexAttrib4hNV)
#undef glVertexAttrib4hvNV
#define glVertexAttrib4hvNV (gl.VertexAttrib4hvNV)
#undef glVertexAttribs1hvNV
#define glVertexAttribs1hvNV (gl.VertexAttribs1hvNV)
#undef glVertexAttribs2hvNV
#define glVertexAttribs2hvNV (gl.VertexAttribs2hvNV)
#undef glVertexAttribs3hvNV
#define glVertexAttribs3hvNV (gl.VertexAttribs3hvNV)
#undef glVertexAttribs4hvNV
#define glVertexAttribs4hvNV (gl.VertexAttribs4hvNV)
#undef glPixelDataRangeNV
#define glPixelDataRangeNV (gl.PixelDataRangeNV)
#undef glFlushPixelDataRangeNV
#define glFlushPixelDataRangeNV (gl.FlushPixelDataRangeNV)
#undef glPrimitiveRestartNV
#define glPrimitiveRestartNV (gl.PrimitiveRestartNV)
#undef glPrimitiveRestartIndexNV
#define glPrimitiveRestartIndexNV (gl.PrimitiveRestartIndexNV)
#undef glMapObjectBufferATI
#define glMapObjectBufferATI (gl.MapObjectBufferATI)
#undef glUnmapObjectBufferATI
#define glUnmapObjectBufferATI (gl.UnmapObjectBufferATI)
#undef glStencilOpSeparateATI
#define glStencilOpSeparateATI (gl.StencilOpSeparateATI)
#undef glStencilFuncSeparateATI
#define glStencilFuncSeparateATI (gl.StencilFuncSeparateATI)
#undef glVertexAttribArrayObjectATI
#define glVertexAttribArrayObjectATI (gl.VertexAttribArrayObjectATI)
#undef glGetVertexAttribArrayObjectfvATI
#define glGetVertexAttribArrayObjectfvATI (gl.GetVertexAttribArrayObjectfvATI)
#undef glGetVertexAttribArrayObjectivATI
#define glGetVertexAttribArrayObjectivATI (gl.GetVertexAttribArrayObjectivATI)
#undef glDepthBoundsEXT
#define glDepthBoundsEXT (gl.DepthBoundsEXT)
#undef glBlendEquationSeparateEXT
#define glBlendEquationSeparateEXT (gl.BlendEquationSeparateEXT)
#undef OSMesaColorClamp
#define OSMesaColorClamp (gl.OSMesaColorClamp)
#undef OSMesaPostprocess
#define OSMesaPostprocess (gl.OSMesaPostprocess)
#undef OSMesaCreateLDG
#define OSMesaCreateLDG (gl.OSMesaCreateLDG)
#undef OSMesaDestroyLDG
#define OSMesaDestroyLDG (gl.OSMesaDestroyLDG)
#undef max_width
#define max_width (gl.max_width)
#undef max_height
#define max_height (gl.max_height)
#undef information
#define information (gl.information)
#undef exception_error
#define exception_error (gl.exception_error)
#undef gluLookAtf
#define gluLookAtf (gl.gluLookAtf)
#undef glFrustumf
#define glFrustumf (gl.Frustumf)
#undef glOrthof
#define glOrthof (gl.Orthof)
#undef swapbuffer
#define swapbuffer (gl.swapbuffer)
#undef gluLookAt
#define gluLookAt (gl.gluLookAt)
#undef glAccumxOES
#define glAccumxOES (gl.AccumxOES)
#undef glActiveProgramEXT
#define glActiveProgramEXT (gl.ActiveProgramEXT)
#undef glActiveShaderProgram
#define glActiveShaderProgram (gl.ActiveShaderProgram)
#undef glActiveVaryingNV
#define glActiveVaryingNV (gl.ActiveVaryingNV)
#undef glAddSwapHintRectWIN
#define glAddSwapHintRectWIN (gl.AddSwapHintRectWIN)
#undef glAlphaFuncxOES
#define glAlphaFuncxOES (gl.AlphaFuncxOES)
#undef glAttachShader
#define glAttachShader (gl.AttachShader)
#undef glBeginConditionalRender
#define glBeginConditionalRender (gl.BeginConditionalRender)
#undef glBeginConditionalRenderNV
#define glBeginConditionalRenderNV (gl.BeginConditionalRenderNV)
#undef glBeginConditionalRenderNVX
#define glBeginConditionalRenderNVX (gl.BeginConditionalRenderNVX)
#undef glBeginPerfMonitorAMD
#define glBeginPerfMonitorAMD (gl.BeginPerfMonitorAMD)
#undef glBeginPerfQueryINTEL
#define glBeginPerfQueryINTEL (gl.BeginPerfQueryINTEL)
#undef glBeginQueryIndexed
#define glBeginQueryIndexed (gl.BeginQueryIndexed)
#undef glBeginTransformFeedback
#define glBeginTransformFeedback (gl.BeginTransformFeedback)
#undef glBeginTransformFeedbackEXT
#define glBeginTransformFeedbackEXT (gl.BeginTransformFeedbackEXT)
#undef glBeginTransformFeedbackNV
#define glBeginTransformFeedbackNV (gl.BeginTransformFeedbackNV)
#undef glBeginVideoCaptureNV
#define glBeginVideoCaptureNV (gl.BeginVideoCaptureNV)
#undef glBindAttribLocation
#define glBindAttribLocation (gl.BindAttribLocation)
#undef glBindBufferBase
#define glBindBufferBase (gl.BindBufferBase)
#undef glBindBufferBaseEXT
#define glBindBufferBaseEXT (gl.BindBufferBaseEXT)
#undef glBindBufferBaseNV
#define glBindBufferBaseNV (gl.BindBufferBaseNV)
#undef glBindBufferOffsetEXT
#define glBindBufferOffsetEXT (gl.BindBufferOffsetEXT)
#undef glBindBufferOffsetNV
#define glBindBufferOffsetNV (gl.BindBufferOffsetNV)
#undef glBindBufferRange
#define glBindBufferRange (gl.BindBufferRange)
#undef glBindBufferRangeEXT
#define glBindBufferRangeEXT (gl.BindBufferRangeEXT)
#undef glBindBufferRangeNV
#define glBindBufferRangeNV (gl.BindBufferRangeNV)
#undef glBindBuffersBase
#define glBindBuffersBase (gl.BindBuffersBase)
#undef glBindBuffersRange
#define glBindBuffersRange (gl.BindBuffersRange)
#undef glBindFragDataLocation
#define glBindFragDataLocation (gl.BindFragDataLocation)
#undef glBindFragDataLocationEXT
#define glBindFragDataLocationEXT (gl.BindFragDataLocationEXT)
#undef glBindFragDataLocationIndexed
#define glBindFragDataLocationIndexed (gl.BindFragDataLocationIndexed)
#undef glBindFramebuffer
#define glBindFramebuffer (gl.BindFramebuffer)
#undef glBindFramebufferEXT
#define glBindFramebufferEXT (gl.BindFramebufferEXT)
#undef glBindImageTexture
#define glBindImageTexture (gl.BindImageTexture)
#undef glBindImageTextureEXT
#define glBindImageTextureEXT (gl.BindImageTextureEXT)
#undef glBindImageTextures
#define glBindImageTextures (gl.BindImageTextures)
#undef glBindMultiTextureEXT
#define glBindMultiTextureEXT (gl.BindMultiTextureEXT)
#undef glBindProgramPipeline
#define glBindProgramPipeline (gl.BindProgramPipeline)
#undef glBindRenderbuffer
#define glBindRenderbuffer (gl.BindRenderbuffer)
#undef glBindRenderbufferEXT
#define glBindRenderbufferEXT (gl.BindRenderbufferEXT)
#undef glBindSampler
#define glBindSampler (gl.BindSampler)
#undef glBindSamplers
#define glBindSamplers (gl.BindSamplers)
#undef glBindTextures
#define glBindTextures (gl.BindTextures)
#undef glBindTransformFeedback
#define glBindTransformFeedback (gl.BindTransformFeedback)
#undef glBindTransformFeedbackNV
#define glBindTransformFeedbackNV (gl.BindTransformFeedbackNV)
#undef glBindVertexArray
#define glBindVertexArray (gl.BindVertexArray)
#undef glBindVertexBuffer
#define glBindVertexBuffer (gl.BindVertexBuffer)
#undef glBindVertexBuffers
#define glBindVertexBuffers (gl.BindVertexBuffers)
#undef glBindVideoCaptureStreamBufferNV
#define glBindVideoCaptureStreamBufferNV (gl.BindVideoCaptureStreamBufferNV)
#undef glBindVideoCaptureStreamTextureNV
#define glBindVideoCaptureStreamTextureNV (gl.BindVideoCaptureStreamTextureNV)
#undef glBitmapxOES
#define glBitmapxOES (gl.BitmapxOES)
#undef glBlendBarrierNV
#define glBlendBarrierNV (gl.BlendBarrierNV)
#undef glBlendColorxOES
#define glBlendColorxOES (gl.BlendColorxOES)
#undef glBlendEquationIndexedAMD
#define glBlendEquationIndexedAMD (gl.BlendEquationIndexedAMD)
#undef glBlendEquationSeparate
#define glBlendEquationSeparate (gl.BlendEquationSeparate)
#undef glBlendEquationSeparateIndexedAMD
#define glBlendEquationSeparateIndexedAMD (gl.BlendEquationSeparateIndexedAMD)
#undef glBlendEquationSeparatei
#define glBlendEquationSeparatei (gl.BlendEquationSeparatei)
#undef glBlendEquationSeparateiARB
#define glBlendEquationSeparateiARB (gl.BlendEquationSeparateiARB)
#undef glBlendEquationi
#define glBlendEquationi (gl.BlendEquationi)
#undef glBlendEquationiARB
#define glBlendEquationiARB (gl.BlendEquationiARB)
#undef glBlendFuncIndexedAMD
#define glBlendFuncIndexedAMD (gl.BlendFuncIndexedAMD)
#undef glBlendFuncSeparateIndexedAMD
#define glBlendFuncSeparateIndexedAMD (gl.BlendFuncSeparateIndexedAMD)
#undef glBlendFuncSeparatei
#define glBlendFuncSeparatei (gl.BlendFuncSeparatei)
#undef glBlendFuncSeparateiARB
#define glBlendFuncSeparateiARB (gl.BlendFuncSeparateiARB)
#undef glBlendFunci
#define glBlendFunci (gl.BlendFunci)
#undef glBlendFunciARB
#define glBlendFunciARB (gl.BlendFunciARB)
#undef glBlendParameteriNV
#define glBlendParameteriNV (gl.BlendParameteriNV)
#undef glBlitFramebuffer
#define glBlitFramebuffer (gl.BlitFramebuffer)
#undef glBlitFramebufferEXT
#define glBlitFramebufferEXT (gl.BlitFramebufferEXT)
#undef glBufferAddressRangeNV
#define glBufferAddressRangeNV (gl.BufferAddressRangeNV)
#undef glBufferParameteriAPPLE
#define glBufferParameteriAPPLE (gl.BufferParameteriAPPLE)
#undef glBufferStorage
#define glBufferStorage (gl.BufferStorage)
#undef glCheckFramebufferStatus
#define glCheckFramebufferStatus (gl.CheckFramebufferStatus)
#undef glCheckFramebufferStatusEXT
#define glCheckFramebufferStatusEXT (gl.CheckFramebufferStatusEXT)
#undef glCheckNamedFramebufferStatusEXT
#define glCheckNamedFramebufferStatusEXT (gl.CheckNamedFramebufferStatusEXT)
#undef glClampColor
#define glClampColor (gl.ClampColor)
#undef glClampColorARB
#define glClampColorARB (gl.ClampColorARB)
#undef glClearAccumxOES
#define glClearAccumxOES (gl.ClearAccumxOES)
#undef glClearBufferData
#define glClearBufferData (gl.ClearBufferData)
#undef glClearBufferSubData
#define glClearBufferSubData (gl.ClearBufferSubData)
#undef glClearBufferfi
#define glClearBufferfi (gl.ClearBufferfi)
#undef glClearBufferfv
#define glClearBufferfv (gl.ClearBufferfv)
#undef glClearBufferiv
#define glClearBufferiv (gl.ClearBufferiv)
#undef glClearBufferuiv
#define glClearBufferuiv (gl.ClearBufferuiv)
#undef glClearColorIiEXT
#define glClearColorIiEXT (gl.ClearColorIiEXT)
#undef glClearColorIuiEXT
#define glClearColorIuiEXT (gl.ClearColorIuiEXT)
#undef glClearColorxOES
#define glClearColorxOES (gl.ClearColorxOES)
#undef glClearDebugLogMESA
#define glClearDebugLogMESA (gl.ClearDebugLogMESA)
#undef glClearDepthdNV
#define glClearDepthdNV (gl.ClearDepthdNV)
#undef glClearDepthf
#define glClearDepthf (gl.ClearDepthf)
#undef glClearDepthfOES
#define glClearDepthfOES (gl.ClearDepthfOES)
#undef glClearDepthxOES
#define glClearDepthxOES (gl.ClearDepthxOES)
#undef glClearNamedBufferDataEXT
#define glClearNamedBufferDataEXT (gl.ClearNamedBufferDataEXT)
#undef glClearNamedBufferSubDataEXT
#define glClearNamedBufferSubDataEXT (gl.ClearNamedBufferSubDataEXT)
#undef glClearTexImage
#define glClearTexImage (gl.ClearTexImage)
#undef glClearTexSubImage
#define glClearTexSubImage (gl.ClearTexSubImage)
#undef glClientAttribDefaultEXT
#define glClientAttribDefaultEXT (gl.ClientAttribDefaultEXT)
#undef glClientWaitSync
#define glClientWaitSync (gl.ClientWaitSync)
#undef glClipPlanefOES
#define glClipPlanefOES (gl.ClipPlanefOES)
#undef glClipPlanexOES
#define glClipPlanexOES (gl.ClipPlanexOES)
#undef glColor3xOES
#define glColor3xOES (gl.Color3xOES)
#undef glColor3xvOES
#define glColor3xvOES (gl.Color3xvOES)
#undef glColor4xOES
#define glColor4xOES (gl.Color4xOES)
#undef glColor4xvOES
#define glColor4xvOES (gl.Color4xvOES)
#undef glColorFormatNV
#define glColorFormatNV (gl.ColorFormatNV)
#undef glColorMaskIndexedEXT
#define glColorMaskIndexedEXT (gl.ColorMaskIndexedEXT)
#undef glColorMaski
#define glColorMaski (gl.ColorMaski)
#undef glColorP3ui
#define glColorP3ui (gl.ColorP3ui)
#undef glColorP3uiv
#define glColorP3uiv (gl.ColorP3uiv)
#undef glColorP4ui
#define glColorP4ui (gl.ColorP4ui)
#undef glColorP4uiv
#define glColorP4uiv (gl.ColorP4uiv)
#undef glCompileShader
#define glCompileShader (gl.CompileShader)
#undef glCompileShaderIncludeARB
#define glCompileShaderIncludeARB (gl.CompileShaderIncludeARB)
#undef glCompressedMultiTexImage1DEXT
#define glCompressedMultiTexImage1DEXT (gl.CompressedMultiTexImage1DEXT)
#undef glCompressedMultiTexImage2DEXT
#define glCompressedMultiTexImage2DEXT (gl.CompressedMultiTexImage2DEXT)
#undef glCompressedMultiTexImage3DEXT
#define glCompressedMultiTexImage3DEXT (gl.CompressedMultiTexImage3DEXT)
#undef glCompressedMultiTexSubImage1DEXT
#define glCompressedMultiTexSubImage1DEXT (gl.CompressedMultiTexSubImage1DEXT)
#undef glCompressedMultiTexSubImage2DEXT
#define glCompressedMultiTexSubImage2DEXT (gl.CompressedMultiTexSubImage2DEXT)
#undef glCompressedMultiTexSubImage3DEXT
#define glCompressedMultiTexSubImage3DEXT (gl.CompressedMultiTexSubImage3DEXT)
#undef glCompressedTextureImage1DEXT
#define glCompressedTextureImage1DEXT (gl.CompressedTextureImage1DEXT)
#undef glCompressedTextureImage2DEXT
#define glCompressedTextureImage2DEXT (gl.CompressedTextureImage2DEXT)
#undef glCompressedTextureImage3DEXT
#define glCompressedTextureImage3DEXT (gl.CompressedTextureImage3DEXT)
#undef glCompressedTextureSubImage1DEXT
#define glCompressedTextureSubImage1DEXT (gl.CompressedTextureSubImage1DEXT)
#undef glCompressedTextureSubImage2DEXT
#define glCompressedTextureSubImage2DEXT (gl.CompressedTextureSubImage2DEXT)
#undef glCompressedTextureSubImage3DEXT
#define glCompressedTextureSubImage3DEXT (gl.CompressedTextureSubImage3DEXT)
#undef glConvolutionParameterxOES
#define glConvolutionParameterxOES (gl.ConvolutionParameterxOES)
#undef glConvolutionParameterxvOES
#define glConvolutionParameterxvOES (gl.ConvolutionParameterxvOES)
#undef glCopyBufferSubData
#define glCopyBufferSubData (gl.CopyBufferSubData)
#undef glCopyImageSubData
#define glCopyImageSubData (gl.CopyImageSubData)
#undef glCopyImageSubDataNV
#define glCopyImageSubDataNV (gl.CopyImageSubDataNV)
#undef glCopyMultiTexImage1DEXT
#define glCopyMultiTexImage1DEXT (gl.CopyMultiTexImage1DEXT)
#undef glCopyMultiTexImage2DEXT
#define glCopyMultiTexImage2DEXT (gl.CopyMultiTexImage2DEXT)
#undef glCopyMultiTexSubImage1DEXT
#define glCopyMultiTexSubImage1DEXT (gl.CopyMultiTexSubImage1DEXT)
#undef glCopyMultiTexSubImage2DEXT
#define glCopyMultiTexSubImage2DEXT (gl.CopyMultiTexSubImage2DEXT)
#undef glCopyMultiTexSubImage3DEXT
#define glCopyMultiTexSubImage3DEXT (gl.CopyMultiTexSubImage3DEXT)
#undef glCopyPathNV
#define glCopyPathNV (gl.CopyPathNV)
#undef glCopyTextureImage1DEXT
#define glCopyTextureImage1DEXT (gl.CopyTextureImage1DEXT)
#undef glCopyTextureImage2DEXT
#define glCopyTextureImage2DEXT (gl.CopyTextureImage2DEXT)
#undef glCopyTextureSubImage1DEXT
#define glCopyTextureSubImage1DEXT (gl.CopyTextureSubImage1DEXT)
#undef glCopyTextureSubImage2DEXT
#define glCopyTextureSubImage2DEXT (gl.CopyTextureSubImage2DEXT)
#undef glCopyTextureSubImage3DEXT
#define glCopyTextureSubImage3DEXT (gl.CopyTextureSubImage3DEXT)
#undef glCoverFillPathInstancedNV
#define glCoverFillPathInstancedNV (gl.CoverFillPathInstancedNV)
#undef glCoverFillPathNV
#define glCoverFillPathNV (gl.CoverFillPathNV)
#undef glCoverStrokePathInstancedNV
#define glCoverStrokePathInstancedNV (gl.CoverStrokePathInstancedNV)
#undef glCoverStrokePathNV
#define glCoverStrokePathNV (gl.CoverStrokePathNV)
#undef glCreateDebugObjectMESA
#define glCreateDebugObjectMESA (gl.CreateDebugObjectMESA)
#undef glCreatePerfQueryINTEL
#define glCreatePerfQueryINTEL (gl.CreatePerfQueryINTEL)
#undef glCreateProgram
#define glCreateProgram (gl.CreateProgram)
#undef glCreateShader
#define glCreateShader (gl.CreateShader)
#undef glCreateShaderProgramEXT
#define glCreateShaderProgramEXT (gl.CreateShaderProgramEXT)
#undef glCreateShaderProgramv
#define glCreateShaderProgramv (gl.CreateShaderProgramv)
#undef glCreateSyncFromCLeventARB
#define glCreateSyncFromCLeventARB (gl.CreateSyncFromCLeventARB)
#undef glDebugMessageCallback
#define glDebugMessageCallback (gl.DebugMessageCallback)
#undef glDebugMessageCallbackAMD
#define glDebugMessageCallbackAMD (gl.DebugMessageCallbackAMD)
#undef glDebugMessageCallbackARB
#define glDebugMessageCallbackARB (gl.DebugMessageCallbackARB)
#undef glDebugMessageControl
#define glDebugMessageControl (gl.DebugMessageControl)
#undef glDebugMessageControlARB
#define glDebugMessageControlARB (gl.DebugMessageControlARB)
#undef glDebugMessageEnableAMD
#define glDebugMessageEnableAMD (gl.DebugMessageEnableAMD)
#undef glDebugMessageInsert
#define glDebugMessageInsert (gl.DebugMessageInsert)
#undef glDebugMessageInsertAMD
#define glDebugMessageInsertAMD (gl.DebugMessageInsertAMD)
#undef glDebugMessageInsertARB
#define glDebugMessageInsertARB (gl.DebugMessageInsertARB)
#undef glDeleteFramebuffers
#define glDeleteFramebuffers (gl.DeleteFramebuffers)
#undef glDeleteFramebuffersEXT
#define glDeleteFramebuffersEXT (gl.DeleteFramebuffersEXT)
#undef glDeleteNamedStringARB
#define glDeleteNamedStringARB (gl.DeleteNamedStringARB)
#undef glDeleteNamesAMD
#define glDeleteNamesAMD (gl.DeleteNamesAMD)
#undef glDeleteObjectBufferATI
#define glDeleteObjectBufferATI (gl.DeleteObjectBufferATI)
#undef glDeletePathsNV
#define glDeletePathsNV (gl.DeletePathsNV)
#undef glDeletePerfMonitorsAMD
#define glDeletePerfMonitorsAMD (gl.DeletePerfMonitorsAMD)
#undef glDeletePerfQueryINTEL
#define glDeletePerfQueryINTEL (gl.DeletePerfQueryINTEL)
#undef glDeleteProgram
#define glDeleteProgram (gl.DeleteProgram)
#undef glDeleteProgramPipelines
#define glDeleteProgramPipelines (gl.DeleteProgramPipelines)
#undef glDeleteRenderbuffers
#define glDeleteRenderbuffers (gl.DeleteRenderbuffers)
#undef glDeleteRenderbuffersEXT
#define glDeleteRenderbuffersEXT (gl.DeleteRenderbuffersEXT)
#undef glDeleteSamplers
#define glDeleteSamplers (gl.DeleteSamplers)
#undef glDeleteShader
#define glDeleteShader (gl.DeleteShader)
#undef glDeleteSync
#define glDeleteSync (gl.DeleteSync)
#undef glDeleteTransformFeedbacks
#define glDeleteTransformFeedbacks (gl.DeleteTransformFeedbacks)
#undef glDeleteTransformFeedbacksNV
#define glDeleteTransformFeedbacksNV (gl.DeleteTransformFeedbacksNV)
#undef glDeleteVertexArrays
#define glDeleteVertexArrays (gl.DeleteVertexArrays)
#undef glDepthBoundsdNV
#define glDepthBoundsdNV (gl.DepthBoundsdNV)
#undef glDepthRangeArrayv
#define glDepthRangeArrayv (gl.DepthRangeArrayv)
#undef glDepthRangeIndexed
#define glDepthRangeIndexed (gl.DepthRangeIndexed)
#undef glDepthRangedNV
#define glDepthRangedNV (gl.DepthRangedNV)
#undef glDepthRangef
#define glDepthRangef (gl.DepthRangef)
#undef glDepthRangefOES
#define glDepthRangefOES (gl.DepthRangefOES)
#undef glDepthRangexOES
#define glDepthRangexOES (gl.DepthRangexOES)
#undef glDetachShader
#define glDetachShader (gl.DetachShader)
#undef glDisableClientStateIndexedEXT
#define glDisableClientStateIndexedEXT (gl.DisableClientStateIndexedEXT)
#undef glDisableClientStateiEXT
#define glDisableClientStateiEXT (gl.DisableClientStateiEXT)
#undef glDisableIndexedEXT
#define glDisableIndexedEXT (gl.DisableIndexedEXT)
#undef glDisableVertexArrayAttribEXT
#define glDisableVertexArrayAttribEXT (gl.DisableVertexArrayAttribEXT)
#undef glDisableVertexArrayEXT
#define glDisableVertexArrayEXT (gl.DisableVertexArrayEXT)
#undef glDisableVertexAttribAPPLE
#define glDisableVertexAttribAPPLE (gl.DisableVertexAttribAPPLE)
#undef glDisableVertexAttribArray
#define glDisableVertexAttribArray (gl.DisableVertexAttribArray)
#undef glDisablei
#define glDisablei (gl.Disablei)
#undef glDispatchCompute
#define glDispatchCompute (gl.DispatchCompute)
#undef glDispatchComputeGroupSizeARB
#define glDispatchComputeGroupSizeARB (gl.DispatchComputeGroupSizeARB)
#undef glDispatchComputeIndirect
#define glDispatchComputeIndirect (gl.DispatchComputeIndirect)
#undef glDrawArraysIndirect
#define glDrawArraysIndirect (gl.DrawArraysIndirect)
#undef glDrawArraysInstanced
#define glDrawArraysInstanced (gl.DrawArraysInstanced)
#undef glDrawArraysInstancedARB
#define glDrawArraysInstancedARB (gl.DrawArraysInstancedARB)
#undef glDrawArraysInstancedBaseInstance
#define glDrawArraysInstancedBaseInstance (gl.DrawArraysInstancedBaseInstance)
#undef glDrawArraysInstancedEXT
#define glDrawArraysInstancedEXT (gl.DrawArraysInstancedEXT)
#undef glDrawBuffers
#define glDrawBuffers (gl.DrawBuffers)
#undef glDrawElementsBaseVertex
#define glDrawElementsBaseVertex (gl.DrawElementsBaseVertex)
#undef glDrawElementsIndirect
#define glDrawElementsIndirect (gl.DrawElementsIndirect)
#undef glDrawElementsInstanced
#define glDrawElementsInstanced (gl.DrawElementsInstanced)
#undef glDrawElementsInstancedARB
#define glDrawElementsInstancedARB (gl.DrawElementsInstancedARB)
#undef glDrawElementsInstancedBaseInstance
#define glDrawElementsInstancedBaseInstance (gl.DrawElementsInstancedBaseInstance)
#undef glDrawElementsInstancedBaseVertex
#define glDrawElementsInstancedBaseVertex (gl.DrawElementsInstancedBaseVertex)
#undef glDrawElementsInstancedBaseVertexBaseInstance
#define glDrawElementsInstancedBaseVertexBaseInstance (gl.DrawElementsInstancedBaseVertexBaseInstance)
#undef glDrawElementsInstancedEXT
#define glDrawElementsInstancedEXT (gl.DrawElementsInstancedEXT)
#undef glDrawRangeElementsBaseVertex
#define glDrawRangeElementsBaseVertex (gl.DrawRangeElementsBaseVertex)
#undef glDrawTextureNV
#define glDrawTextureNV (gl.DrawTextureNV)
#undef glDrawTransformFeedback
#define glDrawTransformFeedback (gl.DrawTransformFeedback)
#undef glDrawTransformFeedbackInstanced
#define glDrawTransformFeedbackInstanced (gl.DrawTransformFeedbackInstanced)
#undef glDrawTransformFeedbackNV
#define glDrawTransformFeedbackNV (gl.DrawTransformFeedbackNV)
#undef glDrawTransformFeedbackStream
#define glDrawTransformFeedbackStream (gl.DrawTransformFeedbackStream)
#undef glDrawTransformFeedbackStreamInstanced
#define glDrawTransformFeedbackStreamInstanced (gl.DrawTransformFeedbackStreamInstanced)
#undef glEdgeFlagFormatNV
#define glEdgeFlagFormatNV (gl.EdgeFlagFormatNV)
#undef glEnableClientStateIndexedEXT
#define glEnableClientStateIndexedEXT (gl.EnableClientStateIndexedEXT)
#undef glEnableClientStateiEXT
#define glEnableClientStateiEXT (gl.EnableClientStateiEXT)
#undef glEnableIndexedEXT
#define glEnableIndexedEXT (gl.EnableIndexedEXT)
#undef glEnableVertexArrayAttribEXT
#define glEnableVertexArrayAttribEXT (gl.EnableVertexArrayAttribEXT)
#undef glEnableVertexArrayEXT
#define glEnableVertexArrayEXT (gl.EnableVertexArrayEXT)
#undef glEnableVertexAttribAPPLE
#define glEnableVertexAttribAPPLE (gl.EnableVertexAttribAPPLE)
#undef glEnableVertexAttribArray
#define glEnableVertexAttribArray (gl.EnableVertexAttribArray)
#undef glEnablei
#define glEnablei (gl.Enablei)
#undef glEndConditionalRender
#define glEndConditionalRender (gl.EndConditionalRender)
#undef glEndConditionalRenderNV
#define glEndConditionalRenderNV (gl.EndConditionalRenderNV)
#undef glEndConditionalRenderNVX
#define glEndConditionalRenderNVX (gl.EndConditionalRenderNVX)
#undef glEndPerfMonitorAMD
#define glEndPerfMonitorAMD (gl.EndPerfMonitorAMD)
#undef glEndPerfQueryINTEL
#define glEndPerfQueryINTEL (gl.EndPerfQueryINTEL)
#undef glEndQueryIndexed
#define glEndQueryIndexed (gl.EndQueryIndexed)
#undef glEndTransformFeedback
#define glEndTransformFeedback (gl.EndTransformFeedback)
#undef glEndTransformFeedbackEXT
#define glEndTransformFeedbackEXT (gl.EndTransformFeedbackEXT)
#undef glEndTransformFeedbackNV
#define glEndTransformFeedbackNV (gl.EndTransformFeedbackNV)
#undef glEndVideoCaptureNV
#define glEndVideoCaptureNV (gl.EndVideoCaptureNV)
#undef glEvalCoord1xOES
#define glEvalCoord1xOES (gl.EvalCoord1xOES)
#undef glEvalCoord1xvOES
#define glEvalCoord1xvOES (gl.EvalCoord1xvOES)
#undef glEvalCoord2xOES
#define glEvalCoord2xOES (gl.EvalCoord2xOES)
#undef glEvalCoord2xvOES
#define glEvalCoord2xvOES (gl.EvalCoord2xvOES)
#undef glFeedbackBufferxOES
#define glFeedbackBufferxOES (gl.FeedbackBufferxOES)
#undef glFenceSync
#define glFenceSync (gl.FenceSync)
#undef glFinishRenderAPPLE
#define glFinishRenderAPPLE (gl.FinishRenderAPPLE)
#undef glFlushMappedBufferRange
#define glFlushMappedBufferRange (gl.FlushMappedBufferRange)
#undef glFlushMappedBufferRangeAPPLE
#define glFlushMappedBufferRangeAPPLE (gl.FlushMappedBufferRangeAPPLE)
#undef glFlushMappedNamedBufferRangeEXT
#define glFlushMappedNamedBufferRangeEXT (gl.FlushMappedNamedBufferRangeEXT)
#undef glFlushRenderAPPLE
#define glFlushRenderAPPLE (gl.FlushRenderAPPLE)
#undef glFlushStaticDataIBM
#define glFlushStaticDataIBM (gl.FlushStaticDataIBM)
#undef glFogCoordFormatNV
#define glFogCoordFormatNV (gl.FogCoordFormatNV)
#undef glFogxOES
#define glFogxOES (gl.FogxOES)
#undef glFogxvOES
#define glFogxvOES (gl.FogxvOES)
#undef glFrameTerminatorGREMEDY
#define glFrameTerminatorGREMEDY (gl.FrameTerminatorGREMEDY)
#undef glFramebufferDrawBufferEXT
#define glFramebufferDrawBufferEXT (gl.FramebufferDrawBufferEXT)
#undef glFramebufferDrawBuffersEXT
#define glFramebufferDrawBuffersEXT (gl.FramebufferDrawBuffersEXT)
#undef glFramebufferParameteri
#define glFramebufferParameteri (gl.FramebufferParameteri)
#undef glFramebufferReadBufferEXT
#define glFramebufferReadBufferEXT (gl.FramebufferReadBufferEXT)
#undef glFramebufferRenderbuffer
#define glFramebufferRenderbuffer (gl.FramebufferRenderbuffer)
#undef glFramebufferRenderbufferEXT
#define glFramebufferRenderbufferEXT (gl.FramebufferRenderbufferEXT)
#undef glFramebufferTexture
#define glFramebufferTexture (gl.FramebufferTexture)
#undef glFramebufferTexture1D
#define glFramebufferTexture1D (gl.FramebufferTexture1D)
#undef glFramebufferTexture1DEXT
#define glFramebufferTexture1DEXT (gl.FramebufferTexture1DEXT)
#undef glFramebufferTexture2D
#define glFramebufferTexture2D (gl.FramebufferTexture2D)
#undef glFramebufferTexture2DEXT
#define glFramebufferTexture2DEXT (gl.FramebufferTexture2DEXT)
#undef glFramebufferTexture3D
#define glFramebufferTexture3D (gl.FramebufferTexture3D)
#undef glFramebufferTexture3DEXT
#define glFramebufferTexture3DEXT (gl.FramebufferTexture3DEXT)
#undef glFramebufferTextureARB
#define glFramebufferTextureARB (gl.FramebufferTextureARB)
#undef glFramebufferTextureEXT
#define glFramebufferTextureEXT (gl.FramebufferTextureEXT)
#undef glFramebufferTextureFaceARB
#define glFramebufferTextureFaceARB (gl.FramebufferTextureFaceARB)
#undef glFramebufferTextureFaceEXT
#define glFramebufferTextureFaceEXT (gl.FramebufferTextureFaceEXT)
#undef glFramebufferTextureLayer
#define glFramebufferTextureLayer (gl.FramebufferTextureLayer)
#undef glFramebufferTextureLayerARB
#define glFramebufferTextureLayerARB (gl.FramebufferTextureLayerARB)
#undef glFramebufferTextureLayerEXT
#define glFramebufferTextureLayerEXT (gl.FramebufferTextureLayerEXT)
#undef glFrustumfOES
#define glFrustumfOES (gl.FrustumfOES)
#undef glFrustumxOES
#define glFrustumxOES (gl.FrustumxOES)
#undef glGenFramebuffers
#define glGenFramebuffers (gl.GenFramebuffers)
#undef glGenFramebuffersEXT
#define glGenFramebuffersEXT (gl.GenFramebuffersEXT)
#undef glGenNamesAMD
#define glGenNamesAMD (gl.GenNamesAMD)
#undef glGenPathsNV
#define glGenPathsNV (gl.GenPathsNV)
#undef glGenPerfMonitorsAMD
#define glGenPerfMonitorsAMD (gl.GenPerfMonitorsAMD)
#undef glGenProgramPipelines
#define glGenProgramPipelines (gl.GenProgramPipelines)
#undef glGenRenderbuffers
#define glGenRenderbuffers (gl.GenRenderbuffers)
#undef glGenRenderbuffersEXT
#define glGenRenderbuffersEXT (gl.GenRenderbuffersEXT)
#undef glGenSamplers
#define glGenSamplers (gl.GenSamplers)
#undef glGenTransformFeedbacks
#define glGenTransformFeedbacks (gl.GenTransformFeedbacks)
#undef glGenTransformFeedbacksNV
#define glGenTransformFeedbacksNV (gl.GenTransformFeedbacksNV)
#undef glGenVertexArrays
#define glGenVertexArrays (gl.GenVertexArrays)
#undef glGenerateMipmap
#define glGenerateMipmap (gl.GenerateMipmap)
#undef glGenerateMipmapEXT
#define glGenerateMipmapEXT (gl.GenerateMipmapEXT)
#undef glGenerateMultiTexMipmapEXT
#define glGenerateMultiTexMipmapEXT (gl.GenerateMultiTexMipmapEXT)
#undef glGenerateTextureMipmapEXT
#define glGenerateTextureMipmapEXT (gl.GenerateTextureMipmapEXT)
#undef glGetActiveAtomicCounterBufferiv
#define glGetActiveAtomicCounterBufferiv (gl.GetActiveAtomicCounterBufferiv)
#undef glGetActiveAttrib
#define glGetActiveAttrib (gl.GetActiveAttrib)
#undef glGetActiveSubroutineName
#define glGetActiveSubroutineName (gl.GetActiveSubroutineName)
#undef glGetActiveSubroutineUniformName
#define glGetActiveSubroutineUniformName (gl.GetActiveSubroutineUniformName)
#undef glGetActiveSubroutineUniformiv
#define glGetActiveSubroutineUniformiv (gl.GetActiveSubroutineUniformiv)
#undef glGetActiveUniform
#define glGetActiveUniform (gl.GetActiveUniform)
#undef glGetActiveUniformBlockIndex
#define glGetActiveUniformBlockIndex (gl.GetActiveUniformBlockIndex)
#undef glGetActiveUniformBlockName
#define glGetActiveUniformBlockName (gl.GetActiveUniformBlockName)
#undef glGetActiveUniformBlockiv
#define glGetActiveUniformBlockiv (gl.GetActiveUniformBlockiv)
#undef glGetActiveUniformName
#define glGetActiveUniformName (gl.GetActiveUniformName)
#undef glGetActiveUniformsiv
#define glGetActiveUniformsiv (gl.GetActiveUniformsiv)
#undef glGetActiveVaryingNV
#define glGetActiveVaryingNV (gl.GetActiveVaryingNV)
#undef glGetAttachedShaders
#define glGetAttachedShaders (gl.GetAttachedShaders)
#undef glGetAttribLocation
#define glGetAttribLocation (gl.GetAttribLocation)
#undef glGetBooleanIndexedvEXT
#define glGetBooleanIndexedvEXT (gl.GetBooleanIndexedvEXT)
#undef glGetBooleani_v
#define glGetBooleani_v (gl.GetBooleani_v)
#undef glGetBufferParameteri64v
#define glGetBufferParameteri64v (gl.GetBufferParameteri64v)
#undef glGetBufferParameterui64vNV
#define glGetBufferParameterui64vNV (gl.GetBufferParameterui64vNV)
#undef glGetClipPlanefOES
#define glGetClipPlanefOES (gl.GetClipPlanefOES)
#undef glGetClipPlanexOES
#define glGetClipPlanexOES (gl.GetClipPlanexOES)
#undef glGetCompressedMultiTexImageEXT
#define glGetCompressedMultiTexImageEXT (gl.GetCompressedMultiTexImageEXT)
#undef glGetCompressedTextureImageEXT
#define glGetCompressedTextureImageEXT (gl.GetCompressedTextureImageEXT)
#undef glGetConvolutionParameterxvOES
#define glGetConvolutionParameterxvOES (gl.GetConvolutionParameterxvOES)
#undef glGetDebugLogLengthMESA
#define glGetDebugLogLengthMESA (gl.GetDebugLogLengthMESA)
#undef glGetDebugLogMESA
#define glGetDebugLogMESA (gl.GetDebugLogMESA)
#undef glGetDebugMessageLog
#define glGetDebugMessageLog (gl.GetDebugMessageLog)
#undef glGetDebugMessageLogAMD
#define glGetDebugMessageLogAMD (gl.GetDebugMessageLogAMD)
#undef glGetDebugMessageLogARB
#define glGetDebugMessageLogARB (gl.GetDebugMessageLogARB)
#undef glGetDoubleIndexedvEXT
#define glGetDoubleIndexedvEXT (gl.GetDoubleIndexedvEXT)
#undef glGetDoublei_v
#define glGetDoublei_v (gl.GetDoublei_v)
#undef glGetDoublei_vEXT
#define glGetDoublei_vEXT (gl.GetDoublei_vEXT)
#undef glGetFirstPerfQueryIdINTEL
#define glGetFirstPerfQueryIdINTEL (gl.GetFirstPerfQueryIdINTEL)
#undef glGetFixedvOES
#define glGetFixedvOES (gl.GetFixedvOES)
#undef glGetFloatIndexedvEXT
#define glGetFloatIndexedvEXT (gl.GetFloatIndexedvEXT)
#undef glGetFloati_v
#define glGetFloati_v (gl.GetFloati_v)
#undef glGetFloati_vEXT
#define glGetFloati_vEXT (gl.GetFloati_vEXT)
#undef glGetFragDataIndex
#define glGetFragDataIndex (gl.GetFragDataIndex)
#undef glGetFragDataLocation
#define glGetFragDataLocation (gl.GetFragDataLocation)
#undef glGetFragDataLocationEXT
#define glGetFragDataLocationEXT (gl.GetFragDataLocationEXT)
#undef glGetFramebufferAttachmentParameteriv
#define glGetFramebufferAttachmentParameteriv (gl.GetFramebufferAttachmentParameteriv)
#undef glGetFramebufferAttachmentParameterivEXT
#define glGetFramebufferAttachmentParameterivEXT (gl.GetFramebufferAttachmentParameterivEXT)
#undef glGetFramebufferParameteriv
#define glGetFramebufferParameteriv (gl.GetFramebufferParameteriv)
#undef glGetFramebufferParameterivEXT
#define glGetFramebufferParameterivEXT (gl.GetFramebufferParameterivEXT)
#undef glGetGraphicsResetStatusARB
#define glGetGraphicsResetStatusARB (gl.GetGraphicsResetStatusARB)
#undef glGetHistogramParameterxvOES
#define glGetHistogramParameterxvOES (gl.GetHistogramParameterxvOES)
#undef glGetImageHandleARB
#define glGetImageHandleARB (gl.GetImageHandleARB)
#undef glGetImageHandleNV
#define glGetImageHandleNV (gl.GetImageHandleNV)
#undef glGetInteger64i_v
#define glGetInteger64i_v (gl.GetInteger64i_v)
#undef glGetInteger64v
#define glGetInteger64v (gl.GetInteger64v)
#undef glGetIntegerIndexedvEXT
#define glGetIntegerIndexedvEXT (gl.GetIntegerIndexedvEXT)
#undef glGetIntegeri_v
#define glGetIntegeri_v (gl.GetIntegeri_v)
#undef glGetIntegerui64i_vNV
#define glGetIntegerui64i_vNV (gl.GetIntegerui64i_vNV)
#undef glGetIntegerui64vNV
#define glGetIntegerui64vNV (gl.GetIntegerui64vNV)
#undef glGetInternalformati64v
#define glGetInternalformati64v (gl.GetInternalformati64v)
#undef glGetInternalformativ
#define glGetInternalformativ (gl.GetInternalformativ)
#undef glGetLightxOES
#define glGetLightxOES (gl.GetLightxOES)
#undef glGetMapxvOES
#define glGetMapxvOES (gl.GetMapxvOES)
#undef glGetMaterialxOES
#define glGetMaterialxOES (gl.GetMaterialxOES)
#undef glGetMultiTexEnvfvEXT
#define glGetMultiTexEnvfvEXT (gl.GetMultiTexEnvfvEXT)
#undef glGetMultiTexEnvivEXT
#define glGetMultiTexEnvivEXT (gl.GetMultiTexEnvivEXT)
#undef glGetMultiTexGendvEXT
#define glGetMultiTexGendvEXT (gl.GetMultiTexGendvEXT)
#undef glGetMultiTexGenfvEXT
#define glGetMultiTexGenfvEXT (gl.GetMultiTexGenfvEXT)
#undef glGetMultiTexGenivEXT
#define glGetMultiTexGenivEXT (gl.GetMultiTexGenivEXT)
#undef glGetMultiTexImageEXT
#define glGetMultiTexImageEXT (gl.GetMultiTexImageEXT)
#undef glGetMultiTexLevelParameterfvEXT
#define glGetMultiTexLevelParameterfvEXT (gl.GetMultiTexLevelParameterfvEXT)
#undef glGetMultiTexLevelParameterivEXT
#define glGetMultiTexLevelParameterivEXT (gl.GetMultiTexLevelParameterivEXT)
#undef glGetMultiTexParameterIivEXT
#define glGetMultiTexParameterIivEXT (gl.GetMultiTexParameterIivEXT)
#undef glGetMultiTexParameterIuivEXT
#define glGetMultiTexParameterIuivEXT (gl.GetMultiTexParameterIuivEXT)
#undef glGetMultiTexParameterfvEXT
#define glGetMultiTexParameterfvEXT (gl.GetMultiTexParameterfvEXT)
#undef glGetMultiTexParameterivEXT
#define glGetMultiTexParameterivEXT (gl.GetMultiTexParameterivEXT)
#undef glGetMultisamplefv
#define glGetMultisamplefv (gl.GetMultisamplefv)
#undef glGetMultisamplefvNV
#define glGetMultisamplefvNV (gl.GetMultisamplefvNV)
#undef glGetNamedBufferParameterivEXT
#define glGetNamedBufferParameterivEXT (gl.GetNamedBufferParameterivEXT)
#undef glGetNamedBufferParameterui64vNV
#define glGetNamedBufferParameterui64vNV (gl.GetNamedBufferParameterui64vNV)
#undef glGetNamedBufferPointervEXT
#define glGetNamedBufferPointervEXT (gl.GetNamedBufferPointervEXT)
#undef glGetNamedBufferSubDataEXT
#define glGetNamedBufferSubDataEXT (gl.GetNamedBufferSubDataEXT)
#undef glGetNamedFramebufferAttachmentParameterivEXT
#define glGetNamedFramebufferAttachmentParameterivEXT (gl.GetNamedFramebufferAttachmentParameterivEXT)
#undef glGetNamedFramebufferParameterivEXT
#define glGetNamedFramebufferParameterivEXT (gl.GetNamedFramebufferParameterivEXT)
#undef glGetNamedProgramLocalParameterIivEXT
#define glGetNamedProgramLocalParameterIivEXT (gl.GetNamedProgramLocalParameterIivEXT)
#undef glGetNamedProgramLocalParameterIuivEXT
#define glGetNamedProgramLocalParameterIuivEXT (gl.GetNamedProgramLocalParameterIuivEXT)
#undef glGetNamedProgramLocalParameterdvEXT
#define glGetNamedProgramLocalParameterdvEXT (gl.GetNamedProgramLocalParameterdvEXT)
#undef glGetNamedProgramLocalParameterfvEXT
#define glGetNamedProgramLocalParameterfvEXT (gl.GetNamedProgramLocalParameterfvEXT)
#undef glGetNamedProgramStringEXT
#define glGetNamedProgramStringEXT (gl.GetNamedProgramStringEXT)
#undef glGetNamedProgramivEXT
#define glGetNamedProgramivEXT (gl.GetNamedProgramivEXT)
#undef glGetNamedRenderbufferParameterivEXT
#define glGetNamedRenderbufferParameterivEXT (gl.GetNamedRenderbufferParameterivEXT)
#undef glGetNamedStringARB
#define glGetNamedStringARB (gl.GetNamedStringARB)
#undef glGetNamedStringivARB
#define glGetNamedStringivARB (gl.GetNamedStringivARB)
#undef glGetNextPerfQueryIdINTEL
#define glGetNextPerfQueryIdINTEL (gl.GetNextPerfQueryIdINTEL)
#undef glGetObjectLabel
#define glGetObjectLabel (gl.GetObjectLabel)
#undef glGetObjectLabelEXT
#define glGetObjectLabelEXT (gl.GetObjectLabelEXT)
#undef glGetObjectParameterivAPPLE
#define glGetObjectParameterivAPPLE (gl.GetObjectParameterivAPPLE)
#undef glGetObjectPtrLabel
#define glGetObjectPtrLabel (gl.GetObjectPtrLabel)
#undef glGetPathColorGenfvNV
#define glGetPathColorGenfvNV (gl.GetPathColorGenfvNV)
#undef glGetPathColorGenivNV
#define glGetPathColorGenivNV (gl.GetPathColorGenivNV)
#undef glGetPathCommandsNV
#define glGetPathCommandsNV (gl.GetPathCommandsNV)
#undef glGetPathCoordsNV
#define glGetPathCoordsNV (gl.GetPathCoordsNV)
#undef glGetPathDashArrayNV
#define glGetPathDashArrayNV (gl.GetPathDashArrayNV)
#undef glGetPathLengthNV
#define glGetPathLengthNV (gl.GetPathLengthNV)
#undef glGetPathMetricRangeNV
#define glGetPathMetricRangeNV (gl.GetPathMetricRangeNV)
#undef glGetPathMetricsNV
#define glGetPathMetricsNV (gl.GetPathMetricsNV)
#undef glGetPathParameterfvNV
#define glGetPathParameterfvNV (gl.GetPathParameterfvNV)
#undef glGetPathParameterivNV
#define glGetPathParameterivNV (gl.GetPathParameterivNV)
#undef glGetPathSpacingNV
#define glGetPathSpacingNV (gl.GetPathSpacingNV)
#undef glGetPathTexGenfvNV
#define glGetPathTexGenfvNV (gl.GetPathTexGenfvNV)
#undef glGetPathTexGenivNV
#define glGetPathTexGenivNV (gl.GetPathTexGenivNV)
#undef glGetPerfCounterInfoINTEL
#define glGetPerfCounterInfoINTEL (gl.GetPerfCounterInfoINTEL)
#undef glGetPerfMonitorCounterDataAMD
#define glGetPerfMonitorCounterDataAMD (gl.GetPerfMonitorCounterDataAMD)
#undef glGetPerfMonitorCounterInfoAMD
#define glGetPerfMonitorCounterInfoAMD (gl.GetPerfMonitorCounterInfoAMD)
#undef glGetPerfMonitorCounterStringAMD
#define glGetPerfMonitorCounterStringAMD (gl.GetPerfMonitorCounterStringAMD)
#undef glGetPerfMonitorCountersAMD
#define glGetPerfMonitorCountersAMD (gl.GetPerfMonitorCountersAMD)
#undef glGetPerfMonitorGroupStringAMD
#define glGetPerfMonitorGroupStringAMD (gl.GetPerfMonitorGroupStringAMD)
#undef glGetPerfMonitorGroupsAMD
#define glGetPerfMonitorGroupsAMD (gl.GetPerfMonitorGroupsAMD)
#undef glGetPerfQueryDataINTEL
#define glGetPerfQueryDataINTEL (gl.GetPerfQueryDataINTEL)
#undef glGetPerfQueryIdByNameINTEL
#define glGetPerfQueryIdByNameINTEL (gl.GetPerfQueryIdByNameINTEL)
#undef glGetPerfQueryInfoINTEL
#define glGetPerfQueryInfoINTEL (gl.GetPerfQueryInfoINTEL)
#undef glGetPixelMapxv
#define glGetPixelMapxv (gl.GetPixelMapxv)
#undef glGetPixelTransformParameterfvEXT
#define glGetPixelTransformParameterfvEXT (gl.GetPixelTransformParameterfvEXT)
#undef glGetPixelTransformParameterivEXT
#define glGetPixelTransformParameterivEXT (gl.GetPixelTransformParameterivEXT)
#undef glGetPointerIndexedvEXT
#define glGetPointerIndexedvEXT (gl.GetPointerIndexedvEXT)
#undef glGetPointeri_vEXT
#define glGetPointeri_vEXT (gl.GetPointeri_vEXT)
#undef glGetProgramBinary
#define glGetProgramBinary (gl.GetProgramBinary)
#undef glGetProgramEnvParameterIivNV
#define glGetProgramEnvParameterIivNV (gl.GetProgramEnvParameterIivNV)
#undef glGetProgramEnvParameterIuivNV
#define glGetProgramEnvParameterIuivNV (gl.GetProgramEnvParameterIuivNV)
#undef glGetProgramInfoLog
#define glGetProgramInfoLog (gl.GetProgramInfoLog)
#undef glGetProgramInterfaceiv
#define glGetProgramInterfaceiv (gl.GetProgramInterfaceiv)
#undef glGetProgramLocalParameterIivNV
#define glGetProgramLocalParameterIivNV (gl.GetProgramLocalParameterIivNV)
#undef glGetProgramLocalParameterIuivNV
#define glGetProgramLocalParameterIuivNV (gl.GetProgramLocalParameterIuivNV)
#undef glGetProgramPipelineInfoLog
#define glGetProgramPipelineInfoLog (gl.GetProgramPipelineInfoLog)
#undef glGetProgramPipelineiv
#define glGetProgramPipelineiv (gl.GetProgramPipelineiv)
#undef glGetProgramRegisterfvMESA
#define glGetProgramRegisterfvMESA (gl.GetProgramRegisterfvMESA)
#undef glGetProgramResourceIndex
#define glGetProgramResourceIndex (gl.GetProgramResourceIndex)
#undef glGetProgramResourceLocation
#define glGetProgramResourceLocation (gl.GetProgramResourceLocation)
#undef glGetProgramResourceLocationIndex
#define glGetProgramResourceLocationIndex (gl.GetProgramResourceLocationIndex)
#undef glGetProgramResourceName
#define glGetProgramResourceName (gl.GetProgramResourceName)
#undef glGetProgramResourceiv
#define glGetProgramResourceiv (gl.GetProgramResourceiv)
#undef glGetProgramStageiv
#define glGetProgramStageiv (gl.GetProgramStageiv)
#undef glGetProgramSubroutineParameteruivNV
#define glGetProgramSubroutineParameteruivNV (gl.GetProgramSubroutineParameteruivNV)
#undef glGetProgramiv
#define glGetProgramiv (gl.GetProgramiv)
#undef glGetQueryIndexediv
#define glGetQueryIndexediv (gl.GetQueryIndexediv)
#undef glGetQueryObjecti64v
#define glGetQueryObjecti64v (gl.GetQueryObjecti64v)
#undef glGetQueryObjecti64vEXT
#define glGetQueryObjecti64vEXT (gl.GetQueryObjecti64vEXT)
#undef glGetQueryObjectui64v
#define glGetQueryObjectui64v (gl.GetQueryObjectui64v)
#undef glGetQueryObjectui64vEXT
#define glGetQueryObjectui64vEXT (gl.GetQueryObjectui64vEXT)
#undef glGetRenderbufferParameteriv
#define glGetRenderbufferParameteriv (gl.GetRenderbufferParameteriv)
#undef glGetRenderbufferParameterivEXT
#define glGetRenderbufferParameterivEXT (gl.GetRenderbufferParameterivEXT)
#undef glGetSamplerParameterIiv
#define glGetSamplerParameterIiv (gl.GetSamplerParameterIiv)
#undef glGetSamplerParameterIuiv
#define glGetSamplerParameterIuiv (gl.GetSamplerParameterIuiv)
#undef glGetSamplerParameterfv
#define glGetSamplerParameterfv (gl.GetSamplerParameterfv)
#undef glGetSamplerParameteriv
#define glGetSamplerParameteriv (gl.GetSamplerParameteriv)
#undef glGetShaderInfoLog
#define glGetShaderInfoLog (gl.GetShaderInfoLog)
#undef glGetShaderPrecisionFormat
#define glGetShaderPrecisionFormat (gl.GetShaderPrecisionFormat)
#undef glGetShaderSource
#define glGetShaderSource (gl.GetShaderSource)
#undef glGetShaderiv
#define glGetShaderiv (gl.GetShaderiv)
#undef glGetStringi
#define glGetStringi (gl.GetStringi)
#undef glGetSubroutineIndex
#define glGetSubroutineIndex (gl.GetSubroutineIndex)
#undef glGetSubroutineUniformLocation
#define glGetSubroutineUniformLocation (gl.GetSubroutineUniformLocation)
#undef glGetSynciv
#define glGetSynciv (gl.GetSynciv)
#undef glGetTexEnvxvOES
#define glGetTexEnvxvOES (gl.GetTexEnvxvOES)
#undef glGetTexGenxvOES
#define glGetTexGenxvOES (gl.GetTexGenxvOES)
#undef glGetTexLevelParameterxvOES
#define glGetTexLevelParameterxvOES (gl.GetTexLevelParameterxvOES)
#undef glGetTexParameterIiv
#define glGetTexParameterIiv (gl.GetTexParameterIiv)
#undef glGetTexParameterIivEXT
#define glGetTexParameterIivEXT (gl.GetTexParameterIivEXT)
#undef glGetTexParameterIuiv
#define glGetTexParameterIuiv (gl.GetTexParameterIuiv)
#undef glGetTexParameterIuivEXT
#define glGetTexParameterIuivEXT (gl.GetTexParameterIuivEXT)
#undef glGetTexParameterPointervAPPLE
#define glGetTexParameterPointervAPPLE (gl.GetTexParameterPointervAPPLE)
#undef glGetTexParameterxvOES
#define glGetTexParameterxvOES (gl.GetTexParameterxvOES)
#undef glGetTextureHandleARB
#define glGetTextureHandleARB (gl.GetTextureHandleARB)
#undef glGetTextureHandleNV
#define glGetTextureHandleNV (gl.GetTextureHandleNV)
#undef glGetTextureImageEXT
#define glGetTextureImageEXT (gl.GetTextureImageEXT)
#undef glGetTextureLevelParameterfvEXT
#define glGetTextureLevelParameterfvEXT (gl.GetTextureLevelParameterfvEXT)
#undef glGetTextureLevelParameterivEXT
#define glGetTextureLevelParameterivEXT (gl.GetTextureLevelParameterivEXT)
#undef glGetTextureParameterIivEXT
#define glGetTextureParameterIivEXT (gl.GetTextureParameterIivEXT)
#undef glGetTextureParameterIuivEXT
#define glGetTextureParameterIuivEXT (gl.GetTextureParameterIuivEXT)
#undef glGetTextureParameterfvEXT
#define glGetTextureParameterfvEXT (gl.GetTextureParameterfvEXT)
#undef glGetTextureParameterivEXT
#define glGetTextureParameterivEXT (gl.GetTextureParameterivEXT)
#undef glGetTextureSamplerHandleARB
#define glGetTextureSamplerHandleARB (gl.GetTextureSamplerHandleARB)
#undef glGetTextureSamplerHandleNV
#define glGetTextureSamplerHandleNV (gl.GetTextureSamplerHandleNV)
#undef glGetTransformFeedbackVarying
#define glGetTransformFeedbackVarying (gl.GetTransformFeedbackVarying)
#undef glGetTransformFeedbackVaryingEXT
#define glGetTransformFeedbackVaryingEXT (gl.GetTransformFeedbackVaryingEXT)
#undef glGetTransformFeedbackVaryingNV
#define glGetTransformFeedbackVaryingNV (gl.GetTransformFeedbackVaryingNV)
#undef glGetUniformBlockIndex
#define glGetUniformBlockIndex (gl.GetUniformBlockIndex)
#undef glGetUniformBufferSizeEXT
#define glGetUniformBufferSizeEXT (gl.GetUniformBufferSizeEXT)
#undef glGetUniformIndices
#define glGetUniformIndices (gl.GetUniformIndices)
#undef glGetUniformLocation
#define glGetUniformLocation (gl.GetUniformLocation)
#undef glGetUniformOffsetEXT
#define glGetUniformOffsetEXT (gl.GetUniformOffsetEXT)
#undef glGetUniformSubroutineuiv
#define glGetUniformSubroutineuiv (gl.GetUniformSubroutineuiv)
#undef glGetUniformdv
#define glGetUniformdv (gl.GetUniformdv)
#undef glGetUniformfv
#define glGetUniformfv (gl.GetUniformfv)
#undef glGetUniformi64vNV
#define glGetUniformi64vNV (gl.GetUniformi64vNV)
#undef glGetUniformiv
#define glGetUniformiv (gl.GetUniformiv)
#undef glGetUniformui64vNV
#define glGetUniformui64vNV (gl.GetUniformui64vNV)
#undef glGetUniformuiv
#define glGetUniformuiv (gl.GetUniformuiv)
#undef glGetUniformuivEXT
#define glGetUniformuivEXT (gl.GetUniformuivEXT)
#undef glGetVaryingLocationNV
#define glGetVaryingLocationNV (gl.GetVaryingLocationNV)
#undef glGetVertexArrayIntegeri_vEXT
#define glGetVertexArrayIntegeri_vEXT (gl.GetVertexArrayIntegeri_vEXT)
#undef glGetVertexArrayIntegervEXT
#define glGetVertexArrayIntegervEXT (gl.GetVertexArrayIntegervEXT)
#undef glGetVertexArrayPointeri_vEXT
#define glGetVertexArrayPointeri_vEXT (gl.GetVertexArrayPointeri_vEXT)
#undef glGetVertexArrayPointervEXT
#define glGetVertexArrayPointervEXT (gl.GetVertexArrayPointervEXT)
#undef glGetVertexAttribIiv
#define glGetVertexAttribIiv (gl.GetVertexAttribIiv)
#undef glGetVertexAttribIivEXT
#define glGetVertexAttribIivEXT (gl.GetVertexAttribIivEXT)
#undef glGetVertexAttribIuiv
#define glGetVertexAttribIuiv (gl.GetVertexAttribIuiv)
#undef glGetVertexAttribIuivEXT
#define glGetVertexAttribIuivEXT (gl.GetVertexAttribIuivEXT)
#undef glGetVertexAttribLdv
#define glGetVertexAttribLdv (gl.GetVertexAttribLdv)
#undef glGetVertexAttribLdvEXT
#define glGetVertexAttribLdvEXT (gl.GetVertexAttribLdvEXT)
#undef glGetVertexAttribLi64vNV
#define glGetVertexAttribLi64vNV (gl.GetVertexAttribLi64vNV)
#undef glGetVertexAttribLui64vARB
#define glGetVertexAttribLui64vARB (gl.GetVertexAttribLui64vARB)
#undef glGetVertexAttribLui64vNV
#define glGetVertexAttribLui64vNV (gl.GetVertexAttribLui64vNV)
#undef glGetVertexAttribPointerv
#define glGetVertexAttribPointerv (gl.GetVertexAttribPointerv)
#undef glGetVertexAttribdv
#define glGetVertexAttribdv (gl.GetVertexAttribdv)
#undef glGetVertexAttribfv
#define glGetVertexAttribfv (gl.GetVertexAttribfv)
#undef glGetVertexAttribiv
#define glGetVertexAttribiv (gl.GetVertexAttribiv)
#undef glGetVideoCaptureStreamdvNV
#define glGetVideoCaptureStreamdvNV (gl.GetVideoCaptureStreamdvNV)
#undef glGetVideoCaptureStreamfvNV
#define glGetVideoCaptureStreamfvNV (gl.GetVideoCaptureStreamfvNV)
#undef glGetVideoCaptureStreamivNV
#define glGetVideoCaptureStreamivNV (gl.GetVideoCaptureStreamivNV)
#undef glGetVideoCaptureivNV
#define glGetVideoCaptureivNV (gl.GetVideoCaptureivNV)
#undef glGetVideoi64vNV
#define glGetVideoi64vNV (gl.GetVideoi64vNV)
#undef glGetVideoivNV
#define glGetVideoivNV (gl.GetVideoivNV)
#undef glGetVideoui64vNV
#define glGetVideoui64vNV (gl.GetVideoui64vNV)
#undef glGetVideouivNV
#define glGetVideouivNV (gl.GetVideouivNV)
#undef glGetnColorTableARB
#define glGetnColorTableARB (gl.GetnColorTableARB)
#undef glGetnCompressedTexImageARB
#define glGetnCompressedTexImageARB (gl.GetnCompressedTexImageARB)
#undef glGetnConvolutionFilterARB
#define glGetnConvolutionFilterARB (gl.GetnConvolutionFilterARB)
#undef glGetnHistogramARB
#define glGetnHistogramARB (gl.GetnHistogramARB)
#undef glGetnMapdvARB
#define glGetnMapdvARB (gl.GetnMapdvARB)
#undef glGetnMapfvARB
#define glGetnMapfvARB (gl.GetnMapfvARB)
#undef glGetnMapivARB
#define glGetnMapivARB (gl.GetnMapivARB)
#undef glGetnMinmaxARB
#define glGetnMinmaxARB (gl.GetnMinmaxARB)
#undef glGetnPixelMapfvARB
#define glGetnPixelMapfvARB (gl.GetnPixelMapfvARB)
#undef glGetnPixelMapuivARB
#define glGetnPixelMapuivARB (gl.GetnPixelMapuivARB)
#undef glGetnPixelMapusvARB
#define glGetnPixelMapusvARB (gl.GetnPixelMapusvARB)
#undef glGetnPolygonStippleARB
#define glGetnPolygonStippleARB (gl.GetnPolygonStippleARB)
#undef glGetnSeparableFilterARB
#define glGetnSeparableFilterARB (gl.GetnSeparableFilterARB)
#undef glGetnTexImageARB
#define glGetnTexImageARB (gl.GetnTexImageARB)
#undef glGetnUniformdvARB
#define glGetnUniformdvARB (gl.GetnUniformdvARB)
#undef glGetnUniformfvARB
#define glGetnUniformfvARB (gl.GetnUniformfvARB)
#undef glGetnUniformivARB
#define glGetnUniformivARB (gl.GetnUniformivARB)
#undef glGetnUniformuivARB
#define glGetnUniformuivARB (gl.GetnUniformuivARB)
#undef glImportSyncEXT
#define glImportSyncEXT (gl.ImportSyncEXT)
#undef glIndexFormatNV
#define glIndexFormatNV (gl.IndexFormatNV)
#undef glIndexxOES
#define glIndexxOES (gl.IndexxOES)
#undef glIndexxvOES
#define glIndexxvOES (gl.IndexxvOES)
#undef glInsertEventMarkerEXT
#define glInsertEventMarkerEXT (gl.InsertEventMarkerEXT)
#undef glInterpolatePathsNV
#define glInterpolatePathsNV (gl.InterpolatePathsNV)
#undef glInvalidateBufferData
#define glInvalidateBufferData (gl.InvalidateBufferData)
#undef glInvalidateBufferSubData
#define glInvalidateBufferSubData (gl.InvalidateBufferSubData)
#undef glInvalidateFramebuffer
#define glInvalidateFramebuffer (gl.InvalidateFramebuffer)
#undef glInvalidateSubFramebuffer
#define glInvalidateSubFramebuffer (gl.InvalidateSubFramebuffer)
#undef glInvalidateTexImage
#define glInvalidateTexImage (gl.InvalidateTexImage)
#undef glInvalidateTexSubImage
#define glInvalidateTexSubImage (gl.InvalidateTexSubImage)
#undef glIsBufferResidentNV
#define glIsBufferResidentNV (gl.IsBufferResidentNV)
#undef glIsEnabledIndexedEXT
#define glIsEnabledIndexedEXT (gl.IsEnabledIndexedEXT)
#undef glIsEnabledi
#define glIsEnabledi (gl.IsEnabledi)
#undef glIsFramebuffer
#define glIsFramebuffer (gl.IsFramebuffer)
#undef glIsFramebufferEXT
#define glIsFramebufferEXT (gl.IsFramebufferEXT)
#undef glIsImageHandleResidentARB
#define glIsImageHandleResidentARB (gl.IsImageHandleResidentARB)
#undef glIsImageHandleResidentNV
#define glIsImageHandleResidentNV (gl.IsImageHandleResidentNV)
#undef glIsNameAMD
#define glIsNameAMD (gl.IsNameAMD)
#undef glIsNamedBufferResidentNV
#define glIsNamedBufferResidentNV (gl.IsNamedBufferResidentNV)
#undef glIsNamedStringARB
#define glIsNamedStringARB (gl.IsNamedStringARB)
#undef glIsPathNV
#define glIsPathNV (gl.IsPathNV)
#undef glIsPointInFillPathNV
#define glIsPointInFillPathNV (gl.IsPointInFillPathNV)
#undef glIsPointInStrokePathNV
#define glIsPointInStrokePathNV (gl.IsPointInStrokePathNV)
#undef glIsProgram
#define glIsProgram (gl.IsProgram)
#undef glIsProgramPipeline
#define glIsProgramPipeline (gl.IsProgramPipeline)
#undef glIsRenderbuffer
#define glIsRenderbuffer (gl.IsRenderbuffer)
#undef glIsRenderbufferEXT
#define glIsRenderbufferEXT (gl.IsRenderbufferEXT)
#undef glIsSampler
#define glIsSampler (gl.IsSampler)
#undef glIsShader
#define glIsShader (gl.IsShader)
#undef glIsSync
#define glIsSync (gl.IsSync)
#undef glIsTextureHandleResidentARB
#define glIsTextureHandleResidentARB (gl.IsTextureHandleResidentARB)
#undef glIsTextureHandleResidentNV
#define glIsTextureHandleResidentNV (gl.IsTextureHandleResidentNV)
#undef glIsTransformFeedback
#define glIsTransformFeedback (gl.IsTransformFeedback)
#undef glIsTransformFeedbackNV
#define glIsTransformFeedbackNV (gl.IsTransformFeedbackNV)
#undef glIsVertexArray
#define glIsVertexArray (gl.IsVertexArray)
#undef glIsVertexAttribEnabledAPPLE
#define glIsVertexAttribEnabledAPPLE (gl.IsVertexAttribEnabledAPPLE)
#undef glLabelObjectEXT
#define glLabelObjectEXT (gl.LabelObjectEXT)
#undef glLightModelxOES
#define glLightModelxOES (gl.LightModelxOES)
#undef glLightModelxvOES
#define glLightModelxvOES (gl.LightModelxvOES)
#undef glLightxOES
#define glLightxOES (gl.LightxOES)
#undef glLightxvOES
#define glLightxvOES (gl.LightxvOES)
#undef glLineWidthxOES
#define glLineWidthxOES (gl.LineWidthxOES)
#undef glLinkProgram
#define glLinkProgram (gl.LinkProgram)
#undef glLoadMatrixxOES
#define glLoadMatrixxOES (gl.LoadMatrixxOES)
#undef glLoadTransposeMatrixxOES
#define glLoadTransposeMatrixxOES (gl.LoadTransposeMatrixxOES)
#undef glMakeBufferNonResidentNV
#define glMakeBufferNonResidentNV (gl.MakeBufferNonResidentNV)
#undef glMakeBufferResidentNV
#define glMakeBufferResidentNV (gl.MakeBufferResidentNV)
#undef glMakeImageHandleNonResidentARB
#define glMakeImageHandleNonResidentARB (gl.MakeImageHandleNonResidentARB)
#undef glMakeImageHandleNonResidentNV
#define glMakeImageHandleNonResidentNV (gl.MakeImageHandleNonResidentNV)
#undef glMakeImageHandleResidentARB
#define glMakeImageHandleResidentARB (gl.MakeImageHandleResidentARB)
#undef glMakeImageHandleResidentNV
#define glMakeImageHandleResidentNV (gl.MakeImageHandleResidentNV)
#undef glMakeNamedBufferNonResidentNV
#define glMakeNamedBufferNonResidentNV (gl.MakeNamedBufferNonResidentNV)
#undef glMakeNamedBufferResidentNV
#define glMakeNamedBufferResidentNV (gl.MakeNamedBufferResidentNV)
#undef glMakeTextureHandleNonResidentARB
#define glMakeTextureHandleNonResidentARB (gl.MakeTextureHandleNonResidentARB)
#undef glMakeTextureHandleNonResidentNV
#define glMakeTextureHandleNonResidentNV (gl.MakeTextureHandleNonResidentNV)
#undef glMakeTextureHandleResidentARB
#define glMakeTextureHandleResidentARB (gl.MakeTextureHandleResidentARB)
#undef glMakeTextureHandleResidentNV
#define glMakeTextureHandleResidentNV (gl.MakeTextureHandleResidentNV)
#undef glMap1xOES
#define glMap1xOES (gl.Map1xOES)
#undef glMap2xOES
#define glMap2xOES (gl.Map2xOES)
#undef glMapBufferRange
#define glMapBufferRange (gl.MapBufferRange)
#undef glMapGrid1xOES
#define glMapGrid1xOES (gl.MapGrid1xOES)
#undef glMapGrid2xOES
#define glMapGrid2xOES (gl.MapGrid2xOES)
#undef glMapNamedBufferEXT
#define glMapNamedBufferEXT (gl.MapNamedBufferEXT)
#undef glMapNamedBufferRangeEXT
#define glMapNamedBufferRangeEXT (gl.MapNamedBufferRangeEXT)
#undef glMapTexture2DINTEL
#define glMapTexture2DINTEL (gl.MapTexture2DINTEL)
#undef glMapVertexAttrib1dAPPLE
#define glMapVertexAttrib1dAPPLE (gl.MapVertexAttrib1dAPPLE)
#undef glMapVertexAttrib1fAPPLE
#define glMapVertexAttrib1fAPPLE (gl.MapVertexAttrib1fAPPLE)
#undef glMapVertexAttrib2dAPPLE
#define glMapVertexAttrib2dAPPLE (gl.MapVertexAttrib2dAPPLE)
#undef glMapVertexAttrib2fAPPLE
#define glMapVertexAttrib2fAPPLE (gl.MapVertexAttrib2fAPPLE)
#undef glMaterialxOES
#define glMaterialxOES (gl.MaterialxOES)
#undef glMaterialxvOES
#define glMaterialxvOES (gl.MaterialxvOES)
#undef glMatrixFrustumEXT
#define glMatrixFrustumEXT (gl.MatrixFrustumEXT)
#undef glMatrixLoadIdentityEXT
#define glMatrixLoadIdentityEXT (gl.MatrixLoadIdentityEXT)
#undef glMatrixLoadTransposedEXT
#define glMatrixLoadTransposedEXT (gl.MatrixLoadTransposedEXT)
#undef glMatrixLoadTransposefEXT
#define glMatrixLoadTransposefEXT (gl.MatrixLoadTransposefEXT)
#undef glMatrixLoaddEXT
#define glMatrixLoaddEXT (gl.MatrixLoaddEXT)
#undef glMatrixLoadfEXT
#define glMatrixLoadfEXT (gl.MatrixLoadfEXT)
#undef glMatrixMultTransposedEXT
#define glMatrixMultTransposedEXT (gl.MatrixMultTransposedEXT)
#undef glMatrixMultTransposefEXT
#define glMatrixMultTransposefEXT (gl.MatrixMultTransposefEXT)
#undef glMatrixMultdEXT
#define glMatrixMultdEXT (gl.MatrixMultdEXT)
#undef glMatrixMultfEXT
#define glMatrixMultfEXT (gl.MatrixMultfEXT)
#undef glMatrixOrthoEXT
#define glMatrixOrthoEXT (gl.MatrixOrthoEXT)
#undef glMatrixPopEXT
#define glMatrixPopEXT (gl.MatrixPopEXT)
#undef glMatrixPushEXT
#define glMatrixPushEXT (gl.MatrixPushEXT)
#undef glMatrixRotatedEXT
#define glMatrixRotatedEXT (gl.MatrixRotatedEXT)
#undef glMatrixRotatefEXT
#define glMatrixRotatefEXT (gl.MatrixRotatefEXT)
#undef glMatrixScaledEXT
#define glMatrixScaledEXT (gl.MatrixScaledEXT)
#undef glMatrixScalefEXT
#define glMatrixScalefEXT (gl.MatrixScalefEXT)
#undef glMatrixTranslatedEXT
#define glMatrixTranslatedEXT (gl.MatrixTranslatedEXT)
#undef glMatrixTranslatefEXT
#define glMatrixTranslatefEXT (gl.MatrixTranslatefEXT)
#undef glMemoryBarrier
#define glMemoryBarrier (gl.MemoryBarrier)
#undef glMemoryBarrierEXT
#define glMemoryBarrierEXT (gl.MemoryBarrierEXT)
#undef glMinSampleShading
#define glMinSampleShading (gl.MinSampleShading)
#undef glMinSampleShadingARB
#define glMinSampleShadingARB (gl.MinSampleShadingARB)
#undef glMultMatrixxOES
#define glMultMatrixxOES (gl.MultMatrixxOES)
#undef glMultTransposeMatrixxOES
#define glMultTransposeMatrixxOES (gl.MultTransposeMatrixxOES)
#undef glMultiDrawArraysIndirect
#define glMultiDrawArraysIndirect (gl.MultiDrawArraysIndirect)
#undef glMultiDrawArraysIndirectAMD
#define glMultiDrawArraysIndirectAMD (gl.MultiDrawArraysIndirectAMD)
#undef glMultiDrawArraysIndirectBindlessNV
#define glMultiDrawArraysIndirectBindlessNV (gl.MultiDrawArraysIndirectBindlessNV)
#undef glMultiDrawArraysIndirectCountARB
#define glMultiDrawArraysIndirectCountARB (gl.MultiDrawArraysIndirectCountARB)
#undef glMultiDrawElementsBaseVertex
#define glMultiDrawElementsBaseVertex (gl.MultiDrawElementsBaseVertex)
#undef glMultiDrawElementsIndirect
#define glMultiDrawElementsIndirect (gl.MultiDrawElementsIndirect)
#undef glMultiDrawElementsIndirectAMD
#define glMultiDrawElementsIndirectAMD (gl.MultiDrawElementsIndirectAMD)
#undef glMultiDrawElementsIndirectBindlessNV
#define glMultiDrawElementsIndirectBindlessNV (gl.MultiDrawElementsIndirectBindlessNV)
#undef glMultiDrawElementsIndirectCountARB
#define glMultiDrawElementsIndirectCountARB (gl.MultiDrawElementsIndirectCountARB)
#undef glMultiTexBufferEXT
#define glMultiTexBufferEXT (gl.MultiTexBufferEXT)
#undef glMultiTexCoord1bOES
#define glMultiTexCoord1bOES (gl.MultiTexCoord1bOES)
#undef glMultiTexCoord1bvOES
#define glMultiTexCoord1bvOES (gl.MultiTexCoord1bvOES)
#undef glMultiTexCoord1xOES
#define glMultiTexCoord1xOES (gl.MultiTexCoord1xOES)
#undef glMultiTexCoord1xvOES
#define glMultiTexCoord1xvOES (gl.MultiTexCoord1xvOES)
#undef glMultiTexCoord2bOES
#define glMultiTexCoord2bOES (gl.MultiTexCoord2bOES)
#undef glMultiTexCoord2bvOES
#define glMultiTexCoord2bvOES (gl.MultiTexCoord2bvOES)
#undef glMultiTexCoord2xOES
#define glMultiTexCoord2xOES (gl.MultiTexCoord2xOES)
#undef glMultiTexCoord2xvOES
#define glMultiTexCoord2xvOES (gl.MultiTexCoord2xvOES)
#undef glMultiTexCoord3bOES
#define glMultiTexCoord3bOES (gl.MultiTexCoord3bOES)
#undef glMultiTexCoord3bvOES
#define glMultiTexCoord3bvOES (gl.MultiTexCoord3bvOES)
#undef glMultiTexCoord3xOES
#define glMultiTexCoord3xOES (gl.MultiTexCoord3xOES)
#undef glMultiTexCoord3xvOES
#define glMultiTexCoord3xvOES (gl.MultiTexCoord3xvOES)
#undef glMultiTexCoord4bOES
#define glMultiTexCoord4bOES (gl.MultiTexCoord4bOES)
#undef glMultiTexCoord4bvOES
#define glMultiTexCoord4bvOES (gl.MultiTexCoord4bvOES)
#undef glMultiTexCoord4xOES
#define glMultiTexCoord4xOES (gl.MultiTexCoord4xOES)
#undef glMultiTexCoord4xvOES
#define glMultiTexCoord4xvOES (gl.MultiTexCoord4xvOES)
#undef glMultiTexCoordP1ui
#define glMultiTexCoordP1ui (gl.MultiTexCoordP1ui)
#undef glMultiTexCoordP1uiv
#define glMultiTexCoordP1uiv (gl.MultiTexCoordP1uiv)
#undef glMultiTexCoordP2ui
#define glMultiTexCoordP2ui (gl.MultiTexCoordP2ui)
#undef glMultiTexCoordP2uiv
#define glMultiTexCoordP2uiv (gl.MultiTexCoordP2uiv)
#undef glMultiTexCoordP3ui
#define glMultiTexCoordP3ui (gl.MultiTexCoordP3ui)
#undef glMultiTexCoordP3uiv
#define glMultiTexCoordP3uiv (gl.MultiTexCoordP3uiv)
#undef glMultiTexCoordP4ui
#define glMultiTexCoordP4ui (gl.MultiTexCoordP4ui)
#undef glMultiTexCoordP4uiv
#define glMultiTexCoordP4uiv (gl.MultiTexCoordP4uiv)
#undef glMultiTexCoordPointerEXT
#define glMultiTexCoordPointerEXT (gl.MultiTexCoordPointerEXT)
#undef glMultiTexEnvfEXT
#define glMultiTexEnvfEXT (gl.MultiTexEnvfEXT)
#undef glMultiTexEnvfvEXT
#define glMultiTexEnvfvEXT (gl.MultiTexEnvfvEXT)
#undef glMultiTexEnviEXT
#define glMultiTexEnviEXT (gl.MultiTexEnviEXT)
#undef glMultiTexEnvivEXT
#define glMultiTexEnvivEXT (gl.MultiTexEnvivEXT)
#undef glMultiTexGendEXT
#define glMultiTexGendEXT (gl.MultiTexGendEXT)
#undef glMultiTexGendvEXT
#define glMultiTexGendvEXT (gl.MultiTexGendvEXT)
#undef glMultiTexGenfEXT
#define glMultiTexGenfEXT (gl.MultiTexGenfEXT)
#undef glMultiTexGenfvEXT
#define glMultiTexGenfvEXT (gl.MultiTexGenfvEXT)
#undef glMultiTexGeniEXT
#define glMultiTexGeniEXT (gl.MultiTexGeniEXT)
#undef glMultiTexGenivEXT
#define glMultiTexGenivEXT (gl.MultiTexGenivEXT)
#undef glMultiTexImage1DEXT
#define glMultiTexImage1DEXT (gl.MultiTexImage1DEXT)
#undef glMultiTexImage2DEXT
#define glMultiTexImage2DEXT (gl.MultiTexImage2DEXT)
#undef glMultiTexImage3DEXT
#define glMultiTexImage3DEXT (gl.MultiTexImage3DEXT)
#undef glMultiTexParameterIivEXT
#define glMultiTexParameterIivEXT (gl.MultiTexParameterIivEXT)
#undef glMultiTexParameterIuivEXT
#define glMultiTexParameterIuivEXT (gl.MultiTexParameterIuivEXT)
#undef glMultiTexParameterfEXT
#define glMultiTexParameterfEXT (gl.MultiTexParameterfEXT)
#undef glMultiTexParameterfvEXT
#define glMultiTexParameterfvEXT (gl.MultiTexParameterfvEXT)
#undef glMultiTexParameteriEXT
#define glMultiTexParameteriEXT (gl.MultiTexParameteriEXT)
#undef glMultiTexParameterivEXT
#define glMultiTexParameterivEXT (gl.MultiTexParameterivEXT)
#undef glMultiTexRenderbufferEXT
#define glMultiTexRenderbufferEXT (gl.MultiTexRenderbufferEXT)
#undef glMultiTexSubImage1DEXT
#define glMultiTexSubImage1DEXT (gl.MultiTexSubImage1DEXT)
#undef glMultiTexSubImage2DEXT
#define glMultiTexSubImage2DEXT (gl.MultiTexSubImage2DEXT)
#undef glMultiTexSubImage3DEXT
#define glMultiTexSubImage3DEXT (gl.MultiTexSubImage3DEXT)
#undef glNamedBufferDataEXT
#define glNamedBufferDataEXT (gl.NamedBufferDataEXT)
#undef glNamedBufferStorageEXT
#define glNamedBufferStorageEXT (gl.NamedBufferStorageEXT)
#undef glNamedBufferSubDataEXT
#define glNamedBufferSubDataEXT (gl.NamedBufferSubDataEXT)
#undef glNamedCopyBufferSubDataEXT
#define glNamedCopyBufferSubDataEXT (gl.NamedCopyBufferSubDataEXT)
#undef glNamedFramebufferParameteriEXT
#define glNamedFramebufferParameteriEXT (gl.NamedFramebufferParameteriEXT)
#undef glNamedFramebufferRenderbufferEXT
#define glNamedFramebufferRenderbufferEXT (gl.NamedFramebufferRenderbufferEXT)
#undef glNamedFramebufferTexture1DEXT
#define glNamedFramebufferTexture1DEXT (gl.NamedFramebufferTexture1DEXT)
#undef glNamedFramebufferTexture2DEXT
#define glNamedFramebufferTexture2DEXT (gl.NamedFramebufferTexture2DEXT)
#undef glNamedFramebufferTexture3DEXT
#define glNamedFramebufferTexture3DEXT (gl.NamedFramebufferTexture3DEXT)
#undef glNamedFramebufferTextureEXT
#define glNamedFramebufferTextureEXT (gl.NamedFramebufferTextureEXT)
#undef glNamedFramebufferTextureFaceEXT
#define glNamedFramebufferTextureFaceEXT (gl.NamedFramebufferTextureFaceEXT)
#undef glNamedFramebufferTextureLayerEXT
#define glNamedFramebufferTextureLayerEXT (gl.NamedFramebufferTextureLayerEXT)
#undef glNamedProgramLocalParameter4dEXT
#define glNamedProgramLocalParameter4dEXT (gl.NamedProgramLocalParameter4dEXT)
#undef glNamedProgramLocalParameter4dvEXT
#define glNamedProgramLocalParameter4dvEXT (gl.NamedProgramLocalParameter4dvEXT)
#undef glNamedProgramLocalParameter4fEXT
#define glNamedProgramLocalParameter4fEXT (gl.NamedProgramLocalParameter4fEXT)
#undef glNamedProgramLocalParameter4fvEXT
#define glNamedProgramLocalParameter4fvEXT (gl.NamedProgramLocalParameter4fvEXT)
#undef glNamedProgramLocalParameterI4iEXT
#define glNamedProgramLocalParameterI4iEXT (gl.NamedProgramLocalParameterI4iEXT)
#undef glNamedProgramLocalParameterI4ivEXT
#define glNamedProgramLocalParameterI4ivEXT (gl.NamedProgramLocalParameterI4ivEXT)
#undef glNamedProgramLocalParameterI4uiEXT
#define glNamedProgramLocalParameterI4uiEXT (gl.NamedProgramLocalParameterI4uiEXT)
#undef glNamedProgramLocalParameterI4uivEXT
#define glNamedProgramLocalParameterI4uivEXT (gl.NamedProgramLocalParameterI4uivEXT)
#undef glNamedProgramLocalParameters4fvEXT
#define glNamedProgramLocalParameters4fvEXT (gl.NamedProgramLocalParameters4fvEXT)
#undef glNamedProgramLocalParametersI4ivEXT
#define glNamedProgramLocalParametersI4ivEXT (gl.NamedProgramLocalParametersI4ivEXT)
#undef glNamedProgramLocalParametersI4uivEXT
#define glNamedProgramLocalParametersI4uivEXT (gl.NamedProgramLocalParametersI4uivEXT)
#undef glNamedProgramStringEXT
#define glNamedProgramStringEXT (gl.NamedProgramStringEXT)
#undef glNamedRenderbufferStorageEXT
#define glNamedRenderbufferStorageEXT (gl.NamedRenderbufferStorageEXT)
#undef glNamedRenderbufferStorageMultisampleCoverageEXT
#define glNamedRenderbufferStorageMultisampleCoverageEXT (gl.NamedRenderbufferStorageMultisampleCoverageEXT)
#undef glNamedRenderbufferStorageMultisampleEXT
#define glNamedRenderbufferStorageMultisampleEXT (gl.NamedRenderbufferStorageMultisampleEXT)
#undef glNamedStringARB
#define glNamedStringARB (gl.NamedStringARB)
#undef glNormal3xOES
#define glNormal3xOES (gl.Normal3xOES)
#undef glNormal3xvOES
#define glNormal3xvOES (gl.Normal3xvOES)
#undef glNormalFormatNV
#define glNormalFormatNV (gl.NormalFormatNV)
#undef glNormalP3ui
#define glNormalP3ui (gl.NormalP3ui)
#undef glNormalP3uiv
#define glNormalP3uiv (gl.NormalP3uiv)
#undef glObjectLabel
#define glObjectLabel (gl.ObjectLabel)
#undef glObjectPtrLabel
#define glObjectPtrLabel (gl.ObjectPtrLabel)
#undef glObjectPurgeableAPPLE
#define glObjectPurgeableAPPLE (gl.ObjectPurgeableAPPLE)
#undef glObjectUnpurgeableAPPLE
#define glObjectUnpurgeableAPPLE (gl.ObjectUnpurgeableAPPLE)
#undef glOrthofOES
#define glOrthofOES (gl.OrthofOES)
#undef glOrthoxOES
#define glOrthoxOES (gl.OrthoxOES)
#undef glPassThroughxOES
#define glPassThroughxOES (gl.PassThroughxOES)
#undef glPatchParameterfv
#define glPatchParameterfv (gl.PatchParameterfv)
#undef glPatchParameteri
#define glPatchParameteri (gl.PatchParameteri)
#undef glPathColorGenNV
#define glPathColorGenNV (gl.PathColorGenNV)
#undef glPathCommandsNV
#define glPathCommandsNV (gl.PathCommandsNV)
#undef glPathCoordsNV
#define glPathCoordsNV (gl.PathCoordsNV)
#undef glPathCoverDepthFuncNV
#define glPathCoverDepthFuncNV (gl.PathCoverDepthFuncNV)
#undef glPathDashArrayNV
#define glPathDashArrayNV (gl.PathDashArrayNV)
#undef glPathFogGenNV
#define glPathFogGenNV (gl.PathFogGenNV)
#undef glPathGlyphRangeNV
#define glPathGlyphRangeNV (gl.PathGlyphRangeNV)
#undef glPathGlyphsNV
#define glPathGlyphsNV (gl.PathGlyphsNV)
#undef glPathParameterfNV
#define glPathParameterfNV (gl.PathParameterfNV)
#undef glPathParameterfvNV
#define glPathParameterfvNV (gl.PathParameterfvNV)
#undef glPathParameteriNV
#define glPathParameteriNV (gl.PathParameteriNV)
#undef glPathParameterivNV
#define glPathParameterivNV (gl.PathParameterivNV)
#undef glPathStencilDepthOffsetNV
#define glPathStencilDepthOffsetNV (gl.PathStencilDepthOffsetNV)
#undef glPathStencilFuncNV
#define glPathStencilFuncNV (gl.PathStencilFuncNV)
#undef glPathStringNV
#define glPathStringNV (gl.PathStringNV)
#undef glPathSubCommandsNV
#define glPathSubCommandsNV (gl.PathSubCommandsNV)
#undef glPathSubCoordsNV
#define glPathSubCoordsNV (gl.PathSubCoordsNV)
#undef glPathTexGenNV
#define glPathTexGenNV (gl.PathTexGenNV)
#undef glPauseTransformFeedback
#define glPauseTransformFeedback (gl.PauseTransformFeedback)
#undef glPauseTransformFeedbackNV
#define glPauseTransformFeedbackNV (gl.PauseTransformFeedbackNV)
#undef glPixelMapx
#define glPixelMapx (gl.PixelMapx)
#undef glPixelStorex
#define glPixelStorex (gl.PixelStorex)
#undef glPixelTransferxOES
#define glPixelTransferxOES (gl.PixelTransferxOES)
#undef glPixelZoomxOES
#define glPixelZoomxOES (gl.PixelZoomxOES)
#undef glPointAlongPathNV
#define glPointAlongPathNV (gl.PointAlongPathNV)
#undef glPointParameterxvOES
#define glPointParameterxvOES (gl.PointParameterxvOES)
#undef glPointSizePointerAPPLE
#define glPointSizePointerAPPLE (gl.PointSizePointerAPPLE)
#undef glPointSizexOES
#define glPointSizexOES (gl.PointSizexOES)
#undef glPolygonOffsetxOES
#define glPolygonOffsetxOES (gl.PolygonOffsetxOES)
#undef glPopDebugGroup
#define glPopDebugGroup (gl.PopDebugGroup)
#undef glPopGroupMarkerEXT
#define glPopGroupMarkerEXT (gl.PopGroupMarkerEXT)
#undef glPresentFrameDualFillNV
#define glPresentFrameDualFillNV (gl.PresentFrameDualFillNV)
#undef glPresentFrameKeyedNV
#define glPresentFrameKeyedNV (gl.PresentFrameKeyedNV)
#undef glPrimitiveRestartIndex
#define glPrimitiveRestartIndex (gl.PrimitiveRestartIndex)
#undef glPrioritizeTexturesxOES
#define glPrioritizeTexturesxOES (gl.PrioritizeTexturesxOES)
#undef glProgramBinary
#define glProgramBinary (gl.ProgramBinary)
#undef glProgramBufferParametersIivNV
#define glProgramBufferParametersIivNV (gl.ProgramBufferParametersIivNV)
#undef glProgramBufferParametersIuivNV
#define glProgramBufferParametersIuivNV (gl.ProgramBufferParametersIuivNV)
#undef glProgramBufferParametersfvNV
#define glProgramBufferParametersfvNV (gl.ProgramBufferParametersfvNV)
#undef glProgramCallbackMESA
#define glProgramCallbackMESA (gl.ProgramCallbackMESA)
#undef glProgramEnvParameterI4iNV
#define glProgramEnvParameterI4iNV (gl.ProgramEnvParameterI4iNV)
#undef glProgramEnvParameterI4ivNV
#define glProgramEnvParameterI4ivNV (gl.ProgramEnvParameterI4ivNV)
#undef glProgramEnvParameterI4uiNV
#define glProgramEnvParameterI4uiNV (gl.ProgramEnvParameterI4uiNV)
#undef glProgramEnvParameterI4uivNV
#define glProgramEnvParameterI4uivNV (gl.ProgramEnvParameterI4uivNV)
#undef glProgramEnvParameters4fvEXT
#define glProgramEnvParameters4fvEXT (gl.ProgramEnvParameters4fvEXT)
#undef glProgramEnvParametersI4ivNV
#define glProgramEnvParametersI4ivNV (gl.ProgramEnvParametersI4ivNV)
#undef glProgramEnvParametersI4uivNV
#define glProgramEnvParametersI4uivNV (gl.ProgramEnvParametersI4uivNV)
#undef glProgramLocalParameterI4iNV
#define glProgramLocalParameterI4iNV (gl.ProgramLocalParameterI4iNV)
#undef glProgramLocalParameterI4ivNV
#define glProgramLocalParameterI4ivNV (gl.ProgramLocalParameterI4ivNV)
#undef glProgramLocalParameterI4uiNV
#define glProgramLocalParameterI4uiNV (gl.ProgramLocalParameterI4uiNV)
#undef glProgramLocalParameterI4uivNV
#define glProgramLocalParameterI4uivNV (gl.ProgramLocalParameterI4uivNV)
#undef glProgramLocalParameters4fvEXT
#define glProgramLocalParameters4fvEXT (gl.ProgramLocalParameters4fvEXT)
#undef glProgramLocalParametersI4ivNV
#define glProgramLocalParametersI4ivNV (gl.ProgramLocalParametersI4ivNV)
#undef glProgramLocalParametersI4uivNV
#define glProgramLocalParametersI4uivNV (gl.ProgramLocalParametersI4uivNV)
#undef glProgramParameteri
#define glProgramParameteri (gl.ProgramParameteri)
#undef glProgramParameteriARB
#define glProgramParameteriARB (gl.ProgramParameteriARB)
#undef glProgramParameteriEXT
#define glProgramParameteriEXT (gl.ProgramParameteriEXT)
#undef glProgramSubroutineParametersuivNV
#define glProgramSubroutineParametersuivNV (gl.ProgramSubroutineParametersuivNV)
#undef glProgramUniform1d
#define glProgramUniform1d (gl.ProgramUniform1d)
#undef glProgramUniform1dEXT
#define glProgramUniform1dEXT (gl.ProgramUniform1dEXT)
#undef glProgramUniform1dv
#define glProgramUniform1dv (gl.ProgramUniform1dv)
#undef glProgramUniform1dvEXT
#define glProgramUniform1dvEXT (gl.ProgramUniform1dvEXT)
#undef glProgramUniform1f
#define glProgramUniform1f (gl.ProgramUniform1f)
#undef glProgramUniform1fEXT
#define glProgramUniform1fEXT (gl.ProgramUniform1fEXT)
#undef glProgramUniform1fv
#define glProgramUniform1fv (gl.ProgramUniform1fv)
#undef glProgramUniform1fvEXT
#define glProgramUniform1fvEXT (gl.ProgramUniform1fvEXT)
#undef glProgramUniform1i
#define glProgramUniform1i (gl.ProgramUniform1i)
#undef glProgramUniform1i64NV
#define glProgramUniform1i64NV (gl.ProgramUniform1i64NV)
#undef glProgramUniform1i64vNV
#define glProgramUniform1i64vNV (gl.ProgramUniform1i64vNV)
#undef glProgramUniform1iEXT
#define glProgramUniform1iEXT (gl.ProgramUniform1iEXT)
#undef glProgramUniform1iv
#define glProgramUniform1iv (gl.ProgramUniform1iv)
#undef glProgramUniform1ivEXT
#define glProgramUniform1ivEXT (gl.ProgramUniform1ivEXT)
#undef glProgramUniform1ui
#define glProgramUniform1ui (gl.ProgramUniform1ui)
#undef glProgramUniform1ui64NV
#define glProgramUniform1ui64NV (gl.ProgramUniform1ui64NV)
#undef glProgramUniform1ui64vNV
#define glProgramUniform1ui64vNV (gl.ProgramUniform1ui64vNV)
#undef glProgramUniform1uiEXT
#define glProgramUniform1uiEXT (gl.ProgramUniform1uiEXT)
#undef glProgramUniform1uiv
#define glProgramUniform1uiv (gl.ProgramUniform1uiv)
#undef glProgramUniform1uivEXT
#define glProgramUniform1uivEXT (gl.ProgramUniform1uivEXT)
#undef glProgramUniform2d
#define glProgramUniform2d (gl.ProgramUniform2d)
#undef glProgramUniform2dEXT
#define glProgramUniform2dEXT (gl.ProgramUniform2dEXT)
#undef glProgramUniform2dv
#define glProgramUniform2dv (gl.ProgramUniform2dv)
#undef glProgramUniform2dvEXT
#define glProgramUniform2dvEXT (gl.ProgramUniform2dvEXT)
#undef glProgramUniform2f
#define glProgramUniform2f (gl.ProgramUniform2f)
#undef glProgramUniform2fEXT
#define glProgramUniform2fEXT (gl.ProgramUniform2fEXT)
#undef glProgramUniform2fv
#define glProgramUniform2fv (gl.ProgramUniform2fv)
#undef glProgramUniform2fvEXT
#define glProgramUniform2fvEXT (gl.ProgramUniform2fvEXT)
#undef glProgramUniform2i
#define glProgramUniform2i (gl.ProgramUniform2i)
#undef glProgramUniform2i64NV
#define glProgramUniform2i64NV (gl.ProgramUniform2i64NV)
#undef glProgramUniform2i64vNV
#define glProgramUniform2i64vNV (gl.ProgramUniform2i64vNV)
#undef glProgramUniform2iEXT
#define glProgramUniform2iEXT (gl.ProgramUniform2iEXT)
#undef glProgramUniform2iv
#define glProgramUniform2iv (gl.ProgramUniform2iv)
#undef glProgramUniform2ivEXT
#define glProgramUniform2ivEXT (gl.ProgramUniform2ivEXT)
#undef glProgramUniform2ui
#define glProgramUniform2ui (gl.ProgramUniform2ui)
#undef glProgramUniform2ui64NV
#define glProgramUniform2ui64NV (gl.ProgramUniform2ui64NV)
#undef glProgramUniform2ui64vNV
#define glProgramUniform2ui64vNV (gl.ProgramUniform2ui64vNV)
#undef glProgramUniform2uiEXT
#define glProgramUniform2uiEXT (gl.ProgramUniform2uiEXT)
#undef glProgramUniform2uiv
#define glProgramUniform2uiv (gl.ProgramUniform2uiv)
#undef glProgramUniform2uivEXT
#define glProgramUniform2uivEXT (gl.ProgramUniform2uivEXT)
#undef glProgramUniform3d
#define glProgramUniform3d (gl.ProgramUniform3d)
#undef glProgramUniform3dEXT
#define glProgramUniform3dEXT (gl.ProgramUniform3dEXT)
#undef glProgramUniform3dv
#define glProgramUniform3dv (gl.ProgramUniform3dv)
#undef glProgramUniform3dvEXT
#define glProgramUniform3dvEXT (gl.ProgramUniform3dvEXT)
#undef glProgramUniform3f
#define glProgramUniform3f (gl.ProgramUniform3f)
#undef glProgramUniform3fEXT
#define glProgramUniform3fEXT (gl.ProgramUniform3fEXT)
#undef glProgramUniform3fv
#define glProgramUniform3fv (gl.ProgramUniform3fv)
#undef glProgramUniform3fvEXT
#define glProgramUniform3fvEXT (gl.ProgramUniform3fvEXT)
#undef glProgramUniform3i
#define glProgramUniform3i (gl.ProgramUniform3i)
#undef glProgramUniform3i64NV
#define glProgramUniform3i64NV (gl.ProgramUniform3i64NV)
#undef glProgramUniform3i64vNV
#define glProgramUniform3i64vNV (gl.ProgramUniform3i64vNV)
#undef glProgramUniform3iEXT
#define glProgramUniform3iEXT (gl.ProgramUniform3iEXT)
#undef glProgramUniform3iv
#define glProgramUniform3iv (gl.ProgramUniform3iv)
#undef glProgramUniform3ivEXT
#define glProgramUniform3ivEXT (gl.ProgramUniform3ivEXT)
#undef glProgramUniform3ui
#define glProgramUniform3ui (gl.ProgramUniform3ui)
#undef glProgramUniform3ui64NV
#define glProgramUniform3ui64NV (gl.ProgramUniform3ui64NV)
#undef glProgramUniform3ui64vNV
#define glProgramUniform3ui64vNV (gl.ProgramUniform3ui64vNV)
#undef glProgramUniform3uiEXT
#define glProgramUniform3uiEXT (gl.ProgramUniform3uiEXT)
#undef glProgramUniform3uiv
#define glProgramUniform3uiv (gl.ProgramUniform3uiv)
#undef glProgramUniform3uivEXT
#define glProgramUniform3uivEXT (gl.ProgramUniform3uivEXT)
#undef glProgramUniform4d
#define glProgramUniform4d (gl.ProgramUniform4d)
#undef glProgramUniform4dEXT
#define glProgramUniform4dEXT (gl.ProgramUniform4dEXT)
#undef glProgramUniform4dv
#define glProgramUniform4dv (gl.ProgramUniform4dv)
#undef glProgramUniform4dvEXT
#define glProgramUniform4dvEXT (gl.ProgramUniform4dvEXT)
#undef glProgramUniform4f
#define glProgramUniform4f (gl.ProgramUniform4f)
#undef glProgramUniform4fEXT
#define glProgramUniform4fEXT (gl.ProgramUniform4fEXT)
#undef glProgramUniform4fv
#define glProgramUniform4fv (gl.ProgramUniform4fv)
#undef glProgramUniform4fvEXT
#define glProgramUniform4fvEXT (gl.ProgramUniform4fvEXT)
#undef glProgramUniform4i
#define glProgramUniform4i (gl.ProgramUniform4i)
#undef glProgramUniform4i64NV
#define glProgramUniform4i64NV (gl.ProgramUniform4i64NV)
#undef glProgramUniform4i64vNV
#define glProgramUniform4i64vNV (gl.ProgramUniform4i64vNV)
#undef glProgramUniform4iEXT
#define glProgramUniform4iEXT (gl.ProgramUniform4iEXT)
#undef glProgramUniform4iv
#define glProgramUniform4iv (gl.ProgramUniform4iv)
#undef glProgramUniform4ivEXT
#define glProgramUniform4ivEXT (gl.ProgramUniform4ivEXT)
#undef glProgramUniform4ui
#define glProgramUniform4ui (gl.ProgramUniform4ui)
#undef glProgramUniform4ui64NV
#define glProgramUniform4ui64NV (gl.ProgramUniform4ui64NV)
#undef glProgramUniform4ui64vNV
#define glProgramUniform4ui64vNV (gl.ProgramUniform4ui64vNV)
#undef glProgramUniform4uiEXT
#define glProgramUniform4uiEXT (gl.ProgramUniform4uiEXT)
#undef glProgramUniform4uiv
#define glProgramUniform4uiv (gl.ProgramUniform4uiv)
#undef glProgramUniform4uivEXT
#define glProgramUniform4uivEXT (gl.ProgramUniform4uivEXT)
#undef glProgramUniformHandleui64ARB
#define glProgramUniformHandleui64ARB (gl.ProgramUniformHandleui64ARB)
#undef glProgramUniformHandleui64NV
#define glProgramUniformHandleui64NV (gl.ProgramUniformHandleui64NV)
#undef glProgramUniformHandleui64vARB
#define glProgramUniformHandleui64vARB (gl.ProgramUniformHandleui64vARB)
#undef glProgramUniformHandleui64vNV
#define glProgramUniformHandleui64vNV (gl.ProgramUniformHandleui64vNV)
#undef glProgramUniformMatrix2dv
#define glProgramUniformMatrix2dv (gl.ProgramUniformMatrix2dv)
#undef glProgramUniformMatrix2dvEXT
#define glProgramUniformMatrix2dvEXT (gl.ProgramUniformMatrix2dvEXT)
#undef glProgramUniformMatrix2fv
#define glProgramUniformMatrix2fv (gl.ProgramUniformMatrix2fv)
#undef glProgramUniformMatrix2fvEXT
#define glProgramUniformMatrix2fvEXT (gl.ProgramUniformMatrix2fvEXT)
#undef glProgramUniformMatrix2x3dv
#define glProgramUniformMatrix2x3dv (gl.ProgramUniformMatrix2x3dv)
#undef glProgramUniformMatrix2x3dvEXT
#define glProgramUniformMatrix2x3dvEXT (gl.ProgramUniformMatrix2x3dvEXT)
#undef glProgramUniformMatrix2x3fv
#define glProgramUniformMatrix2x3fv (gl.ProgramUniformMatrix2x3fv)
#undef glProgramUniformMatrix2x3fvEXT
#define glProgramUniformMatrix2x3fvEXT (gl.ProgramUniformMatrix2x3fvEXT)
#undef glProgramUniformMatrix2x4dv
#define glProgramUniformMatrix2x4dv (gl.ProgramUniformMatrix2x4dv)
#undef glProgramUniformMatrix2x4dvEXT
#define glProgramUniformMatrix2x4dvEXT (gl.ProgramUniformMatrix2x4dvEXT)
#undef glProgramUniformMatrix2x4fv
#define glProgramUniformMatrix2x4fv (gl.ProgramUniformMatrix2x4fv)
#undef glProgramUniformMatrix2x4fvEXT
#define glProgramUniformMatrix2x4fvEXT (gl.ProgramUniformMatrix2x4fvEXT)
#undef glProgramUniformMatrix3dv
#define glProgramUniformMatrix3dv (gl.ProgramUniformMatrix3dv)
#undef glProgramUniformMatrix3dvEXT
#define glProgramUniformMatrix3dvEXT (gl.ProgramUniformMatrix3dvEXT)
#undef glProgramUniformMatrix3fv
#define glProgramUniformMatrix3fv (gl.ProgramUniformMatrix3fv)
#undef glProgramUniformMatrix3fvEXT
#define glProgramUniformMatrix3fvEXT (gl.ProgramUniformMatrix3fvEXT)
#undef glProgramUniformMatrix3x2dv
#define glProgramUniformMatrix3x2dv (gl.ProgramUniformMatrix3x2dv)
#undef glProgramUniformMatrix3x2dvEXT
#define glProgramUniformMatrix3x2dvEXT (gl.ProgramUniformMatrix3x2dvEXT)
#undef glProgramUniformMatrix3x2fv
#define glProgramUniformMatrix3x2fv (gl.ProgramUniformMatrix3x2fv)
#undef glProgramUniformMatrix3x2fvEXT
#define glProgramUniformMatrix3x2fvEXT (gl.ProgramUniformMatrix3x2fvEXT)
#undef glProgramUniformMatrix3x4dv
#define glProgramUniformMatrix3x4dv (gl.ProgramUniformMatrix3x4dv)
#undef glProgramUniformMatrix3x4dvEXT
#define glProgramUniformMatrix3x4dvEXT (gl.ProgramUniformMatrix3x4dvEXT)
#undef glProgramUniformMatrix3x4fv
#define glProgramUniformMatrix3x4fv (gl.ProgramUniformMatrix3x4fv)
#undef glProgramUniformMatrix3x4fvEXT
#define glProgramUniformMatrix3x4fvEXT (gl.ProgramUniformMatrix3x4fvEXT)
#undef glProgramUniformMatrix4dv
#define glProgramUniformMatrix4dv (gl.ProgramUniformMatrix4dv)
#undef glProgramUniformMatrix4dvEXT
#define glProgramUniformMatrix4dvEXT (gl.ProgramUniformMatrix4dvEXT)
#undef glProgramUniformMatrix4fv
#define glProgramUniformMatrix4fv (gl.ProgramUniformMatrix4fv)
#undef glProgramUniformMatrix4fvEXT
#define glProgramUniformMatrix4fvEXT (gl.ProgramUniformMatrix4fvEXT)
#undef glProgramUniformMatrix4x2dv
#define glProgramUniformMatrix4x2dv (gl.ProgramUniformMatrix4x2dv)
#undef glProgramUniformMatrix4x2dvEXT
#define glProgramUniformMatrix4x2dvEXT (gl.ProgramUniformMatrix4x2dvEXT)
#undef glProgramUniformMatrix4x2fv
#define glProgramUniformMatrix4x2fv (gl.ProgramUniformMatrix4x2fv)
#undef glProgramUniformMatrix4x2fvEXT
#define glProgramUniformMatrix4x2fvEXT (gl.ProgramUniformMatrix4x2fvEXT)
#undef glProgramUniformMatrix4x3dv
#define glProgramUniformMatrix4x3dv (gl.ProgramUniformMatrix4x3dv)
#undef glProgramUniformMatrix4x3dvEXT
#define glProgramUniformMatrix4x3dvEXT (gl.ProgramUniformMatrix4x3dvEXT)
#undef glProgramUniformMatrix4x3fv
#define glProgramUniformMatrix4x3fv (gl.ProgramUniformMatrix4x3fv)
#undef glProgramUniformMatrix4x3fvEXT
#define glProgramUniformMatrix4x3fvEXT (gl.ProgramUniformMatrix4x3fvEXT)
#undef glProgramUniformui64NV
#define glProgramUniformui64NV (gl.ProgramUniformui64NV)
#undef glProgramUniformui64vNV
#define glProgramUniformui64vNV (gl.ProgramUniformui64vNV)
#undef glProgramVertexLimitNV
#define glProgramVertexLimitNV (gl.ProgramVertexLimitNV)
#undef glProvokingVertex
#define glProvokingVertex (gl.ProvokingVertex)
#undef glProvokingVertexEXT
#define glProvokingVertexEXT (gl.ProvokingVertexEXT)
#undef glPushClientAttribDefaultEXT
#define glPushClientAttribDefaultEXT (gl.PushClientAttribDefaultEXT)
#undef glPushDebugGroup
#define glPushDebugGroup (gl.PushDebugGroup)
#undef glPushGroupMarkerEXT
#define glPushGroupMarkerEXT (gl.PushGroupMarkerEXT)
#undef glQueryCounter
#define glQueryCounter (gl.QueryCounter)
#undef glQueryMatrixxOES
#define glQueryMatrixxOES (gl.QueryMatrixxOES)
#undef glQueryObjectParameteruiAMD
#define glQueryObjectParameteruiAMD (gl.QueryObjectParameteruiAMD)
#undef glRasterPos2xOES
#define glRasterPos2xOES (gl.RasterPos2xOES)
#undef glRasterPos2xvOES
#define glRasterPos2xvOES (gl.RasterPos2xvOES)
#undef glRasterPos3xOES
#define glRasterPos3xOES (gl.RasterPos3xOES)
#undef glRasterPos3xvOES
#define glRasterPos3xvOES (gl.RasterPos3xvOES)
#undef glRasterPos4xOES
#define glRasterPos4xOES (gl.RasterPos4xOES)
#undef glRasterPos4xvOES
#define glRasterPos4xvOES (gl.RasterPos4xvOES)
#undef glReadnPixelsARB
#define glReadnPixelsARB (gl.ReadnPixelsARB)
#undef glRectxOES
#define glRectxOES (gl.RectxOES)
#undef glRectxvOES
#define glRectxvOES (gl.RectxvOES)
#undef glReleaseShaderCompiler
#define glReleaseShaderCompiler (gl.ReleaseShaderCompiler)
#undef glRenderbufferStorage
#define glRenderbufferStorage (gl.RenderbufferStorage)
#undef glRenderbufferStorageEXT
#define glRenderbufferStorageEXT (gl.RenderbufferStorageEXT)
#undef glRenderbufferStorageMultisample
#define glRenderbufferStorageMultisample (gl.RenderbufferStorageMultisample)
#undef glRenderbufferStorageMultisampleCoverageNV
#define glRenderbufferStorageMultisampleCoverageNV (gl.RenderbufferStorageMultisampleCoverageNV)
#undef glRenderbufferStorageMultisampleEXT
#define glRenderbufferStorageMultisampleEXT (gl.RenderbufferStorageMultisampleEXT)
#undef glResumeTransformFeedback
#define glResumeTransformFeedback (gl.ResumeTransformFeedback)
#undef glResumeTransformFeedbackNV
#define glResumeTransformFeedbackNV (gl.ResumeTransformFeedbackNV)
#undef glRotatexOES
#define glRotatexOES (gl.RotatexOES)
#undef glSampleCoverageOES
#define glSampleCoverageOES (gl.SampleCoverageOES)
#undef glSampleMaskIndexedNV
#define glSampleMaskIndexedNV (gl.SampleMaskIndexedNV)
#undef glSampleMaski
#define glSampleMaski (gl.SampleMaski)
#undef glSamplePass
#define glSamplePass (gl.SamplePass)
#undef glSamplePassARB
#define glSamplePassARB (gl.SamplePassARB)
#undef glSamplerParameterIiv
#define glSamplerParameterIiv (gl.SamplerParameterIiv)
#undef glSamplerParameterIuiv
#define glSamplerParameterIuiv (gl.SamplerParameterIuiv)
#undef glSamplerParameterf
#define glSamplerParameterf (gl.SamplerParameterf)
#undef glSamplerParameterfv
#define glSamplerParameterfv (gl.SamplerParameterfv)
#undef glSamplerParameteri
#define glSamplerParameteri (gl.SamplerParameteri)
#undef glSamplerParameteriv
#define glSamplerParameteriv (gl.SamplerParameteriv)
#undef glScalexOES
#define glScalexOES (gl.ScalexOES)
#undef glScissorArrayv
#define glScissorArrayv (gl.ScissorArrayv)
#undef glScissorIndexed
#define glScissorIndexed (gl.ScissorIndexed)
#undef glScissorIndexedv
#define glScissorIndexedv (gl.ScissorIndexedv)
#undef glSecondaryColorFormatNV
#define glSecondaryColorFormatNV (gl.SecondaryColorFormatNV)
#undef glSecondaryColorP3ui
#define glSecondaryColorP3ui (gl.SecondaryColorP3ui)
#undef glSecondaryColorP3uiv
#define glSecondaryColorP3uiv (gl.SecondaryColorP3uiv)
#undef glSelectPerfMonitorCountersAMD
#define glSelectPerfMonitorCountersAMD (gl.SelectPerfMonitorCountersAMD)
#undef glSetMultisamplefvAMD
#define glSetMultisamplefvAMD (gl.SetMultisamplefvAMD)
#undef glShaderBinary
#define glShaderBinary (gl.ShaderBinary)
#undef glShaderSource
#define glShaderSource (gl.ShaderSource)
#undef glShaderStorageBlockBinding
#define glShaderStorageBlockBinding (gl.ShaderStorageBlockBinding)
#undef glStencilClearTagEXT
#define glStencilClearTagEXT (gl.StencilClearTagEXT)
#undef glStencilFillPathInstancedNV
#define glStencilFillPathInstancedNV (gl.StencilFillPathInstancedNV)
#undef glStencilFillPathNV
#define glStencilFillPathNV (gl.StencilFillPathNV)
#undef glStencilFuncSeparate
#define glStencilFuncSeparate (gl.StencilFuncSeparate)
#undef glStencilMaskSeparate
#define glStencilMaskSeparate (gl.StencilMaskSeparate)
#undef glStencilOpSeparate
#define glStencilOpSeparate (gl.StencilOpSeparate)
#undef glStencilOpValueAMD
#define glStencilOpValueAMD (gl.StencilOpValueAMD)
#undef glStencilStrokePathInstancedNV
#define glStencilStrokePathInstancedNV (gl.StencilStrokePathInstancedNV)
#undef glStencilStrokePathNV
#define glStencilStrokePathNV (gl.StencilStrokePathNV)
#undef glStringMarkerGREMEDY
#define glStringMarkerGREMEDY (gl.StringMarkerGREMEDY)
#undef glSwapAPPLE
#define glSwapAPPLE (gl.SwapAPPLE)
#undef glSyncTextureINTEL
#define glSyncTextureINTEL (gl.SyncTextureINTEL)
#undef glTessellationFactorAMD
#define glTessellationFactorAMD (gl.TessellationFactorAMD)
#undef glTessellationModeAMD
#define glTessellationModeAMD (gl.TessellationModeAMD)
#undef glTexBuffer
#define glTexBuffer (gl.TexBuffer)
#undef glTexBufferARB
#define glTexBufferARB (gl.TexBufferARB)
#undef glTexBufferEXT
#define glTexBufferEXT (gl.TexBufferEXT)
#undef glTexBufferRange
#define glTexBufferRange (gl.TexBufferRange)
#undef glTexCoord1bOES
#define glTexCoord1bOES (gl.TexCoord1bOES)
#undef glTexCoord1bvOES
#define glTexCoord1bvOES (gl.TexCoord1bvOES)
#undef glTexCoord1xOES
#define glTexCoord1xOES (gl.TexCoord1xOES)
#undef glTexCoord1xvOES
#define glTexCoord1xvOES (gl.TexCoord1xvOES)
#undef glTexCoord2bOES
#define glTexCoord2bOES (gl.TexCoord2bOES)
#undef glTexCoord2bvOES
#define glTexCoord2bvOES (gl.TexCoord2bvOES)
#undef glTexCoord2xOES
#define glTexCoord2xOES (gl.TexCoord2xOES)
#undef glTexCoord2xvOES
#define glTexCoord2xvOES (gl.TexCoord2xvOES)
#undef glTexCoord3bOES
#define glTexCoord3bOES (gl.TexCoord3bOES)
#undef glTexCoord3bvOES
#define glTexCoord3bvOES (gl.TexCoord3bvOES)
#undef glTexCoord3xOES
#define glTexCoord3xOES (gl.TexCoord3xOES)
#undef glTexCoord3xvOES
#define glTexCoord3xvOES (gl.TexCoord3xvOES)
#undef glTexCoord4bOES
#define glTexCoord4bOES (gl.TexCoord4bOES)
#undef glTexCoord4bvOES
#define glTexCoord4bvOES (gl.TexCoord4bvOES)
#undef glTexCoord4xOES
#define glTexCoord4xOES (gl.TexCoord4xOES)
#undef glTexCoord4xvOES
#define glTexCoord4xvOES (gl.TexCoord4xvOES)
#undef glTexCoordFormatNV
#define glTexCoordFormatNV (gl.TexCoordFormatNV)
#undef glTexCoordP1ui
#define glTexCoordP1ui (gl.TexCoordP1ui)
#undef glTexCoordP1uiv
#define glTexCoordP1uiv (gl.TexCoordP1uiv)
#undef glTexCoordP2ui
#define glTexCoordP2ui (gl.TexCoordP2ui)
#undef glTexCoordP2uiv
#define glTexCoordP2uiv (gl.TexCoordP2uiv)
#undef glTexCoordP3ui
#define glTexCoordP3ui (gl.TexCoordP3ui)
#undef glTexCoordP3uiv
#define glTexCoordP3uiv (gl.TexCoordP3uiv)
#undef glTexCoordP4ui
#define glTexCoordP4ui (gl.TexCoordP4ui)
#undef glTexCoordP4uiv
#define glTexCoordP4uiv (gl.TexCoordP4uiv)
#undef glTexEnvxOES
#define glTexEnvxOES (gl.TexEnvxOES)
#undef glTexEnvxvOES
#define glTexEnvxvOES (gl.TexEnvxvOES)
#undef glTexGenxOES
#define glTexGenxOES (gl.TexGenxOES)
#undef glTexGenxvOES
#define glTexGenxvOES (gl.TexGenxvOES)
#undef glTexImage2DMultisample
#define glTexImage2DMultisample (gl.TexImage2DMultisample)
#undef glTexImage2DMultisampleCoverageNV
#define glTexImage2DMultisampleCoverageNV (gl.TexImage2DMultisampleCoverageNV)
#undef glTexImage3DMultisample
#define glTexImage3DMultisample (gl.TexImage3DMultisample)
#undef glTexImage3DMultisampleCoverageNV
#define glTexImage3DMultisampleCoverageNV (gl.TexImage3DMultisampleCoverageNV)
#undef glTexPageCommitmentARB
#define glTexPageCommitmentARB (gl.TexPageCommitmentARB)
#undef glTexParameterIiv
#define glTexParameterIiv (gl.TexParameterIiv)
#undef glTexParameterIivEXT
#define glTexParameterIivEXT (gl.TexParameterIivEXT)
#undef glTexParameterIuiv
#define glTexParameterIuiv (gl.TexParameterIuiv)
#undef glTexParameterIuivEXT
#define glTexParameterIuivEXT (gl.TexParameterIuivEXT)
#undef glTexParameterxOES
#define glTexParameterxOES (gl.TexParameterxOES)
#undef glTexParameterxvOES
#define glTexParameterxvOES (gl.TexParameterxvOES)
#undef glTexRenderbufferNV
#define glTexRenderbufferNV (gl.TexRenderbufferNV)
#undef glTexScissorFuncINTEL
#define glTexScissorFuncINTEL (gl.TexScissorFuncINTEL)
#undef glTexScissorINTEL
#define glTexScissorINTEL (gl.TexScissorINTEL)
#undef glTexStorage1D
#define glTexStorage1D (gl.TexStorage1D)
#undef glTexStorage2D
#define glTexStorage2D (gl.TexStorage2D)
#undef glTexStorage2DMultisample
#define glTexStorage2DMultisample (gl.TexStorage2DMultisample)
#undef glTexStorage3D
#define glTexStorage3D (gl.TexStorage3D)
#undef glTexStorage3DMultisample
#define glTexStorage3DMultisample (gl.TexStorage3DMultisample)
#undef glTexStorageSparseAMD
#define glTexStorageSparseAMD (gl.TexStorageSparseAMD)
#undef glTextureBarrierNV
#define glTextureBarrierNV (gl.TextureBarrierNV)
#undef glTextureBufferEXT
#define glTextureBufferEXT (gl.TextureBufferEXT)
#undef glTextureBufferRangeEXT
#define glTextureBufferRangeEXT (gl.TextureBufferRangeEXT)
#undef glTextureFogSGIX
#define glTextureFogSGIX (gl.TextureFogSGIX)
#undef glTextureImage1DEXT
#define glTextureImage1DEXT (gl.TextureImage1DEXT)
#undef glTextureImage2DEXT
#define glTextureImage2DEXT (gl.TextureImage2DEXT)
#undef glTextureImage2DMultisampleCoverageNV
#define glTextureImage2DMultisampleCoverageNV (gl.TextureImage2DMultisampleCoverageNV)
#undef glTextureImage2DMultisampleNV
#define glTextureImage2DMultisampleNV (gl.TextureImage2DMultisampleNV)
#undef glTextureImage3DEXT
#define glTextureImage3DEXT (gl.TextureImage3DEXT)
#undef glTextureImage3DMultisampleCoverageNV
#define glTextureImage3DMultisampleCoverageNV (gl.TextureImage3DMultisampleCoverageNV)
#undef glTextureImage3DMultisampleNV
#define glTextureImage3DMultisampleNV (gl.TextureImage3DMultisampleNV)
#undef glTexturePageCommitmentEXT
#define glTexturePageCommitmentEXT (gl.TexturePageCommitmentEXT)
#undef glTextureParameterIivEXT
#define glTextureParameterIivEXT (gl.TextureParameterIivEXT)
#undef glTextureParameterIuivEXT
#define glTextureParameterIuivEXT (gl.TextureParameterIuivEXT)
#undef glTextureParameterfEXT
#define glTextureParameterfEXT (gl.TextureParameterfEXT)
#undef glTextureParameterfvEXT
#define glTextureParameterfvEXT (gl.TextureParameterfvEXT)
#undef glTextureParameteriEXT
#define glTextureParameteriEXT (gl.TextureParameteriEXT)
#undef glTextureParameterivEXT
#define glTextureParameterivEXT (gl.TextureParameterivEXT)
#undef glTextureRangeAPPLE
#define glTextureRangeAPPLE (gl.TextureRangeAPPLE)
#undef glTextureRenderbufferEXT
#define glTextureRenderbufferEXT (gl.TextureRenderbufferEXT)
#undef glTextureStorage1DEXT
#define glTextureStorage1DEXT (gl.TextureStorage1DEXT)
#undef glTextureStorage2DEXT
#define glTextureStorage2DEXT (gl.TextureStorage2DEXT)
#undef glTextureStorage2DMultisampleEXT
#define glTextureStorage2DMultisampleEXT (gl.TextureStorage2DMultisampleEXT)
#undef glTextureStorage3DEXT
#define glTextureStorage3DEXT (gl.TextureStorage3DEXT)
#undef glTextureStorage3DMultisampleEXT
#define glTextureStorage3DMultisampleEXT (gl.TextureStorage3DMultisampleEXT)
#undef glTextureStorageSparseAMD
#define glTextureStorageSparseAMD (gl.TextureStorageSparseAMD)
#undef glTextureSubImage1DEXT
#define glTextureSubImage1DEXT (gl.TextureSubImage1DEXT)
#undef glTextureSubImage2DEXT
#define glTextureSubImage2DEXT (gl.TextureSubImage2DEXT)
#undef glTextureSubImage3DEXT
#define glTextureSubImage3DEXT (gl.TextureSubImage3DEXT)
#undef glTextureView
#define glTextureView (gl.TextureView)
#undef glTransformFeedbackAttribsNV
#define glTransformFeedbackAttribsNV (gl.TransformFeedbackAttribsNV)
#undef glTransformFeedbackStreamAttribsNV
#define glTransformFeedbackStreamAttribsNV (gl.TransformFeedbackStreamAttribsNV)
#undef glTransformFeedbackVaryings
#define glTransformFeedbackVaryings (gl.TransformFeedbackVaryings)
#undef glTransformFeedbackVaryingsEXT
#define glTransformFeedbackVaryingsEXT (gl.TransformFeedbackVaryingsEXT)
#undef glTransformFeedbackVaryingsNV
#define glTransformFeedbackVaryingsNV (gl.TransformFeedbackVaryingsNV)
#undef glTransformPathNV
#define glTransformPathNV (gl.TransformPathNV)
#undef glTranslatexOES
#define glTranslatexOES (gl.TranslatexOES)
#undef glUniform1d
#define glUniform1d (gl.Uniform1d)
#undef glUniform1dv
#define glUniform1dv (gl.Uniform1dv)
#undef glUniform1f
#define glUniform1f (gl.Uniform1f)
#undef glUniform1fv
#define glUniform1fv (gl.Uniform1fv)
#undef glUniform1i
#define glUniform1i (gl.Uniform1i)
#undef glUniform1i64NV
#define glUniform1i64NV (gl.Uniform1i64NV)
#undef glUniform1i64vNV
#define glUniform1i64vNV (gl.Uniform1i64vNV)
#undef glUniform1iv
#define glUniform1iv (gl.Uniform1iv)
#undef glUniform1ui
#define glUniform1ui (gl.Uniform1ui)
#undef glUniform1ui64NV
#define glUniform1ui64NV (gl.Uniform1ui64NV)
#undef glUniform1ui64vNV
#define glUniform1ui64vNV (gl.Uniform1ui64vNV)
#undef glUniform1uiEXT
#define glUniform1uiEXT (gl.Uniform1uiEXT)
#undef glUniform1uiv
#define glUniform1uiv (gl.Uniform1uiv)
#undef glUniform1uivEXT
#define glUniform1uivEXT (gl.Uniform1uivEXT)
#undef glUniform2d
#define glUniform2d (gl.Uniform2d)
#undef glUniform2dv
#define glUniform2dv (gl.Uniform2dv)
#undef glUniform2f
#define glUniform2f (gl.Uniform2f)
#undef glUniform2fv
#define glUniform2fv (gl.Uniform2fv)
#undef glUniform2i
#define glUniform2i (gl.Uniform2i)
#undef glUniform2i64NV
#define glUniform2i64NV (gl.Uniform2i64NV)
#undef glUniform2i64vNV
#define glUniform2i64vNV (gl.Uniform2i64vNV)
#undef glUniform2iv
#define glUniform2iv (gl.Uniform2iv)
#undef glUniform2ui
#define glUniform2ui (gl.Uniform2ui)
#undef glUniform2ui64NV
#define glUniform2ui64NV (gl.Uniform2ui64NV)
#undef glUniform2ui64vNV
#define glUniform2ui64vNV (gl.Uniform2ui64vNV)
#undef glUniform2uiEXT
#define glUniform2uiEXT (gl.Uniform2uiEXT)
#undef glUniform2uiv
#define glUniform2uiv (gl.Uniform2uiv)
#undef glUniform2uivEXT
#define glUniform2uivEXT (gl.Uniform2uivEXT)
#undef glUniform3d
#define glUniform3d (gl.Uniform3d)
#undef glUniform3dv
#define glUniform3dv (gl.Uniform3dv)
#undef glUniform3f
#define glUniform3f (gl.Uniform3f)
#undef glUniform3fv
#define glUniform3fv (gl.Uniform3fv)
#undef glUniform3i
#define glUniform3i (gl.Uniform3i)
#undef glUniform3i64NV
#define glUniform3i64NV (gl.Uniform3i64NV)
#undef glUniform3i64vNV
#define glUniform3i64vNV (gl.Uniform3i64vNV)
#undef glUniform3iv
#define glUniform3iv (gl.Uniform3iv)
#undef glUniform3ui
#define glUniform3ui (gl.Uniform3ui)
#undef glUniform3ui64NV
#define glUniform3ui64NV (gl.Uniform3ui64NV)
#undef glUniform3ui64vNV
#define glUniform3ui64vNV (gl.Uniform3ui64vNV)
#undef glUniform3uiEXT
#define glUniform3uiEXT (gl.Uniform3uiEXT)
#undef glUniform3uiv
#define glUniform3uiv (gl.Uniform3uiv)
#undef glUniform3uivEXT
#define glUniform3uivEXT (gl.Uniform3uivEXT)
#undef glUniform4d
#define glUniform4d (gl.Uniform4d)
#undef glUniform4dv
#define glUniform4dv (gl.Uniform4dv)
#undef glUniform4f
#define glUniform4f (gl.Uniform4f)
#undef glUniform4fv
#define glUniform4fv (gl.Uniform4fv)
#undef glUniform4i
#define glUniform4i (gl.Uniform4i)
#undef glUniform4i64NV
#define glUniform4i64NV (gl.Uniform4i64NV)
#undef glUniform4i64vNV
#define glUniform4i64vNV (gl.Uniform4i64vNV)
#undef glUniform4iv
#define glUniform4iv (gl.Uniform4iv)
#undef glUniform4ui
#define glUniform4ui (gl.Uniform4ui)
#undef glUniform4ui64NV
#define glUniform4ui64NV (gl.Uniform4ui64NV)
#undef glUniform4ui64vNV
#define glUniform4ui64vNV (gl.Uniform4ui64vNV)
#undef glUniform4uiEXT
#define glUniform4uiEXT (gl.Uniform4uiEXT)
#undef glUniform4uiv
#define glUniform4uiv (gl.Uniform4uiv)
#undef glUniform4uivEXT
#define glUniform4uivEXT (gl.Uniform4uivEXT)
#undef glUniformBlockBinding
#define glUniformBlockBinding (gl.UniformBlockBinding)
#undef glUniformBufferEXT
#define glUniformBufferEXT (gl.UniformBufferEXT)
#undef glUniformHandleui64ARB
#define glUniformHandleui64ARB (gl.UniformHandleui64ARB)
#undef glUniformHandleui64NV
#define glUniformHandleui64NV (gl.UniformHandleui64NV)
#undef glUniformHandleui64vARB
#define glUniformHandleui64vARB (gl.UniformHandleui64vARB)
#undef glUniformHandleui64vNV
#define glUniformHandleui64vNV (gl.UniformHandleui64vNV)
#undef glUniformMatrix2dv
#define glUniformMatrix2dv (gl.UniformMatrix2dv)
#undef glUniformMatrix2fv
#define glUniformMatrix2fv (gl.UniformMatrix2fv)
#undef glUniformMatrix2x3dv
#define glUniformMatrix2x3dv (gl.UniformMatrix2x3dv)
#undef glUniformMatrix2x3fv
#define glUniformMatrix2x3fv (gl.UniformMatrix2x3fv)
#undef glUniformMatrix2x4dv
#define glUniformMatrix2x4dv (gl.UniformMatrix2x4dv)
#undef glUniformMatrix2x4fv
#define glUniformMatrix2x4fv (gl.UniformMatrix2x4fv)
#undef glUniformMatrix3dv
#define glUniformMatrix3dv (gl.UniformMatrix3dv)
#undef glUniformMatrix3fv
#define glUniformMatrix3fv (gl.UniformMatrix3fv)
#undef glUniformMatrix3x2dv
#define glUniformMatrix3x2dv (gl.UniformMatrix3x2dv)
#undef glUniformMatrix3x2fv
#define glUniformMatrix3x2fv (gl.UniformMatrix3x2fv)
#undef glUniformMatrix3x4dv
#define glUniformMatrix3x4dv (gl.UniformMatrix3x4dv)
#undef glUniformMatrix3x4fv
#define glUniformMatrix3x4fv (gl.UniformMatrix3x4fv)
#undef glUniformMatrix4dv
#define glUniformMatrix4dv (gl.UniformMatrix4dv)
#undef glUniformMatrix4fv
#define glUniformMatrix4fv (gl.UniformMatrix4fv)
#undef glUniformMatrix4x2dv
#define glUniformMatrix4x2dv (gl.UniformMatrix4x2dv)
#undef glUniformMatrix4x2fv
#define glUniformMatrix4x2fv (gl.UniformMatrix4x2fv)
#undef glUniformMatrix4x3dv
#define glUniformMatrix4x3dv (gl.UniformMatrix4x3dv)
#undef glUniformMatrix4x3fv
#define glUniformMatrix4x3fv (gl.UniformMatrix4x3fv)
#undef glUniformSubroutinesuiv
#define glUniformSubroutinesuiv (gl.UniformSubroutinesuiv)
#undef glUniformui64NV
#define glUniformui64NV (gl.Uniformui64NV)
#undef glUniformui64vNV
#define glUniformui64vNV (gl.Uniformui64vNV)
#undef glUnmapNamedBufferEXT
#define glUnmapNamedBufferEXT (gl.UnmapNamedBufferEXT)
#undef glUnmapTexture2DINTEL
#define glUnmapTexture2DINTEL (gl.UnmapTexture2DINTEL)
#undef glUseProgram
#define glUseProgram (gl.UseProgram)
#undef glUseProgramStages
#define glUseProgramStages (gl.UseProgramStages)
#undef glUseShaderProgramEXT
#define glUseShaderProgramEXT (gl.UseShaderProgramEXT)
#undef glVDPAUFiniNV
#define glVDPAUFiniNV (gl.VDPAUFiniNV)
#undef glVDPAUGetSurfaceivNV
#define glVDPAUGetSurfaceivNV (gl.VDPAUGetSurfaceivNV)
#undef glVDPAUInitNV
#define glVDPAUInitNV (gl.VDPAUInitNV)
#undef glVDPAUIsSurfaceNV
#define glVDPAUIsSurfaceNV (gl.VDPAUIsSurfaceNV)
#undef glVDPAUMapSurfacesNV
#define glVDPAUMapSurfacesNV (gl.VDPAUMapSurfacesNV)
#undef glVDPAURegisterOutputSurfaceNV
#define glVDPAURegisterOutputSurfaceNV (gl.VDPAURegisterOutputSurfaceNV)
#undef glVDPAURegisterVideoSurfaceNV
#define glVDPAURegisterVideoSurfaceNV (gl.VDPAURegisterVideoSurfaceNV)
#undef glVDPAUSurfaceAccessNV
#define glVDPAUSurfaceAccessNV (gl.VDPAUSurfaceAccessNV)
#undef glVDPAUUnmapSurfacesNV
#define glVDPAUUnmapSurfacesNV (gl.VDPAUUnmapSurfacesNV)
#undef glVDPAUUnregisterSurfaceNV
#define glVDPAUUnregisterSurfaceNV (gl.VDPAUUnregisterSurfaceNV)
#undef glValidateProgram
#define glValidateProgram (gl.ValidateProgram)
#undef glValidateProgramPipeline
#define glValidateProgramPipeline (gl.ValidateProgramPipeline)
#undef glVertex2bOES
#define glVertex2bOES (gl.Vertex2bOES)
#undef glVertex2bvOES
#define glVertex2bvOES (gl.Vertex2bvOES)
#undef glVertex2xOES
#define glVertex2xOES (gl.Vertex2xOES)
#undef glVertex2xvOES
#define glVertex2xvOES (gl.Vertex2xvOES)
#undef glVertex3bOES
#define glVertex3bOES (gl.Vertex3bOES)
#undef glVertex3bvOES
#define glVertex3bvOES (gl.Vertex3bvOES)
#undef glVertex3xOES
#define glVertex3xOES (gl.Vertex3xOES)
#undef glVertex3xvOES
#define glVertex3xvOES (gl.Vertex3xvOES)
#undef glVertex4bOES
#define glVertex4bOES (gl.Vertex4bOES)
#undef glVertex4bvOES
#define glVertex4bvOES (gl.Vertex4bvOES)
#undef glVertex4xOES
#define glVertex4xOES (gl.Vertex4xOES)
#undef glVertex4xvOES
#define glVertex4xvOES (gl.Vertex4xvOES)
#undef glVertexArrayBindVertexBufferEXT
#define glVertexArrayBindVertexBufferEXT (gl.VertexArrayBindVertexBufferEXT)
#undef glVertexArrayColorOffsetEXT
#define glVertexArrayColorOffsetEXT (gl.VertexArrayColorOffsetEXT)
#undef glVertexArrayEdgeFlagOffsetEXT
#define glVertexArrayEdgeFlagOffsetEXT (gl.VertexArrayEdgeFlagOffsetEXT)
#undef glVertexArrayFogCoordOffsetEXT
#define glVertexArrayFogCoordOffsetEXT (gl.VertexArrayFogCoordOffsetEXT)
#undef glVertexArrayIndexOffsetEXT
#define glVertexArrayIndexOffsetEXT (gl.VertexArrayIndexOffsetEXT)
#undef glVertexArrayMultiTexCoordOffsetEXT
#define glVertexArrayMultiTexCoordOffsetEXT (gl.VertexArrayMultiTexCoordOffsetEXT)
#undef glVertexArrayNormalOffsetEXT
#define glVertexArrayNormalOffsetEXT (gl.VertexArrayNormalOffsetEXT)
#undef glVertexArraySecondaryColorOffsetEXT
#define glVertexArraySecondaryColorOffsetEXT (gl.VertexArraySecondaryColorOffsetEXT)
#undef glVertexArrayTexCoordOffsetEXT
#define glVertexArrayTexCoordOffsetEXT (gl.VertexArrayTexCoordOffsetEXT)
#undef glVertexArrayVertexAttribBindingEXT
#define glVertexArrayVertexAttribBindingEXT (gl.VertexArrayVertexAttribBindingEXT)
#undef glVertexArrayVertexAttribDivisorEXT
#define glVertexArrayVertexAttribDivisorEXT (gl.VertexArrayVertexAttribDivisorEXT)
#undef glVertexArrayVertexAttribFormatEXT
#define glVertexArrayVertexAttribFormatEXT (gl.VertexArrayVertexAttribFormatEXT)
#undef glVertexArrayVertexAttribIFormatEXT
#define glVertexArrayVertexAttribIFormatEXT (gl.VertexArrayVertexAttribIFormatEXT)
#undef glVertexArrayVertexAttribIOffsetEXT
#define glVertexArrayVertexAttribIOffsetEXT (gl.VertexArrayVertexAttribIOffsetEXT)
#undef glVertexArrayVertexAttribLFormatEXT
#define glVertexArrayVertexAttribLFormatEXT (gl.VertexArrayVertexAttribLFormatEXT)
#undef glVertexArrayVertexAttribLOffsetEXT
#define glVertexArrayVertexAttribLOffsetEXT (gl.VertexArrayVertexAttribLOffsetEXT)
#undef glVertexArrayVertexAttribOffsetEXT
#define glVertexArrayVertexAttribOffsetEXT (gl.VertexArrayVertexAttribOffsetEXT)
#undef glVertexArrayVertexBindingDivisorEXT
#define glVertexArrayVertexBindingDivisorEXT (gl.VertexArrayVertexBindingDivisorEXT)
#undef glVertexArrayVertexOffsetEXT
#define glVertexArrayVertexOffsetEXT (gl.VertexArrayVertexOffsetEXT)
#undef glVertexAttrib1d
#define glVertexAttrib1d (gl.VertexAttrib1d)
#undef glVertexAttrib1dv
#define glVertexAttrib1dv (gl.VertexAttrib1dv)
#undef glVertexAttrib1f
#define glVertexAttrib1f (gl.VertexAttrib1f)
#undef glVertexAttrib1fv
#define glVertexAttrib1fv (gl.VertexAttrib1fv)
#undef glVertexAttrib1s
#define glVertexAttrib1s (gl.VertexAttrib1s)
#undef glVertexAttrib1sv
#define glVertexAttrib1sv (gl.VertexAttrib1sv)
#undef glVertexAttrib2d
#define glVertexAttrib2d (gl.VertexAttrib2d)
#undef glVertexAttrib2dv
#define glVertexAttrib2dv (gl.VertexAttrib2dv)
#undef glVertexAttrib2f
#define glVertexAttrib2f (gl.VertexAttrib2f)
#undef glVertexAttrib2fv
#define glVertexAttrib2fv (gl.VertexAttrib2fv)
#undef glVertexAttrib2s
#define glVertexAttrib2s (gl.VertexAttrib2s)
#undef glVertexAttrib2sv
#define glVertexAttrib2sv (gl.VertexAttrib2sv)
#undef glVertexAttrib3d
#define glVertexAttrib3d (gl.VertexAttrib3d)
#undef glVertexAttrib3dv
#define glVertexAttrib3dv (gl.VertexAttrib3dv)
#undef glVertexAttrib3f
#define glVertexAttrib3f (gl.VertexAttrib3f)
#undef glVertexAttrib3fv
#define glVertexAttrib3fv (gl.VertexAttrib3fv)
#undef glVertexAttrib3s
#define glVertexAttrib3s (gl.VertexAttrib3s)
#undef glVertexAttrib3sv
#define glVertexAttrib3sv (gl.VertexAttrib3sv)
#undef glVertexAttrib4Nbv
#define glVertexAttrib4Nbv (gl.VertexAttrib4Nbv)
#undef glVertexAttrib4Niv
#define glVertexAttrib4Niv (gl.VertexAttrib4Niv)
#undef glVertexAttrib4Nsv
#define glVertexAttrib4Nsv (gl.VertexAttrib4Nsv)
#undef glVertexAttrib4Nub
#define glVertexAttrib4Nub (gl.VertexAttrib4Nub)
#undef glVertexAttrib4Nubv
#define glVertexAttrib4Nubv (gl.VertexAttrib4Nubv)
#undef glVertexAttrib4Nuiv
#define glVertexAttrib4Nuiv (gl.VertexAttrib4Nuiv)
#undef glVertexAttrib4Nusv
#define glVertexAttrib4Nusv (gl.VertexAttrib4Nusv)
#undef glVertexAttrib4bv
#define glVertexAttrib4bv (gl.VertexAttrib4bv)
#undef glVertexAttrib4d
#define glVertexAttrib4d (gl.VertexAttrib4d)
#undef glVertexAttrib4dv
#define glVertexAttrib4dv (gl.VertexAttrib4dv)
#undef glVertexAttrib4f
#define glVertexAttrib4f (gl.VertexAttrib4f)
#undef glVertexAttrib4fv
#define glVertexAttrib4fv (gl.VertexAttrib4fv)
#undef glVertexAttrib4iv
#define glVertexAttrib4iv (gl.VertexAttrib4iv)
#undef glVertexAttrib4s
#define glVertexAttrib4s (gl.VertexAttrib4s)
#undef glVertexAttrib4sv
#define glVertexAttrib4sv (gl.VertexAttrib4sv)
#undef glVertexAttrib4ubv
#define glVertexAttrib4ubv (gl.VertexAttrib4ubv)
#undef glVertexAttrib4uiv
#define glVertexAttrib4uiv (gl.VertexAttrib4uiv)
#undef glVertexAttrib4usv
#define glVertexAttrib4usv (gl.VertexAttrib4usv)
#undef glVertexAttribBinding
#define glVertexAttribBinding (gl.VertexAttribBinding)
#undef glVertexAttribDivisor
#define glVertexAttribDivisor (gl.VertexAttribDivisor)
#undef glVertexAttribDivisorARB
#define glVertexAttribDivisorARB (gl.VertexAttribDivisorARB)
#undef glVertexAttribFormat
#define glVertexAttribFormat (gl.VertexAttribFormat)
#undef glVertexAttribFormatNV
#define glVertexAttribFormatNV (gl.VertexAttribFormatNV)
#undef glVertexAttribI1i
#define glVertexAttribI1i (gl.VertexAttribI1i)
#undef glVertexAttribI1iEXT
#define glVertexAttribI1iEXT (gl.VertexAttribI1iEXT)
#undef glVertexAttribI1iv
#define glVertexAttribI1iv (gl.VertexAttribI1iv)
#undef glVertexAttribI1ivEXT
#define glVertexAttribI1ivEXT (gl.VertexAttribI1ivEXT)
#undef glVertexAttribI1ui
#define glVertexAttribI1ui (gl.VertexAttribI1ui)
#undef glVertexAttribI1uiEXT
#define glVertexAttribI1uiEXT (gl.VertexAttribI1uiEXT)
#undef glVertexAttribI1uiv
#define glVertexAttribI1uiv (gl.VertexAttribI1uiv)
#undef glVertexAttribI1uivEXT
#define glVertexAttribI1uivEXT (gl.VertexAttribI1uivEXT)
#undef glVertexAttribI2i
#define glVertexAttribI2i (gl.VertexAttribI2i)
#undef glVertexAttribI2iEXT
#define glVertexAttribI2iEXT (gl.VertexAttribI2iEXT)
#undef glVertexAttribI2iv
#define glVertexAttribI2iv (gl.VertexAttribI2iv)
#undef glVertexAttribI2ivEXT
#define glVertexAttribI2ivEXT (gl.VertexAttribI2ivEXT)
#undef glVertexAttribI2ui
#define glVertexAttribI2ui (gl.VertexAttribI2ui)
#undef glVertexAttribI2uiEXT
#define glVertexAttribI2uiEXT (gl.VertexAttribI2uiEXT)
#undef glVertexAttribI2uiv
#define glVertexAttribI2uiv (gl.VertexAttribI2uiv)
#undef glVertexAttribI2uivEXT
#define glVertexAttribI2uivEXT (gl.VertexAttribI2uivEXT)
#undef glVertexAttribI3i
#define glVertexAttribI3i (gl.VertexAttribI3i)
#undef glVertexAttribI3iEXT
#define glVertexAttribI3iEXT (gl.VertexAttribI3iEXT)
#undef glVertexAttribI3iv
#define glVertexAttribI3iv (gl.VertexAttribI3iv)
#undef glVertexAttribI3ivEXT
#define glVertexAttribI3ivEXT (gl.VertexAttribI3ivEXT)
#undef glVertexAttribI3ui
#define glVertexAttribI3ui (gl.VertexAttribI3ui)
#undef glVertexAttribI3uiEXT
#define glVertexAttribI3uiEXT (gl.VertexAttribI3uiEXT)
#undef glVertexAttribI3uiv
#define glVertexAttribI3uiv (gl.VertexAttribI3uiv)
#undef glVertexAttribI3uivEXT
#define glVertexAttribI3uivEXT (gl.VertexAttribI3uivEXT)
#undef glVertexAttribI4bv
#define glVertexAttribI4bv (gl.VertexAttribI4bv)
#undef glVertexAttribI4bvEXT
#define glVertexAttribI4bvEXT (gl.VertexAttribI4bvEXT)
#undef glVertexAttribI4i
#define glVertexAttribI4i (gl.VertexAttribI4i)
#undef glVertexAttribI4iEXT
#define glVertexAttribI4iEXT (gl.VertexAttribI4iEXT)
#undef glVertexAttribI4iv
#define glVertexAttribI4iv (gl.VertexAttribI4iv)
#undef glVertexAttribI4ivEXT
#define glVertexAttribI4ivEXT (gl.VertexAttribI4ivEXT)
#undef glVertexAttribI4sv
#define glVertexAttribI4sv (gl.VertexAttribI4sv)
#undef glVertexAttribI4svEXT
#define glVertexAttribI4svEXT (gl.VertexAttribI4svEXT)
#undef glVertexAttribI4ubv
#define glVertexAttribI4ubv (gl.VertexAttribI4ubv)
#undef glVertexAttribI4ubvEXT
#define glVertexAttribI4ubvEXT (gl.VertexAttribI4ubvEXT)
#undef glVertexAttribI4ui
#define glVertexAttribI4ui (gl.VertexAttribI4ui)
#undef glVertexAttribI4uiEXT
#define glVertexAttribI4uiEXT (gl.VertexAttribI4uiEXT)
#undef glVertexAttribI4uiv
#define glVertexAttribI4uiv (gl.VertexAttribI4uiv)
#undef glVertexAttribI4uivEXT
#define glVertexAttribI4uivEXT (gl.VertexAttribI4uivEXT)
#undef glVertexAttribI4usv
#define glVertexAttribI4usv (gl.VertexAttribI4usv)
#undef glVertexAttribI4usvEXT
#define glVertexAttribI4usvEXT (gl.VertexAttribI4usvEXT)
#undef glVertexAttribIFormat
#define glVertexAttribIFormat (gl.VertexAttribIFormat)
#undef glVertexAttribIFormatNV
#define glVertexAttribIFormatNV (gl.VertexAttribIFormatNV)
#undef glVertexAttribIPointer
#define glVertexAttribIPointer (gl.VertexAttribIPointer)
#undef glVertexAttribIPointerEXT
#define glVertexAttribIPointerEXT (gl.VertexAttribIPointerEXT)
#undef glVertexAttribL1d
#define glVertexAttribL1d (gl.VertexAttribL1d)
#undef glVertexAttribL1dEXT
#define glVertexAttribL1dEXT (gl.VertexAttribL1dEXT)
#undef glVertexAttribL1dv
#define glVertexAttribL1dv (gl.VertexAttribL1dv)
#undef glVertexAttribL1dvEXT
#define glVertexAttribL1dvEXT (gl.VertexAttribL1dvEXT)
#undef glVertexAttribL1i64NV
#define glVertexAttribL1i64NV (gl.VertexAttribL1i64NV)
#undef glVertexAttribL1i64vNV
#define glVertexAttribL1i64vNV (gl.VertexAttribL1i64vNV)
#undef glVertexAttribL1ui64ARB
#define glVertexAttribL1ui64ARB (gl.VertexAttribL1ui64ARB)
#undef glVertexAttribL1ui64NV
#define glVertexAttribL1ui64NV (gl.VertexAttribL1ui64NV)
#undef glVertexAttribL1ui64vARB
#define glVertexAttribL1ui64vARB (gl.VertexAttribL1ui64vARB)
#undef glVertexAttribL1ui64vNV
#define glVertexAttribL1ui64vNV (gl.VertexAttribL1ui64vNV)
#undef glVertexAttribL2d
#define glVertexAttribL2d (gl.VertexAttribL2d)
#undef glVertexAttribL2dEXT
#define glVertexAttribL2dEXT (gl.VertexAttribL2dEXT)
#undef glVertexAttribL2dv
#define glVertexAttribL2dv (gl.VertexAttribL2dv)
#undef glVertexAttribL2dvEXT
#define glVertexAttribL2dvEXT (gl.VertexAttribL2dvEXT)
#undef glVertexAttribL2i64NV
#define glVertexAttribL2i64NV (gl.VertexAttribL2i64NV)
#undef glVertexAttribL2i64vNV
#define glVertexAttribL2i64vNV (gl.VertexAttribL2i64vNV)
#undef glVertexAttribL2ui64NV
#define glVertexAttribL2ui64NV (gl.VertexAttribL2ui64NV)
#undef glVertexAttribL2ui64vNV
#define glVertexAttribL2ui64vNV (gl.VertexAttribL2ui64vNV)
#undef glVertexAttribL3d
#define glVertexAttribL3d (gl.VertexAttribL3d)
#undef glVertexAttribL3dEXT
#define glVertexAttribL3dEXT (gl.VertexAttribL3dEXT)
#undef glVertexAttribL3dv
#define glVertexAttribL3dv (gl.VertexAttribL3dv)
#undef glVertexAttribL3dvEXT
#define glVertexAttribL3dvEXT (gl.VertexAttribL3dvEXT)
#undef glVertexAttribL3i64NV
#define glVertexAttribL3i64NV (gl.VertexAttribL3i64NV)
#undef glVertexAttribL3i64vNV
#define glVertexAttribL3i64vNV (gl.VertexAttribL3i64vNV)
#undef glVertexAttribL3ui64NV
#define glVertexAttribL3ui64NV (gl.VertexAttribL3ui64NV)
#undef glVertexAttribL3ui64vNV
#define glVertexAttribL3ui64vNV (gl.VertexAttribL3ui64vNV)
#undef glVertexAttribL4d
#define glVertexAttribL4d (gl.VertexAttribL4d)
#undef glVertexAttribL4dEXT
#define glVertexAttribL4dEXT (gl.VertexAttribL4dEXT)
#undef glVertexAttribL4dv
#define glVertexAttribL4dv (gl.VertexAttribL4dv)
#undef glVertexAttribL4dvEXT
#define glVertexAttribL4dvEXT (gl.VertexAttribL4dvEXT)
#undef glVertexAttribL4i64NV
#define glVertexAttribL4i64NV (gl.VertexAttribL4i64NV)
#undef glVertexAttribL4i64vNV
#define glVertexAttribL4i64vNV (gl.VertexAttribL4i64vNV)
#undef glVertexAttribL4ui64NV
#define glVertexAttribL4ui64NV (gl.VertexAttribL4ui64NV)
#undef glVertexAttribL4ui64vNV
#define glVertexAttribL4ui64vNV (gl.VertexAttribL4ui64vNV)
#undef glVertexAttribLFormat
#define glVertexAttribLFormat (gl.VertexAttribLFormat)
#undef glVertexAttribLFormatNV
#define glVertexAttribLFormatNV (gl.VertexAttribLFormatNV)
#undef glVertexAttribLPointer
#define glVertexAttribLPointer (gl.VertexAttribLPointer)
#undef glVertexAttribLPointerEXT
#define glVertexAttribLPointerEXT (gl.VertexAttribLPointerEXT)
#undef glVertexAttribP1ui
#define glVertexAttribP1ui (gl.VertexAttribP1ui)
#undef glVertexAttribP1uiv
#define glVertexAttribP1uiv (gl.VertexAttribP1uiv)
#undef glVertexAttribP2ui
#define glVertexAttribP2ui (gl.VertexAttribP2ui)
#undef glVertexAttribP2uiv
#define glVertexAttribP2uiv (gl.VertexAttribP2uiv)
#undef glVertexAttribP3ui
#define glVertexAttribP3ui (gl.VertexAttribP3ui)
#undef glVertexAttribP3uiv
#define glVertexAttribP3uiv (gl.VertexAttribP3uiv)
#undef glVertexAttribP4ui
#define glVertexAttribP4ui (gl.VertexAttribP4ui)
#undef glVertexAttribP4uiv
#define glVertexAttribP4uiv (gl.VertexAttribP4uiv)
#undef glVertexAttribParameteriAMD
#define glVertexAttribParameteriAMD (gl.VertexAttribParameteriAMD)
#undef glVertexAttribPointer
#define glVertexAttribPointer (gl.VertexAttribPointer)
#undef glVertexBindingDivisor
#define glVertexBindingDivisor (gl.VertexBindingDivisor)
#undef glVertexFormatNV
#define glVertexFormatNV (gl.VertexFormatNV)
#undef glVertexP2ui
#define glVertexP2ui (gl.VertexP2ui)
#undef glVertexP2uiv
#define glVertexP2uiv (gl.VertexP2uiv)
#undef glVertexP3ui
#define glVertexP3ui (gl.VertexP3ui)
#undef glVertexP3uiv
#define glVertexP3uiv (gl.VertexP3uiv)
#undef glVertexP4ui
#define glVertexP4ui (gl.VertexP4ui)
#undef glVertexP4uiv
#define glVertexP4uiv (gl.VertexP4uiv)
#undef glVertexPointSizefAPPLE
#define glVertexPointSizefAPPLE (gl.VertexPointSizefAPPLE)
#undef glVideoCaptureNV
#define glVideoCaptureNV (gl.VideoCaptureNV)
#undef glVideoCaptureStreamParameterdvNV
#define glVideoCaptureStreamParameterdvNV (gl.VideoCaptureStreamParameterdvNV)
#undef glVideoCaptureStreamParameterfvNV
#define glVideoCaptureStreamParameterfvNV (gl.VideoCaptureStreamParameterfvNV)
#undef glVideoCaptureStreamParameterivNV
#define glVideoCaptureStreamParameterivNV (gl.VideoCaptureStreamParameterivNV)
#undef glViewportArrayv
#define glViewportArrayv (gl.ViewportArrayv)
#undef glViewportIndexedf
#define glViewportIndexedf (gl.ViewportIndexedf)
#undef glViewportIndexedfv
#define glViewportIndexedfv (gl.ViewportIndexedfv)
#undef glWaitSync
#define glWaitSync (gl.WaitSync)
#undef glWeightPathsNV
#define glWeightPathsNV (gl.WeightPathsNV)
#undef glBindTextureUnit
#define glBindTextureUnit (gl.BindTextureUnit)
#undef glBlendBarrierKHR
#define glBlendBarrierKHR (gl.BlendBarrierKHR)
#undef glBlitNamedFramebuffer
#define glBlitNamedFramebuffer (gl.BlitNamedFramebuffer)
#undef glBufferPageCommitmentARB
#define glBufferPageCommitmentARB (gl.BufferPageCommitmentARB)
#undef glCallCommandListNV
#define glCallCommandListNV (gl.CallCommandListNV)
#undef glCheckNamedFramebufferStatus
#define glCheckNamedFramebufferStatus (gl.CheckNamedFramebufferStatus)
#undef glClearNamedBufferData
#define glClearNamedBufferData (gl.ClearNamedBufferData)
#undef glClearNamedBufferSubData
#define glClearNamedBufferSubData (gl.ClearNamedBufferSubData)
#undef glClearNamedFramebufferfi
#define glClearNamedFramebufferfi (gl.ClearNamedFramebufferfi)
#undef glClearNamedFramebufferfv
#define glClearNamedFramebufferfv (gl.ClearNamedFramebufferfv)
#undef glClearNamedFramebufferiv
#define glClearNamedFramebufferiv (gl.ClearNamedFramebufferiv)
#undef glClearNamedFramebufferuiv
#define glClearNamedFramebufferuiv (gl.ClearNamedFramebufferuiv)
#undef glClipControl
#define glClipControl (gl.ClipControl)
#undef glCommandListSegmentsNV
#define glCommandListSegmentsNV (gl.CommandListSegmentsNV)
#undef glCompileCommandListNV
#define glCompileCommandListNV (gl.CompileCommandListNV)
#undef glCompressedTextureSubImage1D
#define glCompressedTextureSubImage1D (gl.CompressedTextureSubImage1D)
#undef glCompressedTextureSubImage2D
#define glCompressedTextureSubImage2D (gl.CompressedTextureSubImage2D)
#undef glCompressedTextureSubImage3D
#define glCompressedTextureSubImage3D (gl.CompressedTextureSubImage3D)
#undef glCopyNamedBufferSubData
#define glCopyNamedBufferSubData (gl.CopyNamedBufferSubData)
#undef glCopyTextureSubImage1D
#define glCopyTextureSubImage1D (gl.CopyTextureSubImage1D)
#undef glCopyTextureSubImage2D
#define glCopyTextureSubImage2D (gl.CopyTextureSubImage2D)
#undef glCopyTextureSubImage3D
#define glCopyTextureSubImage3D (gl.CopyTextureSubImage3D)
#undef glCoverageModulationNV
#define glCoverageModulationNV (gl.CoverageModulationNV)
#undef glCoverageModulationTableNV
#define glCoverageModulationTableNV (gl.CoverageModulationTableNV)
#undef glCreateBuffers
#define glCreateBuffers (gl.CreateBuffers)
#undef glCreateCommandListsNV
#define glCreateCommandListsNV (gl.CreateCommandListsNV)
#undef glCreateFramebuffers
#define glCreateFramebuffers (gl.CreateFramebuffers)
#undef glCreateProgramPipelines
#define glCreateProgramPipelines (gl.CreateProgramPipelines)
#undef glCreateQueries
#define glCreateQueries (gl.CreateQueries)
#undef glCreateRenderbuffers
#define glCreateRenderbuffers (gl.CreateRenderbuffers)
#undef glCreateSamplers
#define glCreateSamplers (gl.CreateSamplers)
#undef glCreateStatesNV
#define glCreateStatesNV (gl.CreateStatesNV)
#undef glCreateTextures
#define glCreateTextures (gl.CreateTextures)
#undef glCreateTransformFeedbacks
#define glCreateTransformFeedbacks (gl.CreateTransformFeedbacks)
#undef glCreateVertexArrays
#define glCreateVertexArrays (gl.CreateVertexArrays)
#undef glDeleteCommandListsNV
#define glDeleteCommandListsNV (gl.DeleteCommandListsNV)
#undef glDeleteStatesNV
#define glDeleteStatesNV (gl.DeleteStatesNV)
#undef glDisableVertexArrayAttrib
#define glDisableVertexArrayAttrib (gl.DisableVertexArrayAttrib)
#undef glDrawCommandsAddressNV
#define glDrawCommandsAddressNV (gl.DrawCommandsAddressNV)
#undef glDrawCommandsNV
#define glDrawCommandsNV (gl.DrawCommandsNV)
#undef glDrawCommandsStatesAddressNV
#define glDrawCommandsStatesAddressNV (gl.DrawCommandsStatesAddressNV)
#undef glDrawCommandsStatesNV
#define glDrawCommandsStatesNV (gl.DrawCommandsStatesNV)
#undef glEnableVertexArrayAttrib
#define glEnableVertexArrayAttrib (gl.EnableVertexArrayAttrib)
#undef glFlushMappedNamedBufferRange
#define glFlushMappedNamedBufferRange (gl.FlushMappedNamedBufferRange)
#undef glFragmentCoverageColorNV
#define glFragmentCoverageColorNV (gl.FragmentCoverageColorNV)
#undef glFramebufferSampleLocationsfvNV
#define glFramebufferSampleLocationsfvNV (gl.FramebufferSampleLocationsfvNV)
#undef glFramebufferTextureMultiviewOVR
#define glFramebufferTextureMultiviewOVR (gl.FramebufferTextureMultiviewOVR)
#undef glGenerateTextureMipmap
#define glGenerateTextureMipmap (gl.GenerateTextureMipmap)
#undef glGetCommandHeaderNV
#define glGetCommandHeaderNV (gl.GetCommandHeaderNV)
#undef glGetCompressedTextureImage
#define glGetCompressedTextureImage (gl.GetCompressedTextureImage)
#undef glGetCompressedTextureSubImage
#define glGetCompressedTextureSubImage (gl.GetCompressedTextureSubImage)
#undef glGetCoverageModulationTableNV
#define glGetCoverageModulationTableNV (gl.GetCoverageModulationTableNV)
#undef glGetGraphicsResetStatus
#define glGetGraphicsResetStatus (gl.GetGraphicsResetStatus)
#undef glGetInternalformatSampleivNV
#define glGetInternalformatSampleivNV (gl.GetInternalformatSampleivNV)
#undef glGetNamedBufferParameteri64v
#define glGetNamedBufferParameteri64v (gl.GetNamedBufferParameteri64v)
#undef glGetNamedBufferParameteriv
#define glGetNamedBufferParameteriv (gl.GetNamedBufferParameteriv)
#undef glGetNamedBufferPointerv
#define glGetNamedBufferPointerv (gl.GetNamedBufferPointerv)
#undef glGetNamedBufferSubData
#define glGetNamedBufferSubData (gl.GetNamedBufferSubData)
#undef glGetNamedFramebufferAttachmentParameteriv
#define glGetNamedFramebufferAttachmentParameteriv (gl.GetNamedFramebufferAttachmentParameteriv)
#undef glGetNamedFramebufferParameteriv
#define glGetNamedFramebufferParameteriv (gl.GetNamedFramebufferParameteriv)
#undef glGetNamedRenderbufferParameteriv
#define glGetNamedRenderbufferParameteriv (gl.GetNamedRenderbufferParameteriv)
#undef glGetProgramResourcefvNV
#define glGetProgramResourcefvNV (gl.GetProgramResourcefvNV)
#undef glGetQueryBufferObjecti64v
#define glGetQueryBufferObjecti64v (gl.GetQueryBufferObjecti64v)
#undef glGetQueryBufferObjectiv
#define glGetQueryBufferObjectiv (gl.GetQueryBufferObjectiv)
#undef glGetQueryBufferObjectui64v
#define glGetQueryBufferObjectui64v (gl.GetQueryBufferObjectui64v)
#undef glGetQueryBufferObjectuiv
#define glGetQueryBufferObjectuiv (gl.GetQueryBufferObjectuiv)
#undef glGetStageIndexNV
#define glGetStageIndexNV (gl.GetStageIndexNV)
#undef glGetTextureImage
#define glGetTextureImage (gl.GetTextureImage)
#undef glGetTextureLevelParameterfv
#define glGetTextureLevelParameterfv (gl.GetTextureLevelParameterfv)
#undef glGetTextureLevelParameteriv
#define glGetTextureLevelParameteriv (gl.GetTextureLevelParameteriv)
#undef glGetTextureParameterIiv
#define glGetTextureParameterIiv (gl.GetTextureParameterIiv)
#undef glGetTextureParameterIuiv
#define glGetTextureParameterIuiv (gl.GetTextureParameterIuiv)
#undef glGetTextureParameterfv
#define glGetTextureParameterfv (gl.GetTextureParameterfv)
#undef glGetTextureParameteriv
#define glGetTextureParameteriv (gl.GetTextureParameteriv)
#undef glGetTextureSubImage
#define glGetTextureSubImage (gl.GetTextureSubImage)
#undef glGetTransformFeedbacki64_v
#define glGetTransformFeedbacki64_v (gl.GetTransformFeedbacki64_v)
#undef glGetTransformFeedbacki_v
#define glGetTransformFeedbacki_v (gl.GetTransformFeedbacki_v)
#undef glGetTransformFeedbackiv
#define glGetTransformFeedbackiv (gl.GetTransformFeedbackiv)
#undef glGetVertexArrayIndexed64iv
#define glGetVertexArrayIndexed64iv (gl.GetVertexArrayIndexed64iv)
#undef glGetVertexArrayIndexediv
#define glGetVertexArrayIndexediv (gl.GetVertexArrayIndexediv)
#undef glGetVertexArrayiv
#define glGetVertexArrayiv (gl.GetVertexArrayiv)
#undef glGetnColorTable
#define glGetnColorTable (gl.GetnColorTable)
#undef glGetnCompressedTexImage
#define glGetnCompressedTexImage (gl.GetnCompressedTexImage)
#undef glGetnConvolutionFilter
#define glGetnConvolutionFilter (gl.GetnConvolutionFilter)
#undef glGetnHistogram
#define glGetnHistogram (gl.GetnHistogram)
#undef glGetnMapdv
#define glGetnMapdv (gl.GetnMapdv)
#undef glGetnMapfv
#define glGetnMapfv (gl.GetnMapfv)
#undef glGetnMapiv
#define glGetnMapiv (gl.GetnMapiv)
#undef glGetnMinmax
#define glGetnMinmax (gl.GetnMinmax)
#undef glGetnPixelMapfv
#define glGetnPixelMapfv (gl.GetnPixelMapfv)
#undef glGetnPixelMapuiv
#define glGetnPixelMapuiv (gl.GetnPixelMapuiv)
#undef glGetnPixelMapusv
#define glGetnPixelMapusv (gl.GetnPixelMapusv)
#undef glGetnPolygonStipple
#define glGetnPolygonStipple (gl.GetnPolygonStipple)
#undef glGetnSeparableFilter
#define glGetnSeparableFilter (gl.GetnSeparableFilter)
#undef glGetnTexImage
#define glGetnTexImage (gl.GetnTexImage)
#undef glGetnUniformdv
#define glGetnUniformdv (gl.GetnUniformdv)
#undef glGetnUniformfv
#define glGetnUniformfv (gl.GetnUniformfv)
#undef glGetnUniformiv
#define glGetnUniformiv (gl.GetnUniformiv)
#undef glGetnUniformuiv
#define glGetnUniformuiv (gl.GetnUniformuiv)
#undef glInvalidateNamedFramebufferData
#define glInvalidateNamedFramebufferData (gl.InvalidateNamedFramebufferData)
#undef glInvalidateNamedFramebufferSubData
#define glInvalidateNamedFramebufferSubData (gl.InvalidateNamedFramebufferSubData)
#undef glIsCommandListNV
#define glIsCommandListNV (gl.IsCommandListNV)
#undef glIsStateNV
#define glIsStateNV (gl.IsStateNV)
#undef glListDrawCommandsStatesClientNV
#define glListDrawCommandsStatesClientNV (gl.ListDrawCommandsStatesClientNV)
#undef glMapNamedBuffer
#define glMapNamedBuffer (gl.MapNamedBuffer)
#undef glMapNamedBufferRange
#define glMapNamedBufferRange (gl.MapNamedBufferRange)
#undef glMatrixLoad3x2fNV
#define glMatrixLoad3x2fNV (gl.MatrixLoad3x2fNV)
#undef glMatrixLoad3x3fNV
#define glMatrixLoad3x3fNV (gl.MatrixLoad3x3fNV)
#undef glMatrixLoadTranspose3x3fNV
#define glMatrixLoadTranspose3x3fNV (gl.MatrixLoadTranspose3x3fNV)
#undef glMatrixMult3x2fNV
#define glMatrixMult3x2fNV (gl.MatrixMult3x2fNV)
#undef glMatrixMult3x3fNV
#define glMatrixMult3x3fNV (gl.MatrixMult3x3fNV)
#undef glMatrixMultTranspose3x3fNV
#define glMatrixMultTranspose3x3fNV (gl.MatrixMultTranspose3x3fNV)
#undef glMemoryBarrierByRegion
#define glMemoryBarrierByRegion (gl.MemoryBarrierByRegion)
#undef glMultiDrawArraysIndirectBindlessCountNV
#define glMultiDrawArraysIndirectBindlessCountNV (gl.MultiDrawArraysIndirectBindlessCountNV)
#undef glMultiDrawElementsIndirectBindlessCountNV
#define glMultiDrawElementsIndirectBindlessCountNV (gl.MultiDrawElementsIndirectBindlessCountNV)
#undef glNamedBufferData
#define glNamedBufferData (gl.NamedBufferData)
#undef glNamedBufferPageCommitmentARB
#define glNamedBufferPageCommitmentARB (gl.NamedBufferPageCommitmentARB)
#undef glNamedBufferPageCommitmentEXT
#define glNamedBufferPageCommitmentEXT (gl.NamedBufferPageCommitmentEXT)
#undef glNamedBufferStorage
#define glNamedBufferStorage (gl.NamedBufferStorage)
#undef glNamedBufferSubData
#define glNamedBufferSubData (gl.NamedBufferSubData)
#undef glNamedFramebufferDrawBuffer
#define glNamedFramebufferDrawBuffer (gl.NamedFramebufferDrawBuffer)
#undef glNamedFramebufferDrawBuffers
#define glNamedFramebufferDrawBuffers (gl.NamedFramebufferDrawBuffers)
#undef glNamedFramebufferParameteri
#define glNamedFramebufferParameteri (gl.NamedFramebufferParameteri)
#undef glNamedFramebufferReadBuffer
#define glNamedFramebufferReadBuffer (gl.NamedFramebufferReadBuffer)
#undef glNamedFramebufferRenderbuffer
#define glNamedFramebufferRenderbuffer (gl.NamedFramebufferRenderbuffer)
#undef glNamedFramebufferSampleLocationsfvNV
#define glNamedFramebufferSampleLocationsfvNV (gl.NamedFramebufferSampleLocationsfvNV)
#undef glNamedFramebufferTexture
#define glNamedFramebufferTexture (gl.NamedFramebufferTexture)
#undef glNamedFramebufferTextureLayer
#define glNamedFramebufferTextureLayer (gl.NamedFramebufferTextureLayer)
#undef glNamedRenderbufferStorage
#define glNamedRenderbufferStorage (gl.NamedRenderbufferStorage)
#undef glNamedRenderbufferStorageMultisample
#define glNamedRenderbufferStorageMultisample (gl.NamedRenderbufferStorageMultisample)
#undef glPathGlyphIndexArrayNV
#define glPathGlyphIndexArrayNV (gl.PathGlyphIndexArrayNV)
#undef glPathGlyphIndexRangeNV
#define glPathGlyphIndexRangeNV (gl.PathGlyphIndexRangeNV)
#undef glPathMemoryGlyphIndexArrayNV
#define glPathMemoryGlyphIndexArrayNV (gl.PathMemoryGlyphIndexArrayNV)
#undef glPolygonOffsetClampEXT
#define glPolygonOffsetClampEXT (gl.PolygonOffsetClampEXT)
#undef glProgramPathFragmentInputGenNV
#define glProgramPathFragmentInputGenNV (gl.ProgramPathFragmentInputGenNV)
#undef glRasterSamplesEXT
#define glRasterSamplesEXT (gl.RasterSamplesEXT)
#undef glReadnPixels
#define glReadnPixels (gl.ReadnPixels)
#undef glResolveDepthValuesNV
#define glResolveDepthValuesNV (gl.ResolveDepthValuesNV)
#undef glStateCaptureNV
#define glStateCaptureNV (gl.StateCaptureNV)
#undef glStencilThenCoverFillPathInstancedNV
#define glStencilThenCoverFillPathInstancedNV (gl.StencilThenCoverFillPathInstancedNV)
#undef glStencilThenCoverFillPathNV
#define glStencilThenCoverFillPathNV (gl.StencilThenCoverFillPathNV)
#undef glStencilThenCoverStrokePathInstancedNV
#define glStencilThenCoverStrokePathInstancedNV (gl.StencilThenCoverStrokePathInstancedNV)
#undef glStencilThenCoverStrokePathNV
#define glStencilThenCoverStrokePathNV (gl.StencilThenCoverStrokePathNV)
#undef glSubpixelPrecisionBiasNV
#define glSubpixelPrecisionBiasNV (gl.SubpixelPrecisionBiasNV)
#undef glTextureBarrier
#define glTextureBarrier (gl.TextureBarrier)
#undef glTextureBuffer
#define glTextureBuffer (gl.TextureBuffer)
#undef glTextureBufferRange
#define glTextureBufferRange (gl.TextureBufferRange)
#undef glTextureParameterIiv
#define glTextureParameterIiv (gl.TextureParameterIiv)
#undef glTextureParameterIuiv
#define glTextureParameterIuiv (gl.TextureParameterIuiv)
#undef glTextureParameterf
#define glTextureParameterf (gl.TextureParameterf)
#undef glTextureParameterfv
#define glTextureParameterfv (gl.TextureParameterfv)
#undef glTextureParameteri
#define glTextureParameteri (gl.TextureParameteri)
#undef glTextureParameteriv
#define glTextureParameteriv (gl.TextureParameteriv)
#undef glTextureStorage1D
#define glTextureStorage1D (gl.TextureStorage1D)
#undef glTextureStorage2D
#define glTextureStorage2D (gl.TextureStorage2D)
#undef glTextureStorage2DMultisample
#define glTextureStorage2DMultisample (gl.TextureStorage2DMultisample)
#undef glTextureStorage3D
#define glTextureStorage3D (gl.TextureStorage3D)
#undef glTextureStorage3DMultisample
#define glTextureStorage3DMultisample (gl.TextureStorage3DMultisample)
#undef glTextureSubImage1D
#define glTextureSubImage1D (gl.TextureSubImage1D)
#undef glTextureSubImage2D
#define glTextureSubImage2D (gl.TextureSubImage2D)
#undef glTextureSubImage3D
#define glTextureSubImage3D (gl.TextureSubImage3D)
#undef glTransformFeedbackBufferBase
#define glTransformFeedbackBufferBase (gl.TransformFeedbackBufferBase)
#undef glTransformFeedbackBufferRange
#define glTransformFeedbackBufferRange (gl.TransformFeedbackBufferRange)
#undef glUnmapNamedBuffer
#define glUnmapNamedBuffer (gl.UnmapNamedBuffer)
#undef glVertexArrayAttribBinding
#define glVertexArrayAttribBinding (gl.VertexArrayAttribBinding)
#undef glVertexArrayAttribFormat
#define glVertexArrayAttribFormat (gl.VertexArrayAttribFormat)
#undef glVertexArrayAttribIFormat
#define glVertexArrayAttribIFormat (gl.VertexArrayAttribIFormat)
#undef glVertexArrayAttribLFormat
#define glVertexArrayAttribLFormat (gl.VertexArrayAttribLFormat)
#undef glVertexArrayBindingDivisor
#define glVertexArrayBindingDivisor (gl.VertexArrayBindingDivisor)
#undef glVertexArrayElementBuffer
#define glVertexArrayElementBuffer (gl.VertexArrayElementBuffer)
#undef glVertexArrayVertexBuffer
#define glVertexArrayVertexBuffer (gl.VertexArrayVertexBuffer)
#undef glVertexArrayVertexBuffers
#define glVertexArrayVertexBuffers (gl.VertexArrayVertexBuffers)

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



/* Functions generated: 12 OSMesa + 2838 GL + 2 GLU */

#endif /* __NFOSMESA_H__ */
