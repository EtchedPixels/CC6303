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
	ldaa 3,x		; Result into D
	ldab 4,x
	bra retdone
ret0:
	clra
	clrb
retdone:
	jmp pop2
