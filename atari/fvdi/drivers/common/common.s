*-------------------------------------------------------*
*	fVDI->driver interface (assembly routines)	*	
*-------------------------------------------------------*
*
* Copyright 1997-2000, Johan Klockars 
* This software is licensed under the GNU General Public License.
* Please, see LICENSE.TXT for further information.

both		equ	1	; Write in both FastRAM and on screen
upto8		equ	0	; Handle 8 bit drawing

	include		"vdi.inc"

  ifne lattice
	include	"macros.dev"
  else
	include	"macros.tas"
  endc

	xdef		_line
	xdef		_set_pixel
	xdef		_get_pixel
	xdef		_expand
	xdef		_fill
	xdef		_fillpoly
	xdef		_blit
	xdef		_text
	xdef		_mouse
	xdef		_set_palette
	xdef		_initialize_palette
	xdef		get_colour_masks

	xref		_line_draw,_write_pixel,_read_pixel,_expand_area
	xref		_fill_area,_fill_polygon,_blit_area,_text_area,_mouse_draw
	xref		_set_colours
	xref		_colour
	xref		_fallback_line,_fallback_text,_fallback_fill
	xref		_fallback_fillpoly,_fallback_expand,_fallback_blit
	xref		clip_line


	text

	dc.b		"set_pixel",0
*---------
* Set a coloured pixel
* In:	a0	VDI struct, destination MFDB (odd address marks table operation)
*	d0	colour
*	d1	x or table address
*	d2	y or table length (high) and type (0 - coordinates)
* Call:	a1	VDI struct, destination MFDB (odd address marks table operation)
*	d0	colour
*	d1	x or table address
*	d2	y or table length (high) and type (0 - coordinates)
*---------
_set_pixel:
set_pixel:
	movem.l		d0-d7/a0-a6,-(sp)	; Used to have -3/4/6 for normal/both/upto8

	move.l		a0,a1

	bsr		_write_pixel
	tst.l		d0
	bgt		.write_done

	tst.w		d2
	bne		.write_done		; Only straight coordinate tables available so far
	move.l		8*4(a7),d3		; Fetch a0
	bclr		#0,d3
	move.l		d3,-(a7)
	move.l		4+4(a7),-(a7)		; Fetch d1
	move.w		8+8(a7),-(a7)		; Fetch d2 (high)
.write_loop:
	move.l		2(a7),a2
	move.w		(a2)+,d1
	move.w		(a2)+,d2
	move.l		a2,2(a7)
	move.l		6(a7),a1
	move.l		10+0(a7),d0		; Fetch d0
	bsr		_write_pixel
	subq.w		#1,(a7)
	bne		.write_loop
	add.w		#10,a7
.write_done:

	movem.l		(sp)+,d0-d7/a0-a6
	rts


	dc.b		"get_pixel",0
*---------
* Get a coloured pixel
* In:	a0	VDI struct, source MFDB
*	d1	x
*	d2	y
* Call:	a1	VDI struct, source MFDB
*	d1	x
*	d2	y
* Out:	d0	line colour
*---------
_get_pixel:
get_pixel:
	movem.l		d1-d7/a0-a6,-(sp)	; Used to have -3/4/6 for normal/both/upto8

	move.l		a0,a1
	
	bsr		_read_pixel

	movem.l		(sp)+,d1-d7/a0-a6
	rts


	dc.b		"line"
*---------
* Draw a colored line between 2 points
* In:	a0	VDI struct (odd address marks table operation)
*	d0	colour
*	d1	x1 or table address
*	d2	y1 or table length (high) and type (0 - coordinate pairs, 1 - pairs+moves)
*	d3	x2 or move point count
*	d4	y2 or move index address
*	d5	pattern
* Call:	a1	VDI struct (odd adress marks table operation)
*	d0	logic operation
*	d1	x1 or table address
*	d2	y1 or table length (high) and type (0 - coordinate pairs, 1 - pairs+moves)
*	d3	x2 or move point count
*	d4	y2 or move index address
*	d5	pattern
*	d6	colour
*---------
_line:
line:
	movem.l		d0-d7/a0-a6,-(sp)	; Used to have -3/4/6 for normal/both/upto8

	move.l		a0,a1

	move.l		d0,d6

	move.l		a0,d0				; Must even out a0!!!  [010109]
	and.b		#$fe,d0
	move.l		d0,a0
	moveq		#0,d0
	move.w		vwk_mode(a0),d0		; Probably not a good idea to do here!!!!
	move.l		a1,a0

	bsr		_line_draw
	tst.l		d0
