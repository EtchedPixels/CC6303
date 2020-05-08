;
;	Uninlined pulx equivalent
;
	.export dopulx
	.code

dopulx:
	tsx
	ldx 1,x
	stx @tmp
	ldx 3,x
	ins
	ins
	ins
	ins
	jmp jmptmp
