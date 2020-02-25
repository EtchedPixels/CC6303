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
	ldd	3,x
	; Get the value
	std	@sreg
	ldd	5,x
noshift:
	jmp pop4
ret0:
	clra
	clrb
	std	@sreg
	bra	noshift