;	bgt		1$
	lbgt	.l1,1
;	bmi		2$
	lbmi	.l2,2
	move.l		_fallback_line,d0
	bra		give_up
;1$:
 label .l1,1

	movem.l		(sp)+,d0-d7/a0-a6
	rts

;2$:					; Transform multiline to single ones
 label .l2,2
	move.w		8+2(a7),d0
;	tst.w		d0
;	bne		.line_done		; Only coordinate pairs available so far
	move.w		d3,d7
	cmp.w		#1,d0
	bhi		.line_done		; Only coordinate pairs and pairs+marks available so far
	beq		.use_marks
	moveq		#0,d7			; Move count
.use_marks:
	swap		d7
	move.w		#1,d7			; Currrent index in high word
	swap		d7

	move.l		8*4(a7),d3		; Fetch a0
	bclr		#0,d3
	move.l		d3,a1
	move.l		1*4(a7),a2		; Table address

	move.l		d4,a6
	tst.w		d7
	beq		.no_start_move
	add.w		d7,a6
	add.w		d7,a6
	subq.l		#2,a6
	cmp.w		#-4,(a6)
	bne		.no_start_movex
	subq.l		#2,a6
	subq.w		#1,d7
.no_start_movex:
	cmp.w		#-2,(a6)
	bne		.no_start_move
	subq.l		#2,a6
	subq.w		#1,d7
.no_start_move:
	bra		.loop_end
.line_loop:
	movem.w		(a2),d1-d4
	move.l		a1,a0
	bsr		clip_line
	bvs		.no_draw
	move.l		0(a7),d6		; Colour
	move.l		5*4(a7),d5		; Pattern
	move.w		vwk_mode(a0),d0		; Probably not a good idea to do here!!!!
	movem.l		d7/a1-a2/a6,-(a7)
	bsr		_line_draw
	movem.l		(a7)+,d7/a1-a2/a6
.no_draw:
	tst.w		d7
	beq		.no_marks
	swap		d7
	addq.w		#1,d7
	move.w		d7,d4
	add.w		d4,d4
	subq.w		#4,d4
	cmp.w		(a6),d4
	bne		.no_move
	subq.l		#2,a6
	addq.w		#1,d7
	swap		d7
	subq.w		#1,d7
	swap		d7
	addq.l		#4,a2
	subq.w		#1,2*4(a7)
.no_move:
	swap		d7
.no_marks:
	addq.l		#4,a2
.loop_end:
	subq.w		#1,2*4(a7)
	bgt		.line_loop
.line_done:
	movem.l		(sp)+,d0-d7/a0-a6
	rts


	dc.b		"expand"
*---------
* Expand a monochrome area to multiple bitplanes
* In:	a0	VDI struct, destination MFDB, VDI struct, source MFDB
*	d0	colour
*	d1-d2	x1,y1 source
*	d3-d6	x1,y1 x2,y2 destination
*	d7	logic operation
* Call:	a1	VDI struct, destination MFDB, VDI struct, source MFDB
*	d0	height and width to move (high and low word)
*	d1-d2	source coordinates
*	d3-d4	destination coordinates
*	d6	background and foreground colour
*	d7	logic operation
*---------
_expand:
expand:
	movem.l		d0-d7/a0-a6,-(sp)	; Used to have -3(/6)/4(/6)/6 for normal/both/upto8

	move.l		a0,a1

	exg		d0,d6
	sub.w		d4,d0
	addq.w		#1,d0
	swap		d0
	move.w		d5,d0
	sub.w		d3,d0
	addq.w		#1,d0

	bsr		_expand_area
	tst.l		d0
