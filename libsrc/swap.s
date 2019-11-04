;
;	6803 lacks xgdx...
;
	.code
	.globl swapstk

swapstk:
	pulx
	stx tmp
	psha
	pshb
	ldd tmp
	rts
