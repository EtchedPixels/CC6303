
		.export _isprint

		.code

_isprint:
		clra
		tsx
		ldab 3,x
		cmpb #32
		blo fail
		cmpb #127
		bhs fail
		; Any non zero value is valid
		rts
fail:		clrb
		rts
