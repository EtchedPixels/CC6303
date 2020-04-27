;
;	Shift the 32bit primary left 1-4 bits
;

	.export shleax1
	.export shleax2
	.export shleax3
	.export shleax4

	.code

shleax4:
	lslb
	rola
	rol @sreg+1
	rol @sreg
shleax3:
	lslb
	rola
	rol @sreg+1
	rol @sreg
shleax2:
	lslb
	rola
	rol @sreg+1
	rol @sreg
shleax1:
	lslb
	rola
	rol @sreg+1
	rol @sreg
	rts

