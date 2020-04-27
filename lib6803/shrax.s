;
;	Right unsigned shift
;
	.export shrax7
	.export shrax6
	.export shrax5
	.export shrax4
	.export shrax3

	.setcpu 6803
	.code
shrax7:
	lsrd
shrax6:
	lsrd
shrax5:
	lsrd
shrax4:
	lsrd
shrax3:
	lsrd
	lsrd
	lsrd
	rts