;	bgt		1$
	lbgt	.l1,1
	move.l		_fallback_expand,d0
	bra		give_up
;1$:
 label .l1,1

	movem.l		(sp)+,d0-d7/a0-a6
	rts


	dc.b		"fill"
*---------
* Fill a multiple bitplane area using a monochrome pattern
* In:	a0	VDI struct (odd address marks table operation)
*	d0	colour
*	d1	x1 destination or table address
*	d2	y1    - " -    or table length (high) and type (0 - y/x1/x2 spans)
*	d3-d4	x2,y2 destination
*	d5	pattern address
* Call:	a1	VDI struct (odd address marks table operation)
*	d0	height and width to fill (high and low word)
*	d1	x or table address
*	d2	y or table length (high) and type (0 - y/x1/x2 spans)
*	d3	pattern address
*	d4	colour
**	+colour in a really dumb way...
*---------
_fill:
fill:
	movem.l		d0-d7/a0-a6,-(sp)	; Used to have -3/4/6 for normal/both/upto8

	move.l		a0,a1

; The colour fetching has been expanded to put the address
; of the background colour on the stack.
; That's needed for non-solid replace mode.
; None of this is any kind of good idea except for bitplanes!

	ifne 0
; ******************** Should probably use get_colour_masks like the rest **************
	lea		_colour,a3
	ifne	upto8
;	move.w		d0,d5
;	lsr.w		#1,d5		; (d5 >> 4) << 3
;	and.w		#$0078,d5
;	move.l		(a3,d5.w),a5
;	move.l		4(a3,d5.w),a6
	move.l		d0,d5
	lsr.l		#1,d5		; (d5 >> 4) << 3
	and.l		#$00780078,d5
	swap		d5
	pea		(a3,d5.w)
	swap		d5
	move.l		(a3,d5.w),a5
	move.l		4(a3,d5.w),a6
	endc
;	and.w		#$000f,d0
;	lsl.w		#3,d0
;	move.l		(a3,d0.w),a2
;	move.l		4(a3,d0.w),a3
	and.l		#$000f000f,d0
	lsl.l		#3,d0
	swap		d0
	pea		0(a3,d0.w)
	swap		d0
	move.l		0(a3,d0.w),a2
	move.l		4(a3,d0.w),a3
	endc

	exg		d4,d0
;	move.w		d4,d0
	sub.w		d2,d0
	addq.w		#1,d0
	swap		d0
	move.w		d3,d0
	sub.w		d1,d0
	addq.w		#1,d0

	move.l		d5,d3

	bsr		_fill_area
	tst.l		d0
;	bgt		1$
	lbgt	.l1,1
;	bmi		2$
	lbmi	.l2,2
	ifne 0
	ifeq	upto8
	addq.l		#4,a7
	endc
	ifne	upto8
	addq.l		#8,a7
	endc
	endc
	move.l		_fallback_fill,d0
	bra		give_up
;1$:
 label .l1,1
	ifne 0
	ifeq	upto8
	addq.l		#4,a7
	endc
	ifne	upto8
	addq.l		#8,a7
	endc
	endc
	movem.l		(sp)+,d0-d7/a0-a6
	rts

;2$:				; Transform table fill into ordinary one
 label .l2,2
	move.w		8+2(a7),d0
	tst.w		d0
	bne		.fill_done		; Only y/x1/x2 spans available so far
	move.l		8*4(a7),d3		; Fetch a0
	bclr		#0,d3
	move.l		d3,a1
	move.l		4(a7),a2		; Fetch d1
.fill_loop:
	moveq		#0,d2
	move.w		(a2)+,d2
	moveq		#0,d1
	move.w		(a2)+,d1
	moveq		#1,d0
	swap		d0
	move.w		(a2)+,d0
	sub.w		d1,d0
	addq.w		#1,d0
	move.l		5*4(a7),d3
	move.l		0(a7),d4
	movem.l		a1-a2,-(a7)
	bsr		_fill_area
	movem.l		(a7)+,a1-a2
	subq.w		#1,2*4(a7)
	bne		.fill_loop
