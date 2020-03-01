;
;	Based on the 6502 runtime. CC68 is a bit different as we've got
;	better operations
;

	.code

	.export tosand0ax
	.export tosandeax

tosand0ax:
	clr	sreg
	clr	sreg+1
;
;	and D and @sreg with the top of stack (3,X as called)
;
tosandeax:
	tsx
	anda	5,x
	andb	6,x
	std	@tmp
	ldd	@sreg
	anda	3,x
	andb	4,x
	jmp	swap32pop4
