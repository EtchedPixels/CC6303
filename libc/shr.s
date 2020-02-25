;
;	Right shift unsigned  TOS >> D
;

	.export tosshrax

	.code

tosshrax:
	cmpb #15		; More bits are meaningless
	bgt ret0
	tsx
	tstb
	beq retdone
loop:
	lsr 3,x
	lsr 4,x
	decb
	bne loop
	ldd 3,x			; Result into D
	bra retdone
ret0:
	ldd @zero
retdone:
	jmp pop2
