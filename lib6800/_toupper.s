
		.export _toupper

		.code

_toupper:
		tsx
		clra
		ldab 4,x
		cmpb #'a'
		blt done
		cmpb #'z'
		bhi done
		subb #$20
done:		rts
