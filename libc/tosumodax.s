;
;	D = TOS / D (unsigned)
;

	.export tosudivax

	.code

tosudivax:
	tsx
	ldx 3,x			; get top of stack
	xgdx			; swich over for divide helper
	jsr div16x16		; D is now quotient
	rts
	