;
;	Right shift unsigned  TOS >> D
;

	.export tosshrax

	.code

tosshrax:
	cmpb #15		; More bits are meaningless
	bgt ret0
	tsx
loop:
	tstb
	beq retdone
	lsr 2,x
	ror 3,x
	decb
	bra loop
ret0:
	clra
	clrb
	jmp pop2
retdone:
	jmp pop2get
