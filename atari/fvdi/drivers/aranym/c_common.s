*-------------------------------------------------------*
*	fVDI->driver interface (C functions)		*	
*-------------------------------------------------------*

	include		"vdi.inc"

  ifne lattice
	include		"macros.dev"
  else
	include		"macros.tas"
  endc

	xdef		_c_line
	xdef		_c_set_pixel
	xdef		_c_get_pixel
	xdef		_c_expand
	xdef		_c_fill
	xdef		_c_fillpoly
	xdef		_c_blit
	xdef		_c_text
	xdef		_c_mouse
	xdef		_c_set_palette
	xdef		_c_get_colour
	xdef		_c_initialize_palette

  ifne 0
	xref		_line_draw_r,_write_pixel_r,_read_pixel_r,_expand_area_r
	xref		_fill_area_r,_fill_poly_r,_blit_area_r,_text_area_r
	xref		_mouse_draw_r,_set_colours_r,_get_colour_r
  else
	xref		_c_line_draw,_c_write_pixel,_c_read_pixel,_c_expand_area
	xref		_c_fill_area,_c_fill_polygon,_c_blit_area,_c_text_area
	xref		_c_mouse_draw,_set_colours_r,_get_colour_r
  endc
	xref		_fallback_line,_fallback_text,_fallback_fill
	xref		_fallback_fillpoly,_fallback_expand,_fallback_blit
	xref		clip_line

	text

	dc.b		"set_pixel",0
*---------
* Set a coloured pixel
* c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour)
* In:	a0	VDI struct, destination MFDB (odd address marks table operation)
*	d0	pixel colour
*	d1	x or table address
*	d2	y or table length (high) and type (0 - coordinates)
*---------
_c_set_pixel:
c_set_pixel:
	movem.l		d0-d2/a0-a2,-(a7)

	ext.l		d1
	ext.l		d2
	move.l		d0,-(a7)
	move.l		d2,-(a7)
	move.l		d1,-(a7)
	move.l		4(a0),-(a7)
	move.l		(a0),-(a7)
	jsr		_c_write_pixel
	tst.l		d0
	bgt		.write_done

	tst.w		d2
	bne		.write_done			; Only straight coordinate tables available so far
	move.l		20+3*4(a7),d3		; Fetch a0
	bclr		#0,d3
	move.l		d3,0(a7)
.write_loop:
	move.l		20+5*4(a7),a2
	moveq		#0,d0
	move.w		(a2)+,d0
	move.l		d0,8(a7)
	move.w		(a2)+,d0
	move.l		d0,12(a7)
	move.l		a2,20+5*4(a7)
	jsr		_c_write_pixel
	subq.w		#1,2*4(a7)
	bne		.write_loop

.write_done:
	add.w		#20,a7
	movem.l		(a7)+,d0-d2/a0-a2
	rts


	dc.b		"get_pixel",0
*---------
* Get a coloured pixel
* c_read_pixel(Virtual *vwk, MFDB *mfdb, long x, long y)
* In:	a0	VDI struct, source MFDB
*	d1	x
*	d2	y
* Out:	d0	pixel colour
*---------
_c_get_pixel:
c_get_pixel:
	movem.l		d1-d2/a0-a2,-(a7)

	ext.l		d1
	ext.l		d2
	move.l		d2,-(a7)
	move.l		d1,-(a7)
	move.l		4(a0),-(a7)
	move.l		(a0),-(a7)
	jsr		_c_read_pixel
	add.w		#16,a7

	movem.l		(a7)+,d1-d2/a0-a2
	rts


	dc.b		"line"
*---------
* Draw a colored line between 2 points
* c_draw_line(Virtual *vwk, long x1, long y1, long x2, long y2, long pattern, long colour)
* In:	a0	VDI struct (odd address marks table operation)
*	d0	line colour
*	d1	x1 or table address
*	d2	y1 or table length (high) and type (0 - coordinate pairs, 1 - pairs+moves)
*	d3	x2 or move point count
*	d4	y2 or move index address
*	d5	pattern
*---------
_c_line:
c_line:
	movem.l		d0-d2/a0-a2,-(a7)

;	ext.l		d1
;	ext.l		d2
	move.l		d0,-(a7)
;	move.w		d5,d0
;	ext.l		d0
	move.l		d5,-(a7)
