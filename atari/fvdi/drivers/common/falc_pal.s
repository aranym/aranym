*-------------------------------------------------------*
*	Set Falcon palette colours			*	
*-------------------------------------------------------*
*
* Copyright 1997-2000, Johan Klockars 
* This software is licensed under the GNU General Public License.
* Please, see LICENSE.TXT for further information.

	include		"vdi.inc"

colour_bits	equ	8

	xdef		_set_colours
	xdef		get_colour,_get_colour

;	xref		_bitplane_palette_colour_mode


	text

* Never called
* Just here to make sure no bad linking is done
;dummy:
;	move.w	_bitplane_palette_colour_mode,d0
;	rts


* Converts colours from VDI to TOS format
* Todo:	This should not use a fixed table!
* In:	a0	VDI struct
*	d0	VDI colours in high and low word
* Out:	d0	TOS colours in high and low word
_get_colour:
get_colour:
	move.l	a1,-(a7)
	lea	tos_colours,a1
	move.b	0(a1,d0.w),d0
	swap	d0
	move.b	0(a1,d0.w),d0
	swap	d0
	move.l	(a7)+,a1
	rts


* In:	a0		VDI struct
*	d0		number of entries and start entry
*	a1		Requested colour values (3 word/entry)
*	a2		VDI palette array
_set_colours:
	move.w	d0,d1
	mulu	#colour_struct_size,d1
	add.w	d1,a2
	bra	.loop1_end
.loop1:
	swap	d0
	moveq	#2,d1
	moveq	#0,d2
	bra	.loop_start
.loop:
	lsl.l	#colour_bits,d2
.loop_start:
	move.w	(a1)+,d3
	move.w	d3,(a2)+
	mulu	#(1<<colour_bits)-1,d3
	add.l	#500,d3
	divu	#1000,d3
	move.w	d3,4(a2)	; This is not at all what should be there
	or.b	d3,d2
	dbra	d1,.loop
	add.w	#colour_struct_size-colour_hw,a2
;	movem.l	d0-d2/a0-a2,-(a7)
;	pea	-4(a2)
;	move.w	#1,-(a7)
;	move.w	d0,-(a7)
;	move.w	#xxxx,-(a7)
;	trap	#xxx
;	addq.l	#10,a7
;	movem.l	(a7)+,d0-d2/a0-a2
	addq.w	#1,d0
.loop1_end:
	swap	d0
	dbra	d0,.loop1	

	rts


vdi_colours:
	dc.b	0,2,3,6,4,7,5,8,9,10,11,14,12,15,13,1

tos_colours:
	dc.b	0,15,1,2,4,6,3,5,7,8,9,10,12,14,11,13

	end
