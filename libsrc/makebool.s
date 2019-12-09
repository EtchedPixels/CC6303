;
;	These are based ont the CC65 runtime. 6803 is much the same
;	as 6502 here except we've got ldd to load two bytes (and the
;	optimizer will turn it into a 2 byte load of 'one' in the direct page)
;

	.code

	.export booleq
	.export boolne
	.export boolle
	.export boollt
	.export boolge
	.export boolgt
	.export boolule
	.export boolult
	.export booluge
	.export boolugt

booleq:
	bne	ret0
ret1:
	ldd	@one
	rts
boolne:
	bne	ret1
ret0:
	clra
	clrb
	rts
boolle:
	beq	ret1
boollt:
	blt	ret1
	clra
	clrb
	rts
boolgt:
	beq	ret0
boolge:	bge	ret1
	clra
	clrb
	rts
boolule:
	beq	ret1
boolult:
	bcc	ret1
	clra
	clrb
	rts
boolugt:
	beq	ret0
booluge:			; use C flag
	ldd	@zero
	rolb
	rts
