
	.export _memset

	.setcpu 6803
	.code

_memset:
	tsx
	ldd	6,x		; start
	addd	2,x		; + length
	std	@tmp		; end stop
	ldab	5,x		; fill byte
	ldx	6,x		; start

loop:
	stab	,x
	inx
	cpx	@tmp
	bne	loop
	tsx
	ldx	6,x		; returns the start passed in
	rts
