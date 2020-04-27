;
;	This algorithm is taken from the manual
;
	.export div16x16
	.setcpu 6303

	.code

;
;	16bit divide
;
;	X = dividend
;	D = divisor
;
;	Uses tmp,tmp1
;
;	Returnds D = quotient
;		 X = remainder
;
div16x16:
	; TODO - should be we spot div by 0 and trap out ?
	std @tmp1		; divisor
	ldaa #16		; bit count
	staa @tmp		; counter
	clra
	clrb
loop:
	xgdx
	asld			; shift X left one bit at a time (dividend)
	xgdx
	rolb
	rola
	subd @tmp1		; divisor
	inx
	bcc skip
	addd @tmp1
	dex
skip:
	dec tmp
	bne loop
	rts
