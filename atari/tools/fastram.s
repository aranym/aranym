	COMMENT	HEAD=7

	clr.l	-(sp)
	move.w	#32,-(sp)
	trap	#1		go into Supervisor mode
	addq	#6,sp
	move.l	d0,-(sp)		uschovat USERSTACK pointer na z sobn¡k

	pea	$2000000		32 MB RAM
	pea	$1000000		located at 16 MB boundary
	move.w	#20,-(sp)
	trap	#1
	lea	10(sp),sp
	
	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp

	pea	infotext(PC)
	move	#9,-(SP)
	trap	#1
	addq	#6,SP

pryc	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp
	
	clr.l	-(sp)
	trap	#1

	section DATA
infotext	dc.b	13,10,27,'p',"  ARAnyM TT-RAM 1.0 ",27,'q',13,10,10,0

	section BSS
kilobyte	ds.l	256

konec
