;
;	Based on the 6502 runtime. CC68 is a bit different as we've got
;	better operations
;

	.code

	.export tosxor0ax
	.export tosxoreax

tosxor0ax:
	clr	sreg
	clr	sreg+1
;
;	xor D and @sreg with the top of stack (1,X as called)
;
tosxoreax:
	tsx
	eora	3,x
	eorb	4,x
	std	@tmp
	ldd	@sreg
	eora	1,x
	eorb	2,x
	std	@sreg
	; and unstack
unwind4:
	pulx		; return
	ins	
	ins
	ins
	ins
	jmp ,x