.fill_done:
	movem.l	(a7)+,d0-d7/a0-a6
	rts


	dc.b		"fillpoly"
*---------
* Fill a multiple bitplane polygon using a monochrome pattern
* In:	a0	VDI struct (odd address marks table operation)
*	d0	colour
*	d1	points address
*	d2	number of points
*	d3	index address
*	d4	number of indices
*	d5	pattern address
* Call:	a1	VDI struct (odd address marks table operation)
*	d0	number of points and indices (high and low word)
*	d1	points address
*	d2	index address
*	d3	pattern address
*	d4	colour
**	+colour in a really dumb way...
*---------
_fillpoly:
fillpoly:
	movem.l		d0-d7/a0-a6,-(sp)	; Used to have -3/4/6 for normal/both/upto8

	move.l		a0,a1

; The colour fetching has been expanded to put the address
; of the background colour on the stack.
; That's needed for non-solid replace mode.
; None of this is any kind of good idea except for bitplanes!

	ifne 0
; ******************** Should probably use get_colour_masks like the rest **************
	lea		_colour,a3
	ifne	upto8
;	move.w		d0,d5
;	lsr.w		#1,d5		; (d5 >> 4) << 3
;	and.w		#$0078,d5
;	move.l		(a3,d5.w),a5
;	move.l		4(a3,d5.w),a6
	move.l		d0,d5
	lsr.l		#1,d5		; (d5 >> 4) << 3
	and.l		#$00780078,d5
	swap		d5
	pea		(a3,d5.w)
	swap		d5
	move.l		(a3,d5.w),a5
	move.l		4(a3,d5.w),a6
	endc
;	and.w		#$000f,d0
;	lsl.w		#3,d0
;	move.l		(a3,d0.w),a2
;	move.l		4(a3,d0.w),a3
	and.l		#$000f000f,d0
	lsl.l		#3,d0
	swap		d0
	pea		0(a3,d0.w)
	swap		d0
	move.l		0(a3,d0.w),a2
	move.l		4(a3,d0.w),a3
	endc

	swap	d2
	move.w	d4,d2
	move.l	d0,d4
	move.l	d2,d0
	move.l	d3,d2
	move.l	d5,d3

	bsr		_fill_polygon
	tst.l		d0
;	bgt		1$
	lbgt	.l1,1
;	bmi		2$
	lbmi	.l2,2
;2$:
 label .l2,2
	ifne 0
	ifeq	upto8
	addq.l		#4,a7
	endc
	ifne	upto8
	addq.l		#8,a7
	endc
	endc
	move.l		_fallback_fillpoly,d0
	bra		give_up
;1$:
 label .l1,1
	ifne 0
	ifeq	upto8
	addq.l		#4,a7
	endc
	ifne	upto8
	addq.l		#8,a7
	endc
	endc
	movem.l		(sp)+,d0-d7/a0-a6
	rts


	dc.b		"blit"
*---------
* Blit an area
* In:	a0	VDI struct, destination MFDB, VDI struct, source MFDB
*	d0	logic operation
*	d1-d2	x1,y1 source
*	d3-d6	x1,y1 x2,y2 destination
* Call:	a1	VDI struct,destination MFDB, VDI struct, source MFDB
*	d0	height and width to move (high and low word)
*	d1-d2	source coordinates
*	d3-d4	destination coordinates
*	d5	logic operation
*---------
_blit:
blit:
	movem.l		d0-d7/a0-a6,-(sp)	; Used to have -3/4/6 for normal/both/upto8

	move.l		a0,a1

	move.l		d0,d7

	move.w		d6,d0
	sub.w		d4,d0
	addq.w		#1,d0
	swap		d0
	move.w		d5,d0
	sub.w		d3,d0
	addq.w		#1,d0

	bsr		_blit_area
	tst.l		d0
