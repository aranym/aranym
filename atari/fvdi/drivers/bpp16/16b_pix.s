*-------------------------------------------------------*
*	Draw in single plane modes			*	
*-------------------------------------------------------*

	include		"..\..\vdi.inc"

	xdef		_write_pixel
	xdef		write_pixel
	xdef		_read_pixel
	xdef		read_pixel


		
	text

MACRO	FVDI_DISPATCH.w	opcode
	move.l	#opcode,-(sp)
	dc.w	0x7119				;M68K_EMUL_OP_VIDEO_CONTROL
ENDM


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
;end //STanda

		
	end
