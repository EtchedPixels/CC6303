;
;	Boolean negation
;
	.code
	.export bnega
	.export bnegax


bnegax:	tsta
	bne ret0
nega:	tstb
	bne ret0
	ldd #$0001
	rts
ret0:
	clra
	clrb
	rts
