
		.export _isascii

		.code

_isascii:
		clra
		tsx
		cmpb #127
		bhs fail
		ldab #1
		rts
fail:		clrb
		rts
