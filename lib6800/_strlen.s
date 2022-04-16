

	.code
	.export _strlen

	.setcpu 6800

_strlen:
	tsx
	ldx 2,x
	clra
	clrb
cl:	tst ,x
	beq to_rts
	inx
	addb #1
	adca #0
	bra cl
to_rts:
	jmp ret2
