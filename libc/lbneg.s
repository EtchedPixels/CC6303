;
;	Boolean negation for a long
;
	.export bnegeax

	.code

bnegeax:
	subd @zero	; tstd in effect
	bne nonz	; non zero
	ldd @sreg
	bne nonz
	ldd @one	; Is zero so negated is 1
	rts
nonz:	ldd @zero
	rts
