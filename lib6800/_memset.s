
	.export _memset

	.code

_memset:
	tsx
	ldaa	5,x
	ldab	6,x
	stab	@tmp		; destination
	ldaa	3,x		; length
	ldab	4,x
	ldx	7,x		; destination
	bsr	nextblock
	tsx
	ldaa	7,x
	ldab	8,x
	rts

nextblock:
	tsta
	beq	tailset
	; Set 256 bytes
	pshb
	psha
	clrb
	bsr	tailset
	pula
	pulb
	deca
	bra	nextblock

tailset:
	ldaa	@tmp
clearloop:
	staa	,x
	inx
	decb
	bne	clearloop
	rts
