	COMMENT	HEAD=7

zacatek	bra	init

***************************************
GetCookie	movem.l	D1-D2/A0,-(sp)
	MOVE.L	D0,D2		CookieJar
	MOVE.L	$5A0.W,D0
	BEQ.S	L00A2
	MOVEA.L	D0,A0
L00A1	MOVE.L	(A0)+,D1
	MOVE.L	(A0)+,D0
	CMP.L	D2,D1
	BEQ.S	L00A3
	TST.L	D1
	BNE.S	L00A1
L00A2	MOVEQ	#-1,D1		set NotEqual a Negative bity
L00A3	movem.l	(sp)+,D1-D2/A0
	RTS
***************************************
* D0 = key, D1 = value
SetCookie	MOVE.L	$5A0.W,D3
	BEQ.S	L00FF
	MOVEA.L	D3,A1
	MOVEQ	#0,D4
L00FE	ADDQ.W	#1,D4
	MOVEM.L	(A1)+,D2-D3
	CMP.L	D0,D2
	BEQ.S	COOKFOUND
	TST.L	D2
	BNE.S	L00FE
	CMP.L	D3,D4
	BEQ.S	L00FF
	MOVEM.L	D0-D3,-8(A1)
	BRA.S	L00FF
COOKFOUND	MOVEM.L	D0-D1,-8(A1)
L00FF	RTS
***************************************
init	clr.l	-(sp)
	move.w	#32,-(sp)
	trap	#1		go into Supervisor mode
	addq	#6,sp
	move.l	d0,-(sp)		uschovat USERSTACK pointer na z sobn¡k

	move.l	#'MOGR',d0
	bsr	GetCookie
	beq.s	pryc

	move.l	#'MOGR',d0
	move.l	#kilobyte,d1
	bsr	SetCookie

	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp

	pea	infotext(PC)
	move	#9,-(SP)
	trap	#1
	addq	#6,SP

	clr	-(SP)
	pea	konec-zacatek+$100
	move	#49,-(SP)
	trap	#1

pryc	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp
	
	clr.l	-(sp)
	trap	#1

	section DATA
infotext	dc.b	13,10,27,'p',"  ARAnyM GEMRAM 1.0 ",27,'q',13,10,10,0

	section BSS
kilobyte	ds.l	256

konec