;	bgt		1$
	lbgt	.l1,1
	move.l		_fallback_blit,d0
	bra		give_up
;1$:
 label .l1,1

	movem.l		(sp)+,d0-d7/a0-a6
	rts


	dc.b		"text"
*---------
* Draw some text
* In:	a0	VDI struct
*	a1	string address
*	a2	offset table or zero
*	d0	string length
*	d1	x1,y1 destination
* Call:	a1	VDI struct
*	a2	offset table or zero
*	d0	string length
*	d3-d4	destination coordinates
*	a4	string address
*---------
_text:
text:
	movem.l		d0-d7/a0-a6,-(sp)	; Was d2-d7/a3-a6

	move.l		a1,a4
	move.l		a0,a1

	move.w		d1,d4
	swap		d1
	move.w		d1,d3

	bsr		_text_area
	tst.l		d0
;	bgt		1$
	lbgt	.l1,1
	move.l		_fallback_text,d0
	bra		give_up
;1$:
 label .l1,1

	movem.l		(sp)+,d0-d7/a0-a6
	rts


	dc.b		"mouse",0
*---------
* Draw the mouse
* In:	a1	Pointer to Workstation struct
*	d0/d1	x,y
*	d2	0 (move), 1 (hide), 2 (show), Mouse* (change)
* Call:	a1	Pointer to Workstation struct
*	d0/d1	x,y
*	d2	0 (move), 1 (hide), 2 (show), Mouse* (change)
*---------
_mouse:
mouse:
	bsr		_mouse_draw
	rts


	dc.b		"set_palette",0
*---------
* Set palette colours
* In:	a0	VDI struct
*	d0	number of entries, start entry
*	a1	requested colour values (3 word/entry)
*	a2	colour palette
*---------
_set_palette:
set_palette:
	movem.l		d0-d7/a0-a6,-(sp)	; Overkill

	bsr		_set_colours

	movem.l		(sp)+,d0-d7/a0-a6
	rts


	ifne	1
	dc.b		"initialize_palette"
*---------
* Set palette colours
* initialize_palette(Virtual *vwk, int start, int n, int requested[][3], Colour palette[])
* To be called from C
*---------
_initialize_palette:
	movem.l		d0-d7/a0-a6,-(sp)	; Overkill

	move.l		15*4+4(a7),a0
	move.l		15*4+8(a7),d1
	move.l		15*4+12(a7),d0
	swap		d0
	move.w		d1,d0
	move.l		15*4+16(a7),a1
	move.l		15*4+20(a7),a2

	bsr		_set_colours

	movem.l		(sp)+,d0-d7/a0-a6
	rts
	endc


*---------
* Get colour masks
* get_colour_masks(int colour)
* In:	d0	background colour, foreground colour
* Out:	a2-a3	First four colour bits
*	a5-a6	Last four colour bits (only when 'upto8')
*	d0	Pointer to colour bits for background
* XXX:	d0
*---------
get_colour_masks:
	lea		_colour,a3
	ifne	upto8
	move.w		d0,a2
	lsr.l		#1,d0		; (d5 >> 4) << 3
	and.l		#$00780078,d0
	move.l		(a3,d0.w),a5
	move.l		4(a3,d0.w),a6
	move.w		a2,d0
	endc
	and.l		#$000f000f,d0
	lsl.l		#3,d0
	swap		d0
	pea		0(a3,d0.w)
	swap		d0
	move.l		0(a3,d0.w),a2
	move.l		4(a3,d0.w),a3
	move.l		(a7)+,d0
	rts


*---------
* Give up and try other function
* This routine should only be branched to, it's not a subroutine!
* In:	d0	Address to other function
* Call:	d0-a6	Same values as at original call
*---------
give_up:
	pea	.return
	move.l	d0,-(a7)
	movem.l	8(a7),d0-d7/a0-a6
	rts
.return:
	movem.l	(a7)+,d0-d7/a0-a6
	rts

	end
