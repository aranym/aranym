*****
* fVDI device driver graphics mode check, by Johan Klockars
*
* This file is put in the public domain.
* It's not copyrighted or under any sort of license.
*****

doaline		equ	0	; Do actual drawing w/ linea (clip)
mul		equ	1	; Multiply rather than use table
shift		equ	1

	include		"vdi.inc"
  ifne	doaline
	include		"linea.inc"
  endc

	xdef		_check_linea
  ifeq		shift
	xdef		dot,line,rline
  endc
  ifeq		mul
	xdef		row
  endc
	xdef		_font_table


	text

* The tables are mostly outdated
	
;colour:
;	dc.l		$00000000,$00000000
;	dc.l		$ffff0000,$00000000
;	dc.l		$0000ffff,$00000000
;	dc.l		$ffffffff,$00000000
;	dc.l		$00000000,$ffff0000
;	dc.l		$ffff0000,$ffff0000
;	dc.l		$0000ffff,$ffff0000
;	dc.l		$ffffffff,$ffff0000
;	dc.l		$00000000,$0000ffff
;	dc.l		$ffff0000,$0000ffff
;	dc.l		$0000ffff,$0000ffff
;	dc.l		$ffffffff,$0000ffff
;	dc.l		$00000000,$ffffffff
;	dc.l		$ffff0000,$ffffffff
;	dc.l		$0000ffff,$ffffffff
;	dc.l		$ffffffff,$ffffffff

  ifeq	shift
dot:	dc.l		$80008000,$40004000,$20002000,$10001000
	dc.l		$08000800,$04000400,$02000200,$01000100
	dc.l		$00800080,$00400040,$00200020,$00100010
	dc.l		$00080008,$00040004,$00020002,$00010001

lline:	dc.l		$ffffffff,$7fff7fff,$3fff3fff,$1fff1fff
	dc.l		$0fff0fff,$07ff07ff,$03ff03ff,$01ff01ff
	dc.l		$00ff00ff,$007f007f,$003f003f,$001f001f
	dc.l		$000f000f,$00070007,$00030003,$00010001

rline:	dc.l		$80008000,$c000c000,$e000e000,$f000f000
	dc.l		$f800f800,$fc00fc00,$fe00fe00,$ff00ff00
	dc.l		$ff80ff80,$ffc0ffc0,$ffe0ffe0,$fff0fff0
	dc.l		$fff8fff8,$fffcfffc,$fffefffe,$ffffffff
  endc

  ifeq	mul
row:	ds.l		1024
  endc

; This used to setup clip as x,y,w,h
* _check_linea - Initialization function
* Sets up hardware dependent workstation information
* Todo: ?
* In:	4(a7)	Pointer to workstation struct
_check_linea:
	movem.l		d0-d3/a0-a4,-(a7)
	move.l		4+9*4(a7),a4

	dc.w		$a000
	move.l		a1,_font_table
	move.l		a0,wk_screen_linea(a4)	; Needed?
	move.w		2(a0),d0
	move.w		d0,wk_screen_wrap(a4)
	move.w		-12(a0),d1
	move.w		d1,wk_screen_mfdb_width(a4)
	move.w		-4(a0),d1
	move.w		d1,wk_screen_mfdb_height(a4)
	move.w		(a0),wk_screen_mfdb_bitplanes(a4)

  ifne	doaline
	move.w		#$ffff,lnmask(a0)	; solid line
	move.w		#0,wmode(a0)	; replace mode
	move.w		#0,lst_lin(a0)	; don't write last pixel
  endc

  ifeq	mul
	move.l		wk_screen_mfdb_address(a4),d0
	moveq		#0,d1
	move.w		wk_screen_wrap(a4),d1
	move.w		#1024-1,d7
	lea		row(pc),a0

.row_l:
	move.l		d0,(a0)+
	add.l		d1,d0
	dbra		d7,.row_l
  endc

	movem.l	(a7)+,d0-d3/a0-a4
	rts


	data

_font_table:
	dc.l		0	; Pointer to font head table (3)
	
	end
