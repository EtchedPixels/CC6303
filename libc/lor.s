;
;	Based on the 6502 runtime. CC68 is a bit different as we've got
;	better operations
;

	.code

	.export tosor0ax
	.export tosoreax

tosor0ax:
	clr	@sreg
	clr	@sreg+1
;
;	or D and @sreg with the top of stack (1,X as called)
;
tosoreax:
	oraa	3,x
	orab	4,x
	std	@tmp
	ldd	@sreg
	oraa	1,x
	orab	2,x
	std	@sreg
	; and unstack
unwind4:
	pulx		; return
	ins	
	ins
	ins
	ins
	jmp ,x
