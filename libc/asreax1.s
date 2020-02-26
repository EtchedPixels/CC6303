;
;	Right shift the 32bit working register
;
	.export asreax1

	.code

asreax1:
	asr sreg
	ror sreg+1
	rora
	rorb
	rts
