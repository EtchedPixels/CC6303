;
;	Multiply 3,x by sreg:d
;
;

		.export umul32x32

		.code
umul32x32:
		pshx		; working space 1,x->4,x
		pshx		; moves argument to 7-10,x
		std @tmp
;
;	Do 32bit x low 8bits
;	This gives us a 32bit result plus 8bits discarded
;
		ldaa 10,x
		ldab @tmp+1
		mul		; calculate low 8 and overflow
		std 3,x		; fill in low 16bits
		ldaa 9,x
		ldab @tmp+1
		mul
		addb 3,x	; next 16 bits plus carry
		adca #0
		std 2,x		; and save
		ldaa 8,x
		ldab @tmp+1
		mul
		addb 2,x	; next 16bits plus carry
		adca #0
		std 1,x		; and save
		ldaa 7,x
		ldab @tmp+1
		mul		; top 8bits (and overflow is lost)
		addb 1,x
		stab 1,x
;
;	Now repeat this with the next 8bits but we only need to do 24bits
;	as the top 8 will overflow
;
		ldaa 10,x
		ldab @tmp
		mul
		addb 3,x	; add in the existing
		adca 2,x
		std 2,x
		ldaa 9,x
		ldab @tmp	; again
		mul
		addb 2,x
		adca 1,x
		std 1,x
		ldaa 8,x
		ldab @tmp
		mul
		addb 1,x
		stab 1,x	; rest overflows, all of top byte overflows
;
;	Now repeat for the next 8bits but we only need to do 16bit as the
;	top 16 will overflow. Spot a 16bit zero and short cut as this
;	is common (eg for uint * ulong cases)
;
		ldd @sreg	; again (b = sreg + 1)
		beq is_done
		ldaa 10,x
		mul
		addb 2,x
		adca 1,x
		std 1,x
		ldaa 9,x
		ldab @sreg+1
		mul
		addb 1,x
		stab 1,x	; rest overflows, all of top byte overflows
;
;	And finally the top 8bits so almost everything overflows
;
		ldaa 10,x
		ldab @sreg
		beq is_done
		mul
		addb 1,x
		stab 1,x	; rest overflows, all of top byte overflows
;
;	Into our working register
;
is_done:
		pulx
		stx @sreg
		pulb
		pula
		rts
