;
;	Unsigned 32bit divide TOS by sreg:d
;

		.export tosudiveax
		.code

tosudiveax:
		; Arrange stack for the divide helper. TOS is already right
		; so push the other 4 bytes we need. The divide helper knows
		; about the fact there is junk (return address) between the
		; two
		pshb
		psha
		ldaa @sreg
		ldab @sreg+1
		pshb
		psha
		tsx
		jsr div32x32
		; Extract the result
		ldaa 6,x
		ldab 7,x
		staa @sreg
	        stab @sreg+1
		ldaa 8,x
		ldab 9,x
		; Fix up the stack
		ins
		ins
		ins
		ins
		jmp pop4
