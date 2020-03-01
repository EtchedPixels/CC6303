;
;	Multiply D by 7
;
	.export mulax7

	.code

mulax7:
	std @tmp1
	lsld
	lsld
	addd @tmp1
	addd @tmp1
	addd @tmp1
	rts
