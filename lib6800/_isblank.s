
		.export _isblank

		.code

_isblank:
		clra
		tsx
		ldab 4,x
		cmpb #' '
		beq good
		cmpb #9		; tab
		beq good
		; Any non zero value is valid
		clrb
good:
		rts
