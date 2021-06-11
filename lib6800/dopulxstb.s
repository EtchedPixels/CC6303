;
;	Uninlined pulx equivalent
;
	.export dopulxstb
	.code

dopulxstb:
	tsx
	ldx ,x
	stx @tmp
	tsx
	ldx 2,x
	ins
	ins
	ins
	ins
	stab ,x
	jmp jmptmp
