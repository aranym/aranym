*****
* fVDI device driver clip function
*
* Copyright 1997-2000, Johan Klockars 
* This software is licensed under the GNU General Public License.
* Please, see LICENSE.TXT for further information.
*****

	include	"vdi.inc"

	xdef	clip_line
	xdef	_clip_line

	text


* int clip_line(Virtual *vwk, short *x1, short *y1, short *x2, short *y2)
	dc.b	"clip_line",0
_clip_line:
	movem.l	d2-d4,-(a7)
	move.l	3*4+4(a7),a0
	move.l	3*4+8(a7),a1
	move.l	(a1),d1
	move.l	3*4+12(a7),a1
	move.l	(a1),d2
	move.l	3*4+16(a7),a1
	move.l	(a1),d3
	move.l	3*4+20(a7),a1
	move.l	(a1),d4
	bsr	clip_line
	svc	d0
	ext.w	d0
	ext.l	d0
	move.l	3*4+8(a7),a1
	ext.l	d1
	move.l	d1,(a1)
	move.l	3*4+12(a7),a1
	ext.l	d2
	move.l	d2,(a1)
	move.l	3*4+16(a7),a1
	ext.l	d3
	move.l	d3,(a1)
	move.l	3*4+20(a7),a1
	ext.l	d4
	move.l	d4,(a1)
	movem.l	(a7)+,d2-d4
	rts


* clip_line - Internal function
*
* Clips line coordinates
* Todo:	?
* In:	a0	VDI struct
*	d1-d2	x1,y1
*	d3-d4	x2,y2
* Out:	d1-d4	clipped coordinates
clip_line:
	movem.l		d0/d5-d7,-(a7)
	tst.w		vwk_clip_on(a0)
	beq		.end_clip

	moveq		#0,d0		; Coordinate flip flag
	move.w		vwk_clip_rectangle_x1(a0),d6
	move.w		vwk_clip_rectangle_y1(a0),d7
	sub.w		d6,d1		; Change to coordinates
	sub.w		d7,d2		;  relative to the clip rectangle
	sub.w		d6,d3
	sub.w		d7,d4

;	move.w		clip_h(pc),d7
	move.w		vwk_clip_rectangle_y2(a0),d7
	sub.w		vwk_clip_rectangle_y1(a0),d7	; d7 - max y-coordinate of clip rectangle
;	addq.w		#1,d7		; d7 - height of clip rectangle
	cmp.w		d2,d4
	bge.s		.sort_y1y2	; Make sure max y-coordinate for <d3,d4>  (Was 'bpl')
	exg		d1,d3
	exg		d2,d4
	not.w		d0		; Mark as flipped

.sort_y1y2:
	cmp.w		d7,d2
	bgt		.error		; All below screen?  (Was 'bpl', with d7++)
	move.w		d4,d6
	blt		.error		; All above screen?  (Was 'bmi')
	sub.w		d2,d6		; d6 = dy (pos)
	move.w		d3,d5
	sub.w		d1,d5		; d5 = dx
	bne.s		.no_vertical

	tst.w		d2		; Clip vertical
	bge.s		.y1in		; Line to top/bottom  (Was 'bpl')
	moveq		#0,d2
.y1in:	cmp.w		d7,d4
	ble.s		.vertical_done	; (Was 'bmi', with d7++)
	move.w		d7,d4
	bra.s		.vertical_done

.no_vertical:
	tst.w		d2
	bge.s		.y1_inside	; (Was 'bpl')
	muls.w		d5,d2		; dx * (y1 - tc)
	divs.w		d6,d2		; dx * (y1 - tc) / (y2 - y1)
	sub.w		d2,d1		; x1' = x1 - (dx * (y1 - tc)) / (y2 - y1)
	moveq		#0,d2		; y1' = ty

.y1_inside:
	sub.w		d4,d7
	bge.s		.y2_inside	; (Was 'bpl', with d7++ (which was probably wrong))
; Is a d7++ needed now when it isn't there above?
	muls.w		d7,d5		; dx * (bc - y2)
	divs.w		d6,d5		; dx * (bc - y2) / (y2 - y1)
	add.w		d5,d3		; x2' = x2 + (dx * (bc - y2)) / (y2 - y1)
	add.w		d7,d4		; y2' = by

.y2_inside:
.vertical_done:
;	move.w		clip_w(pc),d7
	move.w		vwk_clip_rectangle_x2(a0),d7
	sub.w		vwk_clip_rectangle_x1(a0),d7	; d7 - max x-coordinate of clip rectangle
;	addq.w		#1,d7		; d7 - width of clip rectangle
	cmp.w		d1,d3
	bge.s		.sort_x1x2	; Make sure max x-coordinate for <d3,d4>  (Was 'bpl')
	exg		d1,d3
	exg		d2,d4
	not.w		d0		; Mark as flipped

.sort_x1x2:
	cmp.w		d7,d1
	bgt		.error		; All right of screen?  (Was 'bpl', with d7++)
	move.w		d3,d5
	blt		.error		; All left of screen?   (Was 'bmi')
	sub.w		d1,d5		; d5 = dx (pos)
	move.w		d4,d6
	sub.w		d2,d6		; d6 = dy
	bne.s		.no_horizontal

	tst.w		d1		; Clip horizontal
	bge.s		.x1in		; Line to left/right  (Was 'bpl')
	moveq		#0,d1
.x1in:	cmp.w		d7,d3
	ble.s		.horizontal_done	; (Was 'bmi', with d7++)
	move.w		d7,d3
	bra.s		.horizontal_done

.no_horizontal:
	tst.w		d1
	bge.s		.x1_inside	; (Was 'bpl')
	muls.w		d6,d1		; dy * (x1 - lc)
	divs.w		d5,d1		; dy * (x1 - lc) / (x2 - x1)
	sub.w		d1,d2		; y1' = y1 - (dy * (x1 - lc)) / (x2 - x1)
	moveq		#0,d1		; x1' = lc

.x1_inside:
	sub.w		d3,d7
	bge.s		.x2_inside	; (Was 'bpl', with d7++ (which was probably wrong))
; Is a d7++ needed now when it isn't there above?
	muls.w		d7,d6		; dy * (rc - x2)
	divs.w		d5,d6		; dy * (rc - x2) / (x2 - x1)
	add.w		d6,d4		; y2' = y2 + (dx * (bc - y2)) / (y2 - y1)
	add.w		d7,d3		; x2' = rc

.x2_inside:
.horizontal_done:
	move.w		vwk_clip_rectangle_x1(a0),d6
	move.w		vwk_clip_rectangle_y1(a0),d7
	add.w		d6,d1		; Change back to real coordinates
	add.w		d7,d2
	add.w		d6,d3
	add.w		d7,d4

	tst.w		d0
	beq		.end_clip
	exg		d1,d3		; Flip back again if needed
	exg		d2,d4
.end_clip:
	movem.l		(a7)+,d0/d5-d7
	rts

.error:
	movem.l		(a7)+,d0/d5-d7
	move.w		#2,-(a7)	; Return with the overflow flag set
	rtr

	end
