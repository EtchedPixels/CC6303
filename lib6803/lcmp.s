	.setcpu 6803

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
	ldd 5,x
	subd @sreg	; Check the high word
	beq chklow
	rts
chklow:
	ldd 7,x		; High is same so compare low
	subd @tmp
	rts
