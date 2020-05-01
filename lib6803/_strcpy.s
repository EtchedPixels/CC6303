;
;	Another one that's really hard to do nicely
;

	.export _strcpy

	.setcpu 6303
	.code

_strcpy:
	tsx
	ldd	5,x
	std	@temp		; destination
	ldx	3,x		; src
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
	ldd	5,x
	rts
