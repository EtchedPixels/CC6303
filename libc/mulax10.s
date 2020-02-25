;
;	Multiply D by 10
;

	.export mulax10

	.code

mulax10:
	asld			; x 2
	std @tmp		; save
	asld			; x 4
	asld			; x 8
	addd @tmp		; x 10
	rts
