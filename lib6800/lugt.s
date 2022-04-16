
	.code
	.export tosugteax

tosugteax:
	jsr toslucmp
	jsr boolugt
	jmp pop4flags
