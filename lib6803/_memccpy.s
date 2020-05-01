
		.export _memccpy
		.setcpu 6803
		.code
_memccpy:
		tsx
		ldd 3,x
		addd 9,x
		std @tmp	; end mark
		ldd 9,x
		std @tmp2	; save 
		ldab 6,x	; match char
		ldx 7,x		; get source int X
loop:		ldaa ,x		; get a source byte
		stx @tmp3
		ldx @tmp2
		staa ,x		; save it
		inx
		stx @tmp2	; swap back
		ldx @tmp3
		cba		; did we match
		beq match
		inx
		cpx @tmp	; are we done ?
		bne loop
		clra
		clrb
		rts
match:		ldd @tmp3
		rts
