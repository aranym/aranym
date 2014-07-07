|
| The following external symbols are declared with .globl rather than
| .extern in order to ensure they will be linked into the executable
| even if they are not used in this file.

	.globl	__base		| BASEPAGE *, defined in globals.c
	.globl	__heapbase	| void *
	.globl	__stksize	| long, defined by user or in stksiz.c
	.globl	_main		| for references

|
| externs to pull ident strings of all used libraries into the
| executable; if a library is not used, then the extern is
| satisfied by a dummy in the library

	.globl	___Ident_gem

|
| Functions defined elsewhere.

	.extern	__crtinit	| void _crtinit(void);

|
| Assumption: basepage is passed in a0 for accessories; for programs
| a0 is always 0.

	.text
	.even
	.globl	__start
__start:
	subl	a6, a6		| clear a6 for debuggers
	cmpl	#0, a0		| test if acc or program
	beqs	__startprg	| if a program, go elsewhere
	tstl	a0@(36)		| also test parent basepage pointer
	bnes	__startprg	| for accs, it must be 0
noreturn:
    bras noreturn       | stupid user installd ldg as acc ;=

|
| program startup code: doesn''t actually do much, other than push
| the basepage onto the stack and call _start1 in crtinit.c
|
__startprg:
	movel	sp@(4), a0	| get basepage
	movel	a0, __base	| save it
	movel	a0@(4), d0	| get _base->p_hitpa
	bclr	#0, d0		| round off
	movel	d0, sp		| set stack (temporarily)
	subl	#64, sp		| see note in mintlib/setstack.S
	jmp	__crtinit	| in crtinit.c
