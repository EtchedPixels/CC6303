;
;	Signed 32bit remainder TOS by sreg:d
;
;	C99 says that the sign of the remainder shall be the sign of the
;	dividend, older C says nothing but it seems unwise to do other
;	things
;

		.export tosmodeax
		.code

tosmodeax:
		tst @sreg
		bpl nosignfix
		jsr negeax
nosignfix:
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
		ldaa 6,x	; sign of TOS
		staa @tmp4
		bpl nosignfix2
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
		ins
		ins
		ins
		ins
		;
		;	At this point @tmp2/@tmp3 hold the positive signed
		;	dividend
		;
		ldab @tmp2+1
		ldaa @tmp2
		stab @sreg+1
		staa @sreg
		ldab @tmp3+1
		ldaa @tmp3
		tst @tmp4	; check if negative dividend
		bpl nonega
		jsr negeax
nonega:
		jmp pop4
