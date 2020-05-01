
		.export _tolower

		.code

_tolower:
		tsx
		clra
		ldab 4,x
		cmpb #'A'
		blt done
		cmpb #'Z'
		bhi done
		addb #$20
done:		rts
