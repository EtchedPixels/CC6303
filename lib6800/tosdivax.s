;
;	D = TOS / D signed
;
;	The rules for signed divide are
;	Dividend and remainder have the same sign
;	Quotient is negative if signs disagree
;
;	So we do the maths in unsigned then fix up
;
	.export tosdivax
	.export tosmodax

	.code

tosmodax:
	tsx
	bsr absd
	psha
	ldaa 2,x
	bita #$80
	beq negmod
	pula
	ldx 2,x
	jsr div16x16		; do the unsigned divide
				; X = quotient, D = remainder
	jmp pop2
negmod:
	pshb
	ldab 3,x
	bsr negd
	stab 3,x
	staa 2,x
	ldx 2,x
	pulb
	pula
	jsr div16x16
	bsr negd
	jmp pop2
	

tosdivax:
	tsx
	clr @tmp4
	bsr absd
	pshb
	psha
	ldaa 2,x
	ldab 3,x
	bsr absd
	staa 2,x
	stab 3,x
	ldx 2,x
	pula
	pulb
	jsr div16x16		; do the unsigned divide
				; X = quotient, D = remainder
	stx @tmp
	ldaa @tmp4
	rora
	bcs negdiv
	ldaa @tmp
	ldab @tmp+1
	jmp pop2
negdiv:
	ldaa @tmp
	ldab @tmp+1
	bsr negd
	jmp pop2

absd:
	bita #$80
	beq ispos
	inc @tmp4
negd:
	decb
	sbca #0
	coma
	comb
ispos:
	rts
