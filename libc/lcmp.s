	.code
	.export toslcmp

;
;	Compare the 4 bytes stack top with the 32bit accumulator
;	We are always offset because we are called with a jsr from
;	the true helper
;
toslcmp:
	tsx
	std @tmp	; Save the low 16bits
	ldd @sreg
	subd 5,x	; Compare the high word
	beq chklow
chklow:			; High is same so compare low
	ldd @tmp
	subd 7,x
	rts
