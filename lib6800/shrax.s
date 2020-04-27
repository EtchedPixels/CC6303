;
;	Right unsigned shift
;
	.export shrax7
	.export shrax6
	.export shrax5
	.export shrax4
	.export shrax3

	.code
shrax7:
	lsra
	rorb
shrax6:
	lsra
	rorb
shrax5:
	lsra
	rorb
shrax4:
	lsra
	rorb
shrax3:
	lsra
	rorb
	lsra
	rorb
	lsra
	rorb
	rts

