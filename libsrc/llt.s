
	.code
	.export toslteax

toslteax:
	jsr toslcmp
	jmp boollt
