	OPT		P=68030
	COMMENT		HEAD=7		; Fastload & TT flags

STARTFRAM	= $1000000
MAXFRAM	= 3840
FRBSIZE	= 64*1024
RAMTOP	= $5a4
RAMVALID	= $1357BD13

zacatek	MOVEA.L	4(A7),A0
	MOVEA.L	$C(A0),A1
	ADDA.L	$14(A0),A1
	ADDA.L	$1C(A0),A1
	LEA	$100(A1),A1
	PEA	(A1)
	PEA	(A0)
	CLR.W	-(A7)
	MOVE.W	#$4A,-(A7)
	TRAP	#1
	LEA	$C(A7),A7

	pea	infotext(PC)
	move.w	#9,-(SP)
	trap	#1
	addq.l	#6,SP

* go to supervisor mode
	clr.l	-(sp)
	move.w	#32,-(sp)
	trap	#1		go into Supervisor mode
	addq	#6,sp
	move.l	d0,-(sp)

* is FastRAM known already
	ifd	tos404doesresetramtop
	
	lea	RAMTOP\w,a0
	cmp.l	#RAMVALID,4(a0)
	bne.s	findfram
	cmp.l	#STARTFRAM,(a0)
	bgt	frb
	
	else
	
	move.w	#1,-(sp)		FastRAM only
	pea	-1		inquiry free ram
	move.w	#68,-(sp)
	trap	#1
	addq.l	#8,sp
	tst.l	d0
	bne	frb		if nonzero FastRAM
	
	endc
	
***************************************
* find the FastRAM size
findfram

** clear counter
	moveq	#0,d6

** disable IRQ
	move.w	sr,d7
	or.w	#$700,sr
			
** set up alternate bus error handler
	move.l	sp,a6
	move.l	$8.w,a5
	move.l	#buserr,$8.w

** start from STARTFRAM
	lea	STARTFRAM,a0

** test by blocks of 1MB size
	move.l	#1024*1024,d0

loop	lea	(a0,d0.l),a1
	tst.l	-(a1)
	addq.l	#1,d6
	add.l	d0,a0
	cmp.l	#MAXFRAM,d6
	blt.s	loop
	
** alternate bus error handler here
buserr	move.l	a5,$8.w
	move.l	a6,sp

** enable IRQ
	move.w	d7,sr
	
***************************************
* allocate it
	move.l	d6,d0
	beq	nofram

	moveq	#20,d1
	asl.l	d1,d0		megabytes to bytes
	move.l	d0,d5
	move.l	d0,-(sp)
	pea	STARTFRAM		located at 16 MB boundary
	move.w	#20,-(sp)
	trap	#1		MaddAlt
	lea	10(sp),sp
	
* configure ramtop variables
	lea	RAMTOP\w,a0
	add.l	#STARTFRAM,d5
	move.l	d5,(a0)+
	move.l	#RAMVALID,(a0)

* decimal conversion of fastram size
	move.l	d6,d0
	lea	printsize(pc),a0
.loop	divul	#10,d1:d0
	add.b	#'0',d1
	move.b	d1,-(a0)
	tst.l	d0
	bne.s	.loop

* display size
	pea	infosize(PC)
	move.w	#9,-(SP)
	trap	#1
	addq.l	#6,SP

***************************************
* create _FRB buffer
frb	move.l	#'_FRB',d0
	bsr	GetCookie
	bpl.s	tsr

* allocate _FRB block of memory
	clr.w	-(sp)		ST-RAM only
	move.l	#(FRBSIZE+8192),-(sp)
	move.w	#68,-(sp)
	trap	#1		MxAlloc()
	addq.l	#8,sp

	tst.l	d0		test if the pointer
	bne.s	.round		is valid (non NULL)
	pea	allocfail
	bra.s	frbprint
		
.round	add.l	#8192-1,d0
	and.l	#-8192,d0		round to 8kB boundary
	move.l	d0,d1
	move.l	#'_FRB',d0
	bsr	SetCookie

	pea	infofrb(PC)
frbprint	move.w	#9,-(SP)
	trap	#1
	addq.l	#6,SP
	
* go back to user mode	
tsr	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp

* terminate and stay resident (to keep the _FRB buffer valid)
	clr.w	-(SP)
	pea	konec-zacatek+$100
	move	#49,-(SP)
	trap	#1

***************************************
* fastram already allocated
framno	pea	iramno(PC)
	bra.s	printit
	
***************************************
* no fastram found
nofram	pea	inoram(PC)

printit	move.w	#9,-(SP)
	trap	#1
	addq.l	#6,SP

* go back to user mode	
	move.w	#32,-(sp)
	trap	#1		go into User mode
	addq	#6,sp
* end
failend	clr.l	-(sp)
	trap	#1

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
SetCookie	movem.l	d2-d4/a1,-(sp)
	MOVE.L	$5A0.W,D3
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
L00FF	movem.l	(sp)+,d2-d4/a1
	RTS
***************************************

	section DATA
infotext	dc.b	13,10,27,'p'," ARAnyM FastRAM v2.0 by Petr Stehlik ",27,'q',13,10,0
infosize	dc.b	"    "
printsize	dc.b	" MB of FastRAM found.",13,10,0
infofrb	dc.b	"64kB _FRB buffer created.",13,10,10,0
allocfail	dc.b	"_FRB buffer allocation failed.",13,10,10,0
inoram	dc.b	"No FastRAM found.",13,10,10,0
iramno	dc.b	"FastRAM allocated already.",13,10,10,0
	even
	
	section BSS
fastram	ds.l	1
oldbus	ds.l	1
stoploop	ds.b	1

konec
