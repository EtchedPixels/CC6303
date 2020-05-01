
		.export _islower

		.code

_islower:
		clra
		tsx
		ldab 4,x
		cmpb #'a'
		blo fail
		cmpb #'z'
		bhi fail
		; Any non zero value is valid
		rts
fail:		clrb
		rts
