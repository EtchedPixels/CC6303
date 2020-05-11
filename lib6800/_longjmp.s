
	.export	_longjmp

	.code

;
;	We don't have much work to do. X is scratch, D is the return
;	code so really we only have S and the return address to worry
;	about
;
_longjmp:
	tsx
	ldaa	2,x		; retval
	ldab	3,x
	bne	retok
	tsta
	bne	retok
	incb			; retval 1 if 0 requested
retok:
	ldx	4,x		; get the setjmp buffer
	lds	,x		; recover the stack pointer
	ldx	2,x		; and the restored pc
	ins			; adjust the stack
	ins
	ins			; pop the setjmp arguments
	ins
	ins
	ins
	jmp	,x
