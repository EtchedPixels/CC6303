;
;	Based on the 6502 runtime. CC68 is a bit different as we've got
;	better operations
;

	.code

	.export tosand0ax
	.export tosandeax

tosand0ax:
	clr	@sreg
	clr	@sreg+1
;
;	and D and @sreg with the top of stack (1,X as called)
;
tosandeax:
	anda	3,x
	andb	4,x
	std	@tmp
	ldd	@sreg
	anda	1,x
	andb	2,x
	std	@sreg
	; and unstack
unwind4:
	pulx		; return
	ins	
	ins
	ins
	ins
	jmp ,x
