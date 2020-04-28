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
		; Arrange stack for the divide helper. TOS is already right
		; so push the other 4 bytes we need. The divide helper knows
		; about the fact there is junk (return address) between the
		; two
		clr @tmp4		; sign tracking
		ldaa @sreg+1
		bpl nosignfix
		pula
		jsr negeax
		inc @tmp4		; remember how many negations
		bra signfixed
nosignfix:
		;	Stack the 32bit working register
		pula
signfixed:
		psha
		pshx
		psha
		pshb
		;
		;	Sign rules
		;
		ldaa 10,x		; sign of TOS
		bpl nosignfix2
		inc @tmp4
		ldd 9,x
		subd #1
		std 9,x
		ldd 7,x
		sbcb #0
		sbca #0
		coma
		comb
		std 7,x
		com 9,x
		com 10,x
nosignfix2:
		tsx
		jsr div32x32
		; We now have the positive result. Bit 0 of @tmp4 tells us
		; if we need to negate the answer
		ldab @tmp4
		anda #1
		beq nosignfix3
		pulb
		pula
		pulx
		stx @sreg
		jsr negeax
		jmp pop4
nosignfix3:
		pulb
		pula
		pulx
		stx @sreg
		jmp pop4

