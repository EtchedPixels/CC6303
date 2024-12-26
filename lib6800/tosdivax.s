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
	tsta
	bpl tosmod2
	bsr negd
tosmod2:
	ldx 2,x
	bmi negmod
	jsr div16x16		; do the unsigned divide
				; X = quotient, D = remainder
	jmp pop2
negmod:
	tsx
	com 2,x
	neg 3,x
	bne negmod2
	inc 2,x
negmod2:
	ldx 2,x
	jsr div16x16
negdret:
	bsr negd
	jmp pop2
	
tosdivax:
	tsx
	staa @tmp4
	bpl posdiv1
	bsr negd
posdiv1:
	ldx 2,x
	bpl posdiv3
	com @tmp4
	tsx
	com 2,x
	neg 3,x
	bne negdiv2
	inc 2,x
negdiv2:
	ldx 2,x
posdiv3:
	jsr div16x16		; do the unsigned divide
				; X = quotient, D = remainder
	stx @tmp
	ldab @tmp+1
	ldaa @tmp
	tst @tmp4
	bmi negdret
	jmp pop2
negd:
	nega
	negb
	sbca #0
ispos:
	rts
