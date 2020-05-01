		.export _memchr
		.setcpu 6803
		.code

_memchr:
		tsx
		ldd 3,x
		addd 7,x
		std @tmp		; stop point
		ldab 6,x
		ldx 7,x
		bra memchrn
		; Must do the compare before the end check, see the C
		; standard.
memchrl:
		cmpb ,x
		beq strmatch
		inx
memchrn:
		cpx @tmp
		bne memchrl
		clra
		clrb
		rts
strmatch:
		stx @tmp
		ldd @tmp
		rts
