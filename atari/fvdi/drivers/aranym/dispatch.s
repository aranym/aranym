*-------------------------------------------------------*
*	Drawing function dispatch			*	
*-------------------------------------------------------*
	include		"vdi.inc"

	xdef		_write_pixel
	xdef		_read_pixel
	xdef		_mouse_draw
	xdef		_line_draw
	xdef		_expand_area
	xdef		_blit_area
	xdef		_fill_area
	xdef		_fill_polygon
	xdef		_set_colour_hook
	xdef		_set_resolution
	xdef		_debug_aranym


  ifne lattice
FVDI_DISPATCH	macro	opcode
	move.l	#\1,-(a7)
	dc.w	0x7119				; M68K_EMUL_OP_VIDEO_CONTROL
	endm
  else
	macro	FVDI_DISPATCH.w	opcode
	move.l	#opcode,-(a7)
	dc.w	0x7119				; M68K_EMUL_OP_VIDEO_CONTROL
	endm
  endc

	text

_read_pixel:
	FVDI_DISPATCH	1
	rts

_write_pixel:
	FVDI_DISPATCH	2
	rts

_mouse_draw:
	FVDI_DISPATCH	3
	rts

_expand_area:
	FVDI_DISPATCH	4
	rts

_fill_area:
	FVDI_DISPATCH	5
	rts

_blit_area:
	FVDI_DISPATCH	6
	rts

_line_draw:
	FVDI_DISPATCH	7
	rts

_fill_polygon:
	FVDI_DISPATCH	8
	rts

_set_colour_hook:
	FVDI_DISPATCH	9
	rts

_set_resolution:
	FVDI_DISPATCH	10
	rts

_debug_aranym:
	FVDI_DISPATCH	20
	rts
		
	end

