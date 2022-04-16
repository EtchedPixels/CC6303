
	.code
	.export toslteax

toslteax:
	jsr toslcmp
	jsr boolult		; toslcmp generates c and z not n
	jmp pop4flags
