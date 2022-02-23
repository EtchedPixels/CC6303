;
;	Multiply D by 7
;
	.export mulax7

	.setcpu 6800
	.code

mulax7:
	staa @tmp1
	stab @tmp1+1
	lslb
	rola
	lslb
	rola
	addb @tmp1
	adca @tmp1+1
	addb @tmp1
	adca @tmp1+1
	addb @tmp1
	adca @tmp1+1
	rts
