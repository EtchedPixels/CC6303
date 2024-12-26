;
;	On entry X points to the object
;
;	FIXME: need a review and rewrite as we sort out the 680x calling
;	conventions for this versus 6502
;
	.code
	.export laddeqa
	.export laddeqysp
	.export laddeq

; In this form X is the stack offset. Turn that into X is a pointer and
; fall into the static form
; (IX),SP += @sreg:D
laddeqysp:
	stx @tmp	; IX = IX+SP+3
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
	bsr laddeq
;	The caller of laddeqysp expects the result of the addition to be in @sreg:D
	staa @sreg
	stab @sreg+1
	ldaa 2,x
	ldab 3,x
	rts
laddeqa:
	clr @tmp+1
	clr @tmp
laddeq:			; Add the 32bit sreg/d to the variable at X
	addb 3,x	; do the low 16bits
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
