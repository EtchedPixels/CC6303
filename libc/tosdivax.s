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

	.setcpu 6303

tosmodax:
	anda #$7F		; make positive
	tsx
	ldx 3,x
	xgdx
	; D is now the dividend
	psha			; save the sign of the dividend
	anda #$7F		; make positive
	jsr div16x16		; do the unsigned divide
				; D = quotient, X = remainder
	stx @temp		; save remainder whilst we fix up the sign
	pula			; sign of the dividend
signfix:
	bita #$80		; negative ?
	beq dd_unsigned
	ldd @temp		; Dividend is signed, fix up remainder
	oraa #$80		; Force negative
	jmp pop2
dd_unsigned:
	ldd @temp		; Will be unsigned
	jmp pop2
	

tosdivax:
	psha			; save sign of divisor
	anda #$7F		; make positive
	tsx
	ldx 3,x
	xgdx
	; D is now the dividend
	psha			; save the sign of the dividend
	anda #$7F		; make positive
	jsr div16x16		; do the unsigned divide
				; D = quotient, X = remainder
	std @temp		; save quotient whilst we fix up the sign
	pula			; sign of the dividend
	tsx
	eora 1,x		; A bit 7 is now the xor of the signs
	ins
	bra signfix		; shared sign fixing
