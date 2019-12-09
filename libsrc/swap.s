;
;	6803 lacks xgdx...
;
	.code
	.export swapstk

swapstk:
	pulx
	stx @tmp
	psha
	pshb
	ldd @tmp
	rts
