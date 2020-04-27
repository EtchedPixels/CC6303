;
;	Shift the 32bit primary left 1-4 bits
;

	.export asleax1
	.export asleax2
	.export asleax3
	.export asleax4
	.export shleax1
	.export shleax2
	.export shleax3
	.export shleax4

	.code

asleax4:
shleax4:
	aslb
	rola
	rol @sreg+1
	rol @sreg
asleax3:
shleax3:
	lsld
	rol @sreg+1
	rol @sreg
asleax2:
shleax2:
	lsld
	rol @sreg+1
	rol @sreg
asleax1:
shleax1:
	lsld
	rol @sreg+1
	rol @sreg
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