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
	tstb
	beq noshift
	tsx
loop:
	asl	6,x
	rol	5,x
	rol	4,x
	rol	3,x
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
