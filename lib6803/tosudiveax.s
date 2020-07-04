;
;	Unsigned 32bit divide TOS by sreg:d
;

		.export tosudiveax
		.setcpu 6803
		.code

tosudiveax:
		; Arrange stack for the divide helper. TOS is already right
		; so push the other 4 bytes we need. The divide helper knows
		; about the fact there is junk (return address) between the
		; two
		ldx @sreg
		pshb
		psha
		pshx
		tsx
		jsr div32x32
		; Extract the result
		ldd 6,x
		std @sreg
		ldd 8,x
		; Fix up the stack
		pulx
		pulx
		jmp pop4
