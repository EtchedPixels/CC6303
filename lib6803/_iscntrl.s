
		.export _iscntrl

		.code

_iscntrl:
		clra
		tsx
		ldab 3,x
		cmpb #32
		bhs fail
		ldab #1
		rts
fail:		clrb
		rts
