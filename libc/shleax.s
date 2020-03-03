;
;	Shift the 32bit primary left 1-4 bits
;

	.export shleax1
	.export shleax2
	.export shleax3
	.export shleax4

	.code

shleax4:
	lsld
	rol @sreg+1
	rol @sreg
shleax3:
	lsld
	rol @sreg+1
	rol @sreg
shleax2:
	lsld
	rol @sreg+1
	rol @sreg
shleax1:
	lsld
	rol @sreg+1
	rol @sreg
	rts

