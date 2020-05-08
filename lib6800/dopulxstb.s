;
;	Uninlined pulx equivalent
;
	.export dopulxstb
	.code

dopulxstb:
	tsx
	ldx 1,x
	stx @tmp
	ldx 3,x
	ins
	ins
	ins
	ins
	stab ,x
	jmp jmptmp
