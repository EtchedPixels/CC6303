;
;	D = TOS / D (unsigned)
;

	.export tosumodax
	.code

tosumodax:
	tsx
	ldx 3,x
	jsr div16x16	
	jmp pop2
