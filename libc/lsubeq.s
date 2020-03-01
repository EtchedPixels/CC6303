;
;	On entry X points to the object
;
;	FIXME: 6303 only right now
;
	.code
	.export lsubeqa
	.export lsubeqysp
	.setcpu 6303

; In this form X is the stack offset. Turn that into X is a pointer and
; fall into the static form
lsubeqysp:
	stx @tmp
	tsx
	xgdx
	addd @tmp
lsubeqa:
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

