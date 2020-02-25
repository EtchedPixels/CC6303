;
;	Subtract sreg:d from the top of stack
;

	.export tossubeax

	.code

tossubeax:
	tsx
	std @temp		; save the low 16bits to subtract
	ldd 5,x			; low 16 of TOS
	subd @temp		; subtract the low 16
	std @temp		; remember it
	ldd 3,x			; top 16 of TOS
	sbcb @sreg+1		; no sbcd so do this in 8bit chunks
	sbca @sreg		; D is now sreg - 3,X
	std @sreg		; save the new top 16bits
	ldd @temp		; recover the value for D
	jmp pop4		; and fix up the stack
