;
;	There isn't a nice way to do this on 680x/630x.
;
;
	.export _memcpy

	.code

_memcpy:
	tsx
	ldd	7,x
	std	@temp		; destination
	ldd	3,x		; length
	ldx	5,x		; src
	bsr	nextblock
	tsx
	ldd	7,x
	rts

nextblock:
	tsta
	beq	tailcopy
	; Copy 256 bytes repeatedly until we get to the leftovers
	pshb
	psha
	clrb
	bsr	tailcopy
	pula
	pulb
	deca
	bra	nextblock

tailcopy:
	ldaa	,x
	inx
	pshx
	ldx	@temp
	staa	,x
	inx
	stx	@temp
	pulx
	decb
	bne	tailcopy
	rts
