	.globl	_bconout_stub
	.globl	_old_bconout
	.globl	_handle_char

	.text
	
	dc.l    0x58425241              | XBRA
        dc.l    0x505a4950              | fCon 
_old_bconout:
        ds.l    1

_bconout_stub:
	movem.l	d0-d2/a0-a2,-(sp)
	move.w	6*4+4+2(sp),d0

	move.l	sp,old_stack
	move.l	#stack,sp

	*** Debug by Xconout
	*	moveq	#0,d1
	*	move.w	d0,d1
	*	dc.w			0x712a				

	*** Output by TosWin2
	move.l	d0,-(sp)
	jsr	_handle_char
	addq.l	#4,sp

	move.l	old_stack,sp
	movem.l	(sp)+,d0-d2/a0-a2
	rts

_hndle_char:
	move.l	4(sp),d1
	dc.w			0x712a				
	rts

	.bss
old_stack:
	ds.l	1

	ds.b	16384
stack:

	.end
