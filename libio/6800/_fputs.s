		.export _fputs
		.code

; We ignore the stdio name
_fputs:		tsx
		ldx 5,x
putl:
		ldab ,x
		beq putsdone
		jsr __putc
		inx
		bra putl
putsdone:	rts

