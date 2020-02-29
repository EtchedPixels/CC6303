
	.export _memset

	.code

_memset:
	tsx
	ldd	5,x
	stab	@temp		; destination
	ldd	3,x		; length
	ldx	7,x		; destination
	bsr	nextblock
	tsx
	ldd	7,x
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
	ldaa	@temp
clearloop:
	staa	,x
	inx
	decb
	bne	clearloop
	rts