;	move.w		d4,d0
;	ext.l		d0
	move.l		d4,-(a7)
;	move.w		d3,d0
;	ext.l		d0
	move.l		d3,-(a7)
	move.l		d2,-(a7)
	move.l		d1,-(a7)
	move.l		a0,-(a7)
	jsr		_c_line_draw
	add.w		#28,a7

	tst.l		d0
;	bgt		1$
	lbgt		.l1,1
;	bmi		2$
	lbmi		.l2,2
	move.l		_fallback_line,d0
	bra		give_up
;1$:
 label .l1,1

	movem.l		(a7)+,d0-d2/a0-a2
	rts

;2$:				; Transform multiline into single ones
 label .l2,2
	movem.l		d3-d4/d7/a6,-(a7)
	move.w		16+2*4+2(a7),d0
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

	move.l		16+3*4(a7),d3		; Fetch a0
	bclr		#0,d3
;	move.l		16+0(a7),d0

	sub.w		#28,a7
	move.l		d3,0(a7)
	move.l		d5,20(a7)
;	move.l		d0,24(a7)

	move.l		d4,a6
	tst.w		d7
	beq		.no_start_move
	add.w		d7,a6
	add.w		d7,a6
	subq.l		#2,a6
	cmp.w		#-4,(a6)
	bne		.no_start_movex
	subq.l		#2,a6
	sub.w		#1,d7
.no_start_movex:
	cmp.w		#-2,(a6)
	bne		.no_start_move
	subq.l		#2,a6
	sub.w		#1,d7
.no_start_move:
	bra		.loop_end
.line_loop:
	move.l		28+16+4(a7),a2
	movem.w		(a2),d1-d4
	move.l		0(a7),a0
	bsr		clip_line
	bvs		.no_draw
	move.l		d1,4(a7)
	move.l		d2,8(a7)
	move.l		d3,12(a7)
	move.l		d4,16(a7)
	move.l		28+16+0(a7),24(a7)
	jsr		_c_line_draw
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
	addq.l		#4,28+16+4(a7)
	subq.w		#1,28+16+2*4(a7)
.no_move:
	swap		d7
.no_marks:
	addq.l		#4,28+16+4(a7)
.loop_end:
	subq.w		#1,28+16+2*4(a7)
	bgt		.line_loop
	add.w		#28,a7
.line_done:
	movem.l		(a7)+,d3-d4/d7/a6
	movem.l		(a7)+,d0-d2/a0-a2
	rts


	dc.b		"expand"
*---------
* Expand a monochrome area to multiple bitplanes
* c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour)
* In:	a0	VDI struct, destination MFDB, VDI struct, source MFDB
*	d0	colours
*	d1-d2	x1,y1 source
*	d3-d6	x1,y1 x2,y2 destination
*	d7	logic operation
*---------
_c_expand:
c_expand:
	movem.l		d0-d2/a0-a2,-(a7)

	ext.l		d1
	ext.l		d2
	move.l		d0,-(a7)
	ext.l		d7
	move.l		d7,-(a7)

	move.w		d6,d0
	sub.w		d4,d0
	addq.w		#1,d0
	ext.l		d0
	move.l		d0,-(a7)
	move.w		d5,d0
	sub.w		d3,d0
	addq.w		#1,d0
	ext.l		d0
	move.l		d0,-(a7)

	move.w		d4,d0
	ext.l		d0
	move.l		d0,-(a7)
	move.w		d3,d0
	ext.l		d0
	move.l		d0,-(a7)
	move.l		4(a0),-(a7)
	move.l		d2,-(a7)
	move.l		d1,-(a7)
	move.l		12(a0),-(a7)
	move.l		(a0),-(a7)

	jsr		_c_expand_area
	add.w		#44,a7

	tst.l		d0
;	bgt		1$
	lbgt		.l1,1
	move.l		_fallback_expand,d0
	bra		give_up
;1$:
 label .l1,1

	movem.l		(a7)+,d0-d2/a0-a2
	rts


	dc.b		"fill"
*---------
* Fill a multiple bitplane area using a monochrome pattern
* c_fill_area(Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour)
* In:	a0	VDI struct (odd address marks table operation)
*	d0	colours
*	d1	x1 destination or table address
*	d2	y1    - " -    or table length (high) and type (0 - y/x1/x2 spans)
*	d3-d4	x2,y2 destination
*	d5	pattern address
*---------
_c_fill:
c_fill:
	movem.l		d0-d2/a0-a2,-(a7)

