;
;	Multiply D by 3
;
	.export mulax3

	.code

mulax3:
	std @tmp1
	lsld
	addd @tmp1
	rts
