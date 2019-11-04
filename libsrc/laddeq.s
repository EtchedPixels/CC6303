;
;	On entry X points to the object
;
	.code
	.globl laddeqa
	.globl laddeqysp

; In this form X is the stack offset. Turn that into X is a pointer and
; fall into the static form
laddeqysp:
	stx tmp
	tsx
	xgdx
	add tmp
laddeqa:
	std tmp
	ldd 3,x		; do the low 16bits
	addd tmp
	bcc l1
	inc sreg	; carry - we don't have abcd
l1:
	std 3,x
	ldd 1,x
	addd sreg
	std 1,x
	rts
