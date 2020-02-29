	.code
	.export toslcmp
	.export pop4

;
;	Compare the 4 bytes stack top with the 32bit accumulator
;
toslcmp:
	tsx
	std @tmp		; Save the low 16bits
	ldd @sreg
	subd 3,x	; Compare the high word
	beq chklow
pop4:
	pulx
	ins
	ins
	ins
	ins
	jmp ,x
chklow:			; High is same so compare low
	ldd @tmp
	subd 5,x
	bra pop4
