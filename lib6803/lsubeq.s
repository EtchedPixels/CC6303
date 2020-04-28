;
;	On entry X points to the object
;
	.code
	.export lsubeq
	.export lsubeqa
	.export lsubeqysp
	.setcpu 6803

; In this form X is the stack offset. Turn that into X is a pointer and
; fall into the static form
lsubeqysp:
	stx @tmp
	sts @tmp2
	ldd @tmp2
	addd @tmp
	ldx @tmp
;
;	In this form X is a pointer to the long
;
lsubeq:
	std @tmp
	ldd 5,x		; do the low 16bits
	subd @tmp
	std 5,x
	ldd 3,x
	sbcb @sreg+1
	sbca @sreg
	std 3,x
	ldd 5,x
	rts
;
;	Same as lsubeq but with a 16bit value to subtract
;
lsubeqa:
	clr @sreg
	clr @sreg+1
	bra lsubeq
