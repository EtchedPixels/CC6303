
	.export tossubax


	.code
;
;	TODO - could we do this better with neg/com and add ?
;
tossubax:
	std @tmp
	tsx
	ldd 3,x		; top of maths stack as seen by caller
	subd @tmp
	rts
