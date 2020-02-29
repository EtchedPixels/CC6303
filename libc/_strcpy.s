;
;	Another one that's really hard to do nicely
;

	.export _strcpy

	.code

_strcpy:
	tsx
	ldd	7,x
	std	@temp		; destination
	ldd	3,x		; length
	ldx	5,x		; src
copyloop:
	ldaa	,x
	inx
	pshx
	ldx	@temp
	staa	,x
	inx
	stx	@temp
	pulx
	tsta
	bne copyloop
	ldd	7,x
	rts
