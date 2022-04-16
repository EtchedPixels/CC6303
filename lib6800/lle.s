
	.code
	.export tosleeax

tosleeax:
	jsr toslcmp
	jsr boolule		; toslcmp generates c and z not n
	jmp pop4flags
