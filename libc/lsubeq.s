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
	ldd 3,x		; do the low 16bits
	subd @tmp
	bcc l1
	inc sreg	; borrow - we don't have sbcd
l1:
	std 3,x
	ldd 1,x
	subd @sreg
	std 1,x
	rts

