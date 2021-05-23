
	.code
	.export tosgteax

tosgteax:
	jsr toslcmp
	jsr boolugt		; toslcmp generates c and z not n
	jmp pop4
