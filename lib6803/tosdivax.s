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

	.setcpu 6803

;
;	The sign of the remainder of a division is not defined until C99.
;	C99 says it's the sign of the dividend.
;
tosmodax:
	tsx
	anda #$7F		; make the divisor unsigned
	std @tmp		; save it
	ldaa 2,x
	anda #$80		; sign bit of dividend
	psha			; save it
	ldaa 2,x		; clear sign bit 
	anda #$7F
	staa 2,x
	ldx 2,x			; get the dividend (unsigned)
	ldd @tmp		; get the divisor unsigned
	jsr div16x16		; do the unsigned divide
				; X = quotient, D = remainder
	stab @tmp		; save the quotient low
	anda #$7F		; clear the sign
	pulb			; get the resulting sign back
	aba			; put the sign back in
	ldab @tmp		; recover the low bits
	jmp pop2
	

;
;	D = TOS/D signed
;
;	The sign of the result is positive if the inputs have the same
;	sign, otherwise negative
;
tosdivax:
	tsx
	staa @tmp		; save the top of the divisor
	eora 2,x		; exclusive or of signs
	anda #$80		; mask so only sign bit
	psha			; save this so we know what to do at the end
	ldaa @tmp		; get the top of our divisor back
	pshb			; stack the divisor
	psha
	ldd 2,x			; get the dividend
	anda #$7F		; make it unsigned
	std @tmp		; move it into X
	ldx @tmp
	pula			; get the divisor back into D
	pulb
	anda #$7F		; make the divisor unsigned
	jsr div16x16		; do the maths
				; X = quotient, D = remainder
	stx @tmp		; save quotient for fixup
	ldaa @tmp
	anda #$7F		; clear sign
	pulb
	aba			; merge correct sign
	ldab @tmp+1
	jmp pop2