;	ext.l		d1				; Perhaps these really should be extended when a0 even
;	ext.l		d2
	move.l		d0,-(a7)
	move.l		d5,-(a7)

	move.w		d4,d0
	sub.w		d2,d0
	addq.w		#1,d0
	ext.l		d0
	move.l		d0,-(a7)
	move.w		d3,d0
	sub.w		d1,d0
	addq.w		#1,d0
	ext.l		d0
	move.l		d0,-(a7)

	move.l		d2,-(a7)
	move.l		d1,-(a7)
	move.l		a0,-(a7)

	jsr		_c_fill_area
	tst.l		d0
;	bgt		1$
	lbgt		.l1,1
;	bmi		2$
	lbmi		.l2,2
	add.w		#28,a7
	move.l		_fallback_fill,d0
	bra		give_up
;1$:
 label .l1,1
	add.w		#28,a7
	movem.l		(a7)+,d0-d2/a0-a2
	rts

;2$:				; Transform table fill into ordinary one
 label .l2,2
	move.w		28+8+2(a7),d0
	tst.w		d0
	bne		.fill_done		; Only y/x1/x2 spans available so far
	move.l		28+3*4(a7),d3		; Fetch a0
	bclr		#0,d3
	move.l		d3,0(a7)
	move.l		#1,16(a7)		; Always 1 high
.fill_loop:
	move.l		28+4(a7),a2
	moveq		#0,d0
	move.w		(a2)+,d0
	move.l		d0,8(a7)
	move.w		(a2)+,d0
	move.l		d0,4(a7)
	sub.w		(a2)+,d0
	neg.w		d0
	addq.w		#1,d0
	move.l		d0,12(a7)
	move.l		a2,28+4(a7)
	jsr		_c_fill_area
	subq.w		#1,28+8(a7)
	bne		.fill_loop
.fill_done:
	add.w	#28,a7
	movem.l	(a7)+,d0-d2/a0-a2
	rts


	dc.b		"fillpoly"
*---------
* Fill a multiple bitplane polygon using a monochrome pattern
* c_fill_polygon(Virtual *vwk, short points[], long n, short index[], long moves, short *pattern, long colour)
* In:	a0	VDI struct
*	d0	colours
*	d1	points address
*	d2	number of points
*	d3	index address
*	d4	number of indices
*	d5	pattern address
*---------
_c_fillpoly:
c_fillpoly:
	movem.l		d0-d2/a0-a2,-(a7)

	move.l		d0,-(a7)
	move.l		d5,-(a7)

	move.w		d4,d0
	ext.l		d0
	move.l		d0,-(a7)

	move.l		d3,-(a7)

	move.w		d2,d0
	ext.l		d0
	move.l		d0,-(a7)

	move.l		d1,-(a7)
	move.l		a0,-(a7)

	jsr		_c_fill_polygon
	tst.l		d0
;	bgt		1$
	lbgt		.l1,1
;	bmi		2$
	lbmi		.l2,2
;2$:
 label .l2,2
	add.w		#28,a7
	move.l		_fallback_fillpoly,d0
	bra		give_up
;1$:
 label .l1,1
	add.w		#28,a7
	movem.l		(a7)+,d0-d2/a0-a2
	rts


	dc.b		"blit"
*---------
* Blit an area
* c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation)
* In:	a0	VDI struct, destination MFDB, VDI struct, source MFDB
*	d0	logic operation
*	d1-d2	x1,y1 source
*	d3-d6	x1,y1 x2,y2 destination
*---------
_c_blit:
c_blit:
	movem.l		d0-d2/a0-a2,-(a7)

	ext.l		d1
	ext.l		d2
	ext.l		d0
	move.l		d0,-(a7)

	move.w		d6,d0
	sub.w		d4,d0
	addq.w		#1,d0
	ext.l		d0
	move.l		d0,-(a7)
	move.w		d5,d0
	sub.w		d3,d0
	addq.w		#1,d0
	ext.l		d0
	move.l		d0,-(a7)

	move.w		d4,d0
	ext.l		d0
	move.l		d0,-(a7)
	move.w		d3,d0
	ext.l		d0
	move.l		d0,-(a7)
	move.l		4(a0),-(a7)
	move.l		d2,-(a7)
	move.l		d1,-(a7)
	move.l		12(a0),-(a7)
	move.l		(a0),-(a7)

	jsr		_c_blit_area
	add.w		#40,a7

	tst.l		d0
