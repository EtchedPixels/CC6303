;
;	Shift TOS left by D
;
	.export tosshlax		; unsigned
	.export tosaslax		; signed
	.export pop2

	.code

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
	clra
	clrb
	staa 3,x
	stab 4,x
shiftdone:
pop2:
	tsx				; no pulx on original 6800
	ldx 1,x				; so do it by hand
	ins
	ins
	ins
	ins
	jmp ,x
