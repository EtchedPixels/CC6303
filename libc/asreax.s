;
;	Right shift the 32bit working register
;
	.export asreax1
	.export asreax2
	.export asreax3
	.export asreax4

	.code

asreax4:
	asr @sreg
	ror @sreg+1
	rora
	rorb
asreax3:
	asr @sreg
	ror @sreg+1
	rora
	rorb
asreax2:
	asr @sreg
	ror @sreg+1
	rora
	rorb
asreax1:
	asr @sreg
	ror @sreg+1
	rora
	rorb
	rts
