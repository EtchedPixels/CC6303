;
;	Uninlined pulx equivalent
;
	.export dopulx
	.code

dopulx:
	tsx
	ldx ,x
	stx @tmp
	tsx
	ldx 2,x
	ins
	ins
	ins
	ins
	jmp jmptmp
