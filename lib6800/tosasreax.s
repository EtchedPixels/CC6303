;
;	Right shift 32bit signed
;
	.export tosasreax

	.code

tosasreax:	
	cmpb	#32
	bcc	ret0
	tsx
	tstb
	beq noshift
loop:
	asr	2,x
	ror	3,x
	ror	4,x
	ror	5,x
	decb
	bne loop
	ldaa	2,x
	ldab	3,x
	; Get the value
	staa	@sreg
	stab	@sreg+1
	ldaa	4,x
	ldab	5,x
	jmp	pop4
ret0:
	clra
	clrb
	staa	@sreg
	stab	@sreg+1
	bra	noshift
noshift:
	ldx	0,x
	ins
	ins
	pula
	staa	@sreg
	pulb
	stab	@sreg+1
	pula
	pulb
	jmp	0,x
