;
;	Left shift 32bit signed
;
	.export tosasleax
	.export tosshleax

	.code

;
;	TODO: optimize 8, 16 steps on asl and asr cases
;
tosasleax:	
tosshleax:
	cmpb	#32
	bcc	ret0
	tsx
	tstb
	beq noshift
loop:
	asl	5,x
	rol	4,x
	rol	3,x
	rol	2,x
	decb
	bne loop
	; Get the value
	ldaa	2,x
	ldab	3,x
	staa	@sreg
	stab	@sreg+1
	ldaa	4,x
	ldab	5,x
	jmp pop4
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
