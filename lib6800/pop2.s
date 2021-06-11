

	.export pop2
	.export pop2flags

	.code

pop2:
	tsx				; no pulx on original 6800
	ldx ,x				; so do it by hand
pop2h:
	ins
	ins
	ins
	ins
	jmp ,x
pop2flags:
	tsx
	ldx ,x
	; B is 0 or 1 and we just need to set Z
	tstb
	bra pop2h
