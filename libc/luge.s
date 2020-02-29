
	.code
	.export tosugeeax
	.export pop4

tosugeeax:
	jsr toslcmp
	jsr booluge

pop4:
	pulx
	ins
	ins
	ins
	ins
	jmp ,x
