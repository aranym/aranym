*-------------------------------------------------------*
*	Draw in single plane modes			*	
*-------------------------------------------------------*

	include		"..\..\vdi.inc"

	xdef		_write_pixel
	xdef		write_pixel
	xdef		_read_pixel
	xdef		read_pixel
	xdef		_mouse_draw
	xdef		mouse_draw
	xdef		_line_draw
	xdef		line_draw
	xdef		_expand_area
	xdef		expand_area
	xdef		_blit_area
	xdef		blit_area
	xdef		_fill_area
	xdef		fill_area


		
	text

MACRO	FVDI_DISPATCH.w	opcode
	move.l	#opcode,-(sp)
	dc.w	0x7119				;M68K_EMUL_OP_VIDEO_CONTROL
ENDM


*----------
* Get pixel
*----------
* In:	a1	VDI struct, source MFDB
*	d1	x
*	d2	y
_read_pixel:
read_pixel:
;begin // STanda
	FVDI_DISPATCH	1
	rts

*----------
* Set pixel
*----------
* In:	a1	VDI struct, destination MFDB (odd address marks table operation)
*	d0	colour
*	d1	x or table address
*	d2	y or table length (high) and type (0 - coordinates)
* XXX:	?
_write_pixel:
write_pixel:
;begin // STanda
	FVDI_DISPATCH	2
	rts
;end //STanda

_mouse_draw:
mouse_draw:
	FVDI_DISPATCH	3
	rts

_expand_area:
expand_area:
	FVDI_DISPATCH	4
	rts

_fill_area:
fill_area:
	FVDI_DISPATCH	5
	rts

_blit_area:
blit_area:
	FVDI_DISPATCH	6
	rts

_line_draw:
line_draw:
	FVDI_DISPATCH	7
	rts

;end //STanda

		
	end
