;
;	Right shift unsigned  TOS >> D
;

	.export tosshrax

	.setcpu 6803
	.code

tosshrax:
	cmpb #15		; More bits are meaningless
	bgt ret0
	tsx
loop:
	tstb
	beq retdone
	lsr 3,x
	lsr 4,x
	decb
	bra loop
ret0:
	ldd @zero
	jmp pop2
retdone:
	jmp pop2get
