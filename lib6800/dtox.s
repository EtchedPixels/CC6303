
	.export dtox

dtox:
	stab @tmp+1
	staa @tmp
	ldx @tmp
	rts
