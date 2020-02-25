;
;	Shift the 32bit primary right one bit
;

	.export shreax1

	.code

shreax1:
	std @tmp1
	ldd @sreg
	lsrd
	std @sreg
	ror @tmp1
	ror @tmp1+1
	ldd @tmp1
	rts

;
;	FIXME: for 6303
;
;	xgdx
;	ldd @sreg
;	lsrd
;	std @sreg
;	xgdx
;	rora
;	rorb
;	rts
;
;	