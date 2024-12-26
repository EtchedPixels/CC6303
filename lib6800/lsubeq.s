;
;	On entry X points to the object
;
	.code
	.export lsubeq
	.export lsubeqa
	.export lsubeqysp

; In this form X is the stack offset. Turn that into X is a pointer and
; fall into the static form
;	IX,S -= sreg:D
lsubeqysp:
	stx @tmp
	sts @tmp2
	pshb
	psha
	ldab @tmp2+1
	ldaa @tmp2
	addb @tmp+1
	adca @tmp
	addb #3
	adca #0
	stab @tmp+1
	staa @tmp
	ldx @tmp
	pula
	pulb
; The caller of lsubeqysp expects the result of the addition to be in @sreg:D
	bsr lsubeq
	stab @sreg+1
	staa @sreg
	ldab 3,x
	ldaa 2,x
	rts
lsubeqa:
	clr @tmp+1
	clr @tmp
;	(0-3,X) -=  sreg:D
lsubeq:
	jsr negeax
	addb 3,x
	stab 3,x
	adca 2,x
	staa 2,x
	ldab @sreg+1
	adcb 1,x
	stab 1,x
	ldaa @sreg
	adca 0,x
	staa 0,x
	rts
