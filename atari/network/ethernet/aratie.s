| 
| Interrupt routine for the ARAnyM Ethernet driver
|
|
|
	.globl _old_interrupt
	.globl _my_interrupt
	.globl _aranym_interrupt

	.text

	dc.l	0x58425241		| XBRA
	dc.l	0x505a4950		| ARAe
_old_interrupt:
	ds.l	1
_my_interrupt:
	movem.l	a0-a2/d0-d2,-(sp)
	bsr	_aranym_interrupt
	movem.l	(sp)+,a0-a2/d0-d2
	move.l	_old_interrupt(PC),-(sp)
	rts
