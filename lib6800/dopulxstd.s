;
;	Uninlined pulx equivalent
;
	.export dopulxstd
	.code

dopulxstd:
	tsx
	ldx 1,x
	stx @tmp
	ldx 3,x
	ins
	ins
	ins
	ins
	staa ,x
	stab 1,x
	jmp jmptmp
