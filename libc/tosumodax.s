;
;	D = TOS / D (unsigned)
;

	.export tosumodax
	.setcpu 6303
	.code

tosumodax:
	tsx
	ldx 3,x
	jsr div16x16	
	jmp pop2
