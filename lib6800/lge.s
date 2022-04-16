
	.code
	.export tosgeeax

tosgeeax:
	jsr toslcmp
	jsr booluge		; toslcmp generates c and z not n
	jmp pop4flags


