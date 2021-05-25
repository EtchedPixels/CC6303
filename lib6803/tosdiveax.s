;
;	Signed 32bit divide TOS by sreg:d
;
;	The result shall be negative if the signs differ
;

		.export tosdiveax
		.setcpu 6803
		.code

tosdiveax:
		; make work room
		psha
		clr @tmp4		; sign tracking
		ldaa @sreg
		bpl nosignfix
		pula
		jsr negeax
		inc @tmp4		; remember how many negations
		bra signfixed
nosignfix:
		pula
signfixed:
		; Stack the 32bit working register
		; Arrange stack for the divide helper. TOS is already right
		; so push the other 4 bytes we need. The divide helper knows
		; about the fact there is junk (return address) between the
		; two
		pshb
		psha
		ldx @sreg
		pshx
		;
		;	Sign rules
		;
		tsx
		ldaa 6,x		; sign of TOS
		bpl nosignfix2
		inc @tmp4
		ldd 8,x
		subd #1
		std 8,x
		ldd 6,x
		sbcb #0
		sbca #0
		coma
		comb
		std 6,x
		com 8,x
		com 9,x
nosignfix2:
		tsx
		jsr div32x32
		; We now have the positive result. Bit 0 of @tmp4 tells us
		; if we need to negate the answer
		ldab @tmp4
		andb #1
		beq nosignfix3
		ldd 6,x
		std @sreg
		ldd 8,x
		jsr negeax
		pulx
		pulx
		jmp pop4
nosignfix3:
		ldd 6,x
		std @sreg
		ldd 8,x
		pulx
		pulx
		jmp pop4

