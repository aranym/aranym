*****
* fVDI helper functions for PureC, by Johan Klockars
*
* This file is put in the public domain.
* It's not copyrighted or under any sort of license.
*****

	xref	_init
	xref	_ulmod,_uldiv,_ldiv,_ulmul,_lmul

	xdef	__ulmod,__uldiv,__ldiv,__ulmul,__lmul
	xdef	_Physbase

	text

	bsr	_init		; Dummy call!

__ulmod:
	jmp	_ulmod

__uldiv:
	jmp	_uldiv
	
__ldiv:
	jmp	_ldiv
	
__lmul:
	jmp	_lmul

__ulmul:
	jmp	_ulmul

_Physbase:
	move.l	a2,-(a7)
	move.w	#$02,-(a7)
	trap	#14
	addq.l	#2,a7
	move.l	d0,a0
	move.l	(a7)+,a2
	rts

	end
