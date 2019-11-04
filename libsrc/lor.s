;
;	Based on the 6502 runtime. CC68 is a bit different as we've got
;	better operations
;

	.code

	.globl tosor0ax
	.globl tosoreax

tosor0ax:
	clr	sreg
	clr	sreg+1
;
;	or D and sreg with the top of stack (1,X as called)
;
tosoreax:
	ora	3,x
	orb	4,x
	std	tmp
	ldd	sreg
	ora	1,x
	orb	2,x
	std	sreg
	; and unstack
unwind4:
	pulx		; return
	ins	
	ins
	ins
	ins
	jmp ,x
