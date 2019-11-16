	.code
	.globl toslcmp

;
;	Compare the 4 bytes stack top with the 32bit accumulator
;
toslcmp:
	std @tmp		; Save the low 16bits
	ldd @sreg
	subd 1,x	; Compare the high word
	beq chklow
pop4:
	pulx
	pulx
	rts
chklow:			; High is same so compare low
	ldd @tmp
	subd 3,x
	bra pop4
