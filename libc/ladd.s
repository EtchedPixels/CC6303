;
;	Add top of stack to 32bit working accumulator, removing it from
;	the stack
;
	.code
	.export tosadd0ax
	.export tosaddeax

tosadd0ax:
	clr	sreg
	clr	sreg+1
tosaddeax:
	tsx
	addd	3,x
	std	@tmp
	ldab	2,x
	adcb	@sreg+1
	ldaa	1,x
	adca	@sreg
	std	@sreg
	ldd	@tmp
	pulx
	ins
	ins
	ins
	ins
	jmp ,x
