;
;	Right shift 32bit signed
;
	.export tosasreax

	.code

tosasreax:	
	cmpb	#32
	bcc	ret0
	tstb
	beq noshift
	tsx
loop:
	asr	3,x
	ror	4,x
	ror	5,x
	ror	6,x
	decb
	bne loop
	ldaa	3,x
	ldab	4,x
	; Get the value
	staa	@sreg
	stab	@sreg+1
	ldaa	5,x
	ldab	6,x
noshift:
	jmp pop4
ret0:
	clra
	clrb
	staa	@sreg
	stab	@sreg+1
	bra	noshift
