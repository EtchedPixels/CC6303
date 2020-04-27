;
;	Shift TOS left by D
;
	.export tosshlax		; unsigned
	.export tosaslax		; signed
	.export pop2

	.setcpu 6803

; For 6303 it might be faster to load into D, shift in D using X as the
; count (as we can xgdx it in)
tosaslax:				; negative shift is not defined
					; anyway
tosshlax:
	clra				; shift of > 15 is meaningless
	cmpb #15
	bgt shiftout
	tsx
shloop:
	tstb
	beq shiftdone
	lsr 3,x
	ror 4,x
	bra shloop
shiftout:
	ldd @zero
	std 3,x
shiftdone:
pop2:
	pulx
	ins
	ins
	jmp ,x