;	bgt		1$
	lbgt		.l1,1
	move.l		_fallback_blit,d0
	bra		give_up
;1$:
 label .l1,1

	movem.l		(a7)+,d0-d2/a0-a2
	rts


	dc.b		"text"
*---------
* Draw some text
* c_text_area(Virtual *vwk, short *text, long length, long dst_x, long dst_y, short *offsets)
* In:	a0	VDI struct
*	a1	string address
*	a2	offset table
*	d0	string length
*	d1	x1,y1 destination
*---------
_c_text:
c_text:
	movem.l		d0-d2/a0-a2,-(a7)	; Was d2

	ext.l		d0
	move.w		d1,d2
	swap		d1
	ext.l		d1
	ext.l		d2

	move.l		a2,-(a7)
	move.l		d2,-(a7)
	move.l		d1,-(a7)
	move.l		d0,-(a7)
	move.l		a1,-(a7)
	move.l		a0,-(a7)

	jsr		_c_text_area
	add.w		#24,a7

	tst.l		d0
;	bgt		1$
	lbgt		.l1,1
	move.l		_fallback_text,d0
	bra		give_up
;1$:
 label .l1,1

	movem.l		(a7)+,d0-d2/a0-a2
	rts


	dc.b		"mouse",0
*---------
* Draw the mouse
* c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse)
* In:	a1	Pointer to Workstation struct
*	d0/d1	x,y
*	d2	0 (move), 1 (hide), 2 (show), Mouse* (change)
*---------
_c_mouse:
c_mouse:
	movem.l		d0-d2/a0-a2,-(a7)

	move.l		d2,-(a7)
	ext.l		d1
	move.l		d1,-(a7)
	ext.l		d0
	move.l		d0,-(a7)
	move.l		a1,-(a7)
	jsr		_c_mouse_draw
	add.w		#16,a7

	movem.l		(a7)+,d0-d2/a0-a2
	rts


	dc.b		"set_palette",0
*---------
* Set palette colours
* c_set_colours(Virtual *vwk, long start, long entries, short requested[3][], Colour palette[])
* In:	a0	VDI struct
*	d0	number of entries, start entry
*	a1	requested colour values (3 word/entry)
*	a2	colour palette
*---------
_c_set_palette:
c_set_palette:
	movem.l		d0-d2/a0-a2,-(a7)

	move.l		a2,-(a7)
	move.l		a1,-(a7)
	move.l		d0,d1
	swap		d1
	ext.l		d1
	move.l		d1,-(a7)
	ext.l		d0
	move.l		d0,-(a7)
	move.l		a0,-(a7)

;	jsr		([_set_colours_r])
	ijsr		_set_colours_r
	add.w		#20,a7

	movem.l		(a7)+,d0-d2/a0-a2
	rts


	dc.b		"get_colour"
*---------
* Get colour
* c_get_colour(Virtual *vwk, long colours)
* In:	a0	VDI struct
*	d0	fore- and background colour
*---------
_c_get_colour:
c_get_colour:
	movem.l		d0-d2/a0-a2,-(a7)

	move.l		d0,-(a7)
	move.l		a0,-(a7)

;	jsr		([_get_colour_r])
	ijsr		_get_colour_r
	addq.l		#8,a7

	movem.l		(a7)+,d0-d2/a0-a2
	rts


	ifne	1
	dc.b		"initialize_palette"
*---------
* Set palette colours
* initialize_palette(Virtual *vwk, long start, long entries, short requested[][3], Colour palette[])
* To be called from C
*---------
_c_initialize_palette:
;	jmp		([_set_colours_r])		; Exactly the same parameters
	ijmp		_set_colours_r		; Exactly the same parameters
;	rts
	endc

*---------
* Give up and try other function
* This routine should only be branched to, it's not a subroutine!
* In:	d0	Address to other function
* Call:	d0-a6	Same values as at original call
*---------
give_up:
	pea	.return
	move.l	d0,-(a7)
	movem.l	8(a7),d0-d2/a0-a2
	rts
.return:
	movem.l	(a7)+,d0-d2/a0-a2
	rts

	end
