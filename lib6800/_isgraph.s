
		.export _isgraph

		.code

_isgraph:
		clra
		tsx
		ldab 4,x
		cmpb #' '
		bls fail
		cmpb #127
		bhs fail
		; Any non zero value is valid
		rts
fail:		clrb
		rts
