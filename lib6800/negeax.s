;
;	Negate the working register
;
	.export negeax

	.code

negeax:
	comb
	coma
	com @sreg+1
	com @sreg
	addb #1
	adca #0
	bcc ret
	inc @sreg+1
	bne ret
	inc @sreg
ret:	rts
