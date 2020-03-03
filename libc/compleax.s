;
;	Complement the 32bit working register
;
	.export compleax

compleax:
	std @tmp
	ldd @sreg
	coma
	comb
	std @sreg
	ldd @tmp
	coma
	comb
	rts
