;
;	Multiply D by 9
;
	.export mulax9

	.code

mulax9:
	std @tmp1
	lsld
	lsld
	lsld
	addd @tmp1
	rts
