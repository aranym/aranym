*****
* fVDI VDI declarations, by Johan Klockars
*
* This file is put in the public domain.
* It's not copyrighted or under any sort of license.
*****

	include	"compiler.inc"

CHUNKY		equ	1
TRUE_COLOUR	equ	2

BLACK 		equ	1
EFFECTS		equ	%00111111

only_fvdi	equ	1

*
* VDI array
*
control		equ	0
intin		equ	4
ptsin		equ	8
intout		equ	12
ptsout		equ	16

*
* Control
*
function	equ	0
L_ptsin		equ	2
L_ptsout	equ	4
L_intin		equ	6
L_intout	equ	8
subfunction	equ	10
handle		equ	12


*
* VDI struct
* Font struct
*
	include	"types.inc"
