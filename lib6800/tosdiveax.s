;
;	Signed 32bit divide TOS by sreg:d
;
;	The result shall be negative if the signs differ
;

		.export tosdiveax
		.code

tosdiveax:
		; make work room
		psha
		; Arrange stack for the divide helper. TOS is already right
		; so push the other 4 bytes we need. The divide helper knows
		; about the fact there is junk (return address) between the
		; two
		ldaa @sreg
		staa @tmp4		; sign tracking
		pula
		bpl signfixed
		jsr negeax
		bra signfixed
		;	Stack the 32bit working register
signfixed:
		pshb
		psha
		ldaa @sreg
		ldab @sreg+1
		pshb
		psha
		;
		;	Sign rules
		;
		tsx
		tst 6,x		; sign of TOS
		bpl nosignfix2
		com @tmp4
		com 9,x
		com 8,x
		com 7,x
		com 6,x
		inc 9,x
		bne nosignfix2
		inc 8,x
		bne nosignfix2
		inc 7,x
		bne nosignfix2
		inc 6,x
nosignfix2:
		jsr div32x32
		; We now have the positive result. Bit 0 of @tmp4 tells us
		; if we need to negate the answer
		ldaa 6,x
		ldab 7,x
		staa @sreg
		stab @sreg+1
		ldaa 8,x
		ldab 9,x
		tst @tmp4
		bpl popout
		jsr negeax
popout:
		ins
		ins
		ins
		ins
		jmp pop4
