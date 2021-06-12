
	.export dtox
	.export dtoxclra
	.export dtoxldb
	.export dtoxldw
	.export dtoxstoretmp2
	.export dtoxstoretmp2b
	.export dtoxstorew0
dtox:
dtoxclra:
	stab @tmp+1
	staa @tmp
	ldx @tmp
	clra
	rts
dtoxldb:
	bsr dtoxclra
	ldab ,x
	rts

dtoxstorew0:
	clr @tmp2+1
	clr @tmp2
dtoxstoretmp2:
	bsr dtox
	ldaa @tmp2
	staa ,x
	ldab @tmp2+1
	stab 1,x
	rts

dtoxstoretmp2b:
	bsr dtoxclra
	ldab @tmp2+1
	stab ,x
	rts

dtoxldw:
	bsr dtox
	ldab 1,x
	ldaa ,x
	rts
